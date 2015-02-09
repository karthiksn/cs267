#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <emmintrin.h>
#include <immintrin.h>
#include <pmmintrin.h>

const char* dgemm_desc = "Simple blocked dgemm.";

#if !defined(BLOCK_SIZE)
#define BLOCK_SIZE 16
#endif

#define min(a,b) (((a)<(b))?(a):(b))

// 16 byte aligned malloc
#define malloc16(N) \
  (((uintptr_t) (malloc(N + 16)) + 16) & ~(uintptr_t) 0x0F)
#define get_align16(ptr) ((int) (((uintptr_t) (ptr)) & 15))
#define check_aligned16(ptr) assert(get_align16(ptr) == 0);

__attribute__((always_inline))
inline double dotSSE(double* a, double* b, int n) {
  // printf("align: %d\n", get_align16(a));
  // printf("align: %d\n\n", get_align16(b));

  // assert(get_align16(a) == 0);
  // assert(get_align16(b) == 0);

  double dot[2];
  __m128d vdot = _mm_set1_pd(0);

  for (int i = 0; i < (n / 2) * 2; i += 2) {
    __m128d v_a = _mm_loadu_pd(&a[i]);
    __m128d v_b = _mm_loadu_pd(&b[i]);

    v_a = _mm_mul_pd(v_a, v_b);
    vdot = _mm_add_pd(vdot, v_a);
  }

  _mm_store_pd(dot, vdot);

  return (n % 2 == 0 ? dot[0] + dot[1] :
          dot[0] + dot[1] + a[n - 1] * b[n - 1]);
}

/* This auxiliary subroutine performs a smaller dgemm operation
 *  C := C + A * B
 * where C is M-by-N, A is M-by-K, and B is K-by-N. */
static void do_block(int lda, int M, int N, int K, double* A_t, double* B,
                     double* C) {
  /* For each row i of A */
  for (int j = 0; j < N; ++j)
  {
    /* For each column j of B */
    for (int i = 0; i < M; ++i)
    {
      /* Compute C(i,j) */
      // double cij = C[i+j*lda];
      // for (int k = 0; k < K; ++k)
      // {
      //   cij += A_t[k+i*lda] * B[k+j*lda];
      // }
      // C[i+j*lda] = cij;

      C[i+j*lda] += dotSSE(&A_t[i*lda], &B[j*lda], K);
    }
  }
}

/* This routine performs a dgemm operation
 *  C := C + A * B
 * where A, B, and C are lda-by-lda matrices stored in column-major
 * format. On exit, A and B maintain their input values. */
void square_dgemm(int lda, double* A, double* B, double* C) {
  // Re-allocate space for A, B, and C. This time, ensure that they're
  // all 16-byte aligned. Also, A needs to be transposed.
  double* al_A_t = (double*) malloc16(sizeof(double) * lda * lda);
  double* al_B = (double*) malloc16(sizeof(double) * lda * lda);
  double* al_C = (double*) malloc16(sizeof(double) * lda * lda);

  // Ensure that the pointers are in fact aligned
  check_aligned16(al_A_t);
  check_aligned16(al_B);
  check_aligned16(al_C);

  // Move A over -- this requires a transpose
  memcpy(al_A_t, A, lda * lda * sizeof(double));
  for (int i = 0; i < lda; ++i) {
    for (int j = 0; j < lda; ++j) {
      al_A_t[j+i*lda] = A[i+j*lda];
    }
  }

  // Move B over -- we can copy this over verbatim
  memcpy(al_B, B, lda * lda * sizeof(double));

  /* For each block-row of A */
  for (int i = 0; i < lda; i += BLOCK_SIZE) {
    /* For each block-column of B */
    for (int j = 0; j < lda; j += BLOCK_SIZE) {
      /* Accumulate block dgemms into block of C */
      for (int k = 0; k < lda; k += BLOCK_SIZE)
      {
        /* Correct block dimensions if block "goes off edge of" the
         * matrix */
        int M = min (BLOCK_SIZE, lda-i);
        int N = min (BLOCK_SIZE, lda-j);
        int K = min (BLOCK_SIZE, lda-k);

        /* Perform individual block dgemm */
        check_aligned16(al_A_t + k + i * lda);

        do_block(lda, M, N, K, al_A_t + k + i*lda, al_B + k + j*lda,
                 C + i + j*lda);
      }
    }
  }
}
