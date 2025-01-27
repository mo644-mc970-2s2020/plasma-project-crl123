/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from /home/luszczek/workspace/plasma/bitbucket/plasma/core_blas/core_zsymm.c, normal z -> s, Fri Sep 28 17:38:23 2018
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "core_lapack.h"

/***************************************************************************//**
 *
 * @ingroup core_symm
 *
 *  Performs one of the matrix-matrix operations
 *
 *     \f[ C = \alpha \times A \times B + \beta \times C \f]
 *  or
 *     \f[ C = \alpha \times B \times A + \beta \times C \f]
 *
 *  where alpha and beta are scalars, A is a symmetric matrix and B and
 *  C are m-by-n matrices.
 *
 *******************************************************************************
 *
 * @param[in] side
 *          Specifies whether the symmetric matrix A appears on the
 *          left or right in the operation as follows:
 *          - PlasmaLeft:  \f[ C = \alpha \times A \times B + \beta \times C \f]
 *          - PlasmaRight: \f[ C = \alpha \times B \times A + \beta \times C \f]
 *
 * @param[in] uplo
 *          Specifies whether the upper or lower triangular part of
 *          the symmetric matrix A is to be referenced as follows:
 *          - PlasmaLower:     Only the lower triangular part of the
 *                             symmetric matrix A is to be referenced.
 *          - PlasmaUpper:     Only the upper triangular part of the
 *                             symmetric matrix A is to be referenced.
 *
 * @param[in] m
 *          The number of rows of the matrix C. m >= 0.
 *
 * @param[in] n
 *          The number of columns of the matrix C. n >= 0.
 *
 * @param[in] alpha
 *          The scalar alpha.
 *
 * @param[in] A
 *          A is an lda-by-ka matrix, where ka is m when side = PlasmaLeft,
 *          and is n otherwise. Only the uplo triangular part is referenced.
 *
 * @param[in] lda
 *          The leading dimension of the array A. lda >= max(1,ka).
 *
 * @param[in] B
 *          B is an ldb-by-n matrix, where the leading m-by-n part of
 *          the array B must contain the matrix B.
 *
 * @param[in] ldb
 *          The leading dimension of the array B. ldb >= max(1,m).
 *
 * @param[in] beta
 *          The scalar beta.
 *
 * @param[in,out] C
 *          C is an ldc-by-n matrix.
 *          On exit, the array is overwritten by the m-by-n updated matrix.
 *
 * @param[in] ldc
 *          The leading dimension of the array C. ldc >= max(1,m).
 *
 ******************************************************************************/
__attribute__((weak))
void plasma_core_ssymm(plasma_enum_t side, plasma_enum_t uplo,
                int m, int n,
                float alpha, const float *A, int lda,
                                          const float *B, int ldb,
                float beta,        float *C, int ldc)
{
    cblas_ssymm(CblasColMajor,
                (CBLAS_SIDE)side, (CBLAS_UPLO)uplo,
                m, n,
                (alpha), A, lda,
                                    B, ldb,
                (beta),  C, ldc);
}

/******************************************************************************/
void plasma_core_omp_ssymm(
    plasma_enum_t side, plasma_enum_t uplo,
    int m, int n,
    float alpha, const float *A, int lda,
                              const float *B, int ldb,
    float beta,        float *C, int ldc,
    plasma_sequence_t *sequence, plasma_request_t *request)
{
    int ak;
    if (side == PlasmaLeft)
        ak = m;
    else
        ak = n;

   int side_new = side;
   int uplo_new = uplo; 


//    #pragma omp task depend(in:A[0:lda*ak]) \
//                     depend(in:B[0:ldb*n]) \
//                     depend(inout:C[0:ldc*n])
    
    int size_A = lda*ak, size_B = ldb*n,size_C =ldc*n;
    if (sequence->status == PlasmaSuccess)
    {
        #pragma omp target nowait                              \
            depend(in:A[0:lda*ak])                              \
            depend(in:B[0:ldb*n])                              \
            depend(inout:C[0:ldc*n])                            \
            firstprivate(m,n,alpha,beta,lda,ak)               \
            firstprivate(ldb,ldc,side_new,uplo_new)            \
            map(to:A[0:size_A],B[0:size_B])                     \
            map(tofrom:C[0:size_C])
        {
        plasma_core_ssymm(side_new, uplo_new,
                       m, n,
                       alpha, A, lda,
                              B, ldb,
                       beta,  C, ldc);
    
	} 
    }
}
