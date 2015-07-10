#include <stdio.h>

#define ENTITY_PAD 0
#define LINK_PAD 0

#include "words.h"

int main(int argc, char *argv[])
{
	printf("#define ENTITY_PAD %ld\n",sizeof(struct entity));
	printf("#define LINK_PAD %ld\n",sizeof(struct link));
	return 0;
}
/* vim: set ts=4 sw=4 tw=0 et : */
