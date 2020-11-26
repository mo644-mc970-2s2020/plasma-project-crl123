/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_ztrssq.c, normal z -> s, Fri Sep 28 17:38:23 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "plasma_internal.h"
#include "core_lapack.h"

#include <math.h>

/******************************************************************************/
// This computation also shows up in plasma_core_ssyssq() and can be factored out.
// LAPACK does real and imag components separately in slassq.
static inline void ssq(float value, float *scale, float *sumsq)
{
    float absa = fabsf(value);
    if (absa != 0.0) { // != propagates nan
        if (*scale < absa) {
            *sumsq = 1.0 + *sumsq*((*scale/absa)*(*scale/absa));
            *scale = absa;
        }
        else {
            *sumsq = *sumsq + ((absa/(*scale))*(absa/(*scale)));
        }
    }
}

/******************************************************************************/
__attribute__((weak))
void plasma_core_strssq(plasma_enum_t uplo, plasma_enum_t diag,
                 int m, int n,
                 const float *A, int lda,
                 float *scale, float *sumsq)
{
    if (uplo == PlasmaUpper) {
        if (diag == PlasmaNonUnit) {
            for (int j = 0; j < n; j++) {
                ssq(A[lda*j], scale, sumsq);
                for (int i = 1; i < imin(j+1, m); i++) {
                    ssq(A[lda*j+i], scale, sumsq);
                }
            }
        }
        else { // PlasmaUnit
            int j;
            for (j = 0; j < imin(n, m); j++) {
                ssq(1.0, scale, sumsq);
                for (int i = 0; i < j; i++) {
                    ssq(A[lda*j+i], scale, sumsq);
                }
            }
            for (; j < n; j++) {
                ssq(A[lda*j], scale, sumsq);
                for (int i = 1; i < m; i++) {
                    ssq(A[lda*j+i], scale, sumsq);
                }
            }
        }
    }
    else { // PlasmaLower
        if (diag == PlasmaNonUnit) {
            for (int j = 0; j < imin(n, m); j++) {
                ssq(A[lda*j+j], scale, sumsq);
                for (int i = j+1; i < m; i++) {
                    ssq(A[lda*j+i], scale, sumsq);
                }
            }
        }
        else { // PlasmaUnit
            for (int j = 0; j < imin(n, m); j++) {
                ssq(1.0, scale, sumsq);
                for (int i = j+1; i < m; i++) {
                    ssq(A[lda*j+i], scale, sumsq);
                }
            }
        }
    }
}

/******************************************************************************/
void plasma_core_omp_strssq(plasma_enum_t uplo, plasma_enum_t diag,
                     int m, int n,
                     const float *A, int lda,
                     float *scale, float *sumsq,
                     plasma_sequence_t *sequence, plasma_request_t *request)
{
    #pragma omp task depend(in:A[0:lda*n]) \
                     depend(out:scale[0:n]) \
                     depend(out:sumsq[0:n])
    {
        if (sequence->status == PlasmaSuccess) {
            *scale = 0.0;
            *sumsq = 1.0;
            plasma_core_strssq(uplo, diag, m, n, A, lda, scale, sumsq);
        }
    }
}
