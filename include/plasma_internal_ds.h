/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/include/plasma_internal_zc.h, mixed zc -> ds, Fri Sep 28 17:38:01 2018
 *
 **/
#ifndef PLASMA_INTERNAL_DS_H
#define PLASMA_INTERNAL_DS_H

#include "plasma_async.h"
#include "plasma_descriptor.h"
#include "plasma_types.h"
#include "plasma_workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
void plasma_pdlag2s(plasma_desc_t A, plasma_desc_t As,
                    plasma_sequence_t *sequence, plasma_request_t *request);

void plasma_pslag2d(plasma_desc_t As, plasma_desc_t A,
                    plasma_sequence_t *sequence, plasma_request_t *request);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // PLASMA_INTERNAL_DS_H
