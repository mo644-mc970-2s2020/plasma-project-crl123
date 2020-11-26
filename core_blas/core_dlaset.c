/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_zlaset.c, normal z -> d, Fri Sep 28 17:38:22 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "plasma_internal.h"
#include "core_lapack.h"

// for memset function
#include <string.h>

/***************************************************************************//**
 *
 * @ingroup core_laset
 *
 *  Sets the elements of the matrix A on the diagonal
 *  to beta and on the off-diagonals to alpha
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          Specifies which elements of the matrix are to be set
 *          - PlasmaUpper: Upper part of A is set;
 *          - PlasmaLower: Lower part of A is set;
 *          - PlasmaUpperLower: ALL elements of A are set.
 *
 * @param[in] m
 *          The number of rows of the matrix A.  m >= 0.
 *
 * @param[in] n
 *         The number of columns of the matrix A.  n >= 0.
 *
 * @param[in] alpha
 *         The constant to which the off-diagonal elements are to be set.
 *
 * @param[in] beta
 *         The constant to which the diagonal elements are to be set.
 *
 * @param[in,out] A
 *         On entry, the m-by-n tile A.
 *         On exit, A has been set accordingly.
 *
 * @param[in] lda
 *         The leading dimension of the array A.  lda >= max(1,m).
 *
 ******************************************************************************/
__attribute__((weak))
void plasma_core_dlaset(plasma_enum_t uplo, int m, int n,
                 double alpha, double beta,
                 double *A, int lda)
{
    if (alpha == 0.0 && beta == 0.0 && uplo == PlasmaGeneral && m == lda) {
        // Use memset to zero continuous memory.
        memset((void*)A, 0, (size_t)m*n*sizeof(double));
    }
    else {
        // Use LAPACKE_dlaset_work to initialize the matrix.
        LAPACKE_dlaset_work(LAPACK_COL_MAJOR, lapack_const(uplo),
                            m, n, alpha, beta, A, lda);
    }
}

/******************************************************************************/
void plasma_core_omp_dlaset(plasma_enum_t uplo,
                     int mb, int nb,
                     int i, int j,
                     int m, int n,
                     double alpha, double beta,
                     double *A)
{
    #pragma omp task depend(out:A[0:mb*nb])
    plasma_core_dlaset(uplo, m, n,
                alpha, beta,
                A+i+j*mb, mb);
}
