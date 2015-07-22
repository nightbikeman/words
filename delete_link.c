#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int
main (int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf (stderr, "Usage %s word1 word2 \n\n delete link between two words\n", argv[0]);
		exit(4);
    }

    WORDS words;

    initialise (&words);
    if (read_all_files(words) == WORDS_SUCCESS)
    {
			entity *w[2]={NULL,NULL};
            if ((w[0]=find_word (words, argv[1])) == NULL)
            {
				fprintf(stderr,"Failed to find word \"%s\"\n",argv[1]);
                exit(2);
            }
            if ((w[1]=find_word (words, argv[2])) == NULL)
            {
				fprintf(stderr,"Failed to find word \"%s\"\n",argv[2]);
                exit(3);
            }

            if (delete_link(w[0],w[1]) == WORDS_SUCCESS ) 
                return 0;
    }
    return 1;
}
/* vim: set ts=4 sw=4 tw=0 et : */
