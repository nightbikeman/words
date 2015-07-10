#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

static void
error (char *msg, int code)
{
    fprintf (stderr, "code=%d:%s", msg, code);
    exit (code);
}

static void
add_ent (redisContext * c, char *word)
{
    redisReply *reply = redisCommand (c, "SET %s \"\"", word);
    if (reply == NULL)
        error ("database set failed", 3);

    freeReplyObject (reply);
}

static void
add_link (redisContext * c, char *key, char *word)
{
    redisReply *reply = redisCommand (c, "APPEND %s ,%s", key, word);
    if (reply == NULL)
        error ("database append failed", 3);

    freeReplyObject (reply);
}

static int
find_entity (char *word, redisContext * c)
{
    redisReply *reply = redisCommand (c, "EXISTS %s", word);
    if (reply == NULL)
        error ("database exists failed", 2);
    int i = reply->type == REDIS_REPLY_INTEGER ? reply->integer : -1;
    freeReplyObject (reply);
    return i;
}

int
main (int argc, char *argv[])
{
    char line[1000000];
    FILE *in, *out;
    int line_length;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
    unsigned long *temp;
    int index;
    int ret;
    int roots, subs;
    int json = 1;


    /* connect to the database */
    redisContext *c = redisConnect ("127.0.0.1", 6379);
    if (c != NULL && c->err)
    {
        printf ("Error: %s\n", c->errstr);
        // handle error
        exit (1);
    }

    in = fopen ("in.txt", "r");
    out = fopen ("out.txt", "w");


    num_entities = -1;
    while (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
    {                           //read one line
        char *word;
        unsigned long focal_root_entity = 0;    //Used when the Root Entity already exists
        char *ptr;
        char *root = strtok_r (line, ",", &ptr);
        int i = 0;

        // First check whether the entry already exists:
        i = find_entity (root, c);

        if (i > num_entities)
        {

            // Initialise this Root Entity:
            add_ent (c, root);

            fprintf (out, "%s\n", root);

            focal_root_entity = num_entities;
            total_num_root_entities++;
        }
        else
        {                       //  If the root entity has been found:

            focal_root_entity = i;
        }                       // if (found == 0)

        for (; word = strtok_r (NULL, ",", &ptr);)
        {
            unsigned long sub_entity = 0;

            //First check whether the entity already exists :
            sub_entity = find_entity (word, c);
            if (sub_entity > num_entities)
            {

                //Initialise this Sub Entity:
                add_ent (c, word);

                total_num_sub_entities++;
                sub_entity = num_entities;

            }                   //End found==0

            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link (c, root, word);
            add_link (c, word, root);

            // Echo the word to the o/p file :
            fprintf (out, "%s\n", word);

        }                       // End for pos
    }
    fclose (out);
    fclose (in);


// Now print out the entire set of Entities:


#ifdef catflap
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
#endif
}
/* vim: set ts=4 sw=4 tw=0 et : */
