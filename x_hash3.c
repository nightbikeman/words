#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "dhash.h"

typedef struct entity
{
    char *name;
    int num_links;                 //Number of links to Sub Entities for this Entity
    struct entity **links;                   //A dymanic array of long pointers to other Root Entities
} entity;

static void delete_callback(hash_entry_t *entry,
                            hash_destroy_enum type,
                            void *pvt)
{
    if (entry->value.type == HASH_VALUE_PTR) free(entry->value.ptr);
}

static entity *add_ent(char *name, hash_table_t *table)
{

        entity *e = malloc(sizeof(*e));
        if ( e == NULL )
        {
            perror("allocating entity");
            exit(1);
        }
        e->name=strdup(name);
        e->num_links= -1;
        e->links=NULL;

        // Add a entry in the hash table for searching
        /* Enter a key named "My Data" and specify it's value as a pointer to my_data */
        hash_key_t key;
        hash_value_t value;
        key.type = HASH_KEY_STRING;
        key.str = strdup(name);
        value.type = HASH_VALUE_PTR;
        value.ptr = e;

        int error;
        if ((error = hash_enter(table, &key, &value)) != HASH_SUCCESS) {
            fprintf(stderr, "cannot add to table \"%s\" (%s)\n", key.str, hash_error_string(error));
            exit(1);
        }
        free(key.str);
        return e;
}

static void add_link(struct entity *e,entity *focal_root)
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

static entity *find_entity(char *word,hash_table_t *table)
{
    entity *e=NULL;
    /* Lookup the key named "My Data" */
    hash_key_t key;
    hash_value_t value;
    key.type = HASH_KEY_STRING;
    key.str = strdup(word);
    int error;
    if ((error = hash_lookup(table, &key, &value)) == HASH_SUCCESS) 
    {
        e=value.ptr;
    }
    free(key.str);

    return e;

}
int main(int argc,char *argv[]){
    char line[1000000];
    FILE *in, *out;
    int line_length;
    int total_num_root_entities;
    int total_num_sub_entities;
    long num_entities;
    unsigned long *temp;
    int index;
    int ret;
    int roots,subs;
    int json=1;
    struct entity *entities=0;

    static hash_table_t *table = NULL;

    /* Create a hash table */
    int error;
    error = hash_create(100000 , &table, delete_callback,  NULL);
    if (error != HASH_SUCCESS) {
        fprintf(stderr, "cannot create hash table (%s)\n", hash_error_string(error));
        return error;
    }


    in  = fopen("in.txt", "r");
    out = fopen("out.txt", "w");


    num_entities= -1;
    while(1==fscanf(in, "%[^\n]%n\n", line, &line_length)){//read one line
        char *word;
        entity *focal_root_entity=NULL;  //Used when the Root Entity already exists
        char *ptr;
        {
            char *root=strtok_r(line,",",&ptr);
            entity *i=NULL;

            // First check whether the entry already exists:
            i=find_entity(root,table); 

            if (i == NULL ) {

                    // Initialise this Root Entity:
                    focal_root_entity=add_ent(root,table);

                    fprintf(out, "%s\n", root);

                    total_num_root_entities++;
            }
            else { //  If the root entity has been found:

                    focal_root_entity=i;
            }    // if (found == 0)
        }

        for(;word=strtok_r(NULL,",",&ptr);) 
        {
            entity *sub_entity;

            //First check whether the entity already exists :
            sub_entity=find_entity(word,table);
            if (sub_entity == NULL) {

                    //Initialise this Sub Entity:
                    sub_entity=add_ent(word,table);

                    total_num_sub_entities++;

            } //End found==0

            // Now link the Sub Entity to the focal_root_entity using the index of the entity arrays :
            add_link(focal_root_entity,sub_entity);
            add_link(sub_entity,focal_root_entity);

            // Echo the word to the o/p file :
            fprintf(out, "%s\n", word);

        } // End for pos
    }
    fclose(out);
    fclose(in);


// Now print out the entire set of Entities:



    if ( json ) printf("[\n");
    {
        /* Visit each entry using iterator object */
        struct hash_iter_context_t *iter;
        iter = new_hash_iter_context(table);
        hash_entry_t *entry;
        int i=0;
        while ((entry = iter->next(iter)) != NULL) {
            struct entity *data = (struct entity *) entry->value.ptr;
            int j;

            for (j=0; j<=data->num_links;j++)  {
                    if (( i > 0 ) && (json)) printf(",");
                    if ( ! json ) printf("Sub Entity is %s\n",data->links[j]->name);
                    if ( json ) printf ("{\n   \"source\" : \"%s\",\n   \"target\" : \"%s\",\n   \"type\" : \"suit\"\n}\n",data->name,data->links[j]->name);
                    subs++;
                    i=1;
            }
        }
        free(iter);
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
