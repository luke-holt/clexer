#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dynamic_array.h"

#include "util.h"

#define DA_FROM_DATA_PTR(p) ((DA *)((char *)p - sizeof(DA)))

typedef struct {
    unsigned int isize;
    unsigned int capacity;
    unsigned int count;
    char data[];
} DA;

void
_da_create(void **arr, size_t isize, size_t capacity)
{
    assert(arr);
    assert(isize);
    assert(capacity);
    DA *a;
    a = malloc(sizeof(DA) + (isize * capacity));
    if (a) {
        a->isize = isize;
        a->capacity = capacity;
        a->count = 0;
    }
    *arr = a->data;
}

void
_da_destroy(void **arr)
{
    DA *a;
    a = DA_FROM_DATA_PTR(*arr);
    free(a);
    *arr = NULL;
}

void
_da_append(void **arr, void *item)
{
    DA *a;
    a = DA_FROM_DATA_PTR(*arr);
    if (a->count == a->capacity) {
        a->capacity *= 2;
        a = erealloc(a, sizeof(DA) + a->capacity * a->isize);
        *arr = a->data;
    }
    memcpy((a->data + a->count * a->isize), item, a->isize);
    a->count++;
}

void
_da_pop(void *arr)
{
    DA *a;
    a = DA_FROM_DATA_PTR(arr);
    if (a->count)
        a->count--;
}

size_t
_da_len(void *arr)
{
    DA *a;
    a = DA_FROM_DATA_PTR(arr);
    return a->count;
}

void
_da_clear(void *arr)
{
    DA *a;
    a = DA_FROM_DATA_PTR(arr);
    a->count = 0;
}

