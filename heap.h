#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdbool.h>

typedef size_t heapptr_t;       // Offset (pointer) within the user dats array
typedef ssize_t heapsptr_t;     // Signed offset

typedef struct {
    heapptr_t start;            // Base address
    size_t    size;             // 30th bit is used to denote free/nonfree
    int       userdata;
} region_desc_t;

typedef struct {
    region_desc_t* regions;     
    size_t         capacity;    // Length of the region array
    size_t         count;       // Actual number of regions
} heap_t;

/* Custom memmove function that takes four arguments (in order):
 * - user pointer passed to heap_defrag()
 * - occupied region
 * - absolute destination address 
 */
typedef int (*heap_movefunc_t)( void*, region_desc_t, heapptr_t );

void heap_clear( heap_t* h, size_t totalsize );

heapsptr_t heap_alloc( heap_t* h, size_t size, int userdata );
int heap_free( heap_t* h, heapptr_t ptr );

void heap_defrag( heap_t* h, heap_movefunc_t, void* user );
size_t heap_regionSize( region_desc_t* );

void heap_debugPrint( heap_t* h );

#endif
