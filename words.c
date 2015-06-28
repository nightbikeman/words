#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "words.h"

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
            strcat(result, "adverbs");
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
read_files (WORDS w, int number)
{
    int j;
    for (j = 0; j < number; j++)
    {
        if (load (w, books[j].filename, books[j].type) == WORDS_FAIL)
			return WORDS_FAIL;
    }
	return WORDS_SUCCESS;
}
