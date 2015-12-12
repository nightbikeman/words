#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <omp.h>

#include "dhash.h"
#include "words.h"

typedef struct words_impl
{
    hash_table_t *table;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
} WORDS_IMPL;

int
create_in_txt (int num_lines, char *file)
{
    int i, j;
    char buf[100];
    FILE *out;

    void mkrndstr_ipa (size_t length, char *randomString)
    {                           // const size_t length, supra

        static char charset[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

        if (length)
        {
            if (randomString)
            {
                int l = (int) (sizeof (charset) - 1);
				int n;
                for (n = 0; n < length; n++)
                {
                    int key = rand () % l;      // per-iteration instantiation
                    randomString[n] = charset[key];
                }

                randomString[length] = '\0';
            }
        }
    }

    out = fopen (file, "wb");
    if (out == NULL)
        exit (1);

    printf ("Writing %d lines of O/P into %s \n", num_lines, file);

    for (i = 0; i < num_lines; i++)
    {
        for (j = 0; j < (rand () % 40 + 10); j++)
        {

            mkrndstr_ipa ((rand () % 4 + rand () % 8) + rand () % 8 +
                          rand () % 8 + 1, buf);
            fprintf (out, "%s,", buf);
        }
        mkrndstr_ipa ((rand () % 4 + rand () % 8) + rand () % 8 +
                      rand () % 8 + 1, buf);
        fprintf (out, "%s\n", buf);
    }
    fclose (out);

    return 0;
}

static void
delete_callback (hash_entry_t * entry, hash_destroy_enum type, void *pvt)
{
    if (entry->value.type == HASH_VALUE_PTR)
    {
        entity *e = entry->value.ptr;

        free (e->name);
        free (e->links);
        free (entry->value.ptr);
    }
}

static entity *
add_ent (char *name, hash_table_t * table)
{

    entity *e = malloc (sizeof (*e));
    if (e == NULL)
    {
        perror ("allocating entity");
        exit (1);
    }
	e->flag=0;
    e->name = strdup (name);
    e->type = UNKNOWN;
    e->num_links = 0;
    e->links = NULL;
	e->flag=0;

    // Add a entry in the hash table for searching
    /* Enter a key named "My Data" and specify it's value as a pointer to my_data */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = name;

    value.type = HASH_VALUE_PTR;
    value.ptr = e;

    int error;
    if ((error = hash_enter (table, &key, &value)) != HASH_SUCCESS)
    {
        fprintf (stderr, "cannot add to table \"%s\" (%s)\n", key.str,
                 hash_error_string (error));
        exit (1);
    }
    return e;
}


WORDS_STAT add_link(struct entity *e, entity * focal_root, int weight, struct entity *relation)
{
    assert(e);
    assert(focal_root);
    void *temp = 0;
    e->num_links++;
    temp = realloc (e->links, e->num_links  * sizeof ((e->links[0])));
    if (temp == 0)
    {
        perror ("add_link");
        return WORDS_FAIL;
    }
    e->links = temp;
    e->links[e->num_links-1].entity = focal_root;
    e->links[e->num_links-1].weight = weight;
    e->links[e->num_links-1].relation = relation;

    return WORDS_SUCCESS;
}

static entity *
find_entity (char *word, hash_table_t * table)
{
    entity *e = NULL;
    /* Lookup the key named "My Data" */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = word;
    int error;
    if ((error = hash_lookup (table, &key, &value)) == HASH_SUCCESS)
    {
        e = value.ptr;
    }
    return e;

}

WORDS_STAT
initialise (WORDS * w)
{

    WORDS_IMPL *words;
    words = malloc (sizeof (*words));
    if (words == NULL)
    {
        perror ("failed to create words structure");
        return WORDS_FAIL;
    }

    /* Create a hash table */
    int error;
    error = hash_create (INITIAL_HASH, &words->table, delete_callback, NULL);
    if (error != HASH_SUCCESS)
    {
        fprintf (stderr, "cannot create hash table (%s)\n",
                 hash_error_string (error));
        return error;
    }
    words->num_entities = -1;
    words->total_num_root_entities=0;
    words->total_num_sub_entities=0;

    *w = (WORDPTR*)words;
    return WORDS_SUCCESS;
}

WORDS_STAT
load (WORDS w, const char *file, const WORD_TYPE type)
{
    FILE *in;
#ifdef DEBUG
    int lines = 0;
#endif

    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    in = fopen (file, "r");
    if (in == NULL)
    {
        if ( VERBOSE )
            fprintf(stderr,"Input file %s not found \n",file);
		return WORDS_FAIL;
    }

    while (!feof(in))
    {   
		//read one line
		char line[1000000];
		int line_length;
		
		if (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
		{
			char *word;
			entity *focal_root_entity = NULL;       //Used when the Root Entity already exists
			char *ptr;
			line[line_length+1]=0;
			{
				char *root = strtok_r (line, ",", &ptr);
				entity *i = NULL;
#ifdef DEBUG
				lines++;
				printf("adding %s as %s\n",root,word_type_str(type));

					if ((lines % STAGE) == 0)
					{
						time_t timer;
						char buffer[26];
						struct tm *tm_info;
						time (&timer);
						tm_info = localtime (&timer);
						strftime (buffer, sizeof(buffer), "%Y:%m:%d %H:%M:%S", tm_info);
						assert(strlen(buffer) < sizeof(buffer));
						printf("%s: Total lines ingested are %d - latest root entry is %s\n", buffer, lines, root);
					}
#endif

				// First check whether the entry already exists:
				i = find_entity (root, words->table);

				if (i == NULL)
				{
					// Initialise this Root Entity:
					focal_root_entity = add_ent (root, words->table);
					focal_root_entity->type=type;
					words->total_num_root_entities++;
				}
				else
				{

					focal_root_entity = i;
					focal_root_entity->type|=type;
				}
			}

			for (; (word = strtok_r (NULL, ",", &ptr));)
			{
				entity *sub_entity;

				words->num_entities++;
				//First check whether the entity already exists :
				sub_entity = find_entity (word, words->table);
				if (sub_entity == NULL)
				{

					//Initialise this Sub Entity:
					sub_entity = add_ent (word, words->table);

					words->total_num_sub_entities++;
				}

				// Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
				add_link (focal_root_entity, sub_entity,128,NULL);
				add_link (sub_entity, focal_root_entity,128,NULL);
				/* we don't know what type of word this is */
				sub_entity->type=TRUTHS|NOUNS;

			}
		}
		else
		{
			// read the blank line which I think is the only way we get into this block!
			fscanf(in,"\n");
		}
    }
    fclose (in);

    return WORDS_SUCCESS;
}

#define add_word(a,b,c) ((e_add_word(a,b,c) == NULL ) ? WORDS_FAIL : WORDS_SUCCESS)
struct entity*
e_add_word (WORDS w, char *word, const WORD_TYPE type)
{
// It is assumed that the root word doesn't exist at this point
//

	assert(word != NULL);
	assert(w != NULL);
    // make the pointer non-opaque
    WORDS_IMPL *words= (WORDS_IMPL *) w;

    // make sure the word we are adding doesn't already exist
	assert(find_entity(word,words->table) == NULL);

    entity *entity =  add_ent (word, words->table);
	assert(entity != NULL);
    entity->type=type;
    words->total_num_root_entities++;

    return entity;
}

WORDS_STAT
add_linked_word (WORDS w, char *word, const WORD_TYPE type, struct entity *root)
{
	assert(root != NULL);
	assert(word != NULL);
	assert(w != NULL);
    // make the pointer non-opaque
    WORDS_IMPL *words= (WORDS_IMPL *) w;
	
    root->type|=type;
    words->num_entities++;

	//First check root the entity already exists :
    entity *sub_entity = find_entity (word, words->table);
    if (sub_entity == NULL)
    {
		//Initialise this Sub Entity:
		sub_entity = add_ent (word, words->table);
		assert(sub_entity != NULL);

		sub_entity->type=LEARNT_WORDS;
		words->total_num_sub_entities++;
    }

    // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
    add_link (root, sub_entity,128,NULL);
    add_link (sub_entity, root,128,NULL);

    return WORDS_SUCCESS;
}

WORDS_STAT
fdump_chain_json (struct chain *chain, FILE *out)
{
    assert(chain);
    assert(chain->length>0);
    int i;
    fprintf (out, "[\n");
    for(i=0;i<chain->length;i++ )
    {
        fprintf (out,
                 "{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",
                 chain->entity[i]->name, chain->entity[i+1]->name
                );
    }
    fprintf (out, "]\n");

    return WORDS_SUCCESS;
}
WORDS_STAT
fdump_json (const WORDS w, FILE *out)
{
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    /* Visit each entry using iterator object */
    struct hash_iter_context_t *iter;
    iter = new_hash_iter_context (words->table);
    hash_entry_t *entry;
    int i = 0;

    fprintf (out, "[\n");
    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;
        int j;

        if ( data->num_links > 0)
        {
            for (j = 0; j < data->num_links; j++)
            {
                if (i > 0)
                    fprintf (out, ",");
                fprintf (out,
                         "{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",
                         data->name, data->links[j].entity->name);
                i = 1;
            }
        }
    }
    free (iter);
    fprintf (out, "]\n");

    return (WORDS_SUCCESS);
}

WORDS_STAT 
dump_json (const WORDS w, char *filename)
{
    assert(w);
    assert(filename);
    FILE *out;

    out = fopen (filename, "wb");
    if ( out != NULL )
    {
        fdump_json(w,out);
        fclose (out);
        return (WORDS_SUCCESS);
    }
    return (WORDS_FAIL);
}

WORDS_STAT
dump_formatted (const WORDS w)
{

    int j;
    FILE *out;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    /* Visit each entry using iterator object */
    struct hash_iter_context_t *iter;
    iter = new_hash_iter_context (words->table);
    hash_entry_t *entry;

    out = fopen ("data/entities.out", "wb");

    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;

        if (data->num_links > 0)
        {
            fprintf (out, "\nRoot Entity is '%s' and the number of links are %d\n", data->name, data->num_links);

            for (j = 0; j < data->num_links; j++)
                fprintf (out, "Sub Entity is '%s'\n", data->links[j].entity->name);
        }
    }
    free (iter);

    fprintf (out, "\nThe total number of Root Entities are %d\n",
             words->total_num_root_entities);
    fprintf (out, "The total number of Sub Entities are %d\n",
             words->total_num_sub_entities);

    fclose (out);

    return (WORDS_SUCCESS);
}

WORDS_STAT
dump_txt (FILE *out, const WORDS w)
{

    int j;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    /* Visit each entry using iterator object */
    struct hash_iter_context_t *iter;
    iter = new_hash_iter_context (words->table);
    hash_entry_t *entry;

    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;

        if (data->num_links > 0)
        {
            fprintf (out, "%s %d", data->name, data->num_links);

            for (j = 0; j < data->num_links; j++)
                fprintf (out, ",(%s,%s,%d)", data->links[j].entity->name,data->links[j].relation->name,data->links[j].weight);

            fprintf (out, "\n");
        }
    }
    free (iter);

    return (WORDS_SUCCESS);
}

// traverse the tree from a seed point looking for another entity
// marking ones as searched as we go
int 
traverse_tree(entity *seed,entity *target,int depth,const int max_depth,const int mark, struct chain *chain)
{

	if ((void*)seed == (void*)target)
    {
        chain->entity = malloc(sizeof(chain->entity)*(depth+1));
        chain->length=depth;
        chain->entity[depth]=seed;
        return WORDS_SUCCESS;
    }

	if ( depth == max_depth )
		return WORDS_FAIL;

	// mark this one as visited
	seed->flag=mark;

    {
        int i;
        int found=WORDS_FAIL;
        for(i=0; (i< seed->num_links) ; i ++)
        {
            if ( seed->links[i].entity->flag != mark )
            {
                found = traverse_tree(seed->links[i].entity,target,(depth+1),max_depth,mark,chain);
                if (found == WORDS_SUCCESS )
                {
                    chain->entity[depth]=seed;
                    break;
                }
            }
        }

        return found;
    }
}
WORDS_STAT
word_search_r (const WORDS w, long nth_order, char *entity1, char *entity2, struct chain *chain)
{
    WORDS_STAT ret = WORDS_FAIL;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;
    static int mark=0;

    mark ++;
    entity *found_entity1 = find_entity (entity1, words->table);
    if ( found_entity1 != NULL )
    {
        entity *found_entity2 = find_entity (entity2, words->table);
        if ( found_entity2 != NULL )
        {
            ret = traverse_tree(found_entity1,found_entity2,0,nth_order,mark,chain);
        }
    }
    return ret;
}

WORDS_STAT
word_search (const WORDS w, long nth_order, long quick, char *entity1, char *entity2)
{

    int j, k;
    int found = 0;
    long nthreads __attribute__ ((unused)) ;
    long tid;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;
    entity *found_entity1, *found_entity2;

    tid = omp_get_thread_num ();
    //   printf("Hello from thread = %d\n", tid);

    if (tid == 0)
    {
        nthreads = omp_get_num_threads ();
        //           printf("Number of threads = %d\n", nthreads);
    }


    switch (nth_order)
    {
    case 1:                    // Perform 1st order search
        // Entity1 & Entity2 are supplied.  Does Entity2 exist within Entity1 and vice versa ? 
        // A-bomb was found in => abandoned
        //

        found_entity1 = find_entity (entity1, words->table);
        if (found_entity1 != NULL)
        {
            if (VERBOSE)
                printf ("Found Entity %s\n", entity1);

            found_entity2 = find_entity (entity2, words->table);
            if (found_entity2 != NULL)
            {
                if (VERBOSE)
                    printf ("Found Entity %s\n", entity2);

                // Scan each link in Enity1 for Entity2->name  &&
                // Scan each link in Enity2 for Entity1->name
                //
                if ((found_entity1->num_links > 0)
                    || (found_entity2->num_links > 0))
                {
                    //                     printf("Entity1: Root is %s and num_links are %d\n",found_entity1->name,found_entity1->num_links);
                    //                     printf("Entity2: Root is %s and num_links are %d\n",found_entity2->name,found_entity2->num_links);

                    for (j = 0; j < found_entity1->num_links; j++)
                    {
                        //                      printf("%d %s %s \n",j,found_entity1->links[j]->name, found_entity2->name);
                        if (strcmp
                            (found_entity1->links[j].entity->name,
                             found_entity2->name) == 0)
                        {
                            printf
                                ("#1# Thread id(%ld) %s was found in => %s \n",
                                 tid, found_entity1->links[j].entity->name,
                                 found_entity1->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }
                    }

                    for (j = 0; j < found_entity2->num_links; j++)
                    {
                        //                      printf("%d %s %s \n",j,found_entity2->links[j]->name, found_entity1->name);
                        if (strcmp
                            (found_entity2->links[j].entity->name,
                             found_entity1->name) == 0)
                        {
                            printf
                                ("#1# Thread id(%ld) %s was found in => %s \n",
                                 tid, found_entity2->links[j].entity->name,
                                 found_entity2->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }
                    }

                }
                else if (VERBOSE)
                    printf ("This is a Sub entry with no links\n");

            }
            else
            {
                if (VERBOSE)
                    printf ("Entity %s was not found \n", entity2);
                return (0);     // Entity 2 not found
            }

        }
        else
        {

            if (VERBOSE)
                printf ("Entity %s was not found \n", entity1);
            return (0);         // Entity 1 not found
        }

        break;

    case 2:                    // Perform 2nd order search
        // If both entities share the same sub entity or root name then it is returned:
        // A-bomb => bob was found in abandoned => bob
        //

        found_entity1 = find_entity (entity1, words->table);
        if (found_entity1 != NULL)
        {
            if (VERBOSE)
                printf ("Found Entity %s\n", entity1);

            found_entity2 = find_entity (entity2, words->table);
            if (found_entity2 != NULL)
            {
                if (VERBOSE)
                    printf ("Found Entity %s\n", entity2);

                // Scan each link in Enity1 for Entity2->link[k]->name  &&
                // Scan each link in Enity2 for Entity1->link[j]->name
                //
                if ((found_entity1->num_links > 0)
                    && (found_entity2->num_links > 0))
                {
                    //                      if(VERBOSE)printf("Entity1: Root is %s and num_links are %d\n",found_entity1->name,found_entity1->num_links);
                    //                      if(VERBOSE)printf("Entity2: Root is %s and num_links are %d\n",found_entity2->name,found_entity2->num_links);

                    for (j = 0; j < found_entity1->num_links; j++)
                    {
                        //                      printf("%d,%d %s %s \n",j,k,found_entity1->links[j]->name, found_entity2->name);
                        if (strcmp
                            (found_entity1->links[j].entity->name,
                             found_entity2->name) == 0)
                        {
                            printf ("*2* %s was found in => %s \n",
                                    found_entity1->links[j].entity->name,
                                    found_entity1->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }

                        for (k = 0; k < found_entity2->num_links; k++)
                        {
                            //                              printf("%d,%d %s %s \n",j,k,found_entity1->links[j]->name, found_entity2->links[k]->name);
                            if (strcmp
                                (found_entity1->links[j].entity->name,
                                 found_entity2->links[k].entity->name) == 0)
                            {
                                printf
                                    ("*2* Thread id(%ld) %s was found common to Entities %s and %s\n",
                                     tid, found_entity2->links[k].entity->name,
                                     found_entity1->name,
                                     found_entity2->name);
                                found++;
                                if (quick)
                                    break;      // Only scan for one 
                            }
                        }

                    }

                    for (k = 0; k < found_entity2->num_links; k++)
                    {
                        //                      printf("%d,%d %s %s \n",j,k,found_entity1->name, found_entity2->links[k]->name);
                        if (strcmp
                            (found_entity1->name,
                             found_entity2->links[k].entity->name) == 0)
                        {
                            printf ("*2* Thread id(%ld) %s was found in %s\n",
                                    tid, found_entity2->links[k].entity->name,
                                    found_entity2->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }
                    }


                }
                else if (VERBOSE)
                    printf ("One of the Sub Entities has no links\n");

            }
            else
            {
                if (VERBOSE)
                    printf ("Entity %s was not found \n", entity2);
                return (0);     // Entity 2 not found
            }

        }
        else
        {

            if (VERBOSE)
                printf ("Entity %s was not found \n", entity1);
            return (0);         // Entity 1 not found
        }

        break;

    default:
        printf ("Invalid search depth %ld\n", nth_order);
        return (0);
    }

    return (found);
}


entity *
find_word (const WORDS w, char *word)
{
    WORDS_IMPL *words = (WORDS_IMPL *) w;

    /* Visit each entry using iterator object */
    entity *e = find_entity (word, words->table);

    return e;
}
static 
void remove_link(struct entity *e,int link)
{
    assert(e);
    assert( link < e->num_links);

    int i;
    // remove the link
    for(i=link;i<e->num_links;i++)
    {
       e->links[i]=e->links[i+1];
    }
    // realloc the space
    e->num_links--;
    void *temp = realloc (e->links, (e->num_links) * sizeof ((e->links[0])));
    assert(temp || (e->num_links == 0));
    e->links=temp;
}
static WORDS_STAT delete_link_one_way(struct entity *e1,struct entity *e2)
{
    assert(e1 != NULL );
    assert(e2 != NULL );
    int i;
    int removed=0;
    for(i=0;i<e1->num_links;i++)
        if ( e1->links[i].entity == e2) 
        {
            removed++;
            remove_link(e1,i);
        }
    return (removed==0)?WORDS_FAIL:WORDS_SUCCESS;
}
WORDS_STAT delete_link(struct entity *e1,struct entity *e2)
{
    assert(e1 != NULL );
    assert(e2 != NULL );
    int n=0;
    if (delete_link_one_way(e1,e2) == WORDS_SUCCESS ) n++;
    if (delete_link_one_way(e2,e1) == WORDS_SUCCESS ) n++;

    return (n==0)?WORDS_FAIL:WORDS_SUCCESS;
}

static WORDS_STAT is_link_one_way(struct entity *e1,struct entity *e2)
{
    assert(e1 != NULL );
    assert(e2 != NULL );
    int i;
    for(i=0;i<e1->num_links;i++)
        if ( e1->links[i].entity == e2) return WORDS_SUCCESS;

    return WORDS_FAIL;
}
WORDS_STAT is_link(struct entity *e1,struct entity *e2)
{
    assert(e1 != NULL );
    assert(e2 != NULL );
    if (is_link_one_way(e1,e2) == WORDS_SUCCESS) return WORDS_SUCCESS;
    return is_link_one_way(e2,e1);
}
WORDS_STAT update_weight(struct link *l,int weight)
{
    assert(l != NULL );
   
    l->weight=weight; 
   
    return WORDS_SUCCESS; 
}
WORDS_STAT stat_link(struct entity *e1,int depth);

/* vim: set ts=4 sw=4 tw=0 et : */
