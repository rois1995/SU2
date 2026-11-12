template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>
  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,
                                simd::Array<F, N> mask = 1) {
    /*--- "Transpose" and scale input vector. ---*/
    constexpr size_t nVar = VecTypeSIMD::StaticSize;
    assert(nVar == this->nVar);
    ScalarType unpacked[N][nVar];
    UnpackBlock(vector, mask, unpacked);

    /*--- Update one by one skipping if mask is 0. ---*/
    for (size_t k = 0; k < N; ++k) {
      if (mask[k] == 0) continue;
      SU2_OMP_SIMD
      for (size_t i = 0; i < nVar; ++i) {
        vec_val[iPoint[k] * nVar + i] += unpacked[k][i];
        vec_val[jPoint[k] * nVar + i] -= unpacked[k][i];
      }
    }
  }
