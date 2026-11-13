void CGeometry::SetEdges() {
  nEdge = 0;
  for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) {
    for (auto iNode = 0u; iNode < nodes->GetnPoint(iPoint); iNode++) {
      auto jPoint = nodes->GetPoint(iPoint, iNode);
      for (auto jNode = 0u; jNode < nodes->GetnPoint(jPoint); jNode++) {
        if (nodes->GetPoint(jPoint, jNode) == iPoint) {
          auto TestEdge = nodes->GetEdge(jPoint, jNode);
          if (TestEdge == -1) {
            nodes->SetEdge(iPoint, nEdge, iNode);
            nodes->SetEdge(jPoint, nEdge, jNode);
            nEdge++;
          }
          break;
        }
      }
    }
  }

  edges = new CEdge(nEdge, nDim);

  for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) {
    for (auto jPoint : nodes->GetPoints(iPoint)) {
      if (iPoint < jPoint) {
        auto iEdge = FindEdge(iPoint, jPoint);
        edges->SetNodes(iEdge, iPoint, jPoint);
      }
    }
  }
  edges->SetPaddingNodes();
}
