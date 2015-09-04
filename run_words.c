#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <omp.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>

#include "words.h"

long total_found_count=0;
long total_not_found_count=0;
const char const* sentence_delimiter = ".?!\"";

char *mmap_null_terminated(int prot, int flags, int fd)
{
    off_t pos = lseek(fd, 0, SEEK_CUR);
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, pos, SEEK_SET);

    char *ptr;
    int pagesize = getpagesize();
    if (size % pagesize != 0) {
        ptr = mmap(NULL, size + 1, prot, flags, fd, 0);
    } else {
        int fullsize = size + pagesize;
        ptr = mmap(NULL, fullsize, PROT_NONE, MAP_PRIVATE | MAP_ANON, 0, 0);
        ptr = mmap(ptr, fullsize, prot, flags | MAP_FIXED, fd, 0);
    }
    assert(ptr[size] == 0);
    return ptr;
}


#define WORD_SIZE 1000
// FIXME there is a better wat to do this.
#define SWORD_SIZE "1000"
    int
scan_file(const WORDS wds, const char *file)
{
    int found_count = 0;
    int not_found_count = 0;
    WORDS_STAT ret;
    char word[WORD_SIZE];
    FILE *f=fopen(file ,"r");
    if ( f != 0 )
    {
#define ACCEPT_SEQUENCE "a-zA-Z"
        while ( ! feof(f) )
        {
            if ( fscanf(f,"%"SWORD_SIZE"["ACCEPT_SEQUENCE"]",word) !=0 )
            {
                int is_lower=0;
                char lword[WORD_SIZE];
                // Create a lower case version of the word, indicate if we the word is lowercase
                {
                    char *s,*d;
                    for (s=word,d=lword; *s; s++,d++)
                    {
                        *d=tolower(*s);
                        if ( *d != *s )  is_lower++;
                    }
                    *d=0;
                }

                int found_word=0;                            

                entity *le=NULL;
                entity *e=find_word (wds, word); 
                if ( e != NULL )
                {
                    found_word=1;
                    found_count++;
                }
                if (is_lower)
                {
                    le=find_word (wds, lword); 
                    if ( le != NULL )
                    {
                        found_word=1;
                        found_count++;
                    }
                }
                if (found_word == 0) 
                {
                    not_found_count++;
                    // Add a new root entry in
                    if ( e == NULL )
                        e = e_add_word (wds, word, LEARNT_WORDS);

                    // If also lower case add in a new sub entity link it back to the 
                    if (is_lower)
                    {
                        ret = add_linked_word (wds, lword, LEARNT_WORDS, e);
                        assert(ret == WORDS_SUCCESS);
                    }
                }
            }
            // discard the next sequence of characters
            fscanf(f,"%*[^"ACCEPT_SEQUENCE"]");
        }
    }
    else printf("%s An error occured from read_in_file() \n",file);

    printf("The number of words found were %d and NOT found (but added) were %d\n",found_count,not_found_count);
    total_found_count=total_found_count+found_count;
    total_not_found_count=total_not_found_count+not_found_count;

    fclose(f);

    return(0);
}

    int
main (int argc, char **argv)
{

    WORDS words;
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
    time_t begin, end;
    char *dir_path = NULL;

    DIR           *d;
    struct dirent *dir;

    initialise(&words);

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
            printf ("There is no directory 'data' in the current dir.  Create it and place in the *.txt files\n");
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
                    if (sscanf (argv[3], "%ld", &cores) != 1)
                    {
                        printf ("Error %s is not an integer", argv[3]);
                        exit (1);
                    }
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

                if (sscanf (optarg, "%ld", &iterations) != 1)
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
                        test_words = (char **) realloc (test_words, sizeof (char *) * new_size);
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
                    for (j = strlen (test_words[i]) - 1; j >= 0 && (test_words[i][j] == '\n' || test_words[i][j] == '\r'); j--);
                    test_words[i][j + 1] = '\0';
                }
                fclose (fp);


                // Load in words
                //              WORDS truths;

                printf ("Reading input files ");
                read_all_files(words);

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

                        printf ("Running %ld iterations \n", iterations);
                    }

#pragma omp barrier


                    while (iters++ < iterations)
                    {
                        rand1 = rand () % 354000;
                        rand2 = rand () % 354000;

                        begin = time (NULL);

                        //              printf("Thread: %d - Testing test_words[%s] and test_words[%s] \n",tid,test_words[rand1],test_words[rand2]);

                        nth_order = 1;
                        ret = word_search (words, nth_order, FULL, test_words[rand1], test_words[rand2]);

                        nth_order = 2;
                        ret = word_search (words, nth_order, FULL, test_words[rand1], test_words[rand2]);

                    }

                }                   // End paralllel region

                for (; i >= 0; i--)
                    free (test_words[i]);
                free (test_words);

                end = time (NULL);
                printf ("The Benchmark took %f wall clock seconds to complete.\n\n", difftime (end, begin));

                break;


            case 'c':              // Create an o/p file
                if (argc != 4)
                {
                    printf ("Create input file: Usage -c inum_lines words_file\n");
                    exit (1);
                }

                // Create a new input words file comprising n lines of random alpha numeric strings

                if (sscanf (optarg, "%ld", &intvar) != 1)
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
                    printf ("The Entity generation took %f wall clock seconds to complete.\n\n", difftime (end, begin));

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
                ret = load (words, optarg, TRUTHS);

                end = time (NULL);
                printf ("The Entity generation took %f seconds to complete.\n\n", difftime (end, begin));


                if ((ret) == WORDS_SUCCESS)
                {
                    begin = time (NULL);

                    dump_json (words,"data/entities.json");
                    dump_formatted (words);
                    dump_txt (words);

                    end = time (NULL);
                    printf ("Writing the data out took %f seconds to complete.\n\n", difftime (end, begin));

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
                if (sscanf (optarg, "%ld", &nth_order) != 1)
                {
                    printf ("Error %s is not an integer", optarg);
                    exit (1);
                }

                // Load in words
                begin = time (NULL);
                printf ("Reading input file\n" );
                ret = read_file(words,truths);
                end = time (NULL);
                printf ("The Entity generation took %f seconds to complete.\n\n", difftime (end, begin));

                // Now search
                begin = time (NULL);

                // Choose QUICK or FULL scans
                ret = word_search (words, nth_order, FULL, argv[3], argv[4]);
                end = time (NULL);
                printf ("Writing the data out took %f seconds to complete.\n\n", difftime (end, begin));

                if (ret == 0)
                    printf ("The nth order search failed \n");
                else
                    printf ("The nth order search took %f seconds to complete and found %d matches.\n\n", difftime (end, begin), ret);

                break;


            case 't':

                if (argc != 3)
                {
                    printf ("Usage -t input_file | input_directory\n");
                    exit (1);
                }

                // Check whether the specifier is a file or directory
                struct stat s;
                int err = stat(optarg, &s);

                if(-1 == err) {
                    if(ENOENT == errno) {
                        printf("%s file or directory does not exist\n",optarg);
                        exit(1);
                    } else {
                        perror("stat");
                        exit(1);
                    }
                } else {

                    // If we have a valid file or directory
                    if(S_ISDIR(s.st_mode)) {
                        printf("An input directory was detected - scanning files: \n");
                        d = opendir(optarg);

                        // Now read in the input files from the input dir and process....
                        if (d)
                        {
                            begin = time (NULL);
                            read_all_files(words);

                            // for each file in the directory...
                            int num_file=0;
                            while ((dir = readdir(d)) != NULL)
                            {
                                // Get rid of . and ..
                                if ((strncmp(dir->d_name,".",1) != 0) && (strncmp(dir->d_name,"..",2) != 0))  {
                                    // Combine OPTARG and d_name as a PATH

                                    dir_path = realloc(dir_path, (strlen(optarg) + strlen("/") + strlen(dir->d_name) + 1) * sizeof(char));

                                    if (!dir_path) {
                                        printf("Path allocation failed \n");
                                        exit(1);
                                    }
                                    int size_path=sizeof(dir_path);
                                    memset (dir_path, '\0', size_path);
                                    strcat(dir_path, optarg);
                                    strcat(dir_path, "/");
                                    strcat(dir_path, dir->d_name);

                                    printf("Scanning input file %d: %s\n", ++num_file,dir_path);
                                    scan_file(words,dir_path);
                                }
                            }

                            closedir(d);
                            end = time (NULL);
                        }
                        printf ("Processing the input directory took %f seconds to complete.\n\n", difftime (end, begin));

                        printf("\nThe total number of words found were %ld \n",total_found_count);
                        printf("The total number of words NOT found were %ld \n",total_not_found_count);

                        begin = time (NULL);

                        dump_json (words,"data/entities.json");
                        dump_formatted (words);
                        dump_txt (words);

                        end = time (NULL);
                        printf ("\nWriting the data out took %f seconds to complete.\n\n", difftime (end, begin));

                    } else {

                        // The input file exists so process....
                        begin = time (NULL);
                        read_all_files(words);

                        printf("An input file %s was detected; Scanning\n",optarg);
                        scan_file(words,optarg);

                        end = time (NULL);
                        printf ("\nProcessing the input file took %f seconds to complete.\n\n", difftime (end, begin));

                        begin = time (NULL);

                        dump_json (words,"data/entities.json");
                        dump_formatted (words);
                        dump_txt (words);

                        end = time (NULL);
                        printf ("\nWriting the data out took %f seconds to complete.\n\n", difftime (end, begin));

                    }
                }



                break;

            case '?':
                fprintf (stderr, "Read input file: Usage <filename>\n");
                fprintf (stderr, "Run Benchmark: Usage -b iters <threads>\n");
                fprintf (stderr, "Create input file: Usage -c inum_lines words_file\n");
                fprintf (stderr, "Output into format: Usage -o input_file\n");
                fprintf (stderr, "Search: Usage -s nth_order<1|2> entity1 entity2\n");
                fprintf (stderr, "Ingest Test file: Usage -t input_file | input_directory\n");
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
            ret = load (words, argv[index], TRUTHS);
            if (ret == WORDS_SUCCESS)
                printf ("%s read in successfully \n", argv[index]);
            else
                printf ("%s  - error on reading \n", argv[index]);
        }

    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et : */
