template <class VectorType>
  FORCEINLINE void AddBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) {
    SetBlock<VectorType, false>(iPoint, block, alpha);
  }
