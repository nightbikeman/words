#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int
main (int argc, char *argv[])
{
    int ret = 1;
    if (argc < 2)
    {
        fprintf (stderr, "Usage %s word... \n\n Attempt to classify word\n", argv[0]);
		exit(1);
    }

    WORDS words;

    initialise (&words);
    int c;
    if (read_all_files(words) == WORDS_SUCCESS)
    {
        for (c = 1; c < argc; c++)
        {
			entity *e=NULL;
            if ((e=find_word (words, argv[c])) != NULL)
            {
				printf ("Found %s %s (%d)\n", e->name, word_type_str (e->type),e->type);
                ret = 0;
            }
            else
            {
                printf ("Not Found %s\n",argv[c]);
            }
        }
    }
    exit (ret);
}
