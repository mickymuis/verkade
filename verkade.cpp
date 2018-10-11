#include <cstdio>
#include <ctime>
#include <cstring>
#include <math.h>
#include <assert.h>

#include "matrix.h"
#include "lup.h"

/* Globals. Yuk. */

static double X_REF[N_REF_VECTORS][MAX_N_ROWS];
static double B_REF[N_REF_VECTORS][MAX_N_ROWS];
static double X_OUT[N_REF_VECTORS][MAX_N_ROWS];
static double C_TMP[MAX_N_ROWS];

static matrix_t M;

static void
dump_crs( size_t m, size_t nz ) {
    printf( "M.row_ptr_begin " );
    for( size_t i =0; i < m; i++ ) {
        printf( "%4ld ", M.row_ptr_begin[i] );
    }
    printf( "\nM.row_ptr_end   " );
    for( size_t i =0; i < m; i++ ) {
        printf( "%4ld ", M.row_ptr_end[i] );
    }
    printf( "\nROW_NEXT      " );
    for( size_t i =0; i < m; i++ ) {
        printf( "%4ld ", M.row_order[i] );
    }
    printf( "\nM.col_ind       " );
    for( size_t i =0; i <nz; i++ ) {
        printf( "%4ld ", M.col_ind[i] );
    }
    printf( "\nM.values        " );
    for( size_t i =0; i <nz; i++ ) {
        printf( "%1.2f ", M.values[i] );
    }
    printf( "\n" );
}

static void
make_reference_vectors( size_t m ) {
    /* We fill five reference vectors with these patterns
       - all one
       - all 0.1
       - sign alternating 1s, 5s and 100s
    */
    for( size_t i =0; i < m; i++ ) {
        X_REF[0][i] = 1.0;
        X_REF[1][i] = 0.1;
        X_REF[2][i] = i%2 ? -1.0 : 1.0;
        X_REF[3][i] = i%2 ? -5.0 : 5.0;
        X_REF[4][i] = i%2 ? -100.0 : 100.0;
    }

    for( size_t i =0; i < N_REF_VECTORS; i++ ) {
        mult_matvec( &B_REF[i][0],
                     &M,
                     &X_REF[i][0] );
/*        printf( "(i) b_%ld = ", i );
        print_vec( &B_REF[i][0], m, NULL );*/
    }
}

/* Code taken from the GLIBC manual.
 *
 * Subtract the ‘struct timespec’ M.values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */
static int
timespec_subtract (struct timespec *result,
                   struct timespec *x,
                   struct timespec *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_nsec < y->tv_nsec) {
    int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
    y->tv_nsec -= 1000000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_nsec - y->tv_nsec > 1000000000) {
    int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
    y->tv_nsec += 1000000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_nsec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
      fprintf(stderr, "(i) Usage: %s <filename>\n", argv[0]);
      return -1;
    }

    bool ok(false);

    ok = load_matrix_market(argv[1], MAX_N_ELEMENTS, MAX_N_ROWS,
                          M.count, M.m, M.n,
                          M.values, M.col_ind, M.row_ptr_begin, M.row_ptr_end);
    if (!ok)
    {
      fprintf(stderr, "(e) Failed to load matrix.\n");
      return -1;
    }

    // Set the M.row_order array to encode an unpermuted row order
    for( size_t i =0; i < M.m; i++ ) M.row_order[i] =i;

    /* For debugging, can be removed when implementation is finished. */
    //printf( "Sparse matrix data:\n" );
    //dump_nonzeros(M.m, M.values, M.col_ind, M.row_ptr_begin, M.row_ptr_end);
    //printf( "Dense matrix:\n" );
    //print_dense( M.values, M.col_ind, M.row_ptr_begin, M.row_ptr_end, M.row_order, M.m, M.n );

    printf( "Computing reference vectors: ..." );
    make_reference_vectors( M.m );
    printf( "\b\b\bdone.\n" );


    struct timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    /* Perform LU factorization here */
    const size_t orig_count =M.count;
    printf( "Computing LUP: ...." );
    lup( &M );
    printf( "\b\b\b\bdone.\n(i) Fill-in ratio is %f (%ld KiB / %ld KiB)\n", 
            (double)M.count/(double)orig_count, M.count, orig_count );

    for( size_t i =0; i < N_REF_VECTORS; i++ ) {
        printf( "%ld: l_subst", i );
        l_subst( C_TMP, &M, B_REF[i] );
        printf( ", u_subst" );
        u_subst( X_OUT[i], &M, C_TMP );
        //printf( "(i) c_%ld = ", i ); print_vec( C_TMP, M.m, M.row_order );
        //printf( "(i) x_%ld = ", i ); print_vec( X_OUT[i], M.m, M.row_order );
        double variance =compute_variance( X_OUT[i], X_REF[i], M.row_order, M.m );
        printf( ", done. Variance(X_%ld): %2.3f\n", i, variance );
    }

    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);

    printf( "Result dense matrix:\n" );
    //print_dense( M.values, M.col_ind, M.row_ptr_begin, M.row_ptr_end, M.row_order, M.m, M.n );
    //dump_crs( M.m, M.count );
    

    struct timespec elapsed_time;
    timespec_subtract(&elapsed_time, &end_time, &start_time);

    double elapsed = (double)elapsed_time.tv_sec +
      (double)elapsed_time.tv_nsec / 1000000000.0;
    fprintf(stderr, "elapsed time: %f s\n", elapsed);

    return 0;
}
