#include "heap.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

void 
heap_clear( heap_t* h, size_t totalsize ) {
    memset( h->regions, 0, h->capacity * sizeof( region_desc_t ) );
    h->count =1;
    h->regions[0].start    =0;
    h->regions[0].size     =totalsize;
    h->regions[0].userdata =0;
    h->regions[0].free     =true;
}

heapsptr_t 
heap_alloc( heap_t* h, size_t size, int userdata ) {

    size_t loss  = INT_MAX, i;
    ssize_t best = -1;
    for( i =0; i < h->count; i++ ) {
        region_desc_t* r =&h->regions[i];
        if( !r->free ) continue;
        if( size <= r->size && r->size - size < loss ) {
            loss = r->size - size;
            best = i;
        }
    }
    if( best == -1 ) {
        fprintf( stderr, "(e) heap_alloc(): insufficient free space to allocate %ld elements.\n", size );
        return -1;
    }
    if( loss != 0 && h->count == h->capacity ) {
        fprintf( stderr, "(e) heap_alloc(): insufficient free in meta array.\n" );
        return -1;
    }

    heapptr_t ptr =h->regions[best].start;

    if( loss != 0 ) {
        if( best != h->count-1 )
            memmove( &h->regions[best+2], &h->regions[best+1], (h->count-best-1) * sizeof(region_desc_t) );
        h->count++;
        h->regions[best+1].start    =h->regions[best].start + size;
        h->regions[best+1].size     =h->regions[best].size - size;
        h->regions[best+1].free     =true;
        h->regions[best+1].userdata =0;
    }

    h->regions[best].size     =size;
    h->regions[best].free     =false;
    h->regions[best].userdata =userdata;

    return ptr;
}

int
heap_free( heap_t* h, heapptr_t ptr ) {

    // First, find the the pointer in the region array
    bool valid =false;
    size_t i =0;
    for( i =0; i < h->count; i++ ) {
        region_desc_t* r =&h->regions[i];
        if( r->start == ptr ) {
            if( !r->free ) {
                valid =true;
            }
            break;
        }
    }
    if( !valid ) {
        fprintf( stderr, "(e) heap_free(): invalid free: region already (partially) freed.\n" );
        return -1;
    }

    // Lastly we check if adjacent regions are also free
    int move =0;
    heapptr_t base =h->regions[i].start;
    heapptr_t size =h->regions[i].size;
//    const size_t next =i+1;

    if( i != h->count && h->regions[i+1].free ) {
        move++;
        size += h->regions[i+1].size;
    }

    if( i != 0 && h->regions[i-1].free ) {
        move++;
        size += h->regions[i-1].size;
        base = h->regions[i-1].start;
        i--;
    }

    if( move ) {
        memmove( &h->regions[i], &h->regions[i+move], (h->count-(i+move)) * sizeof(region_desc_t) );

        h->regions[i].start =base;
        h->regions[i].size  =size;
        h->count -= move;

    }
    // Set the region to free
    h->regions[i].free  =true;
    h->regions[i].userdata =0;

    return 0;
}

void 
heap_defrag( heap_t* h, heap_movefunc_t func, void* user ) {
/*
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
    h->regions[0].size  =empty_size;*/
}

void 
heap_debugPrint( heap_t* h ) {
    printf( "(i) Heap: total regions %ld, capacity %ld.\n", h->count, h->capacity );
    size_t totalsize =0, fragsize =0;
    for( size_t i =0; i < h->count; i++ ) {
        printf( "(*) Region %ld (%s), start %ld, size %ld\n", 
            i, h->regions[i].free ? "free" : "nonfree", h->regions[i].start, h->regions[i].size );
        if( h->regions[i].free) {
            totalsize += h->regions[i].size;
            if( i != h->count-1 )
                fragsize += h->regions[i].size;
        }
    }
    printf( "(i) Heap: total size in free regions %ld, of which %ld fragmented.\n", totalsize, fragsize );
}
