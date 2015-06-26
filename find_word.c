#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int
main (int argc, char *argv[])
{
    WORDS w;

    if (argc < 3)
    {
        fprintf (stderr,
                "Usage %s file word\n find word in file after loading into the hash\n",
                argv[0]);
        exit (1);
    }
	initialise(&w);
    WORDS_STAT ret = load (w, argv[1], TRUTHS);
	if ( ret == WORDS_SUCCESS)
	{
    entity *e = find_word (w, argv[2]);

    if (e)
    {
        printf ("found %s %s\n", e->name, word_type_str (e->type));
        exit (0);
    }
	}
	else
	{
		fprintf(stderr,"Failed to load data");
	}
    exit (1);
}
