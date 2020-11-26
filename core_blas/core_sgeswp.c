/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_zgeswp.c, normal z -> s, Fri Sep 28 17:38:19 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_internal.h"
#include "plasma_types.h"
#include "core_lapack.h"

#define A(m, n) (float*)plasma_tile_addr(A, m, n)

/******************************************************************************/
__attribute__((weak))
void plasma_core_sgeswp(plasma_enum_t colrow,
                 plasma_desc_t A, int k1, int k2, const int *ipiv, int incx)
{
    //================
    // PlasmaRowwise
    //================
    if (colrow == PlasmaRowwise) {
        if (incx > 0) {
            for (int m = k1-1; m <= k2-1; m += incx) {
                if (ipiv[m]-1 != m) {
                    int m1 = m;
                    int m2 = ipiv[m]-1;

                    int lda1 = plasma_tile_mmain(A, m1/A.mb);
                    int lda2 = plasma_tile_mmain(A, m2/A.mb);

                    cblas_sswap(A.n,
                                A(m1/A.mb, 0) + m1%A.mb, lda1,
                                A(m2/A.mb, 0) + m2%A.mb, lda2);
                }
            }
        }
        else {
            for (int m = k2-1; m >= k1-1; m += incx) {
                if (ipiv[m]-1 != m) {
                    int m1 = m;
                    int m2 = ipiv[m]-1;

                    int lda1 = plasma_tile_mmain(A, m1/A.mb);
                    int lda2 = plasma_tile_mmain(A, m2/A.mb);

                    cblas_sswap(A.n,
                                A(m1/A.mb, 0) + m1%A.mb, lda1,
                                A(m2/A.mb, 0) + m2%A.mb, lda2);
                }
            }
        }
    }
    //===================
    // PlasmaColumnwise
    //===================
    else {
        if (incx > 0) {
            for (int n = k1-1; n <= k2-1; n += incx) {
                if (ipiv[n]-1 != n) {
                    int n1 = n;
                    int n2 = ipiv[n]-1;

                    int lda0 = plasma_tile_mmain(A, 0);

                    cblas_sswap(A.m,
                                A(0, n1/A.nb) + (n1%A.nb)*lda0, 1,
                                A(0, n2/A.nb) + (n2%A.nb)*lda0, 1);
                }
            }
        }
        else {
            for (int n = k2-1; n >= k1-1; n += incx) {
                if (ipiv[n]-1 != n) {
                    int n1 = n;
                    int n2 = ipiv[n]-1;

                    int lda0 = plasma_tile_mmain(A, 0);

                    cblas_sswap(A.m,
                                A(0, n1/A.nb) + (n1%A.nb)*lda0, 1,
                                A(0, n2/A.nb) + (n2%A.nb)*lda0, 1);
                }
            }
        }
    }
}
