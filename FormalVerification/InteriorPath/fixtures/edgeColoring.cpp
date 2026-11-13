const CCompressedSparsePatternUL& CGeometry::GetEdgeColoring(su2double* efficiency, bool maximizeEdgeColorGroupSize) {
  /*--- Check for dry run mode with dummy geometry. ---*/
  if (nEdge == 0) return edgeColoring;

  /*--- Build if required. ---*/
  if (edgeColoring.empty()) {
    /*--- When not using threading use the natural coloring to reduce overhead. ---*/
    if (omp_get_max_threads() == 1) {
      SetNaturalEdgeColoring();
      if (efficiency != nullptr) *efficiency = 1.0;  // by definition
      return edgeColoring;
    }

    /*--- Create a temporary sparse pattern from the edges. ---*/
    su2vector<unsigned long> outerPtr(nEdge + 1);
    su2vector<unsigned long> innerIdx(nEdge * 2);

    for (unsigned long iEdge = 0; iEdge < nEdge; ++iEdge) {
      outerPtr(iEdge) = 2 * iEdge;
      innerIdx(iEdge * 2 + 0) = edges->GetNode(iEdge, 0);
      innerIdx(iEdge * 2 + 1) = edges->GetNode(iEdge, 1);
    }
    outerPtr(nEdge) = 2 * nEdge;

    CCompressedSparsePatternUL pattern(move(outerPtr), move(innerIdx));

    /*--- Color the edges. ---*/
    constexpr bool balanceColors = true;

    /*--- If requested, find an efficient coloring with maximum color group size (up to edgeColorGroupSize). ---*/
    if (maximizeEdgeColorGroupSize) {
      auto upperEdgeColorGroupSize = edgeColorGroupSize + 1; /* upper bound that is deemed too large */
      auto nextEdgeColorGroupSize = edgeColorGroupSize;      /* next value that we are going to try */
      auto lowerEdgeColorGroupSize = 1ul;                    /* lower bound that is known to work */

      bool admissibleColoring = false; /* keep track wether the last tested coloring is admissible */

      while (true) {
        edgeColoring = colorSparsePattern(pattern, nextEdgeColorGroupSize, false, balanceColors);

        /*--- If the coloring fails, reduce the color group size. ---*/
        if (edgeColoring.empty()) {
          upperEdgeColorGroupSize = nextEdgeColorGroupSize;
          admissibleColoring = false;
        }
        /*--- If the coloring succeeds, check the efficiency. ---*/
        else {
          const su2double currentEfficiency =
              coloringEfficiency(edgeColoring, omp_get_max_threads(), nextEdgeColorGroupSize);

          /*--- If the coloring is not efficient, reduce the color group size. ---*/
          if (currentEfficiency < COLORING_EFF_THRESH) {
            upperEdgeColorGroupSize = nextEdgeColorGroupSize;
            admissibleColoring = false;
          }
          /*--- Otherwise, enlarge the color group size. ---*/
          else {
            lowerEdgeColorGroupSize = nextEdgeColorGroupSize;
            admissibleColoring = true;
          }
        }

        const auto increment = (upperEdgeColorGroupSize - lowerEdgeColorGroupSize) / 2;
        nextEdgeColorGroupSize = lowerEdgeColorGroupSize + increment;

        /*--- Terminating condition. ---*/
        if (increment == 0) {
          break;
        }
      }

      edgeColorGroupSize = nextEdgeColorGroupSize;

      /*--- If the last tested coloring was not admissible, recompute the final coloring. ---*/
      if (!admissibleColoring) {
        edgeColoring = colorSparsePattern(pattern, edgeColorGroupSize, false, balanceColors);
      }
    }
    /*--- No adaptivity. ---*/
    else {
      edgeColoring = colorSparsePattern(pattern, edgeColorGroupSize, false, balanceColors);
    }

    /*--- If the coloring fails use the natural coloring. This is a
     *    "soft" failure as this "bad" coloring should be detected
     *    downstream and a fallback strategy put in place. ---*/
    if (edgeColoring.empty()) SetNaturalEdgeColoring();
  }

  if (efficiency != nullptr) {
    *efficiency = coloringEfficiency(edgeColoring, omp_get_max_threads(), edgeColorGroupSize);
  }
  return edgeColoring;
}
