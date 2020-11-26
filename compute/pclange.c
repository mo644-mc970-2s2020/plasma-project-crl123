/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/compute/pzlange.c, normal z -> c, Fri Sep 28 17:38:12 2018
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

/***************************************************************************//**
 *  Parallel tile calculation of max, one, infinity or Frobenius matrix norm
 *  for a general matrix.
 ******************************************************************************/
void plasma_pclange(plasma_enum_t norm,
                    plasma_desc_t A, float *work, float *value,
                    plasma_sequence_t *sequence, plasma_request_t *request)
{
    // Return if failed sequence.
    if (sequence->status != PlasmaSuccess)
        return;

    switch (norm) {
    float stub;
    float *workspace;
    float *scale;
    float *sumsq;
    //================
    // PlasmaMaxNorm
    //================
    case PlasmaMaxNorm:
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            for (int n = 0; n < A.nt; n++) {
                int nvan = plasma_tile_nview(A, n);
                plasma_core_omp_clange(PlasmaMaxNorm,
                                mvam, nvan,
                                A(m, n), ldam,
                                &stub, &work[A.mt*n+m],
                                sequence, request);
            }
        }
        #pragma omp taskwait
        plasma_core_omp_slange(PlasmaMaxNorm,
                        A.mt, A.nt,
                        work, A.mt,
                        &stub, value,
                        sequence, request);
        break;
    //================
    // PlasmaOneNorm
    //================
    case PlasmaOneNorm:
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            for (int n = 0; n < A.nt; n++) {
                int nvan = plasma_tile_nview(A, n);
                plasma_core_omp_clange_aux(PlasmaOneNorm,
                                    mvam, nvan,
                                    A(m, n), ldam,
                                    &work[A.n*m+n*A.nb],
                                    sequence, request);
            }
        }
        #pragma omp taskwait
        workspace = work + A.mt*A.n;
        plasma_core_omp_slange(PlasmaInfNorm,
                        A.n, A.mt,
                        work, A.n,
                        workspace, value,
                        sequence, request);
        break;
    //================
    // PlasmaInfNorm
    //================
    case PlasmaInfNorm:
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            for (int n = 0; n < A.nt; n++) {
                int nvan = plasma_tile_nview(A, n);
                plasma_core_omp_clange_aux(PlasmaInfNorm,
                                    mvam, nvan,
                                    A(m, n), ldam,
                                    &work[A.m*n+m*A.mb],
                                    sequence, request);
            }
        }
        #pragma omp taskwait
        workspace = work + A.nt*A.m;
        plasma_core_omp_slange(PlasmaInfNorm,
                        A.m, A.nt,
                        work, A.m,
                        workspace, value,
                        sequence, request);
        break;
    //======================
    // PlasmaFrobeniusNorm
    //======================
    case PlasmaFrobeniusNorm:
        scale = work;
        sumsq = work + A.mt*A.nt;
        for (int m = 0; m < A.mt; m++) {
            int mvam = plasma_tile_mview(A, m);
            int ldam = plasma_tile_mmain(A, m);
            for (int n = 0; n < A.nt; n++) {
                int nvan = plasma_tile_nview(A, n);
                plasma_core_omp_cgessq(mvam, nvan,
                                A(m, n), ldam,
                                &scale[A.mt*n+m], &sumsq[A.mt*n+m],
                                sequence, request);
            }
        }
        #pragma omp taskwait
        plasma_core_omp_sgessq_aux(A.mt*A.nt,
                            scale, sumsq,
                            value,
                            sequence, request);
        break;
    }
}
