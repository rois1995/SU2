T createNaturalColoring(Index_t numInnerIndexes) {
  /*--- One color. ---*/
  su2vector<Index_t> outerPtr(2);
  outerPtr(0) = 0;
  outerPtr(1) = numInnerIndexes;

  /*--- Containing all indexes in ascending order. ---*/
  su2vector<Index_t> innerIdx(numInnerIndexes);
  std::iota(innerIdx.data(), innerIdx.data() + numInnerIndexes, 0);

  return T(std::move(outerPtr), std::move(innerIdx));
}
