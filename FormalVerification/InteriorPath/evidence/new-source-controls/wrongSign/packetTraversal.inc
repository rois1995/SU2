for(auto k = 0ul; k < color.size; k += Double::Size) {
      Int iEdge;
      Double mask;
      for (auto j = 0ul; j < Double::Size; ++j) {
        bool in = (k+j < color.size);
        mask[j] = in;
        iEdge[j] = color.indices[k+j*in];
      }

      if (ReducerStrategy) {
        edgeNumerics->ComputeFlux(iEdge, *config, *geometry, *nodes, UpdateType::REDUCTION, mask, EdgeFluxes, Jacobian, massFluxes);
      } else {
        edgeNumerics->ComputeFlux(iEdge, *config, *geometry, *nodes, UpdateType::COLORING, mask, LinSysRes, Jacobian, massFluxes);
      }
      if (MGLevel == MESH_0) {
        for (auto j = 0ul; j < Double::Size; ++j)
          counterLocal += (nodes->NonPhysicalEdgeCounter[iEdge[j]] > 0);
      }
    }
