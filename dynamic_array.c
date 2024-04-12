#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dynamic_array.h"

#include "util.h"

typedef struct {
    unsigned int isize;
    unsigned int capacity;
    unsigned int count;
    char data[];
} DA;

void *
_da_create(size_t isize, size_t capacity)
{
    assert(isize);
    assert(capacity);
    DA *a;
    a = malloc(sizeof(*a) - 1 + (isize * capacity));
    assert(a);
    a->isize = isize;
    a->capacity = capacity;
    a->count = 0;
    return a->data;
}

void
_da_destroy(void **arr)
{
    DA *a;
    a = (DA *)((char *)*arr - sizeof(*a));
    free(a);
    *arr = NULL;
}

void
_da_append(void **arr, void *item)
{
    DA *a;
    a = (DA *)((char *)*arr - sizeof(*a));
    if (a->count == a->capacity) {
        a->capacity *= 2;
        a = erealloc(a, sizeof(*a) - 1 + a->capacity * a->isize);
        *arr = a->data;
    }
    memcpy((a->data + a->count * a->isize), item, a->isize);
    a->count++;
}

void
_da_pop(void *arr)
{
    DA *a;
    a = (DA *)((char *)arr - sizeof(*a));
    if (a->count)
        a->count--;
}

size_t
_da_len(void *arr)
{
    DA *a;
    a = (DA *)((char *)arr - sizeof(*a));
    return a->count;
}

