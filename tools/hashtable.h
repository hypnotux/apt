#ifndef _WUTIL_H_
#define _WUTIL_H_

/*
 * Hashtables from WINGs. 
 * Alfredo K. Kojima (kojima@conectiva.com.br)
 */

typedef struct _HashTable HashTable;

/* DO NOT ACCESS THE CONTENTS OF THIS STRUCT */
typedef struct {
    void *table;
    void *nextItem;
    int index;
} HashEnumerator;


typedef struct {
    /* NULL is pointer hash */
    unsigned 	(*hash)(const void *);
    /* NULL is pointer compare */
    int		(*keyIsEqual)(const void *, const void *);
    /* NULL does nothing */
    void*	(*retainKey)(const void *);
    /* NULL does nothing */
    void	(*releaseKey)(const void *);    
} HashTableCallbacks;

    
    

HashTable *CreateHashTable(HashTableCallbacks callbacks);

void FreeHashTable(HashTable *table);

void ResetHashTable(HashTable *table);

void *HashGet(HashTable *table, const void *key);

/* put data in table, replacing already existing data and returning
 * the old value */
void *HashInsert(HashTable *table, void *key, void *data);

void HashRemove(HashTable *table, const void *key);

/* warning: do not manipulate the table while using these functions */
HashEnumerator EnumerateHashTable(HashTable *table);

void *NextHashEnumeratorItem(HashEnumerator *enumerator);

unsigned CountHashTable(HashTable *table);




/* some predefined callback sets */

extern const HashTableCallbacks IntHashCallbacks;
/* sizeof(keys) are <= sizeof(void*) */

extern const HashTableCallbacks StringHashCallbacks;
/* keys are strings. Strings will be copied with wstrdup() 
 * and freed with free() */

extern const HashTableCallbacks StringPointerHashCallbacks;
/* keys are strings, bug they are not copied */


#endif
