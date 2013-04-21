/* trie.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define MAX_WORD 64

char * strReverse(char *srcStr, char* dstStr)
{
        int i, length;
            if (srcStr == NULL || dstStr == NULL)
                        return NULL;

                length = strlen(srcStr);
                    for ( i =0; i < length; i++)
                                dstStr[i] = srcStr[length - i - 1 ];
                        dstStr[length] = '\0';

                            return dstStr;
}

trieNode_t *TrieCreateNode( char key, trieVal_t data)
{
    trieNode_t * node = NULL;
    node = (trieNode_t *) malloc(sizeof(trieNode_t));

    if ( NULL == node )
    {
        printf("Malloca failed\n");
        return node;
    }

    node->key = key;
    node->next = NULL;
    node->children = NULL;
    node->value = data;
    node->prev = NULL;
    node->parent = NULL;
    return node;
}

trieNode_t * TrieInit( )
{
    return TrieCreateNode( ENDCHAR, 0xffffffff);
}

void TrieAdd (trieNode_t ** root, char *key, trieVal_t data)
{ 
    trieNode_t * pTrav = NULL;

    if( NULL == * root) 
    {
        printf("NULL Tree \n");
        return ;
    }
    #ifdef DEBUG
        printf("Now begin to insert key %s:\n", key);
    #endif
    pTrav = (*root) -> children;

    if (TrieSearch(pTrav, key))
    {
        printf("Duplicated !\n");
        return ;
    }

    if(pTrav == NULL )
    {
        /*first node */
        #ifdef DEBUG
            printf("First node\n");
        #endif

        for ( pTrav = * root ; *key; pTrav = pTrav -> children)
        {
            pTrav -> children = TrieCreateNode ( *key , 0xffffffff);
            pTrav -> children->parent = pTrav;
            #ifdef DEBUG
                printf("\t Inserting: %c \n", pTrav->children->key);
            #endif
            key ++;
        }

        pTrav->children = TrieCreateNode (ENDCHAR, data);
        pTrav->children->parent = pTrav;
        #ifdef DEBUG
            printf("\t Inserting: %c(END)\n", pTrav->children->key);
        #endif
        return;
    }

    //search in the children chain for prefix
    while ( *key != ENDCHAR )
    {
        if( *key == pTrav->key)
        {
            key ++;
            #ifdef DEBUG
                printf("\t Traversing child: %c \n", pTrav->children->key);
            #endif
            pTrav = pTrav ->children;
        }
        else
            break;
    }

    //seach the sibling
    while (pTrav -> next) 
    {
        if( *key == pTrav->next->key) //find the matched char, and add it as a new child
        {
            key++;
            TrieAdd( &(pTrav->next), key, data);
            return ;
        }
        //otherwise, continue to the last sibling
        pTrav = pTrav -> next;
    }

    //Now, create a new node, and add it to the rightmost of siblings
    pTrav->next = TrieCreateNode( *key, 0xffffffff);
    pTrav->next->parent = pTrav->parent;
    pTrav->next->prev = pTrav;

    #ifdef DEBUG
        printf("\t Inserting %c as neighbour of %c \n", pTrav->next->key,pTrav->key);
    #endif

    key++;

    //Now create a new chain for the rest of string 
    for(pTrav = pTrav->next; *key; pTrav = pTrav->children)
    {
        pTrav->children = TrieCreateNode(*key, 0xffffffff);
        pTrav->children->parent = pTrav;
        #ifdef DEBUG
            printf("\t Inserting: %c \n", pTrav->children->key);
        #endif
        key++;
    }

    pTrav->children = TrieCreateNode(ENDCHAR, data);
    pTrav->children->parent = pTrav;
    #ifdef DEBUG
        printf("\t Inserting: %c\n",pTrav->children->key);
    #endif
    return;
}

trieNode_t* TrieSearch(trieNode_t *root, const char *key)
{
    trieNode_t *level = root;
    trieNode_t *pPtr = NULL;

    int lvl = 0;
    while(1)
    {
        trieNode_t * found = NULL;
        trieNode_t * curr;

        for ( curr = level ; curr != NULL; curr = curr->next )
        {
            if( curr->key == *key)
            {
                found = curr;
                lvl ++;
                break;
            }
            else if( curr->key == WILDCAST ) 
                // WILDCAST(*) will match any characters
            {
                return curr-> children; 
            }
        }

        if (found == NULL )
            return NULL;
        
        if( *key == ENDCHAR )
        {
            pPtr = curr;
            return pPtr;
        }

        level = found -> children ;
        key ++;
    } //while
}

void TrieTravelE( trieNode_t * tree)
{
    char buffer[MAX_WORD];
    int index=0;

    if (tree == NULL) 
        return;

    memset(buffer, 0, sizeof(buffer));
    TrieTravel(tree->children, buffer, index);

}

void TrieTravel( trieNode_t * tree, char * prefix, int idx)
{
    //trieNode_t *tPtr;
    //char string[MAX_WORD];

    if( tree  == NULL)
        return ; 
    if ( tree-> key != ENDCHAR)
    {
        *(prefix + idx)  = tree->key; 
        //printf("%s%c", tree->key);
    }
    else 
    {
        *(prefix + idx)  = ENDCHAR; 
        printf("%s$:%d\n", prefix, tree->value);
        //return ;
    }

    if ( tree->children)
        TrieTravel( tree->children, prefix, idx+1);
    if ( tree->next)
        TrieTravel( tree->next, prefix,idx);

}
void TrieRemove(trieNode_t ** root, char * key)
{
    trieNode_t * tPtr = NULL;
    trieNode_t * tmp  = NULL;

    if( NULL == * root || NULL == key)
        return ;
    
    tPtr = TrieSearch( (*root)->children, key);
    
    if( NULL == tPtr)
    {
        printf("Key not found in the trie \n");
        return ;
    }

    while(1)
    {
        if( tPtr->prev && tPtr->next )
        {
            tmp = tPtr;
            tPtr->next->prev = tPtr->prev;
            tPtr->prev->next = tPtr->next;
            free(tmp);
            break;
        }
        else if ( tPtr->prev && !(tPtr->next))
        {
            tmp = tPtr;
            tPtr->prev->next = NULL;
            free(tmp);
            break;
        }
        else if (!(tPtr->prev) && tPtr->next)
        {
            tmp = tPtr;
            tPtr->parent->children = tPtr->next;
            free(tmp);
            break;
        }
        else // prev == NUL && next == NULL
        {
            tmp=tPtr;
            tPtr = tPtr->parent;
            free(tmp);
           
        }
    }

}
int TrieLoad(trieNode_t * tree, char * file_name)
{
    char line[MAX_WORD];
    char * str, *token, *tofree;
    FILE * fp;
    char * name;
    char r_name[MAX_WORD];
    int  val, set, tmp;

    if ( (fp=fopen(file_name, "r") )==NULL)
    {
        printf("ERROR on open file :%s \n", file_name);
        return -1;
    }
    while( fgets(line, MAX_WORD, fp) != NULL )
    {
        set = 0x0;
        printf("input:%s\n", line);
        if (line[0] == '#' || line[0] =='\0' || line[0] == ';')
            continue;
        tofree = str = strdup(line);
        name = strsep(&str, ":"); 
        
        while ( (token = strsep(&str,":")) != NULL)
        {
            #ifdef DEBUG
                printf("token:%s\n", token);
            #endif
            val = atoi(token );
            tmp = (1<< val); 
            set = set | tmp;
            #ifdef DEBUG
                printf("set=:%d\n", set);
            #endif
        } 
        strReverse(name, r_name);
        TrieAdd(&tree, r_name, set); 
        free(tofree);
    }
    fclose(fp);
    return 0;
}

