#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int
main (int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf (stderr, "Usage %s \n\n dump data in json format\n", argv[0]);
		exit(4);
    }

    WORDS words;

    initialise (&words);
    if (read_all_files(words) == WORDS_SUCCESS)
    {
        dump_json(words,"test.json");
    }
    return 0;
}
/* vim: set ts=4 sw=4 tw=0 et : */
