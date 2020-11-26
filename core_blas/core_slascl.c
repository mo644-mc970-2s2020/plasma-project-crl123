/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_zlascl.c, normal z -> s, Fri Sep 28 17:38:21 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "core_lapack.h"

/******************************************************************************/
__attribute__((weak))
void plasma_core_slascl(plasma_enum_t uplo,
                 float cfrom, float cto,
                 int m, int n,
                 float *A, int lda)
{
    // LAPACKE_slascl is not available in LAPACKE < 3.6.0
    int kl;
    int ku;
    int info;
    char type = lapack_const(uplo);
    LAPACK_slascl(&type,
                  &kl, &ku,
                  &cfrom, &cto,
                  &m, &n,
                  A, &lda, &info);
}

/******************************************************************************/
void plasma_core_omp_slascl(plasma_enum_t uplo,
                     float cfrom, float cto,
                     int m, int n,
                     float *A, int lda,
                     plasma_sequence_t *sequence, plasma_request_t *request)
{
    #pragma omp task depend(inout:A[0:lda*n])
    {
        if (sequence->status == PlasmaSuccess)
            plasma_core_slascl(uplo,
                        cfrom, cto,
                        m, n,
                        A, lda);
    }
}
