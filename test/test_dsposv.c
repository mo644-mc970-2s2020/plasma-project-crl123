/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/test/test_zcposv.c, mixed zc -> ds, Fri Sep 28 17:38:26 2018
 *
 **/

#include "plasma.h"
#include <plasma_core_blas.h>
#include "core_lapack.h"
#include "flops.h"
#include "test.h"

#include <assert.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REAL

#define A(i_, j_) A[(i_) + (size_t)lda*(j_)]

/***************************************************************************//**
 *
 * @brief Tests DSPOSV
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets flags in param indicating which parameters are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_dsposv(param_value_t param[], bool run)
{
    //================================================================
    // Mark which parameters are used.
    //================================================================
    param[PARAM_UPLO   ].used = true;
    param[PARAM_DIM    ].used = PARAM_USE_N;
    param[PARAM_NRHS   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_PADB   ].used = true;
    param[PARAM_NB     ].used = true;
    param[PARAM_ZEROCOL].used = true;
    param[PARAM_ITERSV ].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters
    //================================================================
    plasma_enum_t uplo = plasma_uplo_const(param[PARAM_UPLO].c);

    int n    = param[PARAM_DIM].dim.n;
    int nrhs = param[PARAM_NRHS].i;
    int lda  = imax(1, n + param[PARAM_PADA].i);
    int ldb  = imax(1, n + param[PARAM_PADB].i);
    int ldx  = ldb;
    int ITER;

    int    test = param[PARAM_TEST].c == 'y';
    double tol  = param[PARAM_TOL].d * LAPACKE_dlamch('E');

    //================================================================
    // Set tuning parameters
    //================================================================
    plasma_set(PlasmaTuning, PlasmaDisabled);
    plasma_set(PlasmaNb, param[PARAM_NB].i);

    //================================================================
    // Allocate and initialize arrays
    //================================================================
    double *A = (double *)malloc(
    (size_t)lda*n*sizeof(double));
    assert(A != NULL);

    double *B = (double *)malloc(
        (size_t)ldb*nrhs*sizeof(double));
    assert(B != NULL);

    double *X = (double *)malloc(
    (size_t)ldx*nrhs*sizeof(double));
    assert(X != NULL);

    // Initialize A for random symmetric (Symmetric) matrix
    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_dlarnv(1, seed, (size_t)lda*n, A);
    assert(retval == 0);

    for (int i = 0; i < n; ++i) {
        A(i,i) = creal(A(i,i)) + n;
        for (int j = 0; j < i; ++j) {
            A(j,i) = (A(i,j));
        }
    }

    int zerocol = param[PARAM_ZEROCOL].i;
    if (zerocol >= 0 && zerocol < n) {
        LAPACKE_dlaset_work(
            LAPACK_COL_MAJOR, 'F', n, 1, 0.0, 0.0, &A(0, zerocol), lda);
        LAPACKE_dlaset_work(
            LAPACK_COL_MAJOR, 'F', 1, n, 0.0, 0.0, &A(zerocol, 0), lda);
    }

    // Initialize B
    retval = LAPACKE_dlarnv(1, seed, (size_t)ldb*nrhs, B);
    assert(retval == 0);

    double *Aref = NULL;
    if (test) {
        Aref = (double *)malloc(
            (size_t)lda*n*sizeof(double));
        assert(Aref != NULL);
        memcpy(Aref, A, (size_t)lda*n*sizeof(double));
    }

    //================================================================
    // Run and time PLASMA
    //================================================================
    plasma_time_t start = omp_get_wtime();
    int plainfo = plasma_dsposv(uplo, n, nrhs, A, lda, B, ldb, X, ldx, &ITER);
    plasma_time_t stop = omp_get_wtime();
    plasma_time_t time = stop-start;
    double flops = flops_dpotrf(n) + flops_dpotrs(n, nrhs);
    param[PARAM_ITERSV].i = ITER;
    param[PARAM_TIME].d   = time;
    param[PARAM_GFLOPS].d = flops / time / 1e9;

    //================================================================
    // Test results by checking the residual
    //
    //                      || B - AX ||_I
    //                --------------------------- < epsilon
    //                 || A ||_I * || X ||_I * N
    //
    //================================================================
    if (test) {
        if (plainfo == 0) {
            double alpha =  1.0;
            double beta  = -1.0;

            lapack_int mtrxLayout = LAPACK_COL_MAJOR;
            lapack_int mtrxNorm   = 'I';

            double *work = (double *)malloc(n*sizeof(double));
            assert(work != NULL);

            // Calculate infinite norms of matrices A_ref and X
            double Anorm = LAPACKE_dlange_work(mtrxLayout, mtrxNorm, n, n, Aref,
                                               lda, work);
            double Xnorm = LAPACKE_dlange_work(mtrxLayout, mtrxNorm, n, nrhs, X,
                                               ldx, work);

            // Calculate residual R = A*X-B, store result in B
            cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, nrhs, n,
                        (alpha), Aref, lda,
                                            X,    ldx,
                        (beta),  B,    ldb);

            // Calculate infinite norm of residual matrix R
            double Rnorm = LAPACKE_dlange_work(mtrxLayout, mtrxNorm, n, nrhs, B,
                                               ldb, work);
            // Calculate relative error
            double residual = Rnorm / ( n*Anorm*Xnorm );

            param[PARAM_ERROR].d   = residual;
            param[PARAM_SUCCESS].i = residual < tol;

            free(work);
        }
        else {
            int lapinfo = LAPACKE_dsposv(
                              LAPACK_COL_MAJOR, lapack_const(uplo),
                              n, nrhs, A, lda, B, ldb, X, ldx, &ITER);
            if (plainfo == lapinfo) {
                param[PARAM_ERROR].d = 0.0;
                param[PARAM_SUCCESS].i = 1;
            }
            else {
                param[PARAM_ERROR].d = INFINITY;
                param[PARAM_SUCCESS].i = 0;
            }
        }
        free(Aref);
    }

    //================================================================
    // Free arrays
    //================================================================
    free(A); free(B); free(X);
}
