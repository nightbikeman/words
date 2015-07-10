#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <search.h>
#include <errno.h>




struct hsearch_data htab;
struct entity
{
    char *entity_name;          //This Entity
    int num_links;              //Number of links to Sub Entities for this Entity
    struct entity **links;      //A dymanic array of long pointers to other Root Entities

};

static struct entity *
add_ent (char *name)
{
    struct entity *e = malloc (sizeof (*e));
    e->entity_name = malloc (strlen (name) + 1);
    if (e->entity_name == 0)
    {
        perror ("Unable to allocate memory for name\n");
        exit (2);

    }
    strcpy (e->entity_name, name);
    e->num_links = -1;
    e->links = (void *) 0;

    ENTRY *ent;
    ent = malloc (sizeof (*ent));
    if (ent == NULL)
    {
        perror ("cannot allocate entry");
        exit (2);
    }
    ent->key = e->entity_name;
    ent->data = e;
    if (hsearch_r (name, ENTER, &ent, &htab) != 0)
    {
        perror ("failed to add word ");
        exit (4);
    }
    return e;
}

static void
add_link (struct entity *e, struct entity *focal_root)
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

static struct entity *
find_entity (struct entity *e, int max, char *word)
{
    ENTRY *ent;
    struct entity *ret = NULL;
    if (hsearch_r (word, FIND, &ent, &htab) != 0)
    {
        ret = ent->data;
    }
    return ret;
}

int
main (int argc, char *argv[])
{
    char line[1000000];
    FILE *in, *out;
    int line_length;
    int total_num_root_entities;
    int total_num_sub_entities;
    int num_entities;
    unsigned long *temp;
    int index;
    int ret;
    int roots, subs;
    int json = 1;

    if (hcreate_r (200000, &htab) == 0)
    {
        perror ("Failed to create hash");
        exit (1);
    }

    struct entity *entities = 0;


    in = fopen ("in.txt", "r");
    out = fopen ("out.txt", "w");


    num_entities = -1;
    while (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
    {                           //read one line
        char *word;
        struct entity *focal_root = 0;  //Used when the Root Entity already exists
        char *ptr;
        {
            char *root = strtok_r (line, ",", &ptr);
            int i = 0;

            // First check whether the entry already exists:
            focal_root = find_entity (entities, num_entities, root);

            if (focal_root == NULL)
            {

                // Initialise this Root Entity:
                focal_root = add_ent (root);

                fprintf (out, "%s\n", root);

                total_num_root_entities++;
            }
        }

        for (; word = strtok_r (NULL, ",", &ptr);)
        {
            struct entity *entity = 0;

            //First check whether the entity already exists :
            entity = find_entity (entities, num_entities, word);
            if (entity == NULL)
            {

                //Initialise this Sub Entity:
                entity = add_ent (word);
            }

            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link (focal_root, entity);
            add_link (entity, focal_root);

            // Echo the word to the o/p file :
            fprintf (out, "%s\n", word);

        }                       // End for pos
    }
    fclose (out);
    fclose (in);


// Now print out the entire set of Entities:



    if (json)
        printf ("[\n");
    {
        int i;
        roots = subs = 0;
        for (i = 0; i <= num_entities; i++)
        {
            int j;
            if (entities[i].num_links >= 0)
            {
                if (!json)
                    printf ("Root Entity '%s' discovered with %d sub links\n",
                            entities[i].entity_name, entities[i].num_links);
                roots++;

                for (j = 0; j <= entities[i].num_links; j++)
                {
                    if (((i > 0) || (j > 0)) && (json))
                        printf (",");
                    if (!json)
                        printf ("Sub Entity is %s\n",
                                entities[entities[i].links[j]].entity_name);
                    if (json)
                        printf
                            ("{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",
                             entities[i].entity_name,
                             entities[entities[i].links[j]].entity_name);
                    subs++;
                }
                printf ("\n");

            }

        }
    }
    if (json)
        printf ("]\n");

    if (!json)
    {
        printf
            ("The number of root entities found were %d and the number of subs found were %d\n",
             roots, subs);
        printf ("The total number of Entities are %d\n", num_entities);
        printf ("The total number of Root Entities are %d\n",
                total_num_root_entities);
        printf ("The total number of Sub Entities are %d\n",
                total_num_sub_entities);
    }




    hdestroy_r (&htab);


    return (0);
// fwrite the array of structs out to save them:
    out = fopen ("entities.bin", "wb");
    ret = fwrite (entities, sizeof (entities), 1, out);
    fclose (out);

// fread the array of structs in test:
    in = fopen ("entities.bin", "rb");
    ret = fread (entities, sizeof (entities), 1, in);
    fclose (in);

    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et : */
