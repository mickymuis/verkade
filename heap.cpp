#include "heap.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

void 
heap_clear( heap_t* h ) {
    memset( h->regions, 0, h->capacity * sizeof( region_desc_t ) );
    h->count =0;
}

heapsptr_t 
heap_alloc( heap_t* h, size_t size ) {

    size_t loss  = INT_MAX, i;
    ssize_t best = -1;
    for( i =0; i < h->count; i++ ) {
        if( size <= h->regions[i].size && h->regions[i].size - size < loss ) {
            loss = h->regions[i].size - size;
            best = i;
        }
    }
    if( best == -1 ) {
        fprintf( stderr, "(e) heap_alloc(): insufficient free space to allocate %ld elements.\n", size );
        return -1;
    }

    heapptr_t ptr =h->regions[best].start;
    // Resize the existing region
    h->regions[best].start +=size;
    h->regions[best].size  -=size;
    if( h->regions[best].size == 0 ) {
        h->count--;
        if( best != h->count )
            memmove( &h->regions[best], &h->regions[best+1], (h->count - best) * sizeof(region_desc_t) );
    }

    return ptr;
}

int
heap_free( heap_t* h, heapptr_t ptr, size_t size ) {

    // First, find the first larger pointer in the region array
    size_t i =0;
    for( i =0; i < h->count; i++ ) {
        if( h->regions[i].start >= ptr ) {
            if( ptr+size > h->regions[i].start ) {
                fprintf( stderr, "(e) heap_free(): invalid free: region already (partially) freed.\n" );
                return -1;
            }
            break;
        }
    }

    int move = (i != h->count) ? 1 : 0;
    const size_t next =i;

    // Next region is adjacent
    if( i != h->count && h->regions[i].start == ptr+size ) {
        move--;
        size += h->regions[i].size;
    }
    // Previous region is adjacent
    if( i != 0 && h->regions[i-1].start + h->regions[i-1].size == ptr ) {
        move--;
        ptr = h->regions[i-1].start;
        size+=h->regions[i-1].size;
        i--;
    }
    
    if( h->count+move > h->capacity ) { 
        fprintf( stderr, "(e) heap_free(): out of space in meta array, cannot free any more regions.\n" );
        return -1;
    }

    if( move != 0 )
        memmove( &h->regions[next+move], &h->regions[next], (h->count-next) * sizeof(region_desc_t) );

    h->regions[i].start =ptr;
    h->regions[i].size  =size;
    h->count += (move == 0) ? (int)(i == h->count) : move;

    return 0;
}

void 
heap_defrag( heap_t* h, heap_movefunc_t func, void* user ) {

    if( h->count < 2 ) return;
    size_t first_empty =0; // Offset of the first empty element

    for( size_t i =0; i < h->count-1; i++ ) {
        region_desc_t *desc = &h->regions[i];
        region_desc_t *ndesc= &h->regions[i+1];
        // Number of elements inbetween two free regions
        size_t count = ndesc->start - (desc->start+desc->size);
        if( func( user, first_empty, desc->start+desc->size, count ) != 0 ) {
            fprintf( stderr, "(e) heap_defrag(): user memmove function returned non-zero.\n" );
            return;
        }

        first_empty += count;
    }

    region_desc_t *last =&h->regions[h->count-1];
    size_t after_last   =last->start + last->size;
    size_t empty_size   =after_last - first_empty;

    heap_clear( h );
    h->count =1;
    h->regions[0].start =first_empty;
    h->regions[0].size  =empty_size;
}

void 
heap_debugPrint( heap_t* h ) {
    printf( "(i) Heap. Total regions %ld, capacity %ld.\n", h->count, h->capacity );
    for( size_t i =0; i < h->count; i++ )
        printf( "(*) Region %ld, start %ld, size %ld\n", i, h->regions[i].start, h->regions[i].size );
}
