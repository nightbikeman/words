
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

#define acronyms       0
#define adjectives     1
#define adverbs        2
#define cia_factbook   3
#define common_stuff   4
#define compound_words 5
#define crosswords     6
#define female_names   7
#define male_names     8
#define nouns          9
#define places        10
#define single_words  11
#define truths        12
#define verbs         13
#define uk_places     14
#define uk_county     15
#define learnt_words  16
#define MAX_WORDS     17

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
    VERBS = 8192,
    UK_PLACE = 16384,
    UK_COUNTY = 32768,
    LEARNT_WORDS = 65536
} WORD_TYPE;

struct link;
typedef struct entity
{
	int flag;
    char *name;
	WORD_TYPE type;
    int num_links;              //Number of links to Sub Entities for this Entity
    struct link *links;      //A dymanic array of long pointers to other Root Entities
} entity;

struct link
{
    struct entity *entity;      // A pointer to the word we are related to
	struct entity *relation;    // The type of relation
	int weight;                // The weight of that relationship;
};

struct chain
{
    int length;
    struct entity **entity;      // a list of the entities in the chain
};
typedef struct book
{
    char *filename;
    char *name;
    WORD_TYPE type;
} BOOK;


const char *word_type_str(WORD_TYPE);
WORDS_STAT initialise (WORDS * words);
WORDS_STAT load (WORDS words, const char *filename, const WORD_TYPE type);
#define add_word(a,b,c) ((e_add_word(a,b,c) == NULL ) ? WORDS_FAIL : WORDS_SUCCESS)
struct entity* e_add_word (WORDS w, char *word, const WORD_TYPE type);
WORDS_STAT add_linked_word (WORDS words, char *word, const WORD_TYPE type, struct entity *root);
WORDS_STAT save (const WORDS words, char *filename);
WORDS_STAT search (const WORDS words, char *word);
entity    *find_word (const WORDS words, char *word);
WORD_TYPE  cat_word (const WORDS words, char *word);
WORDS_STAT dump (const WORDS words);
WORDS_STAT dump_json (const WORDS words, char *filename);
WORDS_STAT dump_formatted (const WORDS w);
WORDS_STAT dump_txt (FILE *out,const WORDS w);
WORDS_STAT word_search (const WORDS w, long nth_order, long quick, char *entity1, char *entity2);
WORDS_STAT word_search_r (const WORDS w, long nth_order, char *entity1, char *entity2, struct chain *chain);
int create_in_txt (int num_lines, char *file);
WORDS_STAT read_files (WORDS w, int start, int number);
#define read_file(a,b) read_files(a,b,b)
#define read_all_files(a) read_files(a,0,MAX_WORDS)
WORDS_STAT add_link(struct entity *e, entity * focal_root, int weight, struct entity *relation);


// dumps a json descibing the array of links pased to it to the file handle
WORDS_STAT fdump_chain_json (struct chain *chain,FILE *out);
/** Returns WORDS_SUCCESS if it can delete a link between the two words */
WORDS_STAT delete_link(struct entity *e1,struct entity *e2);


/** Returns WORDS_SUCCESS if there is a link between the two words */
WORDS_STAT is_link(struct entity *e1,struct entity *e2);

/* updates the weight of an entity */
WORDS_STAT update_weight(struct link *e1,int weight);
WORDS_STAT stat_link(struct entity *e1,int depth);


#endif
/* vim: set ts=4 sw=4 tw=0 et : */
