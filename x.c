#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    int num_links;              //Number of links to Sub Entities for this Entity
    struct entity **links;      //A dymanic array of long pointers to other Root Entities
} entity;

int
create_in_txt (int num_lines, char *file)
{
    int n;
    int i, j;
    char buf[100];
    FILE *out;

    void mkrndstr_ipa (size_t length, char *randomString)
    {                           // const size_t length, supra

        static char charset[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        int n;

        if (length)
        {
            if (randomString)
            {
                int l = (int) (sizeof (charset) - 1);
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

    printf ("Writing %ld lines of O/P into %s \n", num_lines, file);

    for (i = 0; i < num_lines; i++)
    {
        n = rand () % 60000;

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
    e->name = strdup (name);
    e->num_links = -1;
    e->links = NULL;

    // Add a entry in the hash table for searching
    /* Enter a key named "My Data" and specify it's value as a pointer to my_data */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = strdup (name);
    value.type = HASH_VALUE_PTR;
    value.ptr = e;

    int error;
    if ((error = hash_enter (table, &key, &value)) != HASH_SUCCESS)
    {
        fprintf (stderr, "cannot add to table \"%s\" (%s)\n", key.str,
                 hash_error_string (error));
        exit (1);
    }
    free (key.str);
    return e;
}

static void
add_link (struct entity *e, entity * focal_root)
{
    void *temp = 0;
    e->num_links++;
    temp = realloc (e->links, (e->num_links + 1) * sizeof ((e->links[0])));
    if (temp == 0)
    {
        printf ("Unable to allocate memory \n");
        exit (1);
    }
    e->links = temp;
    e->links[e->num_links] = focal_root;
}

static entity *
find_entity (char *word, hash_table_t * table)
{
    entity *e = NULL;
    /* Lookup the key named "My Data" */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = strdup (word);
    int error;
    if ((error = hash_lookup (table, &key, &value)) == HASH_SUCCESS)
    {
        e = value.ptr;
    }
    free (key.str);

    return e;

}

WORDS_STAT
load (WORDS * w, const char *file)
{
    char line[1000000];
    FILE *in;
    int line_length;
    unsigned long *temp;
    int index;
    int ret;
    int roots;
    int lines = 0;
    int json = 0;
    char buffer[26];
    struct tm *tm_info;
    time_t timer;
    struct entity *entities = 0;

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

    in = fopen (file, "r");
    if (in == NULL)
    {
        printf ("Input file %s not found \n");
        exit (1);
    }

    words->num_entities = -1;
    while (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
    {                           //read one line
        char *word;
        entity *focal_root_entity = NULL;       //Used when the Root Entity already exists
        char *ptr;
        {
            char *root = strtok_r (line, ",", &ptr);
            entity *i = NULL;
            lines++;

            if (!json)
                if ((lines % STAGE) == 0)
                {
                    time (&timer);
                    tm_info = localtime (&timer);
                    strftime (buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
                    printf
                        ("%s: Total lines ingested are %d - latest root entry is '%s'\n",
                         buffer, lines, root);
                }

            // First check whether the entry already exists:
            i = find_entity (root, words->table);

            if (i == NULL)
            {
                // Initialise this Root Entity:
                focal_root_entity = add_ent (root, words->table);
                words->total_num_root_entities++;
            }
            else
            {

                focal_root_entity = i;
            }
        }

        for (; word = strtok_r (NULL, ",", &ptr);)
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
            add_link (focal_root_entity, sub_entity);
            add_link (sub_entity, focal_root_entity);

        }
    }
    fclose (in);


    *w = words;
    return WORDS_SUCCESS;
}

WORDS_STAT
dump_json (const WORDS w)
{
    FILE *out;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    /* Visit each entry using iterator object */
    struct hash_iter_context_t *iter;
    iter = new_hash_iter_context (words->table);
    hash_entry_t *entry;
    int i = 0;

    out = fopen ("entities.json", "wb");

    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;
        int j;

        if (data->name, data->num_links > 0)
        {
            for (j = 0; j <= data->num_links; j++)
            {
                if (i > 0)
                    fprintf (out, ",");
                fprintf (out,
                         "{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",
                         data->name, data->links[j]->name);
                i = 1;
            }
        }
    }
    free (iter);
    fprintf (out, "]\n");

    fclose (out);

    return (WORDS_SUCCESS);
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

    out = fopen ("entities.out", "wb");

    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;

        if (data->name, data->num_links > 0)
        {
            fprintf (out,
                     "\nRoot Entity is '%s' and the number of links are %d\n",
                     data->name, data->num_links);

            for (j = 0; j <= data->num_links; j++)
                fprintf (out, "Sub Entity is '%s'\n", data->links[j]->name);
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
dump_txt (const WORDS w)
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

    out = fopen ("entities.txt", "wb");

    while ((entry = iter->next (iter)) != NULL)
    {
        struct entity *data = (struct entity *) entry->value.ptr;

        if (data->name, data->num_links > 0)
        {
            fprintf (out, "%s", data->name, data->num_links);

            for (j = 0; j <= data->num_links; j++)
                fprintf (out, ",%s", data->links[j]->name);

            fprintf (out, "\n");
        }
    }
    free (iter);

    fclose (out);

    return (WORDS_SUCCESS);
}

WORDS_STAT
word_search (const WORDS w, long nth_order, long quick, char *entity1,
             char *entity2)
{

    int j, k;
    int found = 0;
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;
    hash_entry_t *entry;
    entity *found_entity1, *found_entity2, *found_entity3;

    switch (nth_order)
    {
    case 1:                    // Perform 1st order search
        found_entity1 = find_entity (entity1, words->table);
        if (found_entity1 != NULL)
        {
            printf ("Found Entity %s\n", entity1);

            found_entity2 = find_entity (entity2, words->table);
            if (found_entity2 != NULL)
            {
                printf ("Found Entity %s\n", entity2);

                // Scan each link in Enity1 for Entity2->name  &&
                // Scan each link in Enity2 for Entity1->name
                //
                if ((found_entity1->name, found_entity1->num_links > 0)
                    || (found_entity2->name, found_entity2->num_links > 0))
                {
                    printf ("Entity1: Root is %s and num_links are %d\n",
                            found_entity1->name, found_entity1->num_links);
                    printf ("Entity2: Root is %s and num_links are %d\n",
                            found_entity2->name, found_entity2->num_links);

                    for (j = 0; j < found_entity1->num_links; j++)
                    {
                        //              printf("%d %s %s \n",j,found_entity1->links[j]->name, found_entity2->name);
                        if (strcmp
                            (found_entity1->links[j]->name,
                             found_entity2->name) == 0)
                        {
                            printf ("%s was found in => %s \n",
                                    found_entity2->name, found_entity1->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }
                    }

                    for (j = 0; j < found_entity2->num_links; j++)
                    {
                        //              printf("%d %s %s \n",j,found_entity2->links[j]->name, found_entity1->name);
                        if (strcmp
                            (found_entity2->links[j]->name,
                             found_entity1->name) == 0)
                        {
                            printf ("%s was found in => %s \n",
                                    found_entity1->name, found_entity2->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }
                    }

                }
                else
                    printf ("This is a Sub entry with no links\n");

            }
            else
            {
                printf ("Entity %s was not found \n", entity2);
                return (0);     // Entity 2 not found
            }

        }
        else
        {

            printf ("Entity %s was not found \n", entity1);
            return (0);         // Entity 1 not found
        }

        break;

    case 2:                    // Perform 2nd order search
        found_entity1 = find_entity (entity1, words->table);
        if (found_entity1 != NULL)
        {
            printf ("Found Entity %s\n", entity1);

            found_entity2 = find_entity (entity2, words->table);
            if (found_entity2 != NULL)
            {
                printf ("Found Entity %s\n", entity2);

                // Scan each link in Enity1 for Entity2->link[k]->name  &&
                // Scan each link in Enity2 for Entity1->link[j]->name
                //
                if ((found_entity1->name, found_entity1->num_links > 0)
                    && (found_entity2->name, found_entity2->num_links > 0))
                {
                    printf ("Entity1: Root is %s and num_links are %d\n",
                            found_entity1->name, found_entity1->num_links);
                    printf ("Entity2: Root is %s and num_links are %d\n",
                            found_entity2->name, found_entity2->num_links);

                    for (j = 0; j < found_entity1->num_links; j++)
                    {
                        printf ("%d,%d %s %s \n", j, k,
                                found_entity1->links[j]->name,
                                found_entity2->name);
                        if (strcmp
                            (found_entity1->links[j]->name,
                             found_entity2->name) == 0)
                        {
                            printf ("%s was found in => %s \n",
                                    found_entity1->links[j]->name,
                                    found_entity2->name);
                            found++;
                            if (quick)
                                break;  // Only scan for one 
                        }

                        for (k = 0; k < found_entity2->num_links; k++)
                        {
                            printf ("%d,%d %s %s \n", j, k,
                                    found_entity1->links[j]->name,
                                    found_entity2->links[k]->name);
                            if (strcmp
                                (found_entity1->links[j]->name,
                                 found_entity2->links[k]->name) == 0)
                            {
                                printf ("%s => %s was found in %s => %s\n",
                                        found_entity1->name,
                                        found_entity1->links[j]->name,
                                        found_entity2->name,
                                        found_entity2->links[k]->name);
                                found++;
                                if (quick)
                                    break;      // Only scan for one 
                            }
                        }

                    }


                }
                else
                    printf ("One of the Sub Entities has no links\n");

            }
            else
            {
                printf ("Entity %s was not found \n", entity2);
                return (0);     // Entity 2 not found
            }

        }
        else
        {

            printf ("Entity %s was not found \n", entity1);
            return (0);         // Entity 1 not found
        }

        break;

    default:
        printf ("Invalid search depth %d\n", nth_order);
        return (0);
    }

    return (found);
}
