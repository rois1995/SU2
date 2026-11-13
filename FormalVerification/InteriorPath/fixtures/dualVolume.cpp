su2double CEdge::GetVolume(const su2double* coord_Edge_CG, const su2double* coord_Elem_CG,
                           const su2double* coord_Point) {
  constexpr unsigned long nDim = 2;

  su2double vec_a[nDim] = {0.0}, vec_b[nDim] = {0.0};

  Distance(nDim, coord_Elem_CG, coord_Point, vec_a);
  Distance(nDim, coord_Edge_CG, coord_Point, vec_b);

  return 0.5 * fabs(vec_a[0] * vec_b[1] - vec_a[1] * vec_b[0]);
}
