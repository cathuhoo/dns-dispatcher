#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "mystring.h"

#define DEFAULT_VALUE 0x0


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
    //return TrieCreateNode( ENDCHAR, 0xffffffff);
    return TrieCreateNode( ENDCHAR, DEFAULT_VALUE);
}

void TrieAdd (trieNode_t ** root, char *key, trieVal_t data)
{ 
    trieNode_t * pTrav = NULL, *pFound;

    if( NULL == * root) 
    {
        printf("NULL Tree \n");
        return ;
    }
    pTrav = (*root) -> children;

    if (NULL != (pFound = TrieSearch(pTrav, key)))
    {
        //printf("Duplicated !\n");
        // Unionify 
        pFound->value |= data ;
        return ; 
    }

    if(pTrav == NULL )
    {
        /*first node */
        for ( pTrav = * root ; *key; pTrav = pTrav -> children)
        {
            //pTrav -> children = TrieCreateNode ( *key , 0xffffffff);
            pTrav -> children = TrieCreateNode ( *key , DEFAULT_VALUE);
            pTrav -> children->parent = pTrav;
            #ifdef DEBUG
                //printf("\t Inserting: %c \n", pTrav->children->key);
            #endif
            key ++;
        }

        pTrav->children = TrieCreateNode (ENDCHAR, data);
        pTrav->children->parent = pTrav;
        return;
    }

    //search in the children chain for prefix
    while ( *key != ENDCHAR )
    {
        if( *key == pTrav->key)
        {
            key ++;
            #ifdef DEBUG
                //printf("\t Traversing child: %c \n", pTrav->children->key);
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
    //pTrav->next = TrieCreateNode( *key, 0xffffffff);
    pTrav->next = TrieCreateNode( *key, DEFAULT_VALUE);
    pTrav->next->parent = pTrav->parent;
    pTrav->next->prev = pTrav;

    #ifdef DEBUG
        //printf("\t Inserting %c as neighbour of %c \n", pTrav->next->key,pTrav->key);
    #endif

    key++;

    //Now create a new chain for the rest of string 
    for(pTrav = pTrav->next; *key; pTrav = pTrav->children)
    {
        //pTrav->children = TrieCreateNode(*key, 0xffffffff);
        pTrav->children = TrieCreateNode(*key, DEFAULT_VALUE);
        pTrav->children->parent = pTrav;
        #ifdef DEBUG
            //printf("\t Inserting: %c \n", pTrav->children->key);
        #endif
        key++;
    }

    pTrav->children = TrieCreateNode(ENDCHAR, data);
    pTrav->children->parent = pTrav;
    #ifdef DEBUG
        //printf("\t Inserting: %c\n",pTrav->children->key);
    #endif
    return;
}

// search reverse of key, from trie root
trieVal_t * trie_search(trieNode_t * root, const char *key)
{
    trieNode_t * pt = NULL;
    char str_r[MAX_WORD], str_trim[MAX_WORD];

    if ( root == NULL)
        return NULL;

    strtrim2(str_trim, MAX_WORD, key);

    strReverse(str_trim, str_r);

    pt = TrieSearch(root->children, str_r);

    if (pt)
    {
        //fprintf(stdout, "pt:%lx\n", pt);
        //fprintf(stdout, "*pt->value:%lx\n", pt->value);
        return &pt->value;
    }
    else
    {
        pt = TrieSearch(root->children, "*");
        if (pt)
            return &pt->value;
    }
    return NULL;
    
}
//search from the children of trie root
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
            else if( curr->key == WILDCARD ) 
                // WILDCARD(*) will match any characters
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
        printf("%s$:0x%lx\n", prefix, tree->value);
        //return ;
    }

    if ( tree->children)
        TrieTravel( tree->children, prefix, idx+1);
    if ( tree->next)
        TrieTravel( tree->next, prefix,idx);

}
void trie_free( trieNode_t * tree)
{

    if( tree  == NULL)
        return ; 

    if ( tree->children)
        trie_free( tree->children); 
    if ( tree->next)
        trie_free( tree->next);

    if(tree->parent)
        tree->parent->children = NULL; 
    free(tree);
}
int trie_setall(trieNode_t * tree, RuleSet set)
{
    if( tree  == NULL)
        return 0 ; 

    if ( tree-> key == ENDCHAR)
    {
        tree->value |= set;
    }

    if ( tree->children)
        trie_setall( tree->children, set);
    if ( tree->next)
        trie_setall( tree->next, set);
    return 0;
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
int TrieLoad(trieNode_t * tree, char * file_name, int rule_no)
{
    char line[MAX_WORD];
    char * str; 
    FILE * fp;
    char r_name[MAX_WORD];
    RuleSet set;

    if ( (fp=fopen(file_name, "r") )==NULL)
    {
        printf("ERROR on open file :%s \n", file_name);
        return -1;
    }
    while( fgets(line, MAX_WORD, fp) != NULL )
    {
        set = 0x0;
        str=strtrim(line);

        if (str[0] == '#' || str[0] =='\0' || str[0] == ';')
            continue;
        set = 1<< rule_no;

        strReverse(str, r_name);
        TrieAdd(&tree, r_name, set); 
        //free(tofree);
    }
    fclose(fp);

    return 0;
}


/*
// To test the code related to trie
//
int main ( int argc , char * argv[])
{
    trieNode_t *tree;
    trieNode_t *srch; 
    char * str, tmpStr[MAX_WORD], str_r[MAX_WORD];
    int i;

    //tree = TrieCreateNode( ENDCHAR, 0xffffffff);

    trieNode_t * tree_ip, *tree_domain;
    tree_ip = TrieInit();
    printf("Load domain: blacklist_domain.txt\n");
    TrieLoad( tree_ip, "blacklist_domain.txt", 1);
    printf("Load domain: video.txt\n");
    TrieLoad( tree_ip, "video.txt", 2);
    printf("Trie Travel:\n");
    TrieTravelE(tree_ip);
    for ( i=1; i < argc ; i++) 
    {
        str = argv[i];
        strReverse(str, str_r);
        srch = TrieSearch ( tree_ip->children, str_r);
        //srch = TrieSearch ( tree, str);
        if (srch == NULL)
        {
            printf("%s not found \n", str);
        }
        else
        {
            printf("%s found, value:0x%lx \n", str, srch->value);
        }
    }
}
*/
