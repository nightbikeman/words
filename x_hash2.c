#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#define __USE_GNU
#include <search.h>
#include "words.h"


typedef struct words_impl
{
    struct entity *entities;
    struct hsearch_data htab;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
} WORDS_IMPL;

struct entity
{
    char *entity_name;          //This Entity
    int num_links;              //Number of links to Sub Entities for this Entity
    long *links;                //A dymanic array of long pointers to other Root Entities
};

static void
add_ent (struct entity **ents, long *no, char *name,
         struct hsearch_data *htab)
{

    (*no)++;
    *ents = realloc (*ents, ((*no) + 1) * sizeof (*ents[0]));
    struct entity *e = &((*ents)[*no]);
    e->entity_name = malloc (strlen (name) + 1);
    if (e->entity_name == 0)
    {
        printf ("Unable to allocate memory for name\n");
        exit (2);

    }
    strcpy (e->entity_name, name);
    e->num_links = -1;
    e->links = (long *) 0;

    // Add a entry in the hash table for searching
    ENTRY ent;
    ENTRY *ent_ptr = 0;;
    ent.key = e->entity_name;

    // This is very naughty, but we only want a integer!
    ent.data = (void *) *no;

    if (hsearch_r (ent, ENTER, &ent_ptr, htab) == 0)
    {
        perror ("failed to add word ");
        exit (4);
    }
}

static void
add_link (struct entity *e, int focal_root)
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

static long
find_entity (char *word, struct hsearch_data *htab, long max)
{
    int i = max + 1;
    ENTRY ent;
    ENTRY *e = &ent;

    ent.key = word;
    if (hsearch_r (ent, FIND, &e, htab) != 0)
    {
        i = (long) e->data;
    }
    return i;

}

WORDS_STAT
load (WORDS * w, const char *file)
{
    char line[1000000];
    FILE *in;
    int line_length;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
    unsigned long *temp;
    int index;
    int ret;
    int roots, subs;
    int json = 1;

    WORDS_IMPL *words;
    words = malloc (sizeof (*words));
    if (words == NULL)
    {
        perror ("failed to create words structure");
        return WORDS_FAIL;
    }

    if (hcreate_r (200000, &words->htab) == 0)
    {
        perror ("Could not create hash table");
        exit (2);
    }

    in = fopen (file, "r");

    words->num_entities = -1;

    while (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
    {                           //read one line
        char *word;
        unsigned long focal_root_entity = 0;    //Used when the Root Entity already exists
        char *ptr;
        {
            char *root = strtok_r (line, ",", &ptr);
            int i = 0;

            // First check whether the entry already exists:
            i = find_entity (root, &words->htab, words->num_entities);

            if (i > words->num_entities)
            {

                // Initialise this Root Entity:
                add_ent (&words->entities, &words->num_entities, root,
                         &words->htab);

                focal_root_entity = words->num_entities;
                words->total_num_root_entities++;
            }
            else
            {                   //  If the root entity has been found:

                focal_root_entity = i;
            }                   // if (found == 0)
        }

        for (; word = strtok_r (NULL, ",", &ptr);)
        {
            unsigned long sub_entity = 0;

            //First check whether the entity already exists :
            sub_entity =
                find_entity (word, &words->htab, words->num_entities);
            if (sub_entity > words->num_entities)
            {

                //Initialise this Sub Entity:
                add_ent (&words->entities, &words->num_entities, word,
                         &words->htab);

                words->total_num_sub_entities++;
                sub_entity = num_entities;

            }                   //End found==0

            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link (&words->entities[focal_root_entity], sub_entity);
            add_link (&words->entities[sub_entity], focal_root_entity);

        }                       // End for pos
    }
    fclose (in);

    *w = words;
}

WORDS_STAT
dump_json (const WORDS w)
{
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    printf ("[\n");
    {
        int i;
        int roots = 0;
        int subs = 0;
        for (i = 0; i <= words->num_entities; i++)
        {
            int j;
            if (words->entities[i].num_links >= 0)
            {
                roots++;
                for (j = 0; j <= words->entities[i].num_links; j++)
                {
                    if (((i > 0) || (j > 0)))
                        printf (",");
                    printf
                        ("{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",
                         words->entities[i].entity_name,
                         words->entities[words->entities[i].links[j]].
                         entity_name);
                    subs++;
                }
                printf ("\n");

            }

        }
    }
    printf ("]\n");

    return WORDS_SUCCESS;
}

WORDS_STAT
dump (const WORDS w)
{
    WORDS_IMPL *words;
    // make the pointer non-opaque
    words = (WORDS_IMPL *) w;

    int i;
    int roots = 0;
    int subs = 0;
    for (i = 0; i <= words->num_entities; i++)
    {
        int j;
        if (words->entities[i].num_links >= 0)
        {
            printf ("Root Entity '%s' discovered with %d sub links\n",
                    words->entities[i].entity_name,
                    words->entities[i].num_links);
            roots++;

            for (j = 0; j <= words->entities[i].num_links; j++)
            {
                printf ("Sub Entity is %s\n",
                        words->entities[words->entities[i].links[j]].
                        entity_name);
                subs++;
            }
            printf ("\n");

        }

    }
    printf
        ("The number of root entities found were %d and the number of subs found were %d\n",
         roots, subs);
    printf ("The total number of Entities are %d\n", words->num_entities);
    printf ("The total number of Root Entities are %d\n",
            words->total_num_root_entities);
    printf ("The total number of Sub Entities are %d\n",
            words->total_num_sub_entities);

    return (0);
}
/* vim: set ts=4 sw=4 tw=0 et : */
