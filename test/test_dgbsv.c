/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/test/test_zgbsv.c, normal z -> d, Fri Sep 28 17:38:26 2018
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

#define A(m, n) (double*)plasma_tile_addr(A, m, n)

#define REAL

/***************************************************************************//**
 *
 * @brief Tests DGBSV.
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets flags in param indicating which parameters are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_dgbsv(param_value_t param[], bool run)
{
    //================================================================
    // Mark which parameters are used.
    //================================================================
    param[PARAM_DIM    ].used = PARAM_USE_N;
    param[PARAM_KL     ].used = true;
    param[PARAM_KU     ].used = true;
    param[PARAM_NRHS   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_NB     ].used = true;
    param[PARAM_IB     ].used = true;
    param[PARAM_MTPF   ].used = true;
    param[PARAM_ZEROCOL].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters.
    //================================================================
    int n    = param[PARAM_DIM].dim.n;
    int kl   = param[PARAM_KL].i;
    int ku   = param[PARAM_KU].i;
    int lda  = imax(1, n+param[PARAM_PADA].i);
    int nrhs = param[PARAM_NRHS].i;
    int ldb  = imax(1, n + param[PARAM_PADB].i);
    int ldx  = ldb;

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
    double *A = (double*)malloc(
        (size_t)lda*n*sizeof(double));
    assert(A != NULL);
    double *X = (double*)malloc(
        (size_t)ldx*nrhs*sizeof(double));
    assert(X != NULL);
    int *ipiv = (int*)malloc((size_t)n*sizeof(int));
    assert(ipiv != NULL);

    // set up right-hand-sides X
    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_dlarnv(1, seed, (size_t)ldb*nrhs, X);
    assert(retval == 0);
    // copy X to B for test
    double *B = NULL;
    if (test) {
        B = (double*)malloc(
            (size_t)ldb*nrhs*sizeof(double));
        assert(B != NULL);
        LAPACKE_dlacpy_work(LAPACK_COL_MAJOR, 'F', n, nrhs, X, ldx, B, ldb);
    }

    // set up matrix A
    retval = LAPACKE_dlarnv(1, seed, (size_t)lda*n, A);
    assert(retval == 0);
    // zero out elements outside the band
    for (int i = 0; i < n; i++) {
        for (int j = i+ku+1; j < n; j++) A[i + j*lda] = 0.0;
    }
    for (int j = 0; j < n; j++) {
        for (int i = j+kl+1; i < n; i++) A[i + j*lda] = 0.0;
    }

    int zerocol = param[PARAM_ZEROCOL].i;
    if (zerocol >= 0 && zerocol < n)
        memset(&A[zerocol*lda], 0, n*sizeof(double));

    // save A for test
    double *Aref = NULL;
    if (test) {
        Aref = (double*)malloc(
            (size_t)lda*n*sizeof(double));
        assert(Aref != NULL);
        memcpy(Aref, A, (size_t)lda*n*sizeof(double));
    }

    int nb = param[PARAM_NB].i;
    // band matrix A in skewed LAPACK storage
    int kut  = (ku+kl+nb-1)/nb; // # of tiles in upper band (not including diagonal)
    int klt  = (kl+nb-1)/nb;    // # of tiles in lower band (not including diagonal)
    int ldab = (kut+klt+1)*nb;  // since we use dgetrf on panel, we pivot back within panel.
                                // this could fill the last tile of the panel,
                                // and we need extra NB space on the bottom
    double *AB = NULL;
    AB = (double*)malloc((size_t)ldab*n*sizeof(double));
    assert(AB != NULL);
    // convert into LAPACK's skewed storage
    for (int j = 0; j < n; j++) {
        int i_kl = imax(0,   j-ku);
        int i_ku = imin(n-1, j+kl);
        for (int i = 0; i < ldab; i++)
            AB[i + j*ldab] = 0.0;
        for (int i = i_kl; i <= i_ku; i++)
            AB[kl + i-(j-ku) + j*ldab] = A[i + j*lda];
    }
    double *ABref = NULL;
    if (test) {
        ABref = (double*)malloc(
            (size_t)ldab*n*sizeof(double));
        assert(ABref != NULL);

        memcpy(ABref, AB, (size_t)ldab*n*sizeof(double));
    }

    //================================================================
    // Run and time PLASMA.
    //================================================================
    plasma_time_t start = omp_get_wtime();
    int plainfo = plasma_dgbsv(n, kl, ku, nrhs, AB, ldab, ipiv, X, ldx);

    plasma_time_t stop = omp_get_wtime();
    plasma_time_t time = stop-start;

    param[PARAM_TIME].d = time;
    param[PARAM_GFLOPS].d = 0.0;

    //================================================================
    // Test results by comparing to a reference implementation.
    //================================================================
    if (test) {
        if (plainfo == 0) {
            // compute residual vector
            double zone  =  1.0;
            double zmone = -1.0;
            cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, nrhs, n,
                        (zmone), Aref, lda,
                                            X, ldx,
                        (zone),  B, ldb);

            // compute various norms
            double *work = NULL;
            work = (double*)malloc((size_t)n*sizeof(double));
            assert(work != NULL);

            double Anorm = LAPACKE_dlange_work(
                LAPACK_COL_MAJOR, 'F', n, n,    A, lda, work);
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
            int lapinfo = LAPACKE_dgbsv(
                              LAPACK_COL_MAJOR,
                              n, kl, ku, nrhs, ABref, ldab, ipiv, X, ldx);
            if (plainfo == lapinfo) {
                param[PARAM_ERROR].d = 0.0;
                param[PARAM_SUCCESS].i = 1;
            }
            else {
                param[PARAM_ERROR].d = INFINITY;
                param[PARAM_SUCCESS].i = 0;
            }
        }
        free(B);
        free(Aref);
        free(ABref);
    }

    //================================================================
    // Free arrays.
    //================================================================
    free(A);
    free(AB);
    free(ipiv);
    free(X);
}
