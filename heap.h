#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdbool.h>

typedef size_t heapptr_t;       // Offset (pointer) within the user dats array
typedef ssize_t heapsptr_t;     // Signed offset

typedef struct {
    heapptr_t start;
    size_t    size;
    bool      free;
    int       userdata;
} region_desc_t;

typedef struct {
    region_desc_t* regions;     // Empty regions
    size_t         capacity;
    size_t         count;
} heap_t;

/* Custom memmove function that takes four arguments (in order):
 * - user pointer
 * - destination offset
 * - source offset
 * - total number of elements moved
 */
typedef int (*heap_movefunc_t)( void*, heapptr_t, heapptr_t, size_t);

void heap_clear( heap_t* h, size_t totalsize );

heapsptr_t heap_alloc( heap_t* h, size_t size, int userdata );
int heap_free( heap_t* h, heapptr_t ptr );

void heap_defrag( heap_t* h, heap_movefunc_t, void* user );

void heap_debugPrint( heap_t* h );

#endif
