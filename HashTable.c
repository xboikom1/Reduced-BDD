#include <stdlib.h>

typedef struct Node Node;

typedef struct HashTableElem {
    int level;
    Node *low;
    Node *high;
    Node *node;
} HashTableElem;

typedef struct HashTable {
    HashTableElem** elems;
    int size;
} HashTable;

HashTable* create_table(int size) {
    HashTable *table = malloc(sizeof(HashTable));
    table->size = size;
    table->elems = (HashTableElem**)calloc(table->size, sizeof(HashTableElem*));
    return table;
}

unsigned int hash_func(int level, const Node* low, const Node* high, int size) {    //djb2 hash function
    unsigned long hash = 5381;
    hash = hash * 33 + level;
    hash = hash * 33 + (unsigned long)(uintptr_t)low;
    hash = hash * 33 + (unsigned long)(uintptr_t)high;
    return hash % size;
}