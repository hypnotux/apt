#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hashtable.h"

#define INITIAL_CAPACITY	23


#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define inline inline
#else
# define inline
#endif


typedef struct HashItem {
    void *key;
    void *data;
    
    struct HashItem *next;	       /* collided item list */
} HashItem;


struct _HashTable {
    HashTableCallbacks callbacks;

    unsigned itemCount;
    unsigned size;		       /* table size */

    HashItem **table;
};




#define HASH(table, key)  (((table)->callbacks.hash ? \
	    (*(table)->callbacks.hash)(key) : hashPtr(key)) % (table)->size)

#define DUPKEY(table, key) ((table)->callbacks.retainKey ? \
		    (*(table)->callbacks.retainKey)(key) : (key))

#define RELKEY(table, key) if ((table)->callbacks.releaseKey) \
		    (*(table)->callbacks.releaseKey)(key)




static inline unsigned
hashString(const char *key)
{
    unsigned ret = 0;
    unsigned ctr = 0;

    while (*key) {
	ret ^= *(char*)key++ << ctr;
	ctr = (ctr + 1) % sizeof (char *);
    }
    
    return ret;
}



static inline unsigned
hashPtr(const void *key)
{
    return ((size_t)key / sizeof(char*));
}




static void
rellocateItem(HashTable *table, HashItem *item)
{
    unsigned h;

    h = HASH(table, item->key);

    item->next = table->table[h];
    table->table[h] = item;
}


static void
rebuildTable(HashTable *table)
{
    HashItem *next;
    HashItem **oldArray;
    int i;
    int oldSize;
    int newSize;

    oldArray = table->table;
    oldSize = table->size;

    newSize = table->size*2;

    table->table = malloc(sizeof(char*)*newSize);
    if (!table->table) {
	fprintf(stderr, "hashtable: out of memory while rebuilding hash table \n");
	table->table = oldArray;
	return;    
    }
    memset(table->table, 0, sizeof(char*)*newSize);
    table->size = newSize;
    
    for (i = 0; i < oldSize; i++) {
	while (oldArray[i]!=NULL) {
	    next = oldArray[i]->next;
	    rellocateItem(table, oldArray[i]);
	    oldArray[i] = next;
	}
    }
    free(oldArray);
}



HashTable*
CreateHashTable(HashTableCallbacks callbacks)
{
    HashTable *table;
    
    table = malloc(sizeof(HashTable));
    if (!table)
	    return NULL;
    memset(table, 0, sizeof(HashTable));
    
    table->callbacks = callbacks;
    
    table->size = INITIAL_CAPACITY;

    table->table = malloc(sizeof(HashItem*)*table->size);
    if (!table->table) {
	    table->table = NULL;
	    return NULL;
    }
    memset(table->table, 0, sizeof(HashItem*)*table->size);
    
    return table;
}


void
ResetHashTable(HashTable *table)
{
    HashItem *item, *tmp;
    int i;

    for (i = 0; i < table->size; i++) {
	item = table->table[i];
	while (item) {
	    tmp = item->next;
	    RELKEY(table, item);
	    free(item);
	    item = tmp;
	}
    }

    table->itemCount = 0;
    
    if (table->size > INITIAL_CAPACITY) {
	void *tmp = table->table;
	table->table = malloc(sizeof(HashItem*)*INITIAL_CAPACITY);
	if (!table->table) {
		table->table = tmp;
	} else {
		table->size = INITIAL_CAPACITY;
		free(tmp);
	}
    }
    memset(table->table, 0, sizeof(HashItem*)*table->size);
}


void
FreeHashTable(HashTable *table)
{
    HashItem *item, *tmp;
    int i;
    
    for (i = 0; i < table->size; i++) {
	item = table->table[i];
	while (item) {
	    tmp = item->next;
	    RELKEY(table, item->key);
	    free(item);
	    item = tmp;
	}
    }
    free(table->table);
    free(table);
}



void*
HashGet(HashTable *table, const void *key)
{
    unsigned h;
    HashItem *item;
    
    h = HASH(table, key);
    item = table->table[h];
    
    if (table->callbacks.keyIsEqual) {
	while (item) {
	    if ((*table->callbacks.keyIsEqual)(key, item->key)) {
		break;
	    }
	    item = item->next;
	}
    } else {
	while (item) {
	    if (key == item->key) {
		break;
	    }
	    item = item->next;
	}
    }
    if (item)
	return item->data;
    else
	return NULL;
}



void*
HashInsert(HashTable *table, void *key, void *data)
{
    unsigned h;
    HashItem *item;
    int replacing = 0;
    
    h = HASH(table, key);
    /* look for the entry */
    item = table->table[h];
    if (table->callbacks.keyIsEqual) {
	while (item) {
	    if ((*table->callbacks.keyIsEqual)(key, item->key)) {
		replacing = 1;
		break;
	    }
	    item = item->next;
	}
    } else {
	while (item) {
	    if (key == item->key) {
		replacing = 1;
		break;
	    }
	    item = item->next;
	}
    }
    
    if (replacing) {
	void *old;

	old = item->data;
	item->data = data;
	RELKEY(table, item->key);
	item->key = DUPKEY(table, key);

	return old;
    } else {
	HashItem *nitem;

	nitem = malloc(sizeof(HashItem));
	if (!nitem) {
		fprintf(stderr, "hashtable: out of memory while allocating memory for new item\n");	
		return NULL;
	}
	nitem->key = DUPKEY(table, key);
	nitem->data = data;
	nitem->next = table->table[h];
	table->table[h] = nitem;

	table->itemCount++;
    }
    
    /* OPTIMIZE: put this in an idle handler.*/
    if (table->itemCount > table->size) {
#ifdef DEBUG0
	printf("rebuilding hash table...\n");
#endif
	rebuildTable(table);
#ifdef DEBUG0
	printf("finished rebuild.\n");
#endif
    }
    
    return NULL;
}


static HashItem*
deleteFromList(HashTable *table, HashItem *item, const void *key)
{
    HashItem *next;
    
    if (item==NULL)
	return NULL;

    if ((table->callbacks.keyIsEqual 
	 && (*table->callbacks.keyIsEqual)(key, item->key))
	|| (!table->callbacks.keyIsEqual && key==item->key)) {
	
	next = item->next;
	RELKEY(table, item->key);
	free(item);
	
	table->itemCount--;

	return next;
    }
    
    item->next = deleteFromList(table, item->next, key);

    return item;
}


void
HashRemove(HashTable *table, const void *key)
{
    unsigned h;
    
    h = HASH(table, key);
    
    table->table[h] = deleteFromList(table, table->table[h], key);
}


HashEnumerator
EnumerateHashTable(HashTable *table)
{
    HashEnumerator enumerator;
    
    enumerator.table = table;
    enumerator.index = 0;
    enumerator.nextItem = table->table[0];
    
    return enumerator;
}



void*
NextHashEnumeratorItem(HashEnumerator *enumerator)
{
    void *data = NULL;

    /* this assumes the table doesn't change between 
     * EnumerateHashTable() and NextHashEnumeratorItem() calls */

    if (enumerator->nextItem==NULL) {
	HashTable *table = enumerator->table;
	while (++enumerator->index < table->size) {
	    if (table->table[enumerator->index]!=NULL) {
		enumerator->nextItem = table->table[enumerator->index];
		break;
	    }
	}
    }
    
    if (enumerator->nextItem) {
	data = ((HashItem*)enumerator->nextItem)->data;
	enumerator->nextItem = ((HashItem*)enumerator->nextItem)->next;
    }
    
    return data;
}


unsigned
CountHashTable(HashTable *table)
{
    return table->itemCount;
}


static int
compareStrings(const char *key1, const char *key2)
{
    return strcmp(key1, key2)==0;
}


typedef unsigned (*hashFunc)(const void*);
typedef int (*isEqualFunc)(const void*, const void*);
typedef void* (*retainFunc)(const void*);
typedef void (*releaseFunc)(const void*);


const HashTableCallbacks IntHashCallbacks = {
    NULL,
	NULL,
	NULL,
	NULL
};

const HashTableCallbacks StringHashCallbacks = {
    (hashFunc)hashString,
	(isEqualFunc)compareStrings,
	(retainFunc)strdup,
	(releaseFunc)free
};



const HashTableCallbacks StringPointerHashCallbacks = {
    (hashFunc)hashString,
	(isEqualFunc)compareStrings,
	NULL,
	NULL
};

