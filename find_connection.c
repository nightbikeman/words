#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int main(int argc, char *argv[])
{
	int ret=1;
    struct chain chain;
    chain.entity=NULL;
	if ( argc < 4 )
	{
		fprintf(stderr,"Usage %s file term1 term2\n\n Look for a coonection between term1 and term2 in file\n",argv[0]);
	}

	WORDS words;

	initialise(&words);
	if ( load(words,argv[1],TRUTHS) == WORDS_SUCCESS )
	{
		if ( word_search_r(words,64,argv[2],argv[3],&chain) == WORDS_SUCCESS )
		{
            fdump_chain_json(&chain,stdout);
			printf("Found\n");
			ret=0;
		}
		else
		{
			printf("Not Found\n");
		}
	}
	exit(ret);
}
/* vim: set ts=4 sw=4 tw=0 et : */
