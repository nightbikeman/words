indent: Standard input:79: Warning:old style assignment ambiguity in "=&".  Assuming "= &"

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#define __USE_GNU
#include <search.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#define DEBUG 0
#define LARGE_HASH_TABLE 200000000

#define ENTITIES_FILEPATH "entities_mmapped.bin"
#define ENTITIES_FILESIZE 1000000000    // A large size - make larger than the total problem size

#define HASH_FILEPATH "hash_mmapped.bin"
#define HASH_FILESIZE 2000000000        // A large size - 10* larger that LARGE_HASH_TABLE

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
    int fd_entities_path, fd_hash_path;
    int result;
    int roots, subs;
    int num_lines = 0;
    int num_dups = 0;
    int json = 1;
    struct entity *entities = 0;
    struct hsearch_data htab;

    if (hcreate_r (LARGE_HASH_TABLE, &htab) == 0)
    {
        perror ("Could not create hash table");
        exit (2);
    }

// Begin MMAP stuff:
//
// Open an mmap file for writing.
// - Creating the file if it doesn't exist.
// - Truncating it to 0 size if it already exists. (not really needed)
// Note: "O_WRONLY" mode is not sufficient when mmaping.
//
    fd_entities_path =
        open (ENTITIES_FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
    if (fd_entities_path == -1)
    {
        perror ("Error opening mmap file for writing");
        exit (EXIT_FAILURE);
    }
    fd_hash_path =
        open (HASH_FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
    if (fd_hash_path == -1)
    {
        perror ("Error opening mmap file for writing");
        exit (EXIT_FAILURE);
    }

// Stretch the file size to the size of the (mmapped) array of ints
//
    result = lseek (fd_entities_path, ENTITIES_FILESIZE - 1, SEEK_SET);
    if (result == -1)
    {
        close (fd_entities_path);
        perror ("Error calling lseek() to 'stretch' the file");
        exit (EXIT_FAILURE);
    }
    result = lseek (fd_hash_path, HASH_FILESIZE - 1, SEEK_SET);
    if (result == -1)
    {
        close (fd_hash_path);
        perror ("Error calling lseek() to 'stretch' the file");
        exit (EXIT_FAILURE);
    }

// Something needs to be written at the end of the file to
// have the file actually have the new size.
// Just writing an empty string at the current file position will do.
// 
// Note:
//  - The current position in the file is at the end of the stretched
//    file due to the call to lseek().
//  - An empty string is actually a single '\0' character, so a zero-byte
//    will be written at the last byte of the file.
//    
    result = write (fd_entities_path, "", 1);
    if (result != 1)
    {
        close (fd_entities_path);
        perror ("Error writing last byte of the file");
        exit (EXIT_FAILURE);
    }
    result = write (fd_hash_path, "", 1);
    if (result != 1)
    {
        close (fd_hash_path);
        perror ("Error writing last byte of the file");
        exit (EXIT_FAILURE);
    }

// Now the file is ready to be mmapped.
//
    entities =
        mmap (0, ENTITIES_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
              fd_entities_path, 0);
    if (entities == MAP_FAILED)
    {
        close (fd_entities_path);
        perror ("Error mmapping the entities file");
        exit (EXIT_FAILURE);
    }
    htab =
        mmap (0, HASH_FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
              fd_hash_path, 0);
    if (htab == MAP_FAILED)
    {
        close (fd_hash_path);
        perror ("Error mmapping the entities file");
        exit (EXIT_FAILURE);
    }

// End MMAP stuff


    in = fopen ("in.txt", "r");

    if (!json)
        printf ("Creating the in memory table... \n");

    num_entities = -1;
    while (1 == fscanf (in, "%[^\n]%n\n", line, &line_length))
    {                           //read one line
        char *word;
        unsigned long focal_root_entity = 0;    //Used when the Root Entity already exists
        char *ptr;
        {
            char *root = strtok_r (line, ",", &ptr);
            int i = 0;

            // First check whether the entry already exists:
            i = find_entity (root, &htab, num_entities);

            if (i > num_entities)
            {

                // Initialise this Root Entity:
                add_ent (&entities, &num_entities, root, &htab);

                focal_root_entity = num_entities;
                total_num_root_entities++;
            }
            else
            {                   //  If the root entity has been found:
                if (!json && DEBUG)
                    printf
                        ("***At line %d Root Entity %s was already found \n",
                         num_lines, root);
                num_dups++;


                focal_root_entity = i;
            }                   // if (found == 0)
        }

        for (; word = strtok_r (NULL, ",", &ptr);)
        {
            unsigned long sub_entity = 0;

            //First check whether the entity already exists :
            sub_entity = find_entity (word, &htab, num_entities);
            if (sub_entity > num_entities)
            {

                //Initialise this Sub Entity:
                add_ent (&entities, &num_entities, word, &htab);

                total_num_sub_entities++;
                sub_entity = num_entities;

            }                   //End found==0
            else
            {
                if (!json && DEBUG)
                    printf
                        ("***At line %d Sub Entity %s was already found \n",
                         num_lines, word);
                num_dups++;
            }


            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link (&entities[focal_root_entity], sub_entity);
            add_link (&entities[sub_entity], focal_root_entity);


        }                       // End for pos

        num_lines++;

    }                           // End while fscanf

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
        printf
            ("The total number of Entities are %d read in %d lines with %d duplications.\n",
             num_entities, num_lines, num_dups);
    }


// Don't forget to free the mmapped memory
//
    if (munmap (entities, ENTITIES_FILESIZE) == -1)
    {
        perror ("Error un-mmapping the file");
        /* Decide here whether to close(fd) and exit() or not. Depends... */
    }
    if (munmap (htab, HASH_FILESIZE) == -1)
    {
        perror ("Error un-mmapping the file");
        /* Decide here whether to close(fd) and exit() or not. Depends... */
    }

// Un-mmaping doesn't close the file, so we still need to do that.
//
    close (fd_entities_path);
    close (fd_hash_path);


    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et : */
