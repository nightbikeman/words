#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <omp.h>

#include "words.h"

int
main (int argc, char **argv)
{

    int ret;
    int index;
    int c;
    int lines_allocated = 128;
    int max_line_len = 100;
    int tid, nthreads;
    long nth_order;
    long iterations;
    long cores;
    long iters;
    long intvar;
    char *cvalue = NULL;
    struct tm *tm_info;
    time_t begin, end, timer;

    WORDS words;

    char **test_words = (char **) malloc (sizeof (char *) * lines_allocated);
    if (test_words == NULL)
    {
        fprintf (stderr, "Out of memory (1).\n");
        exit (1);
    }

    opterr = 0;
    cores = 0;


    while ((c = getopt (argc, argv, "b:c:o:s:")) != -1)
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
                cores = strtol (argv[3], NULL, 0);
            }
            else if (argc != 3)
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
            iterations = strtol (optarg, NULL, 0);


            FILE *fp = fopen ("354984single_words", "r");
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
            WORDS words;

            begin = time (NULL);
            printf ("Reading input file %s\n", "in.txt");
            ret = load (&words, "in.txt");
            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n",
                    difftime (end, begin));

            int rand1, rand2;
            iters = 0;


// The basic searchs below are too lightweight- adding threads above 1 for basic searches actually slows things down.
// N threads > 1 is only useful for very deep searches - i.e. large in.txt files with large numbers of sub entities.

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
            intvar = strtol (optarg, NULL, 0);
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
            ret = load (&words, optarg);

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
            nth_order = strtol (optarg, NULL, 0);

// Load in words
            begin = time (NULL);
            printf ("Reading input file %s\n", "in.txt");
            ret = load (&words, "in.txt");
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

        case '?':
            fprintf (stderr, "Run Benchmark: Usage -b iters <threads>\n");
            fprintf (stderr,
                     "Create input file: Usage -c inum_lines words_file\n");
            fprintf (stderr, "Output into format: Usage -o input_file\n");
            fprintf (stderr,
                     "Search: Usage -s nth_order<1|2> entity1 entity2\n");
            exit (1);

        default:
            printf ("Aborting \n");
            abort ();
        }



    return 0;
}
