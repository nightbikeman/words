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

int lines_allocated = 128;
long total_found_count=0;
long total_not_found_count=0;
const char const* sentence_delimiter = ".?!\"";
char **test_words;
WORDS words;

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

int scan_file(const WORDS wds, const char *file)
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

int run_benchmark(int iterations)
{

    int iters;
    int max_line_len = 100;
    long nth_order;
    time_t begin, end;

    lines_allocated=128;

            //  Benchmark a random search
            //
            //


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



            printf ("Running %d iterations \n", iterations);

            while (iters++ < iterations)
            {
                rand1 = rand () % 354000;
                rand2 = rand () % 354000;

                begin = time (NULL);


                nth_order = 1;
                word_search (words, nth_order, FULL, test_words[rand1], test_words[rand2]);

                nth_order = 2;
                word_search (words, nth_order, FULL, test_words[rand1], test_words[rand2]);

            }

            for (; i >= 0; i--)
                free (test_words[i]);

            free (test_words);

            end = time (NULL);
            printf ("The Benchmark took %f wall clock seconds to complete.\n\n", difftime (end, begin));

      return(0);
}


int create_outut_file (int lines, char *file)
{
    int ret;
    time_t begin, end;

            // Create an o/p file
            begin = time (NULL);

            ret = create_in_txt (lines, file);

            end = time (NULL);
            if (ret != 0)
                printf ("The Entity generation failed \n");
            else
                printf ("The Entity generation took %f wall clock seconds to complete.\n\n", difftime (end, begin));

            return(0);
}

int load_truths_and_write (char *file)
{
    int ret;
    time_t begin, end;

            //  Load the truths words file and write out into 3 formats
            begin = time (NULL);

            printf ("Reading input file %s\n", file);
            ret = load (words, file, TRUTHS);

            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n", difftime (end, begin));


            if ((ret) == WORDS_SUCCESS)
            {
                begin = time (NULL);

                dump_json (words,"data/entities.json");
                dump_formatted (words);
                FILE *out = fopen("data/entities.txt","w");
                if (out == NULL) exit(1);
                dump_txt (out, words);
                fclose(out);

                end = time (NULL);
                printf ("Writing the data out took %f seconds to complete.\n\n", difftime (end, begin));

            }
            else
            {
                printf ("The entity generation failed. \n");
                exit (1);
            }

    return(0);
}

int read_input_file (char *path)
{
    int ret;

            printf ("Reading input file %s\n", path);
            ret = load (words, path, TRUTHS);
            if (ret == WORDS_SUCCESS)
                printf ("%s read in successfully \n", path);
            else
                printf ("%s  - error on reading \n", path);

    return(0);
}

int search_for_words (char *find_word_1, char *find_word_2, int order_val)
{
    int ret;
    time_t begin, end;

            // Search for words
            begin = time (NULL);

            printf ("Reading input file\n" );
            ret = read_file(words,truths);
            end = time (NULL);
            printf ("The Entity generation took %f seconds to complete.\n\n", difftime (end, begin));

            // Now search
            begin = time (NULL);

            // Choose QUICK or FULL scans
            ret = word_search (words, order_val, FULL, find_word_1, find_word_2);

            end = time (NULL);
            printf ("Writing the data out took %f seconds to complete.\n\n", difftime (end, begin));

            if (ret == 0)
                printf ("The nth order search failed \n");
            else
                printf ("The nth order search took %f seconds to complete and found %d matches.\n\n", difftime (end, begin), ret);

    return(0);
}


int ingest_files(char *path)
{
    int err;
    char *dir_path = NULL;
    DIR           *d;
    struct dirent *dir;
    struct stat s;
    time_t begin, end;

            // Ingest input files
            // Check whether the specifier is a file or directory
            err = stat(path, &s);

            if(-1 == err) {
                if(ENOENT == errno) {
                    printf("%s file or directory does not exist\n",path);
                    exit(1);
                } else {
                    perror("stat");
                    exit(1);
                }
            } else {

                // If we have a valid file or directory
                total_found_count=0;
                total_not_found_count=0;

                if(S_ISDIR(s.st_mode)) {
                    printf("An input directory was detected - scanning files: \n");
                    d = opendir(path);

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

                                dir_path = realloc(dir_path, (strlen(path) + strlen("/") + strlen(dir->d_name) + 1) * sizeof(char));

                                if (!dir_path) {
                                    printf("Path allocation failed \n");
                                    exit(1);
                                }
                                int size_path=sizeof(dir_path);
                                memset (dir_path, '\0', size_path);
                                strcat(dir_path, path);
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
                    FILE *out = fopen("data/entities.txt","w");
                    if (out == NULL) exit(1);
                    dump_txt (out,words);
                    fclose(out);

                    end = time (NULL);
                    printf ("\nWriting the data out took %f seconds to complete.\n\n", difftime (end, begin));

                } else {

                    // The input file exists so process....
                    begin = time (NULL);
                    read_all_files(words);

                    printf("An input file %s was detected; Scanning\n",path);
                    scan_file(words,path);

                    end = time (NULL);
                    printf ("\nProcessing the input file took %f seconds to complete.\n\n", difftime (end, begin));

                    begin = time (NULL);

                    dump_json (words,"data/entities.json");
                    dump_formatted (words);
                    FILE *out = fopen("data/entities.txt","w");
                    if (out == NULL) exit(1);
                    dump_txt (out,words);
                    fclose(out);

                    end = time (NULL);
                    printf ("\nWriting the data out took %f seconds to complete.\n\n", difftime (end, begin));

                }
        }

    return(0);
}




int main (int argc, char **argv)
{

    int c;
    int iters;
    int order;
    int daemon=0;
    char input_buffer[256];
    char *p2;
    long intvar;
    enum { kMaxArgs = 64 };
    const char s[2] = " ";


    initialise(&words);

    test_words = (char **) malloc (sizeof (char *) * lines_allocated);
    if (test_words == NULL)
    {
        fprintf (stderr, "Out of memory (1).\n");
        exit (1);
    }

    opterr = 0;

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

    while (1)
    {

    if (daemon==1)
    {
            system("clear");
            fprintf (stderr, "'b iters' - Run Benchmark\n");
            fprintf (stderr, "'c num_lines words_file' - Create input file\n");
            fprintf (stderr, "'p' - Print Help\n");
            fprintf (stderr, "'o file' - Output into format\n");
            fprintf (stderr, "'r file' - Read in a file\n");
            fprintf (stderr, "'s <1|2>' entity entity'  - Search for entity match\n");
            fprintf (stderr, "'t <file | dir>' Ingest file\n");
            fprintf (stderr, "'x Exit\n");
            fprintf (stderr, "?\n");

            fgets(input_buffer,256,stdin);

            p2 = strtok(input_buffer, s);
            argc=1;

            while (p2 != NULL)
            {
                argv[argc++] = p2;
                p2 = strtok(NULL, " ");
            }
            argv[argc] = 0;

            if (strncmp(argv[1],"b",1) == 0) c='b'; else
            if (strncmp(argv[1],"c",1) == 0) c='c'; else
            if (strncmp(argv[1],"h",1) == 0) c='h'; else
            if (strncmp(argv[1],"o",1) == 0) c='o'; else
            if (strncmp(argv[1],"p",1) == 0) c='p'; else
            if (strncmp(argv[1],"r",1) == 0) c='r'; else
            if (strncmp(argv[1],"s",1) == 0) c='s'; else
            if (strncmp(argv[1],"t",1) == 0) c='t'; else
            if (strncmp(argv[1],"x",1) == 0) c='x'; else
               printf("Invalid option \n");
    }

    if (daemon == 0) c = getopt (argc, argv, "bcdhorstx:");

    switch (c)
    {
        case 'b':
            //  Benchmark a random search
            //  Run Benchmark: Usage -b iters
            //
            if (argc == 3)
            {
                if (sscanf (argv[2], "%d", &iters) != 1)
                {
                    printf ("Error %s is not an integer\n", argv[2]);
                    exit (1);
                }
            } else
                printf("Benchmark: Usage:  -b iters\n");

            run_benchmark(iters);

            if (daemon == 0) return(0);
            break;

        case 'c':
            // Create a new input words file comprising n lines of random alpha numeric strings
            // Usage -c inum_lines words_file
            //
            if (argc != 4)
            {
                printf ("Create input file: Usage -c inum_lines words_file\n");
                exit (1);
            }

            if (sscanf (argv[2], "%ld", &intvar) != 1)
            {
                printf ("Error - %s is not an integer\n", argv[2]);
                exit (1);
            }
            argv[3]=strtok(argv[3], "\n");  // Remove trailing CR

            create_outut_file (intvar,argv[3]);

            if (daemon == 0) return(0);
            break;

        case 'd':
            // Daemon mode
            daemon=1;
            break;

        case 'h':
            //Print Help 
            fprintf (stderr, "Run Benchmark: Usage -b iters\n");
            fprintf (stderr, "Create input file: Usage -c num_lines words_file\n");
            fprintf (stderr, "Daemon Mode: Usage -d\n");
            fprintf (stderr, "Print Help\n");
            fprintf (stderr, "Output into format: Usage -o file\n");
            fprintf (stderr, "Read in a file: Usage -r input_file\n");
            fprintf (stderr, "Search: Usage -s nth_order<1|2> entity1 entity2\n");
            fprintf (stderr, "Ingest Test file: Usage -t input_file | input_directory\n");

            if (daemon == 0) return(0);
            break;

        case 'o':
            // Output into format
            // Output into format: Usage -o input_file
            if (argc != 3)
            {
                printf ("Output into format: Usage -o input_file\n");
                exit (1);
            }
            argv[2]=strtok(argv[2], "\n");  // Remove trailing CR

            load_truths_and_write (argv[2]);

            if (daemon == 0) return(0);
            break;

        case 'r':
            // Read in a file
            // Read in a file: Usage -r input_file
            if (argc != 3)
            {
                printf ("Usage -r inputfile\n");
                exit (1);
            }
            argv[2]=strtok(argv[2], "\n");  // Remove trailing CR

            read_input_file(argv[2]);

            if (daemon == 0) return(0);
            break;

        case 's':
            // Search for an entity connection
            // Search: Usage -s nth_order<1|2> entity1 entity2
            if (argc != 5)
            {
                printf ("Usage -s nth_order<1|2> entity1 entity2\n");
                exit (1);
            }
            if (sscanf (argv[2], "%d", &order) != 1)
            {
                printf ("Error - %s is not an integer\n", argv[2]);
                exit (1);
            }
            if ((order != 1) && (order != 2))
            {
                printf ("Error - %s order should be 1 | 2\n", argv[2]);
                exit (1);
            }
            argv[4]=strtok(argv[4], "\n");  // Remove trailing CR

            search_for_words(argv[3], argv[4], order);

            if (daemon == 0) return(0);
            break;

        case 't':
            // Ingest Test file
            // Ingest Test file: Usage -t input_file | input_directory
            if (argc != 3)
            {
                printf ("Usage -t input_file | input_directory\n");
                exit (1);
            }
            argv[2]=strtok(argv[2], "\n");  // Remove trailing CR

            ingest_files(argv[2]);

            if (daemon == 0) return(0);
            break;

        case 'x':
            return(0);
            break;

        case '?':
                
                fprintf (stderr, "Invalid Option.\n\n");
                fprintf (stderr, "Run Benchmark: Usage -b iters\n");
                fprintf (stderr, "Create input file: Usage -c num_lines words_file\n");
                fprintf (stderr, "Daemon Mode: Usage -d\n");
                fprintf (stderr, "Print Help\n");
                fprintf (stderr, "Output into format: Usage -o file\n");
                fprintf (stderr, "Read in a file: Usage -r input_file\n");
                fprintf (stderr, "Search: Usage -s nth_order<1|2> entity1 entity2\n");
                fprintf (stderr, "Ingest Test file: Usage -t input_file | input_directory\n");
                exit (1);

        default:
                printf ("No options specified \n");
                printf ("Aborting \n");
                abort ();
        }

        if (argc == 1) {
           printf ("Error - no arguments specified \n");
           if (daemon == 0) return(0);
        }

   } // End while

   return(0);

}
/* vim: set ts=4 sw=4 tw=0 et : */
