#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

struct entity
{
    char *entity_name;                     //This Entity
    int num_links;                         //Number of links to Sub Entities for this Entity
    long *links;                   //A dymanic array of long pointers to other Root Entities

};

void add_ent(struct entity **ents,int *no,char *name)
{

        (*no)++;
        *ents=realloc(*ents,((*no)+1)*sizeof(*ents[0]));
        struct entity *e = &((*ents)[*no]);
        e->entity_name = malloc(strlen(name)+1);
        if ( e->entity_name == 0 )
        {
                printf("Unable to allocate memory for name\n");
                exit(2);

        }
        strcpy(e->entity_name,name);
        e->num_links= -1;
        e->links=(long *)0;
}

void add_link(struct entity *e,int focal_root)
{
        void *temp=0;
        e->num_links++;
        temp=realloc(e->links,(e->num_links+1)*sizeof((e->links[0])));
        if (temp == 0) {
                  printf("Unable to allocate memory \n");
                  exit(1);
        }
        e->links=temp;
        e->links[e->num_links]=focal_root;
}

int find_entity(struct entity *e,int max,char *word)
{
        int i=0; 
        while (i<=max) {
                if (strcmp(word, e[i].entity_name) == 0) {
                        break;
                }
                else
                        i++;
        }
        return i;

}
int main(int argc,char *argv[]){
    char line[10000];
    char sub_word[100];
    FILE *in, *out;
    int line_length;
    int total_num_root_entities;
    int total_num_sub_entities;
    int num_entities;
    unsigned long *temp;
    int index;
    int ret;
    int roots,subs;
    int json=0;
        int pos, len;

struct entity *entities=0;


    in  = fopen("in.txt", "r");
    out = fopen("out.txt", "w");


    num_entities= -1;
    while(1==fscanf(in, "%[^\n]%n\n", line, &line_length)){//read one line
        char word[100];
        unsigned long focal_root_entity=0;  //Used when the Root Entity alredaay exists
        for(pos=0;pos < line_length-1 && 1==sscanf(line + pos, "%[^,]%*[,]%n", word, &len);pos+=len)
        {
            int i;

            printf("XX %s\n",line + pos);
            if (!json)       printf("The word is '%s' %d %d\n",word,pos,len);
            if (pos==0) {

                // First check whether the entry already exists:
                i=find_entity(entities,num_entities,word); 

                if (i > num_entities) {

                        // Initialise this Root Entity:
                        add_ent(&entities,&num_entities,word);

                        fprintf(out, "%s\n", word);

                        focal_root_entity=num_entities;
                        total_num_root_entities++;
                }
                else { //  If the root entity has been found:

                        focal_root_entity=i;
                }    // if (found == 0)

            }        // if pos == 0

            else {   // if pos !=0

                    unsigned long sub_entity=0;
                    //If this is a Sub Entity:

                    //First check whether the entity already exists :
                    sub_entity=find_entity(entities,num_entities,word);
                    if (sub_entity > num_entities) {

                            //Initialise this Sub Entity:
                            add_ent(&entities,&num_entities,word);

                            total_num_sub_entities++;
                            sub_entity=num_entities;

                    } //End found==0

                    // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
                    add_link(&entities[focal_root_entity],sub_entity);
                    add_link(&entities[sub_entity],focal_root_entity);

                    // Echo the word to the o/p file :
                    fprintf(out, "%s\n", word);

               } // End pos!=0

        } // End for pos


// Move to Next Root Entity
    if (!json)       printf("The word is '%s' %d %d\n",word,pos,len);

    }
    fclose(out);
    fclose(in);


// Now print out the entire set of Entities:



    if ( json ) printf("[\n");
    {
        int i;
        roots=subs=0;
        for (i=0;i<=num_entities;i++) 
        {
            int j;
                if (entities[i].num_links >= 0) {
                        if ( ! json ) printf("Root Entity '%s' discovered with %d sub links\n", entities[i].entity_name,entities[i].num_links);
                        roots++;

                        for (j=0; j<=entities[i].num_links;j++)  {
                                if ((( i > 0 )||(j>0)) && (json)) printf(",");
                                if ( ! json ) printf("Sub Entity is %s\n",entities[entities[i].links[j]].entity_name);
                                if ( json ) printf ("{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",entities[i].entity_name,entities[entities[i].links[j]].entity_name);
                                subs++;
                        }
                        printf("\n");

                }

    }
    }
    if ( json ) printf ("]\n");

    if ( !json )
    {
        printf("The number of root entities found were %d and the number of subs found were %d\n",roots,subs);
        printf("The total number of Entities are %d\n",num_entities);
        printf("The total number of Root Entities are %d\n",total_num_root_entities);
        printf("The total number of Sub Entities are %d\n",total_num_sub_entities);
    }






return(0);
// fwrite the array of structs out to save them:
    out = fopen("entities.bin","wb");
    ret=fwrite(entities,sizeof(entities),1,out);
    fclose(out);

// fread the array of structs in test:
    in = fopen("entities.bin","rb");
    ret=fread(entities,sizeof(entities),1,in);
    fclose(in);

    return 0;
}
