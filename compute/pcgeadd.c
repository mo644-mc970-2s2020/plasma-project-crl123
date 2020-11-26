/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/compute/pzgeadd.c, normal z -> c, Fri Sep 28 17:38:10 2018
 *
 **/

#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_types.h"
#include "plasma_workspace.h"
#include <plasma_core_blas.h>

#define A(m,n) (plasma_complex32_t*)plasma_tile_addr(A, m, n)
#define B(m,n) (plasma_complex32_t*)plasma_tile_addr(B, m, n)

/***************************************************************************//**
 * Parallel tile matrix-matrix addition.
 * @see plasma_omp_cgeadd
 ******************************************************************************/
void plasma_pcgeadd(plasma_enum_t transa,
                    plasma_complex32_t alpha,  plasma_desc_t A,
                    plasma_complex32_t beta,   plasma_desc_t B,
                    plasma_sequence_t *sequence, plasma_request_t *request)
{
    // Return if failed sequence.
    if (sequence->status != PlasmaSuccess)
        return;

    //===============
    // PlasmaNoTrans
    //===============
    if (transa == PlasmaNoTrans) {
        for (int m = 0; m < B.mt; m++) {
            int mvbm = plasma_tile_mview(B, m);
            int ldam = plasma_tile_mmain(A, m);
            int ldbm = plasma_tile_mmain(B, m);
            for (int n = 0; n < B.nt; n++) {
                int nvbn = plasma_tile_nview(B, n);
                plasma_core_omp_cgeadd(
                    transa, mvbm, nvbn,
                    alpha, A(m, n), ldam,
                    beta,  B(m, n), ldbm,
                    sequence, request);
            }
        }
    }
    //====================
    // Plasma[_Conj]Trans
    //====================
    else {
        for (int m = 0; m < B.mt; m++) {
            int mvbm = plasma_tile_mview(B, m);
            int ldbm = plasma_tile_mmain(B, m);
            for (int n = 0; n < B.nt; n++) {
                int nvbn = plasma_tile_nview(B, n);
                int ldan = plasma_tile_mmain(A, n);
                plasma_core_omp_cgeadd(
                    transa, mvbm, nvbn,
                    alpha, A(n, m), ldan,
                    beta,  B(m, n), ldbm,
                    sequence, request);
            }
        }
    }
}
