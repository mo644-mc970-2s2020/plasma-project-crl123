/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_zherk.c, normal z -> c, Fri Sep 28 17:38:21 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "core_lapack.h"

/***************************************************************************//**
 *
 * @ingroup core_herk
 *
 *  Performs one of the Hermitian rank k operations
 *
 *    \f[ C = \alpha A \times A^H + \beta C, \f]
 *    or
 *    \f[ C = \alpha A^H \times A + \beta C, \f]
 *
 *  where alpha and beta are real scalars, C is an n-by-n Hermitian
 *  matrix, and A is an n-by-k matrix in the first case and a k-by-n
 *  matrix in the second case.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          - PlasmaUpper: Upper triangle of C is stored;
 *          - PlasmaLower: Lower triangle of C is stored.
 *
 * @param[in] trans
 *          - PlasmaNoTrans:   \f[ C = \alpha A \times A^H + \beta C; \f]
 *          - PlasmaConjTrans: \f[ C = \alpha A^H \times A + \beta C. \f]
 *
 * @param[in] n
 *          The order of the matrix C. n >= 0.
 *
 * @param[in] k
 *          If trans = PlasmaNoTrans, number of columns of the A matrix;
 *          if trans = PlasmaConjTrans, number of rows of the A matrix.
 *
 * @param[in] alpha
 *          The scalar alpha.
 *
 * @param[in] A
 *          A is an lda-by-ka matrix.
 *          If trans = PlasmaNoTrans,   ka = k;
 *          if trans = PlasmaConjTrans, ka = n.
 *
 * @param[in] lda
 *          The leading dimension of the array A.
 *          If trans = PlasmaNoTrans,   lda >= max(1, n);
 *          if trans = PlasmaConjTrans, lda >= max(1, k).
 *
 * @param[in] beta
 *          The scalar beta.
 *
 * @param[in,out] C
 *          C is an ldc-by-n matrix.
 *          On exit, the uplo part of the matrix is overwritten
 *          by the uplo part of the updated matrix.
 *
 * @param[in] ldc
 *          The leading dimension of the array C. ldc >= max(1, n).
 *
 ******************************************************************************/
__attribute__((weak))
void plasma_core_cherk(plasma_enum_t uplo, plasma_enum_t trans,
                int n, int k,
                float alpha, const plasma_complex32_t *A, int lda,
                float beta,        plasma_complex32_t *C, int ldc)
{
    cblas_cherk(CblasColMajor,
                (CBLAS_UPLO)uplo, (CBLAS_TRANSPOSE)trans,
                n, k,
                alpha, A, lda,
                beta,  C, ldc);
}

/******************************************************************************/
void plasma_core_omp_cherk(plasma_enum_t uplo, plasma_enum_t trans,
                    int n, int k,
                    float alpha, const plasma_complex32_t *A, int lda,
                    float beta,        plasma_complex32_t *C, int ldc,
                    plasma_sequence_t *sequence, plasma_request_t *request)
{
    int ak;
    if (trans == PlasmaNoTrans)
        ak = k;
    else
        ak = n;

    #pragma omp task depend(in:A[0:lda*ak]) \
                     depend(inout:C[0:ldc*n])
    {
        if (sequence->status == PlasmaSuccess)
            plasma_core_cherk(uplo, trans,
                       n, k,
                       alpha, A, lda,
                       beta,  C, ldc);
    }
}
