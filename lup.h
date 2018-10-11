#ifndef LUP_H
#define LUP_H

#include "matrix.h"

#define MAX_N_ELEMENTS 1024 * 1024 * 128 // One GB worth of doubles //131072
#define MAX_N_ROWS 16384
#define N_REF_VECTORS 5
#define HEAP_SIZE 2*16384

typedef struct {

    double values[MAX_N_ELEMENTS];

    indx_t col_ind[MAX_N_ELEMENTS];
    indx_t row_ptr_begin[MAX_N_ROWS];
    indx_t row_ptr_end[MAX_N_ROWS];
    indx_t row_order[MAX_N_ROWS];
    //indx_t U_START[MAX_N_ROWS];
    size_t m, n;
    size_t count;

} matrix_t;

void print_dense( matrix_t* m );
void print_vec( double values[], size_t m, size_t* order );

int lup( matrix_t* m );

void mult_matvec( double b[], matrix_t* m, const double x[] );
void l_subst( double c[], matrix_t* m, const double b[] );
void u_subst( double x[], matrix_t* m, const double c[] );
double compute_variance( const double vec[], const double ref[], const indx_t row_order[], size_t m );

#endif
