/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/compute/pzgetri_aux.c, normal z -> d, Fri Sep 28 17:38:12 2018
 *
 **/

#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_types.h"
#include "plasma_workspace.h"
#include <plasma_core_blas.h>

#define A(m, n) (double*)plasma_tile_addr(A, m, n)
#define W(m)    (double*)plasma_tile_addr(W, m, 0)

/***************************************************************************//**
 *  Parallel dgetri auxrialiry routine - dynamic scheduling
 **/
void plasma_pdgetri_aux(plasma_desc_t A, plasma_desc_t W,
                        plasma_sequence_t *sequence, plasma_request_t *request)
{
    if (sequence->status != PlasmaSuccess)
        return;

    for (int k = A.mt-1; k >= 0; k--) {
        int mvak = plasma_tile_mview(A, k);
        int nvak = plasma_tile_nview(A, k);

        int ldak = plasma_tile_mmain(A, k);
        int ldakn= plasma_tile_mmain(A, k);
        int ldwk = plasma_tile_mmain(W, k);

        // copy L(k, k) into W(k)
        plasma_core_omp_dlacpy(
            PlasmaLower, PlasmaNoTrans,
            mvak, nvak,
            A(k, k), ldak, W(k), ldwk,
            sequence, request );
        // zero strictly-lower part of U(k, k)
        plasma_core_omp_dlaset(
            PlasmaLower,
            ldak, ldakn, 1, 0,
            nvak-1, nvak-1,
            0.0, 0.0, A(k, k));

        for (int m = k+1; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            int ldwm = plasma_tile_mmain(W, m);
            // copy L(m, k) to W(m)
            plasma_core_omp_dlacpy(
                PlasmaGeneral, PlasmaNoTrans,
                mvam, nvak,
                A(m, k), ldam, W(m), ldwm,
                sequence, request );
            // zero U(m, k)
            plasma_core_omp_dlaset(
                PlasmaGeneral,
                ldam, ldakn, 0, 0,
                mvam, nvak,
                0.0, 0.0, A(m, k));
        }

        // update A(:, k) = A(:, k)-A(:, k+1:nt)*L(k+1:nt, k)
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            for (int n = k+1; n < A.nt; n++) {
                int nvan = plasma_tile_nview(A, n);
                int ldwn = plasma_tile_mmain(W, n);
                plasma_core_omp_dgemm(
                     PlasmaNoTrans, PlasmaNoTrans,
                     mvam, nvak, nvan,
                     -1.0, A(m, n), ldam,
                           W( n ),  ldwn,
                      1.0, A(m, k), ldam,
                      sequence, request);
            }
        }

        // compute A(:, k) = A(:, k) L(k, k)^{-1}
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            plasma_core_omp_dtrsm(
                PlasmaRight, PlasmaLower,
                PlasmaNoTrans, PlasmaUnit,
                mvam, nvak,
                1.0, W( k ),   ldwk,
                     A( m, k ),ldam,
                sequence, request );
        }
    }
}
