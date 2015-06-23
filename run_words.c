#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <omp.h>
#include <errno.h>

#include "words.h"

#define acronyms       0
#define adjectives     1
#define adverbs        2
#define cia_factbook   3
#define common_stuff   4
#define compound_words 5
#define crosswords     6
#define female_names   7
#define male_names     8
#define nouns          9
#define places        10
#define single_words  11
#define truths        12
#define verbs         13
#define MAX_WORDS     14

typedef struct book 
{
	char *filename;
	char *name;
	WORD_TYPE type;
} BOOK;

WORDS words;


int first = 0;
char *sentence, *word_pntr;
char buffer_string[BUFFER_LEN];

static char
get_sentence ()
{
    const char s[6] = ".?!\"";

    if (first == 0)
    {
        sentence = strtok (buffer_string, s);
        first = 1;
        return (1);
    }
    sentence = strtok (NULL, s);
    if (sentence != NULL)
        return (1);
    else
        return (0);

}

static int
read_in_file (char *file)
{

    int i;
    int nbytes = 100;

    FILE *fd = fopen (file, "r");

    if (NULL == fd)
    {
        fputs ("Error while opening", stderr);
        return 1;
    }

    memset (buffer_string, '\0', sizeof (buffer_string));

    int ret_val = fread (buffer_string, sizeof (char), BUFFER_LEN - 1, fd);
    if (ret_val <= 0)
    {
        fputs ("Error while reading", stderr);
    }
    fclose (fd);


/* Make sure we are dealing with ascii text before looking for sentences */
    for (i = 0; i < nbytes - 1; i++)
    {
        if (!isascii (buffer_string[i]) ||
            (iscntrl (buffer_string[i]) && !isspace (buffer_string[i]) &&
             buffer_string[i] != '\b' && buffer_string[i] != '\032'
             && buffer_string[i] != '\033'))
            return 0;           /* not all ASCII */
    }

/* Change all commas, semi colons, colons and double quotes etc to spaces */

    i = 0;
    while (buffer_string[i] != '\0')
    {
        if ((buffer_string[i] == ',') ||
            (buffer_string[i] == ';') ||
            (buffer_string[i] == ':') ||
            (buffer_string[i] == '_') ||
            (buffer_string[i] == '#') ||
            (buffer_string[i] == '~') ||
            (buffer_string[i] == '!') ||
            (buffer_string[i] == '*') ||
            (buffer_string[i] == '(') ||
            (buffer_string[i] == ')') ||
            (buffer_string[i] == '{') ||
            (buffer_string[i] == '}') ||
            (buffer_string[i] == '<') ||
            (buffer_string[i] == '>') ||
            (buffer_string[i] == '[') ||
            (buffer_string[i] == ']') ||
            (buffer_string[i] == '"') ||
            (buffer_string[i] == '\\') || (buffer_string[i] == '|'))
            buffer_string[i] = ' ';
        i++;
    }

    return (1);
}

int
main (int argc, char **argv)
{

    int ret;
    int index;
    int i;
    int c;
    int lines_allocated = 128;
    int max_line_len = 100;
    int tid, nthreads;
    int is_lower;
    long nth_order;
    long iterations;
    long cores;
    long iters;
    long intvar;
    char *cvalue = NULL;
    struct tm *tm_info;
    time_t begin, end, timer;

    int first = 0;
    char word_in[1024];
    char word_in_lower[1024];
    char *end_word;


    BOOK books[] = { 
		{"data/acronyms.txt","acronyms",ACRONYMS},
		{"data/adjectives.txt","adjectives",ADJECTIVES},
		{"data/adverbs.txt","adverbs",ADVERBS},
		{"data/cia_factbook.txt","cia_factbook",CIA_FACTBOOK},
		{"data/common_stuff.txt","common_stuff",COMMON_STUFF},
		{"data/compound_words.txt","compound_words",COMPOUND_WORDS},
		{"data/crosswords.txt","crosswords",CROSSWORDS},
		{"data/female_names.txt","female_names",FEMALE_NAMES},
		{"data/male_names.txt","male_names",MALE_NAMES},
		{"data/nouns.txt","nouns",NOUNS},
		{"data/places.txt","places",PLACES},
		{"data/single_words.txt","single_words",SINGLE_WORDS},
		{"data/truths.txt","truths",TRUTHS},
		{"data/verbs.txt","verbs",VERBS} 
	};

    char **test_words = (char **) malloc (sizeof (char *) * lines_allocated);
    if (test_words == NULL)
    {
        fprintf (stderr, "Out of memory (1).\n");
        exit (1);
    }

    opterr = 0;
    cores = 0;

    if (0 != access ("data/", F_OK))
    {
        if (ENOENT == errno)
        {
            printf
                ("There is no directory 'data' in the current dir.  Create it and place in the *.txt files\n");
            exit (1);
        }
        if (ENOTDIR == errno)
        {
            printf ("'data' exists but is not a directory!\n");
            exit (1);
        }
    }

    while ((c = getopt (argc, argv, "b:c:o:s:t:")) != -1)
        switch (c)
        {


        case 'b':
//  Benchmark a random search
//
//
            if (argc == 4)
            {
                if (sscanf (argv[3], "%i", &cores) != 1)
                {
                    printf ("Error %s is not an integer", argv[3]);
                    exit (1);
                }
            }
            else
             if (argc != 3)
            {
                printf ("Run Benchmark: Usage -b iters <threads>");
                exit (1);
            }


            omp_set_dynamic (0);        // Explicitly disable dynamic teams
            if (cores == 0)
                omp_set_num_threads (CORES);    // Use CORE threads for all consecutive parallel regions
            else
                omp_set_num_threads (cores);    // Use core threads for all consecutive parallel regions


            if (sscanf (optarg, "%i", &iterations) != 1)
            {
                printf ("Error %s is not an integer", optarg);
                exit (1);
            }


            FILE *fp = fopen ("data/benchmark_words.txt", "r");
            if (fp == NULL)
            {
                fprintf (stderr, "Error opening file.\n");
                exit (2);
            }

            int i;
            for (i = 0; 1; i++)
            {
                int j;

// Have we gone over our line allocation? 
                if (i >= lines_allocated)
                {
                    int new_size;

                    new_size = lines_allocated * 2;
                    test_words =
                        (char **) realloc (test_words,
                                           sizeof (char *) * new_size);
                    if (test_words == NULL)
                    {
                        fprintf (stderr, "Out of memory.\n");
                        exit (3);
                    }
                    lines_allocated = new_size;
                }
                test_words[i] = malloc (max_line_len);
                if (test_words[i] == NULL)
                {
                    fprintf (stderr, "Out of memory (3).\n");
                    exit (4);
                }
                if (fgets (test_words[i], max_line_len - 1, fp) == NULL)
                    break;

// Get rid of CR or LF at end of line 
                for (j = strlen (test_words[i]) - 1;
                     j >= 0 && (test_words[i][j] == '\n'
                                || test_words[i][j] == '\r'); j--);
                test_words[i][j + 1] = '\0';
            }
            fclose (fp);


// Load in words
//              WORDS truths;

            begin = time (NULL);

            printf ("Reading input files ");
            int j;
            for (j = 0; j < MAX_WORDS; j++)
            {
				printf ("%s ",books[j].name);
				ret = load (&words, books[j].filename, books[j].type);
            }
            printf ("\n");

            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n",
                    difftime (end, begin));

            iters = 0;
            int rand1, rand2;


// The basic searchs below are too lightweight- adding threads above 1 for basic searches actually slows things down.
// N threads > 1 is only useful for very deep searches - i.e. large truths.txt files with large numbers of sub entities.

#pragma omp parallel private(nthreads, tid, iters, rand1, rand2)
            {                   // Start parallel region

                tid = omp_get_thread_num ();
                //     printf("Hello from thread = %d\n", tid);

#pragma omp barrier
                if (tid == 0)
                {
                    nthreads = omp_get_num_threads ();
                    printf ("Number of threads = %d\n", nthreads);
                }

#pragma omp barrier

                if (tid == 0)
                {
                    if (cores == 0)
                        iterations = iterations / CORES;
                    else
                        iterations = iterations / cores;

                    printf ("Running %d iterations \n", iterations);
                }

#pragma omp barrier


                while (iters++ < iterations)
                {
                    rand1 = rand () % 354000;
                    rand2 = rand () % 354000;

                    begin = time (NULL);

//              printf("Thread: %d - Testing test_words[%s] and test_words[%s] \n",tid,test_words[rand1],test_words[rand2]);

                    nth_order = 1;
                    ret =
                        word_search (words, nth_order, FULL,
                                     test_words[rand1], test_words[rand2]);

                    nth_order = 2;
                    ret =
                        word_search (words, nth_order, FULL,
                                     test_words[rand1], test_words[rand2]);

                }

            }                   // End paralllel region

            for (; i >= 0; i--)
                free (test_words[i]);
            free (test_words);

            end = time (NULL);
            printf
                ("The Benchmark took %f wall clock seconds to complete.\n\n",
                 difftime (end, begin));

            break;


        case 'c':              // Create an o/p file
            if (argc != 4)
            {
                printf
                    ("Create input file: Usage -c inum_lines words_file\n");
                exit (1);
            }

// Create a new input words file comprising n lines of random alpha numeric strings

            if (sscanf (optarg, "%i", &intvar) != 1)
            {
                printf ("Error - %s is not an integer", optarg);
                exit (1);
            }
            begin = time (NULL);

            ret = create_in_txt (intvar, argv[3]);

            end = time (NULL);
            if (ret != 0)
                printf ("The Entity generation failed \n");
            else
                printf
                    ("The Entity generation took %f wall clock seconds to complete.\n\n",
                     difftime (end, begin));

            break;


        case 'o':
            if (argc != 3)
            {
                printf ("Output into format: Usage -o input_file\n");
                exit (1);
            }

//  Load the truths words file and write out into 3 formats

            begin = time (NULL);

            printf ("Reading input file %s\n", optarg);
            ret = load (&words, optarg,TRUTHS);

            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n",
                    difftime (end, begin));


            if ((ret) == WORDS_SUCCESS)
            {
                begin = time (NULL);

                dump_json (words);
                dump_formatted (words);
                dump_txt (words);

                end = time (NULL);
                printf
                    ("Writing the data out took %f seconds to complete.\n\n",
                     difftime (end, begin));

            }
            else
            {
                printf ("The entity generation failed. \n");
                exit (1);
            }

            break;

        case 's':
            if (argc != 5)
            {
                printf ("Usage -s nth_order<1|2> entity1 entity2\n");
                exit (1);
            }

// Read in the words file and search for 2 strings - if there a common word between them ?
            if (sscanf (optarg, "%i", &nth_order) != 1)
            {
                printf ("Error %s is not an integer", optarg);
                exit (1);
            }

// Load in words
            begin = time (NULL);
            printf ("Reading input file %s\n", books[truths].filename);
            ret = load (&words, books[truths].filename,TRUTHS);
            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n",
                    difftime (end, begin));

// Now search
            begin = time (NULL);

// Choose QUICK or FULL scans
            ret = word_search (words, nth_order, FULL, argv[3], argv[4]);
            end = time (NULL);

            if (ret == 0)
                printf ("The nth order search failed \n");
            else
                printf
                    ("The nth order search took %f seconds to complete and found %d matches.\n\n",
                     difftime (end, begin), ret);

            break;

        case 't':
            if (argc != 3)
            {
                printf ("Usage -t input_file\n");
                exit (1);
            }

            begin = time (NULL);

            printf ("Reading input files %s\n",
                    "truths, adjectives, adverbs, nouns, verbs, acronyms, cia_factbook, common_stuff, compound_words, crosswords, female_names, male_names, places, single_words");

            for (j = 0; j < MAX_WORDS; j++) {
		printf("%s -  ",books[j].name);
                ret = load (&words, books[j].filename,books[j].type);
	    }
            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n",
                    difftime (end, begin));

            printf ("Reading input file %s\n", optarg);
            if (read_in_file (optarg))
            {

                while (get_sentence ())
                {
//              printf( "%s\n", sentence );
                    word_pntr = strtok_r (sentence, " ", &end_word);

                    printf ("\nBegin sentence \n");
                    while (word_pntr != NULL)
                    {
                        if (!isspace (*word_pntr))
                        {
                            strcpy (word_in, word_pntr);

// Create a lower case version of the word
                            strcpy (word_in_lower, word_in);
                            for (i = 0; word_in_lower[i]; i++)
                                word_in_lower[i] = tolower (word_in_lower[i]);
                            
                            printf ("%s\n", word_pntr);

// Print the lower case version is there is one
                            if (strcmp (word_in, word_in_lower) != 0)
                            {
                                printf ("%s\n", word_in_lower);
                                is_lower = 1;
                            }
                            else
                                is_lower = 0;

// Just testing... 

                            for (j = 0; j < MAX_WORDS; j++)
                            {
                                entity *e=find_word (words, word_in); 
                                if ( e != NULL )
                                {
                                    printf ("Found Entity %s in %s\n", word_in, word_type_str(e->type));
                                }
                                if (is_lower)
								{
									entity *e=find_word (words, word_in_lower); 
                                    if ( e != NULL )
                                    {
                                        printf ("Found Entity %s in %s\n", word_in_lower, word_type_str(e->type));
                                    }
								}
                            }

                        }
// Next word
                        word_pntr = strtok_r (NULL, " ", &end_word);
                    }
                    printf ("End sentence \n");
                }
                break;

            }
            else
                printf ("Input file was binary\n");

            break;

        case '?':
            fprintf (stderr, "Read input file: Usage <filename>\n");
            fprintf (stderr, "Run Benchmark: Usage -b iters <threads>\n");
            fprintf (stderr,
                     "Create input file: Usage -c inum_lines words_file\n");
            fprintf (stderr, "Output into format: Usage -o input_file\n");
            fprintf (stderr,
                     "Search: Usage -s nth_order<1|2> entity1 entity2\n");
            fprintf (stderr, "Ingest Test file: Usage -t input_file\n");
            exit (1);

        default:
            printf ("Aborting \n");
            abort ();
        }

    if (argc == 1)
        printf ("Error - no arguments specified \n");
    else if (argc == 2)
        for (index = optind; index < argc; index++)
        {
            printf ("Reading input file %s\n", argv[index]);
            ret = load (&words, argv[index],TRUTHS);
            if (ret == WORDS_SUCCESS)
                printf ("%s read in successfully \n", argv[index]);
            else
                printf ("%s  - error on reading \n", argv[index]);
        }

    return 0;
}
