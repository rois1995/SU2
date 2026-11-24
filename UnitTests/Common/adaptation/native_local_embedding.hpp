/*!
 * \file native_local_embedding.hpp
 * \brief Independent all-cell embedding check for the FP2-C3 fixtures and lab.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace native_fp2_c3 {
struct EmbeddingCounts {
  size_t violations = 0, pairs = 0, calls = 0, peak_bytes = 0;
};

inline EmbeddingCounts EmbeddingViolations(const SU2NativeLocal3D::Mesh& mesh, size_t ceiling = SIZE_MAX,
                                          size_t caller = 0) {
  using transfer_memory::Add;
  using transfer_memory::Mul;
  EmbeddingCounts counts;
  const auto& cells = mesh.Cells();
  const size_t n = std::count_if(cells.begin(), cells.end(), [](const auto& cell) { return cell.live; });
  if (n == 0) return counts;
  const auto admit = [&](size_t bytes) {
    const auto total = Add(caller, bytes);
    if (total == SIZE_MAX || total > ceiling)
      throw std::runtime_error("FP2-C3 embedding check exceeds modeled storage.");
    counts.peak_bytes = std::max(counts.peak_bytes, bytes);
  };
  const auto index_bytes = Mul(n, 6 * sizeof(su2double) + sizeof(SU2NativeLocal3D::Slot));
  const auto build_bytes = Mul(n, 3 * sizeof(unsigned long) + 2 * sizeof(unsigned short));
  // PredictBytes uses unchecked products. Bound the arguments before entering it.
  if (Mul(Mul(4096, Add(n, 1)), size_t(omp_get_max_threads())) == SIZE_MAX)
    throw std::runtime_error("FP2-C3 embedding index dimensions overflow modeled storage.");
  size_t retained = 0, constructor_peak = 0;
  CADTElemClass::PredictBytes(3, 2 * n, n, 2 * n, &retained, &constructor_peak);
  admit(Add(index_bytes, build_bytes, constructor_peak));
  std::vector<su2double> boxes;
  boxes.reserve(6 * n);
  std::vector<SU2NativeLocal3D::Slot> slots;
  slots.reserve(n);
  const auto node = [&](auto slot) { return mesh.Points()[slot].node; };
  for (size_t slot = 0; slot < cells.size(); ++slot) {
    const auto& cell = cells[slot];
    if (!cell.live) continue;
    slots.push_back(static_cast<SU2NativeLocal3D::Slot>(slot));
    const auto p = node(cell.vertices.front()).p;
    std::array<su2double, 6> box{p.x, p.y, p.z, p.x, p.y, p.z};
    for (const auto point : cell.vertices) {
      const auto q = node(point).p;
      const std::array<su2double, 3> xyz{q.x, q.y, q.z};
      for (unsigned d = 0; d < 3; ++d) {
        box[d] = std::min(box[d], xyz[d]);
        box[d + 3] = std::max(box[d + 3], xyz[d]);
      }
    }
    boxes.insert(boxes.end(), box.begin(), box.end());
  }
  auto tree = [&] {
    std::vector<unsigned long> connectivity(2 * n), ids(n);
    std::iota(connectivity.begin(), connectivity.end(), 0UL);
    std::iota(ids.begin(), ids.end(), 0UL);
    std::vector<unsigned short> types(n, LINE), markers(n, 0);
    return CADTElemClass(3, boxes, connectivity, types, markers, ids, false);
  }();
  // RetainedWorkspaceBound covers the new fronts. Add space for the old front during growth.
  admit(Add(index_bytes, tree.GetAllocatedBytes(), Mul(2, tree.RetainedWorkspaceBound(1)),
            Mul(n, sizeof(unsigned long))));
  std::vector<unsigned long> hits;
  hits.reserve(n);
  const auto exact_cell = [&](size_t i) {
    SU2Native3D::Cell cell{};
    const auto& vertices = cells[slots[i]].vertices;
    std::transform(vertices.begin(), vertices.end(), cell.v.begin(), node);
    return cell;
  };
  for (size_t i = 0; i < n; ++i) {
    const auto* a = boxes.data() + 6 * i;
    tree.DetermineIntersectingElements(a, a + 3, hits);
    const auto cell = exact_cell(i);
    for (const auto j : hits) {
      if (j <= i) continue;
      const auto* b = boxes.data() + 6 * j;
      // The ADT inflates its own boxes, so discard those extra hits.
      if (a[0] > b[3] || b[0] > a[3] || a[1] > b[4] || b[1] > a[4] || a[2] > b[5] || b[2] > a[5])
        continue;
      ++counts.pairs;
      ++counts.calls;
      counts.violations += !SU2Native3D::ConformingDisjoint(cell, exact_cell(j));
    }
  }
  return counts;
}
}  // namespace native_fp2_c3
