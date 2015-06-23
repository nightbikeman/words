
#ifndef WORDS_H
#define WORDS_H

#define WORDS_SUCCESS 0
#define WORDS_FAIL 1

#define STAGE 100000
#define INITIAL_HASH 1000000

#define QUICK 1
#define FULL 0

#define VERBOSE 0

#define CORES 1

#define BUFFER_LEN 100000000

typedef int WORDS_STAT;
typedef struct words
{
} WORDPTR;
typedef WORDPTR *WORDS;

typedef enum
{ 
	UNKNOWN = 0,
    ACRONYMS = 1,
    ADJECTIVES = 2,
    ADVERBS = 4,
    CIA_FACTBOOK = 8,
    COMMON_STUFF = 16,
    COMPOUND_WORDS = 32,
    CROSSWORDS = 64,
    FEMALE_NAMES = 128,
    MALE_NAMES = 256,
    NOUNS = 512,
    PLACES = 1024,
    SINGLE_WORDS = 2048,
    TRUTHS = 4096,
    VERBS = 8192
} WORD_TYPE;

typedef struct entity
{
    char *name;
	WORD_TYPE type;
    int num_links;              //Number of links to Sub Entities for this Entity
    struct entity **links;      //A dymanic array of long pointers to other Root Entities
} entity;


const char *word_type_str(WORD_TYPE);
WORDS_STAT load (WORDS * words, const char *filename, const WORD_TYPE type);
WORDS_STAT save (const WORDS words, char *filename);
WORDS_STAT search (const WORDS words, char *word);
entity    *find_word (const WORDS words, char *word);
WORD_TYPE  cat_word (const WORDS words, char *word);
WORDS_STAT dump (const WORDS words);
WORDS_STAT dump_json (const WORDS words);

#endif
