#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int main(int argc, char *argv[])
{
	int ret=1;
	if ( argc < 4 )
	{
		fprintf(stderr,"Usage %s file term1 term2\n\n Look for a coonection between term1 and term2 in file\n",argv[0]);
	}

	WORDS words;

	initialise(&words);
	if ( load(words,argv[1],TRUTHS) == WORDS_SUCCESS )
	{
		if ( word_search_r(words,32,1,argv[2],argv[3]) == WORDS_SUCCESS )
		{
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
