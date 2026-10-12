/*!
 * \file CNativeTopology3D.hpp
 * \brief Indexed tetrahedral dependencies for native private cavity admission.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeMesh3D.hpp"
#include <vector>

namespace SU2Native3D {
using Face = std::array<Id, 3>;
using Edge = std::array<Id, 2>;
struct FaceUse {
  Face key;
  Id cell;
  int orientation;
};
struct EdgeUse {
  Edge key;
  Id cell;
};
struct VertexUse {
  Id key, cell;
};
/*! Immutable incidence of the supplied cells. A patch index cannot certify a globally complete star:
 * the distributed directory must admit every dependency before replacing cells. No embedding certificate. */
class Incidence {
 public:
  explicit Incidence(const std::vector<Cell>& cells, KernelStats* stats = nullptr);
  std::vector<Id> FaceCells(Face face) const;
  std::vector<Id> EdgeCells(Edge edge) const;
  std::vector<Id> VertexCells(Id vertex) const;
  std::vector<Face> Boundary() const;
  size_t FaceComponents() const { return components; }
  size_t FaceEntries() const { return faces.size(); }
  size_t EdgeEntries() const { return edges.size(); }
  size_t VertexEntries() const { return vertices.size(); }

 private:
  std::vector<FaceUse> faces;
  std::vector<EdgeUse> edges;
  std::vector<VertexUse> vertices;
  size_t components = 0;
};
}  // namespace SU2Native3D
