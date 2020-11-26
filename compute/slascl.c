/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/compute/zlascl.c, normal z -> s, Fri Sep 28 17:38:08 2018
 *
 **/

#include "plasma.h"
#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_tuning.h"
#include "plasma_types.h"

#include <math.h>

/******************************************************************************/
int plasma_slascl(plasma_enum_t uplo,
                  float cfrom, float cto,
                  int m, int n,
                  float *pA, int lda)
{
    // Get PLASMA context.
    plasma_context_t *plasma = plasma_context_self();
    if (plasma == NULL) {
        plasma_error("PLASMA not initialized");
        return PlasmaErrorNotInitialized;
    }

    // Check input arguments.
    if ((uplo != PlasmaGeneral) &&
        (uplo != PlasmaUpper) &&
        (uplo != PlasmaLower)) {
        plasma_error("illegal value of uplo");
        return -1;
    }
    if (cfrom == 0.0 || isnan(cfrom)) {
        plasma_error("illegal value of cfrom");
        return -2;
    }
    if (isnan(cto)) {
        plasma_error("illegal value of cto");
        return -3;
    }
    if (m < 0) {
        plasma_error("illegal value of m");
        return -4;
    }
    if (n < 0) {
        plasma_error("illegal value of n");
        return -5;
    }
    if (lda < imax(1, m)) {
        plasma_error("illegal value of lda");
        return -7;
    }

    // quick return
    if (imin(n, m) == 0)
      return PlasmaSuccess;

    // Tune parameters.
    if (plasma->tuning)
        plasma_tune_lascl(plasma, PlasmaRealFloat, m, n);

    // Set tiling parameters.
    int nb = plasma->nb;

    // Create tile matrices.
    plasma_desc_t A;
    int retval;
    retval = plasma_desc_general_create(PlasmaRealFloat, nb, nb,
                                        m, n, 0, 0, m, n, &A);
    if (retval != PlasmaSuccess) {
        plasma_error("plasma_general_desc_create() failed");
        return retval;
    }

    // Initialize sequence.
    plasma_sequence_t sequence;
    retval = plasma_sequence_init(&sequence);

    // Initialize request.
    plasma_request_t request;
    retval = plasma_request_init(&request);

    // asynchronous block
    #pragma omp parallel
    #pragma omp master
    {
        // Translate to tile layout.
        plasma_omp_sge2desc(pA, lda, A, &sequence, &request);

        // Call tile async function.
        plasma_omp_slascl(uplo, cfrom, cto, A, &sequence, &request);

        // Translate back to LAPACK layout.
        plasma_omp_sdesc2ge(A, pA, lda, &sequence, &request);
    }
    // implicit synchronization

    // Free matrices in tile layout.
    plasma_desc_destroy(&A);

    // Return status.
    int status = sequence.status;
    return status;
}

/******************************************************************************/
void plasma_omp_slascl(plasma_enum_t uplo,
                       float cfrom, float cto,
                       plasma_desc_t A,
                       plasma_sequence_t *sequence, plasma_request_t *request)
{
    // Get PLASMA context.
    plasma_context_t *plasma = plasma_context_self();
    if (plasma == NULL) {
        plasma_error("PLASMA not initialized");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }

    // Check input arguments.
    if ((uplo != PlasmaGeneral) &&
        (uplo != PlasmaUpper)   &&
        (uplo != PlasmaLower)) {
        plasma_error("illegal value of uplo");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }
    if (cfrom == 0.0 || isnan(cfrom)) {
        plasma_error("illegal value of cfrom");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }
    if (isnan(cto)) {
        plasma_error("illegal value of cto");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }
    if (plasma_desc_check(A) != PlasmaSuccess) {
        plasma_error("invalid A");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }
    if (sequence == NULL) {
        plasma_error("NULL sequence");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }
    if (request == NULL) {
        plasma_error("NULL request");
        plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
        return;
    }

    // quick return
    if (imin(A.m, A.n) == 0)
        return;

    // Call the parallel function.
    plasma_pslascl(uplo, cfrom, cto, A, sequence, request);
}
