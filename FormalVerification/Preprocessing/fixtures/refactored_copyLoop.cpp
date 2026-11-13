template <class T, class U>
void parallelCopy(size_t size, const T* src, U* dst) {
  SU2_OMP_FOR_STAT(2048)
  for (size_t i = size; i-- > 0;) dst[i] = src[i];
  END_SU2_OMP_FOR
}
