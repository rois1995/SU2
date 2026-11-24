/*!
 * \file p1_cell_blocks.hpp
 * \brief Cell-local off-diagonal block references for P1 Jacobian assembly.
 * \version 8.6.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../geometry/CGeometry.hpp"
#include "../linear_algebra/CSysMatrix.hpp"

#include <cstddef>
#include <type_traits>
#include <vector>

/*--- One map per Jacobian, shared by the P1 flow and turbulence assembly code. ---*/
class P1CellBlocks {
  using Matrix = CSysMatrix<su2mixedfloat>;
  using BlockRef = Matrix::BlockRef;
  static constexpr bool UseBlockRefs = std::is_same_v<su2double, passivedouble>;

  std::vector<std::size_t> offsets;
  std::vector<BlockRef> refs;

 public:
  struct Cell {
    const BlockRef* refs;
    unsigned short nNodes;

    /*! \brief Add the off-diagonal block with the same update as indexed assembly (bitwise target). */
    FORCEINLINE void AddOffDiag(Matrix& matrix, unsigned short a, unsigned short b, unsigned long row,
                               unsigned long col, const su2double* const* values) const {
      if constexpr (UseBlockRefs) {
        if (refs) {
          /*--- Each row packs nNodes - 1 references, skipping its diagonal. ---*/
          matrix.AddBlock(refs[a * (nNodes - 1) + b - (b > a)], values);
          return;
        }
      }
      matrix.AddBlock(row, col, values);
    }
  };

  /*! \brief Pack the ordered off-diagonal pairs for a cell or sub-simplex (bitwise target). */
  template <class GetPoint>
  static void BuildCell(const CGeometry& geometry, const Matrix& matrix, unsigned short nNodes,
                        GetPoint point, BlockRef* refs) {
    for (auto a = 0u; a < nNodes; ++a) {
      const auto row = point(a);
      const bool owned = geometry.nodes->GetDomain(row);
      for (auto b = 0u; b < nNodes; ++b) {
        if (a == b) continue;
        *refs++ = owned ? matrix.GetBlockRef(row, point(b)) : BlockRef::Absent;
      }
    }
  }

  /*! \brief Resolve ordered off-diagonal pairs in the matrix's stored pattern once (bitwise target). */
  void Build(const CGeometry& geometry, const Matrix& matrix) {
    if constexpr (!UseBlockRefs) return;
    if (matrix.IsQuantized()) return;

    const auto nElem = geometry.GetnElem();
    offsets.resize(nElem + 1);
    for (auto iElem = 0ul; iElem < nElem; ++iElem) {
      const auto nNodes = geometry.elem[iElem]->GetnNodes();
      offsets[iElem + 1] = offsets[iElem] + nNodes * (nNodes - 1);
    }
    refs.resize(offsets.back());

    for (auto iElem = 0ul; iElem < nElem; ++iElem) {
      const auto* element = geometry.elem[iElem];
      const auto nNodes = element->GetnNodes();
      BuildCell(geometry, matrix, nNodes, [element](auto a) { return element->GetNode(a); },
                refs.data() + offsets[iElem]);
    }
  }

  /*! \brief Read the cell's packed rows, or select indexed assembly when the map is disabled. */
  Cell Get(unsigned long iElem, unsigned short nNodes) const {
    if constexpr (UseBlockRefs) {
      if (!refs.empty()) return {refs.data() + offsets[iElem], nNodes};
    }
    return {nullptr, nNodes};
  }
};
