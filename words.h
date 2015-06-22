
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
typedef void *WORDS;

WORDS_STAT load (WORDS * words, const char *filename);
WORDS_STAT save (const WORDS words, char *filename);
WORDS_STAT search (const WORDS words, char *word);
WORDS_STAT find_word (const WORDS words, char *word);
WORDS_STAT dump (const WORDS words);
WORDS_STAT dump_json (const WORDS words);

#endif
