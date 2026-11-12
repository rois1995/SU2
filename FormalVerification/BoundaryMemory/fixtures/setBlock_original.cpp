template <class VectorType, bool Overwrite = true>
  FORCEINLINE void SetBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) {
    if (Overwrite) {
      for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i] = alpha * block[i];
    } else {
      for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i] += alpha * block[i];
    }
  }
