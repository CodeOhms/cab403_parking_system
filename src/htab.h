#ifndef  HTAB_H
#define  HTAB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct item item_t;
struct item {
    char *key;
    int value;
    item_t *next;
};
    // A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab {
    item_t **buckets;
    size_t size;
};


void item_print(item_t *i);

bool htab_init(htab_t *h, size_t n);

size_t djb_hash(char *s);

size_t htab_index(htab_t *h, char *key);

item_t *htab_bucket(htab_t *h, char *key);

item_t *htab_bucket(htab_t *h, char *key);

item_t *htab_find(htab_t *h, char *key);

bool htab_add(htab_t *h, char *key, int value);

void htab_print(htab_t *h);

void htab_delete(htab_t *h, char *key);

void htab_destroy(htab_t *h);

#endif //HTAB_H