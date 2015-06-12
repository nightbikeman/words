#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "dhash.h"
#include "words.h"

typedef struct words_impl
{
    hash_table_t *table;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
} WORDS_IMPL;

typedef struct entity
{
    char *name;
    int num_links;                 //Number of links to Sub Entities for this Entity
    struct entity **links;                   //A dymanic array of long pointers to other Root Entities
} entity;

static void delete_callback(hash_entry_t *entry,
                            hash_destroy_enum type,
                            void *pvt)
{
    if (entry->value.type == HASH_VALUE_PTR) 
    {
        entity *e = entry->value.ptr;

        free(e->name);
        free(e->links);
        free(entry->value.ptr);
    }
}

static entity *add_ent(char *name, hash_table_t *table)
{

        entity *e = malloc(sizeof(*e));
        if ( e == NULL )
        {
            perror("allocating entity");
            exit(1);
        }
        e->name=strdup(name);
        e->num_links= -1;
        e->links=NULL;

        // Add a entry in the hash table for searching
        /* Enter a key named "My Data" and specify it's value as a pointer to my_data */
        hash_key_t key;
        hash_value_t value;
        key.type = HASH_KEY_STRING;
        key.str = strdup(name);
        value.type = HASH_VALUE_PTR;
        value.ptr = e;

        int error;
        if ((error = hash_enter(table, &key, &value)) != HASH_SUCCESS) {
            fprintf(stderr, "cannot add to table \"%s\" (%s)\n", key.str, hash_error_string(error));
            exit(1);
        }
        free(key.str);
        return e;
}

static void add_link(struct entity *e,entity *focal_root)
{
        void *temp=0;
        e->num_links++;
        temp=realloc(e->links,(e->num_links+1)*sizeof((e->links[0])));
        if (temp == 0) {
                  printf("Unable to allocate memory \n");
                  exit(1);
        }
        e->links=temp;
        e->links[e->num_links]=focal_root;
}

static entity *find_entity(char *word,hash_table_t *table)
{
    entity *e=NULL;
    /* Lookup the key named "My Data" */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = strdup(word);
    int error;
    if ((error = hash_lookup(table, &key, &value)) == HASH_SUCCESS) 
    {
        e=value.ptr;
    }
    free(key.str);

    return e;

}
WORDS_STAT load(WORDS *w,const char *file)
{
    char line[1000000];
    FILE *in;
    int line_length;
    unsigned long *temp;
    int index;
    int ret;
    int roots;
    int json=1;
    struct entity *entities=0;

    WORDS_IMPL *words;
    words=malloc(sizeof(*words));
    if ( words == NULL )
    {
        perror("failed to create words structure");
        return WORDS_FAIL;
    }

    /* Create a hash table */
    int error;
    error = hash_create(100000 , &words->table, delete_callback,  NULL);
    if (error != HASH_SUCCESS) {
        fprintf(stderr, "cannot create hash table (%s)\n", hash_error_string(error));
        return error;
    }

    in  = fopen(file, "r");

    words->num_entities= -1;
    while(1==fscanf(in, "%[^\n]%n\n", line, &line_length)){//read one line
        char *word;
        entity *focal_root_entity=NULL;  //Used when the Root Entity already exists
        char *ptr;
        {
            char *root=strtok_r(line,",",&ptr);
            entity *i=NULL;

            // First check whether the entry already exists:
            i=find_entity(root,words->table); 

            if (i == NULL ) 
            {
                    // Initialise this Root Entity:
                    focal_root_entity=add_ent(root,words->table);
                    words->total_num_root_entities++;
            }
            else 
            { 

                    focal_root_entity=i;
            }    
        }

        for(;word=strtok_r(NULL,",",&ptr);) 
        {
            entity *sub_entity;

            words->num_entities++;
            //First check whether the entity already exists :
            sub_entity=find_entity(word,words->table);
            if (sub_entity == NULL) {

                    //Initialise this Sub Entity:
                    sub_entity=add_ent(word,words->table);
                    words->total_num_sub_entities++;
            } 

            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link(focal_root_entity,sub_entity);
            add_link(sub_entity,focal_root_entity);

        } 
    }
    fclose(in);


    *w=words;
    return WORDS_SUCCESS;
}

WORDS_STAT dump_json(const WORDS w)
{
        WORDS_IMPL *words;
        // make the pointer non-opaque
        words=(WORDS_IMPL *)w;

        /* Visit each entry using iterator object */
        struct hash_iter_context_t *iter;
        iter = new_hash_iter_context(words->table);
        hash_entry_t *entry;
        int i=0;
        while ((entry = iter->next(iter)) != NULL) {
            struct entity *data = (struct entity *) entry->value.ptr;
            int j;

            for (j=0; j<=data->num_links;j++)  {
                    if ( i > 0 ) 
                        printf(",");
                    printf ("{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",data->name,data->links[j]->name);
                    i=1;
            }
        }
        free(iter);
     printf ("]\n");

    return(WORDS_SUCCESS);
}

WORDS_STAT dump(const WORDS w)
{


    int subs=0;
    int roots=0;
        WORDS_IMPL *words;
        // make the pointer non-opaque
        words=(WORDS_IMPL *)w;

        /* Visit each entry using iterator object */
        struct hash_iter_context_t *iter;
        iter = new_hash_iter_context(words->table);
        hash_entry_t *entry;
        int i=0;
        while ((entry = iter->next(iter)) != NULL) {
            struct entity *data = (struct entity *) entry->value.ptr;
            int j;
            roots++;

            for (j=0; j<=data->num_links;j++)  {
                    printf("Sub Entity is %s\n",data->links[j]->name);
                    subs++;
                    i=1;
            }
        }
        free(iter);
    printf("The number of root entities found were %d and the number of subs found were %d\n",roots,subs);
    printf("The total number of Entities are %d\n",words->num_entities);
    printf("The total number of Root Entities are %d\n",words->total_num_root_entities);
    printf("The total number of Sub Entities are %d\n",words->total_num_sub_entities);






return(WORDS_SUCCESS);
}
