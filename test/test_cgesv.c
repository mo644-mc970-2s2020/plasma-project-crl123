/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/test/test_zgesv.c, normal z -> c, Fri Sep 28 17:38:28 2018
 *
 **/

#include "test.h"
#include "flops.h"
#include "plasma.h"
#include <plasma_core_blas.h>
#include "core_lapack.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omp.h>

#define COMPLEX

#define A(i_, j_) A[(i_) + (size_t)lda*(j_)]

/***************************************************************************//**
 *
 * @brief Tests CGESV.
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets flags in param indicating which parameters are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_cgesv(param_value_t param[], bool run)
{
    //================================================================
    // Mark which parameters are used.
    //================================================================
    param[PARAM_DIM    ].used = PARAM_USE_N;
    param[PARAM_NRHS   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_PADB   ].used = true;
    param[PARAM_NB     ].used = true;
    param[PARAM_IB     ].used = true;
    param[PARAM_MTPF   ].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters.
    //================================================================
    int n = param[PARAM_DIM].dim.n;
    int nrhs = param[PARAM_NRHS].i;

    int lda = imax(1, n+param[PARAM_PADA].i);
    int ldb = imax(1, n+param[PARAM_PADB].i);

    int test = param[PARAM_TEST].c == 'y';
    float tol = param[PARAM_TOL].d * LAPACKE_slamch('E');

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
    plasma_complex32_t *A =
        (plasma_complex32_t*)malloc((size_t)lda*n*sizeof(plasma_complex32_t));
    assert(A != NULL);

    plasma_complex32_t *B =
        (plasma_complex32_t*)malloc(
            (size_t)ldb*nrhs*sizeof(plasma_complex32_t));
    assert(B != NULL);

    int *ipiv = (int*)malloc((size_t)n*sizeof(int));
    assert(ipiv != NULL);

    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_clarnv(1, seed, (size_t)lda*n, A);
    assert(retval == 0);

    retval = LAPACKE_clarnv(1, seed, (size_t)ldb*nrhs, B);
    assert(retval == 0);

    plasma_complex32_t *Aref = NULL;
    plasma_complex32_t *Bref = NULL;
    float *work = NULL;
    if (test) {
        Aref = (plasma_complex32_t*)malloc(
            (size_t)lda*n*sizeof(plasma_complex32_t));
        assert(Aref != NULL);

        Bref = (plasma_complex32_t*)malloc(
            (size_t)ldb*nrhs*sizeof(plasma_complex32_t));
        assert(Bref != NULL);

        memcpy(Aref, A, (size_t)lda*n*sizeof(plasma_complex32_t));
        memcpy(Bref, B, (size_t)ldb*nrhs*sizeof(plasma_complex32_t));
    }

    //================================================================
    // Run and time PLASMA.
    //================================================================
    plasma_time_t start = omp_get_wtime();
    plasma_cgesv(n, nrhs, A, lda, ipiv, B, ldb);
    plasma_time_t stop = omp_get_wtime();
    plasma_time_t time = stop-start;

    float flops = flops_cgetrf(n, n) + flops_cgetrs(n, nrhs);
    param[PARAM_TIME].d = time;
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
        plasma_complex32_t zone  =  1.0;
        plasma_complex32_t zmone = -1.0;

        work = (float*)malloc((size_t)n*sizeof(float));
        assert(work != NULL);

        float Anorm = LAPACKE_clange_work(
            LAPACK_COL_MAJOR, 'I', n, n, Aref, lda, work);
        float Xnorm = LAPACKE_clange_work(
            LAPACK_COL_MAJOR, 'I', n, nrhs, B, ldb, work);

        // Bref -= Aref*B
        cblas_cgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, nrhs, n,
                    CBLAS_SADDR(zmone), Aref, lda,
                                        B,    ldb,
                    CBLAS_SADDR(zone),  Bref, ldb);

        float Rnorm = LAPACKE_clange_work(
            LAPACK_COL_MAJOR, 'I', n, nrhs, Bref, ldb, work);
        float residual = Rnorm/(n*Anorm*Xnorm);

        param[PARAM_ERROR].d = residual;
        param[PARAM_SUCCESS].i = residual < tol;
    }

    //================================================================
    // Free arrays.
    //================================================================
    free(A);
    free(B);
    free(ipiv);
    if (test) {
        free(Aref);
        free(Bref);
        free(work);
    }
}
