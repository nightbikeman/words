#include "words.h"

const char *
word_type_str (WORD_TYPE type )
{
    switch (type)
    {
    case UNKNOWN:
        return "unknown ";
    case ACRONYMS:
        return "acronym ";
    case ADJECTIVES:
        return "adjective ";
    case ADVERBS:
        return "adverbs";
    case CIA_FACTBOOK:
        return "CIA_factbook ";
    case COMMON_STUFF:
        return "common_stuff ";
    case COMPOUND_WORDS:
        return "compound_word ";
    case CROSSWORDS:
        return "crosswords ";
    case FEMALE_NAMES:
        return "female_name ";
    case MALE_NAMES:
        return "male_name ";
    case NOUNS:
        return "noun ";
    case PLACES:
        return "place ";
    case SINGLE_WORDS:
        return "single_word ";
    case TRUTHS:
        return "truth ";
    case VERBS:
        return "verb ";
    case UK_PLACE:
        return "uk_place ";
    case UK_COUNTY:
        return "uk_county ";
    default:
        return "unspecified";
    }
}
