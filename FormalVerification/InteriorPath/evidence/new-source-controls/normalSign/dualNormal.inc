void CEdge::SetNodes_Coord(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_Elem_CG) {
  Normal(iEdge, 0) += coord_Elem_CG[1] - coord_Edge_CG[1];
  Normal(iEdge, 1) += coord_Elem_CG[0] - coord_Edge_CG[0];
}
