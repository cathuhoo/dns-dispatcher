#ifndef  __TRIE_H__
#define  __TRIE_H__

typedef int trieVal_t;

typedef struct trieNode{
    char key;
    trieVal_t value;
    struct trieNode *next;
    struct trieNode *prev;
    struct trieNode * parent;
    struct trieNode *children;
} trieNode_t;

trieNode_t * TrieSearch(trieNode_t * root, const char *key);
void TrieTravel( trieNode_t * tree, char * prefix, int idx);
void TrieRemove(trieNode_t ** root, char * key);

#define ENDCHAR '\0'
#define WILDCAST '*'

#endif
