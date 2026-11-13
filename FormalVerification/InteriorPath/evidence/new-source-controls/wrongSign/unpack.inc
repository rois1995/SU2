template <size_t N, size_t nVar, class VecTypeSIMD, class F>
  FORCEINLINE static void UnpackBlock(const VecTypeSIMD& in, simd::Array<F, N> mask, ScalarType out[][nVar]) {
    static_assert(VecTypeSIMD::StaticSize, "This method requires static size vectors.");
    for (size_t i = 0; i < nVar; ++i) {
      SU2_OMP_SIMD_IF_NOT_AD
      for (size_t k = 0; k < N; ++k) out[k][i] = mask[k] * in[i][k];
    }
  }
