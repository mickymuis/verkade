
#include "lup.h"
#include "heap.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static region_desc_t HEAP_REGIONS[HEAP_SIZE];

void
print_dense( matrix_t* m ) {
    for( size_t ii = 0; ii < m->m; ii++ ) {
        const indx_t i =m->row_order[ii];
        indx_t idx =m->row_ptr_begin[i];
        for( indx_t j =0; j < m->n; j++ ) {
            if( j == m->col_ind[idx] && idx <= m->row_ptr_end[i] ) {
                printf("%-6.2f ", m->values[idx]);
                idx++;
            } else 
                printf("0      ");

        }
        printf( "\n" );
    }
}

void
print_vec( double values[], size_t m, size_t* order ) {
    printf( "(" );
    for( size_t ii =0; ii < m; ii++ ) {
        size_t i =ii;
        if( order )
            i =order[ii];
        printf( "%1.2f", values[i] );
        if( ii != m-1 )
            printf( ", " );
    }
    printf( ")\n" );
}

static inline ssize_t
replace_row( matrix_t* m, 
            double new_values[],
            indx_t new_col_ind[],
            indx_t new_length,
            size_t row,
            heap_t* heap ) {
    size_t old_length = 1 + m->row_ptr_end[row] - m->row_ptr_begin[row];
    ssize_t delta = new_length - old_length;

   // printf( "(i) freeing %ld bytes at %ld.\n", old_length, m->row_ptr_begin[row] );
    heap_free( heap, m->row_ptr_begin[row], old_length );
    //heap_debugPrint( heap );
    size_t start = heap_alloc( heap, new_length );
   // printf( "(i) allocing %ld bytes at %ld.\n", new_length, start );
    //heap_debugPrint( heap );

    m->row_ptr_begin[row] = start;
    m->row_ptr_end[row]   = start + (new_length - 1);

    memcpy( &m->values[start], new_values, new_length * sizeof( double ) );
    memcpy( &m->col_ind[start], new_col_ind, new_length * sizeof( indx_t ) );

    return delta; 
}

/** Reads at most @n elements from @col_ind until @col is encountered.
 *  Returns the offset of @col in @col_ind or -1 if no such value is found.
 */
static inline ssize_t
column_offset( const indx_t col_ind[], size_t n, indx_t col ) {
    for( size_t k =0; k < n; k++ )
        if( col_ind[k] == col ) return k;
        else if( col_ind[k] > col ) return -1;
    return -1;
}

/** Interchanges the values of array elements @i and @k in @m->row_order 
 */
static inline void
swap_rows( indx_t row_order[], size_t i, size_t k ) {
    const indx_t tmp =row_order[i];
    row_order[i]  =row_order[k];
    row_order[k]  =tmp;
}


/** This function is called by the heap_defrag() function (garbage collection)
 */
/*static int 
crs_memmove( matrix_t* m, heapptr_t dest, heapptr_t src, size_t count ) {

    // Move the actual elements first
    memmove( &m->values[dest], &m->values[src], count * sizeof(double) );
    memmove( &m->col_ind[dest], &m->col_ind[src], count * sizeof(indx_t) );

    // Unfortunately, we have to do a linear search
    // in order to know to which row this pointer belongs to

    for( size_t i =0; i < m->m; i++ ) {
        if( m->row_ptr_begin[i] == src ) {
            indx_t delta = m->row_ptr_end[i] - m->row_ptr_begin[i];
            m->row_ptr_begin[i] = dest;
            m->row_ptr_end[i] = dest + delta;
            return 0;
        }
    }

    return -1;
}

static size_t
crs_defrag( matrix_t* m ) {

    size_t next_empty =0;
    for( size_t i =0; i < m->m; i++ ) {
        size_t src =m->row_ptr_begin[i];
        size_t count = 1 + m->row_ptr_end[i] - src;

        if( src != next_empty ) {
            memmove( &m->values[next_empty], &m->values[src], count * sizeof(double) );
            memmove( &m->col_ind[next_empty], &m->col_ind[src], count * sizeof(indx_t) );

            m->row_ptr_begin[i] = next_empty;
            m->row_ptr_end[i] = next_empty + count - 1;
        }

        next_empty += count;
    }
    return next_empty;
}*/

/** Computes the LU-factorisation with partial pivotting.
 *  The input matrix is provided in CRS form in the five arrays,
 *  these arrays are modified to contain both the L and U matrix on return.
 *  This function is not reentrant.
 */
int
lup( matrix_t* m ) {

    double values_tmp[m->n];
    indx_t col_ind_tmp[m->n];
    
    // We prepare m->mory m->nagem->nt using the meta array functions defined in heap.h
    heap_t heap;
    heap.regions = HEAP_REGIONS;
    heap.capacity =HEAP_SIZE;
    heap_clear( &heap );
    heap_free( &heap, m->count, MAX_N_ELEMENTS - m->count );
    //heap_debugPrint( &heap );

    // Iterate all rows except the last
    for( size_t pivot =0; pivot < m->m-1; pivot++ ) {
        const size_t ii =pivot; // For readability;

        // Print a progress indicator
        printf( "\b\b\b\b%3d%%", (pivot * 100) / m->m );

        int pivot_off =-1; // Location of the pivot in the source row
        int best_row =-1;
        double abs_max =-1.0;
        // Iterate over all remaining rows (partial pivotting)
        for( size_t kk =pivot; kk < m->m; kk++ ) {
            const indx_t k =m->row_order[kk];
            double x;
            
            // Find the pivot column in the source row
            ssize_t j = column_offset( &m->col_ind[m->row_ptr_begin[k]],
                                       (m->row_ptr_end[k] - m->row_ptr_begin[k]) + 1,
                                       pivot );
            if( j == -1 ) continue;
            j += m->row_ptr_begin[k];

            x =fabs( m->values[j] );
            abs_max = fmax( abs_max, x );

            if( x >= abs_max ) {
                pivot_off =j - m->row_ptr_begin[k];
                best_row =kk;
            }
        }
        if( pivot_off == -1 ) continue; // Complete column is empty

        swap_rows( m->row_order, ii, best_row );
        const indx_t i =m->row_order[ii];

        // Iterate over rows ii+1..m
        for( size_t kk =ii+1; kk < m->m; kk++ ) {
            const indx_t k =m->row_order[kk];

            // Find the pivot column in the dest row
            //size_t row_length; 
            indx_t i_off = m->row_ptr_begin[i] + pivot_off, k_off = m->row_ptr_begin[k];
            double mult;
            
            // Find the pivot column in the dest row
            ssize_t o = column_offset( &m->col_ind[m->row_ptr_begin[k]],
                                       (m->row_ptr_end[k] - m->row_ptr_begin[k]) + 1,
                                       pivot );
            if( o == -1 ) continue; // Pivot column is empty

            memcpy( values_tmp, &m->values[k_off], o * sizeof(double) );
            memcpy( col_ind_tmp, &m->col_ind[k_off], o * sizeof(indx_t) );

            k_off += o;
            mult = m->values[k_off] / m->values[i_off]; // Calculate the multiplication factor
            values_tmp[o] = mult; // Store the multiplier; it is part of the L matrix
            col_ind_tmp[o] = pivot;

            //printf( "Mult: %6.2f / %6.2f = %6.2f\n", m->values[k_off], m->values[i_off], mult );

            i_off++; k_off++; o++;// Skip the pivot element

            // Substract mult*i from row k using the intermediate buffer array
            for( indx_t jj =pivot+1; jj < m->n; jj++ ) { // Iterate over columns pivot+1..n
                double i_val =0.0, k_val =0.0;

                if( m->col_ind[i_off] == jj && i_off <= m->row_ptr_end[i] ) {
                    i_val =m->values[i_off++];
                }
                if( m->col_ind[k_off] == jj && k_off <= m->row_ptr_end[k] ) {
                    k_val =m->values[k_off++];
                }

                double val =k_val - i_val * mult;
                if( val != 0.0 ) {
                    values_tmp[o] = val;
                    col_ind_tmp[o] = jj;
                    o++;
                }

            }

            m->count += replace_row( m,
                                     values_tmp,
                                     col_ind_tmp,
                                     o,     // Number of non-zeroes in the dest row
                                     k,     // Row number of the dest row
                                     &heap );

        }
       // heap_defrag( &heap, (heap_movefunc_t)crs_memmove, (void*)m );
       // heap_debugPrint( &heap );
       /* size_t next_empty =crs_defrag( m );
        heap_clear( &heap );
        heap_free( &heap, next_empty, MAX_N_ELEMENTS - next_empty ); 
        heap_debugPrint( &heap );*/
    }

    return 0;
}

/** Computes b in Ax = b using naive (square) matrix multiplication.
 */
void 
mult_matvec( double b[],
             matrix_t* m,
             const double x[] ) {
    assert( m->m==m->n );
    memset( b, 0, m->n * sizeof( double ) );
    for( size_t ii =0; ii < m->m; ii++ ) {
        indx_t i = m->row_order[ii];
        for( indx_t j = m->row_ptr_begin[i]; j <= m->row_ptr_end[i]; j++ ) {
            indx_t col = m->col_ind[j];
            b[ii] += x[col] * m->values[j];
        }
    }
}

/** Computes forward substitution of the lower triangular matrix,
 *  i.e. c in Lc = b
 */
void
l_subst( double c[],
         matrix_t* m,
         const double b[] ) {
    assert( m->m==m->n );
    //m->mcpy( c, b, m * sizeof(double) );
    memset( c, 0, m->m * sizeof(double) );
    for( size_t ii =0; ii < m->m; ii++ ) {
        const indx_t i =m->row_order[ii];
        c[i] = b[i];
        for( indx_t j =m->row_ptr_begin[i]; j <= m->row_ptr_end[i]; j++ ) {
            const indx_t jj =m->col_ind[j];
            if( jj >= ii ) break;
            if( !isfinite( m->values[j] * c[m->row_order[jj]] ) ) {
                fprintf( stderr, "(e) l_subst: A[%ld,%ld] * c[%ld] is not a number!\n\t%f * %f\n", 
                        ii, jj, jj, m->values[j], c[m->row_order[jj]] );
                abort();
            }
            c[i] -= m->values[j] * c[m->row_order[jj]];
        }
        if( !isfinite( c[i] ) ) {
            fprintf( stderr, "(e) l_subst: c[%ld] is not a number!\n", i );
            abort();
        }
    }
}
             
/** Computes backward substitution of the upper triangular matrix,
 *  i.e. c in Ux = c
 */
void
u_subst( double x[],
         matrix_t* m,
         const double c[] ) {
    assert( m->m==m->n );
    //m->mcpy( x, c, m * sizeof(double) );
    memset( x, 0, m->m * sizeof(double) );
    for( ssize_t ii =m->m-1; ii >= 0;  ii-- ) {
        const indx_t i =m->row_order[ii];
        double d_value =1.0;
        x[i] = c[i];
        for( indx_t j =m->row_ptr_begin[i]; j <= m->row_ptr_end[i]; j++ ) {
            const sindx_t jj =m->col_ind[j];
            if( jj == ii ) {
                d_value =m->values[j];
                continue;
            }
            else if( jj < ii ) continue;
            x[i] -= m->values[j] * x[m->row_order[jj]];
        }
        x[i] /= d_value;
        if( !isfinite( x[i] ) ) {
            fprintf( stderr, "(e) u_subst: x[%ld] is not a number!\n", i );
            abort();
        }
    }
}

/* Compute the variancy from @ref to @vec using the Euclidian norm.
 * The order of @ref is assumed to be straight while the additional parameter
 * @m->row_order determ->nes the order of elements in @vec.
 * @m gives the total number of elements in @ref and @vec 
 */
double
compute_variance( const double vec[], 
                  const double ref[], 
                  const indx_t row_order[], 
                  size_t m ) {
    // We compute sqrt( sum_i( (ref_i - vec_i)^2 ) ) / sqrt( sum_i( ref_i^2 ) )
    double num =0.0, denom =0.0;
    for( size_t ii =0; ii < m; ii++ ) {
        const indx_t i =row_order[ii];
        num +=  pow( ref[ii] - vec[i], 2 );
        denom +=pow( ref[ii], 2 );
    }
    return sqrt(num)/sqrt(denom);
}
