/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/compute/pzherk.c, normal z -> c, Fri Sep 28 17:38:12 2018
 *
 **/

#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_types.h"
#include "plasma_workspace.h"
#include <plasma_core_blas.h>

#define A(m, n) (plasma_complex32_t*)plasma_tile_addr(A, m, n)
#define C(m, n) (plasma_complex32_t*)plasma_tile_addr(C, m, n)

/***************************************************************************//**
 * Parallel tile Hermitian rank k update.
 * @see plasma_omp_cherk
 ******************************************************************************/
void plasma_pcherk(plasma_enum_t uplo, plasma_enum_t trans,
                   float alpha, plasma_desc_t A,
                   float beta,  plasma_desc_t C,
                   plasma_sequence_t *sequence, plasma_request_t *request)
{
    // Return if failed sequence.
    if (sequence->status != PlasmaSuccess)
        return;

    for (int n = 0; n < C.nt; n++) {
        int nvcn = plasma_tile_nview(C, n);
        int ldan = plasma_tile_mmain(A, n);
        int ldcn = plasma_tile_mmain(C, n);
        //================
        // PlasmaNoTrans
        //================
        if (trans == PlasmaNoTrans) {
            for (int k = 0; k < A.nt; k++) {
                int nvak = plasma_tile_nview(A, k);
                float dbeta = k == 0 ? beta : 1.0;
                plasma_core_omp_cherk(
                    uplo, trans,
                    nvcn, nvak,
                    alpha, A(n, k), ldan,
                    dbeta, C(n, n), ldcn,
                    sequence, request);
            }
            //==============================
            // PlasmaNoTrans / PlasmaLower
            //==============================
            if (uplo == PlasmaLower) {
                for (int m = n+1; m < C.mt; m++) {
                    int mvcm = plasma_tile_mview(C, m);
                    int ldam = plasma_tile_mmain(A, m);
                    int ldcm = plasma_tile_mmain(C, m);
                    for (int k = 0; k < A.nt; k++) {
                        int nvak = plasma_tile_nview(A, k);
                        plasma_complex32_t zbeta = k == 0 ? beta : 1.0;
                        plasma_core_omp_cgemm(
                            trans, PlasmaConjTrans,
                            mvcm, nvcn, nvak,
                            alpha, A(m, k), ldam,
                                   A(n, k), ldan,
                            zbeta, C(m, n), ldcm,
                            sequence, request);
                    }
                }
            }
            //==============================
            // PlasmaNoTrans / PlasmaUpper
            //==============================
            else {
                for (int m = n+1; m < C.mt; m++) {
                    int mvcm = plasma_tile_mview(C, m);
                    int ldam = plasma_tile_mmain(A, m);
                    for (int k = 0; k < A.nt; k++) {
                        int nvak = plasma_tile_nview(A, k);
                        plasma_complex32_t zbeta = k == 0 ? beta : 1.0;
                        plasma_core_omp_cgemm(
                            trans, PlasmaConjTrans,
                            nvcn, mvcm, nvak,
                            alpha, A(n, k), ldan,
                                   A(m, k), ldam,
                            zbeta, C(n, m), ldcn,
                            sequence, request);
                    }
                }
            }
        }
        //=====================
        // Plasma[_Conj]Trans
        //=====================
        else {
            for (int k = 0; k < A.mt; k++) {
                int mvak = plasma_tile_mview(A, k);
                int ldak = plasma_tile_mmain(A, k);
                float dbeta = k == 0 ? beta : 1.0;
                plasma_core_omp_cherk(
                    uplo, trans,
                    nvcn, mvak,
                    alpha, A(k, n), ldak,
                    dbeta, C(n, n), ldcn,
                    sequence, request);
            }
            //===================================
            // Plasma[_ConjTrans] / PlasmaLower
            //===================================
            if (uplo == PlasmaLower) {
                for (int m = n+1; m < C.mt; m++) {
                    int mvcm = plasma_tile_mview(C, m);
                    int ldcm = plasma_tile_mmain(C, m);
                    for (int k = 0; k < A.mt; k++) {
                        int mvak = plasma_tile_mview(A, k);
                        int ldak = plasma_tile_mmain(A, k);
                        plasma_complex32_t zbeta = k == 0 ? beta : 1.0;
                        plasma_core_omp_cgemm(
                            trans, PlasmaNoTrans,
                            mvcm, nvcn, mvak,
                            alpha, A(k, m), ldak,
                                   A(k, n), ldak,
                            zbeta, C(m, n), ldcm,
                            sequence, request);
                    }
                }
            }
            //===================================
            // Plasma[_ConjTrans] / PlasmaUpper
            //===================================
            else {
                for (int m = n+1; m < C.mt; m++) {
                    int mvcm = plasma_tile_mview(C, m);
                    for (int k = 0; k < A.mt; k++) {
                        int mvak = plasma_tile_mview(A, k);
                        int ldak = plasma_tile_mmain(A, k);
                        plasma_complex32_t zbeta = k == 0 ? beta : 1.0;
                        plasma_core_omp_cgemm(
                            trans, PlasmaNoTrans,
                            nvcn, mvcm, mvak,
                            alpha, A(k, n), ldak,
                                   A(k, m), ldak,
                            zbeta, C(n, m), ldcn,
                            sequence, request);
                    }
                }
            }
        }
    }
}
