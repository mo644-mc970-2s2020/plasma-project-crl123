/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/test/test_zhesv.c, normal z -> d, Fri Sep 28 17:38:29 2018
 *
 **/
#include "test.h"
#include "flops.h"
#include "plasma.h"
#include <plasma_core_blas.h>
#include "core_lapack.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omp.h>

#define A(i_, j_) A[(i_) + (size_t)lda*(j_)]

/***************************************************************************//**
 *
 * @brief Tests DSYSV.
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets flags in param indicating which parameters are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_dsysv(param_value_t param[], bool run)
{
    //================================================================
    // Print usage info or return column labels or values.
    //================================================================
    param[PARAM_UPLO   ].used = true;
    param[PARAM_DIM    ].used = PARAM_USE_N;
    param[PARAM_NRHS   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_PADB   ].used = true;
    param[PARAM_NB     ].used = true;
    param[PARAM_MTPF   ].used = true;
    param[PARAM_ZEROCOL].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters.
    //================================================================
    plasma_enum_t uplo = plasma_uplo_const(param[PARAM_UPLO].c);

    int n    = param[PARAM_DIM].dim.n;
    int nb   = param[PARAM_NB].i;
    int nrhs = param[PARAM_NRHS].i;

    int lda = imax(1, n + param[PARAM_PADA].i);
    int ldb = imax(1, n + param[PARAM_PADB].i);
    int ldx = ldb;

    // band matrix A in skewed LAPACK storage
    int kut  = (nb+nb+nb-1)/nb; // # of tiles in upper band (not including diagonal)
    int klt  = (nb+nb-1)/nb;    // # of tiles in lower band (not including diagonal)
    int ldt  = (kut+klt+1)*nb;  // since we use dgetrf on panel, we pivot back within panel.

    int test = param[PARAM_TEST].c == 'y';
    double tol = param[PARAM_TOL].d * LAPACKE_dlamch('E');

    //================================================================
    // Set tuning parameters.
    //================================================================
    plasma_set(PlasmaTuning, PlasmaDisabled);
    plasma_set(PlasmaNb, param[PARAM_NB].i);
    plasma_set(PlasmaIb, param[PARAM_IB].i);
    plasma_set(PlasmaNumPanelThreads, param[PARAM_MTPF].i);

    //================================================================
    // Allocate and initialize arrays.
    //================================================================
    double *A =
        (double*)calloc((size_t)lda*n, sizeof(double));
    assert(A != NULL);
    double *T =
        (double*)calloc((size_t)ldt*n, sizeof(double));
    assert(T != NULL);

    int *ipiv = (int*)calloc((size_t)n, sizeof(int));
    int *ipiv2 = (int*)calloc((size_t)n, sizeof(int));
    assert(ipiv != NULL);
    assert(ipiv2 != NULL);

    // set up right-hand-side B = A*rand
    double *B = (double*)malloc(
        (size_t)ldb*nrhs*sizeof(double));
    assert(B != NULL);
    double *X = (double*)malloc(
        (size_t)ldx*nrhs*sizeof(double));
    assert(X != NULL);

    //================================================================
    // Make the A matrix symmetric.
    // It sets Aji = ( Aij ) for j < i, that is, copy lower
    // triangle to upper triangle.
    //================================================================
    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_dlarnv(1, seed, (size_t)lda*n, A);
    assert(retval == 0);
    for (int i = 0; i < n; ++i) {
        A(i,i) = creal(A(i,i));
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

    double *Aref = NULL;
    if (test) {
        Aref = (double*)malloc(
            (size_t)lda*n*sizeof(double));
        assert(Aref != NULL);

        memcpy(Aref, A, (size_t)lda*n*sizeof(double));
    }

    retval = LAPACKE_dlarnv(1, seed, (size_t)ldb*nrhs, X);
    assert(retval == 0);
    plasma_dgemm(PlasmaNoTrans, PlasmaNoTrans,
                 n, nrhs, n,
                 1.0, A, lda,
                      X, ldx,
                 0.0, B, ldb);
    // copy B to X
    LAPACKE_dlacpy_work(
        LAPACK_COL_MAJOR, 'F', n, nrhs, B, ldb, X, ldx);

    //================================================================
    // Run and time PLASMA.
    //================================================================
    plasma_time_t start = omp_get_wtime();
    int plainfo = plasma_dsysv(uplo, n, nrhs, A, lda, ipiv, T, ldt, ipiv2, X, ldx);
    plasma_time_t stop = omp_get_wtime();
    plasma_time_t time = stop-start;

    param[PARAM_TIME].d = time;
    param[PARAM_GFLOPS].d = flops_dpotrf(n) / time / 1e9;

    //================================================================
    // Test results by comparing to a reference implementation.
    //================================================================
    if (test) {
        if (plainfo == 0) {
            // compute the residual norm ||A-bx||
            // compute residual vector
            plasma_dgemm(PlasmaNoTrans, PlasmaNoTrans,
                         n, nrhs, n,
                         -1.0, Aref, lda,
                                  X, ldx,
                          1.0,    B, ldb);

            // compute various norms
            double *work = NULL;
            work = (double*)malloc((size_t)n*sizeof(double));
            assert(work != NULL);

            double Anorm = LAPACKE_dlange_work(
                LAPACK_COL_MAJOR, 'F', n, n,    Aref, lda, work);
            double Xnorm = LAPACKE_dlange_work(
                LAPACK_COL_MAJOR, 'I', n, nrhs, X, ldb, work);
            double Rnorm = LAPACKE_dlange_work(
                LAPACK_COL_MAJOR, 'I', n, nrhs, B, ldb, work);
            double residual = Rnorm/(n*Anorm*Xnorm);

            param[PARAM_ERROR].d = residual;
            param[PARAM_SUCCESS].i = residual < tol;

            // free workspaces
            free(work);
        }
        else {
            // check for zero column, by updating zerocol with ipiv
            zerocol ++; // zerocol is 0-based, info is 1-based
            for (int i = nb; i < plainfo; i++) {
                if (ipiv[i] == zerocol) {
                    zerocol = i+1;
                }
                else if (i == zerocol) {
                    zerocol = ipiv[i];
                }
            }
            if (plainfo == zerocol) {
                param[PARAM_ERROR].d = 0.0;
                param[PARAM_SUCCESS].i = 1;
            }
            else {
                param[PARAM_ERROR].d = INFINITY;
                param[PARAM_SUCCESS].i = 0;
            }
        }
    }

    //================================================================
    // Free arrays.
    //================================================================
    free(A); free(ipiv); free(ipiv2); free(T);
    free(X); free(B);
    if (test)
        free(Aref);
}
