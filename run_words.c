#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int main(int argc, const char *argv[])
{
    if (argc > 1 )
    {
        WORDS words;
        if (load(&words,argv[1]) == WORDS_SUCCESS )
        {
               dump_json(words);
        }
    }
    else
    {
        fprintf(stderr,"Usage %s words_file\n",argv[0]);
        exit(1);
    }
}
