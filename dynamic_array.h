#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>

#define da_create(isize, capacity)  _da_create(isize, capacity)
#define da_destroy(arr)             _da_destroy((void *)&arr)
#define da_append(arr, item)        _da_append((void *)&arr, item)
#define da_pop(arr)                 _da_pop(arr)
#define da_len(arr)                 _da_len(arr)

void * _da_create(size_t isize, size_t capacity);
void _da_destroy(void **arr);
void _da_append(void **arr, void *item);
void _da_pop(void *arr);
size_t _da_len(void *arr);

#endif // DYNAMIC_ARRAY_H

