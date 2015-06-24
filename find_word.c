#include <stdio.h>
#include <stdlib.h>
#include "words.h"

int main(int argc, char *argv[])
{
	WORDS w;
	WORDS_STAT ret = load (&w, argv[1],TRUTHS);

	entity *e=find_word (w, argv[2]); 

	if (e )
	{
		printf("found %s %s\n",e->name,word_type_str(e->type));
		exit(0);
	}
		exit(1);
}
