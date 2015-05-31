#include <stdio.h>

int main()
{
        
char word[1000];
int pos=0;
int len=0;
char *line="chinchilla";
int y=sscanf(line + pos, "%[^,]%*[,]%n", word, &len);

printf("y=%d\nlen=%d\nword=%s\n",y,len,word);

}
