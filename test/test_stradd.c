/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/test/test_ztradd.c, normal z -> s, Fri Sep 28 17:38:32 2018
 *
 **/

#include "test.h"
#include "flops.h"
#include "plasma.h"
#include <plasma_core_blas.h>
#include "core_lapack.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <omp.h>

#define REAL

/***************************************************************************//**
 *
 * @brief Tests STRADD
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets flags in param indicating which parameters are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_stradd(param_value_t param[], bool run)
{
    //================================================================
    // Mark which parameters are used.
    //================================================================
    param[PARAM_UPLO   ].used = true;
    param[PARAM_TRANSA ].used = true;
    param[PARAM_DIM    ].used = PARAM_USE_M | PARAM_USE_N;
    param[PARAM_ALPHA  ].used = true;
    param[PARAM_BETA   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_PADB   ].used = true;
    param[PARAM_NB     ].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters
    //================================================================
    plasma_enum_t uplo   = plasma_uplo_const(param[PARAM_UPLO].c);
    plasma_enum_t transa = plasma_trans_const(param[PARAM_TRANSA].c);

    int m = param[PARAM_DIM].dim.m;
    int n = param[PARAM_DIM].dim.n;

    int Am, An;
    int Bm, Bn;

    if (transa == PlasmaNoTrans) {
        Am = m;
        An = n;
    }
    else {
        Am = n;
        An = m;
    }

    Bm = m;
    Bn = n;

    int lda = imax(1, Am + param[PARAM_PADA].i);
    int ldb = imax(1, Bm + param[PARAM_PADB].i);

    int    test = param[PARAM_TEST].c == 'y';
    float eps  = LAPACKE_slamch('E');

    //================================================================
    // Set tuning parameters
    //================================================================
    plasma_set(PlasmaTuning, PlasmaDisabled);
    plasma_set(PlasmaNb, param[PARAM_NB].i);

    //================================================================
    // Allocate and initialize arrays
    //================================================================
    float *A =
        (float*)malloc((size_t)lda*An*sizeof(float));
    assert(A != NULL);

    float *B =
        (float*)malloc((size_t)ldb*Bn*sizeof(float));
    assert(B != NULL);

    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_slarnv(1, seed, (size_t)lda*An, A);
    assert(retval == 0);

    retval = LAPACKE_slarnv(1, seed, (size_t)ldb*Bn, B);
    assert(retval == 0);

    float *Bref = NULL;
    if (test) {
        Bref = (float*)malloc(
            (size_t)ldb*Bn*sizeof(float));
        assert(Bref != NULL);

        memcpy(Bref, B, (size_t)ldb*Bn*sizeof(float));
    }

#ifdef COMPLEX
    float alpha = param[PARAM_ALPHA].z;
    float beta  = param[PARAM_BETA].z;
#else
    float alpha = creal(param[PARAM_ALPHA].z);
    float beta  = creal(param[PARAM_BETA].z);
#endif

    //================================================================
    // Run and time PLASMA
    //================================================================
    plasma_time_t start = omp_get_wtime();

    retval = plasma_stradd(uplo, transa, m, n, alpha, A, lda, beta, B, ldb);

    plasma_time_t stop = omp_get_wtime();

    if (retval != PlasmaSuccess) {
        plasma_error("plasma_stradd() failed");
        param[PARAM_TIME].d    = 0.0;
        param[PARAM_GFLOPS].d  = 0.0;
        param[PARAM_ERROR].d   = 1.0;
        param[PARAM_SUCCESS].i = false;
        return;
    }
    else {
        plasma_time_t time    = stop-start;
        param[PARAM_TIME].d   = time;
        param[PARAM_GFLOPS].d = flops_sgeadd(m, n) / time / 1e9;
    }

    //================================================================
    // Test results by comparing to result of plasma_core_stradd function
    //================================================================
    if (test) {
        // Calculate relative error |B_ref - B|_F / |B_ref|_F < 3*eps
        // Using 3*eps covers complex arithmetic

        //=============
        // PlasmaLower
        //=============
        if (uplo == PlasmaLower) {
            switch (transa) {
            //=================
            // PlasmaConjTrans
            //=================
            case PlasmaConjTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = j; i < m; i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * (A[lda*i+j]);
                    }
                }
                break;
            //=================
            // PlasmaTrans
            //=================
            case PlasmaTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = j; i < m; i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * A[lda*i+j];
                    }
                }
                break;
            //=================
            // PlasmaNoTrans
            //=================
            case PlasmaNoTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = j; i < m; i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * A[lda*j+i];
                    }
                }
            }
        }
        //=============
        // PlasmaUpper
        //=============
        else if (uplo == PlasmaUpper) {
            switch (transa) {
            //=================
            // PlasmaConjTrans
            //=================
            case PlasmaConjTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = 0; i < imin(j+1, m); i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * (A[lda*i+j]);
                    }
                }
                break;
            //=================
            // PlasmaTrans
            //=================
            case PlasmaTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = 0; i < imin(j+1, m); i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * A[lda*i+j];
                    }
                }
                break;
            //=================
            // PlasmaNoTrans
            //=================
            case PlasmaNoTrans:
                for (int j = 0; j < n; j++) {
                    for (int i = 0; i < imin(j+1, m); i++) {
                        Bref[ldb*j+i] =
                            beta * Bref[ldb*j+i] + alpha * A[lda*j+i];
                    }
                }
            }
        }

        float work[1];

        // Calculate Frobenius norm of reference result B_ref
        float BnormRef  = LAPACKE_slange_work(
                               LAPACK_COL_MAJOR, 'F', Bm, Bn, Bref, ldb, work);

        // Calculate difference B_ref-B
        float zmone = -1.0;
        cblas_saxpy((size_t)ldb*Bn, (zmone), B, 1, Bref, 1);

        // Calculate Frobenius norm of B_ref-B
        float BnormDiff = LAPACKE_slange_work(
                               LAPACK_COL_MAJOR, 'F', Bm, Bn, Bref, ldb, work);

        // Calculate relative error |B_ref-B|_F / |B_ref|_F
        float error = BnormDiff/BnormRef;

        param[PARAM_ERROR].d   = error;
        param[PARAM_SUCCESS].i = error < 3*eps;
    }

    //================================================================
    // Free arrays
    //================================================================
    free(A);
    free(B);

    if (test)
        free(Bref);
}
