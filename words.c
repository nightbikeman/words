#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "words.h"

const BOOK books[] = {
        {"acronyms.txt", "acronyms", ACRONYMS},
        {"adjectives.txt", "adjectives", ADJECTIVES},
        {"adverbs.txt", "adverbs", ADVERBS},
        {"cia_factbook.txt", "cia_factbook", CIA_FACTBOOK},
        {"common_stuff.txt", "common_stuff", COMMON_STUFF},
        {"compound_words.txt", "compound_words", COMPOUND_WORDS},
        {"crosswords.txt", "crosswords", CROSSWORDS},
        {"female_names.txt", "female_names", FEMALE_NAMES},
        {"male_names.txt", "male_names", MALE_NAMES},
        {"nouns.txt", "nouns", NOUNS},
        {"places.txt", "places", PLACES},
        {"single_words.txt", "single_words", SINGLE_WORDS},
        {"truths.txt", "truths", TRUTHS},
        {"verbs.txt", "verbs", VERBS},
        {"uk_places", "verbs", UK_PLACE},
        {"uk_county", "verbs", UK_COUNTY},
        {"learnt_words", "learnt_words", LEARNT_WORDS},
    };

const char *
word_type_str (WORD_TYPE type)
{
    static char result[1024];
	result[0]=0;
    int j = 1;
    while (type != 0)
    {
		int g=type & j;
		if ( g )
		{
        switch (g)
        {
        case UNKNOWN:
            strcat (result, "unknown ");
			break;
        case ACRONYMS:
            strcat(result, "acronym ");
            break;
        case ADJECTIVES:
            strcat(result, "adjective ");
            break;
        case ADVERBS:
            strcat(result, "adverbs ");
            break;
        case CIA_FACTBOOK:
            strcat(result, "CIA_factbook ");
            break;
        case COMMON_STUFF:
            strcat(result, "common_stuff ");
            break;
        case COMPOUND_WORDS:
            strcat(result, "compound_word ");
            break;
        case CROSSWORDS:
            strcat(result, "crosswords ");
            break;
        case FEMALE_NAMES:
            strcat(result, "female_name ");
            break;
        case MALE_NAMES:
            strcat(result, "male_name ");
            break;
        case NOUNS:
            strcat(result, "noun ");
            break;
        case PLACES:
            strcat(result, "place ");
            break;
        case SINGLE_WORDS:
            strcat(result, "single_word ");
            break;
        case TRUTHS:
            strcat(result, "truth ");
            break;
        case VERBS:
            strcat(result, "verb ");
            break;
        case UK_PLACE:
            strcat(result, "uk_place ");
            break;
        case UK_COUNTY:
            strcat(result, "uk_county ");
            break;
        case LEARNT_WORDS:
            strcat(result, "learnt_words ");
            break;
        default:
            strcat(result, "unspecified ");
            break;
        }
        type &= ~j;
		assert(strlen(result) < sizeof(result));
		}
        j = j << 1;
    }
    return result;
}

WORDS_STAT
read_files (WORDS w, int start, int end)
{
    int j;
	char *data_dir="data/";
	if ( getenv("DATA_DIR") != NULL )
		data_dir=getenv("DATA_DIR");

    for (j = start; j < end; j++)
    {
		char filename[1024];
		sprintf(filename,"%s/%s",data_dir,books[j].filename);	
		assert(strlen(filename) < sizeof(filename));
        if (load (w, filename, books[j].type) == WORDS_FAIL)
		{
			if ( getenv("LOAD_FAIL_OK") == NULL )
				return WORDS_FAIL;
		}
    }
	return WORDS_SUCCESS;
}
/* vim: set ts=4 sw=4 tw=0 et : */
