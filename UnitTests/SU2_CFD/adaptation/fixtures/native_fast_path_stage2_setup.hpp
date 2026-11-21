#pragma once

#include "../../../../Common/include/adaptation/CNativeBoundaryParameters3D.hpp"
#include "../../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../../Common/include/adaptation/CNativeImport3D.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace native_fp2_fixture {
using Clock = std::chrono::steady_clock;
constexpr double Tolerance = 1e-4;

inline SU2NativeGeometry::AtlasBudget Budget(size_t cells, size_t caller, Clock::time_point start, double seconds,
                                           size_t ceiling) {
  constexpr size_t scratch = size_t{16} << 20;
  size_t low = 0, high = scratch * 8;
  while (low < high) {
    const auto bits = low + (high - low + 1) / 2;
    if (transfer_memory::Mul(2, SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits)) <= scratch / 2)
      low = bits;
    else
      high = bits - 1;
  }
  SU2NativeGeometry::AtlasBudget budget;
  const auto work = std::max(size_t{4096}, transfer_memory::Mul(cells, 64));
  budget.maximum_work =
      transfer_memory::Mul(transfer_memory::Add(cells, work), SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  if (!low || budget.maximum_work == SIZE_MAX)
    throw std::length_error("Frozen surface fixture geometry budget overflows.");
  budget.maximum_bytes = ceiling;
  budget.maximum_integer_bits = low;
  budget.caller_bytes = caller;
  budget.within_time = [start, seconds] { return std::chrono::duration<double>(Clock::now() - start).count() < seconds; };
  return budget;
}

inline SU2NativeGeometry::AtlasBudget PrepareIN1(SU2Native3D::ReferenceState& reference, size_t ceiling,
                                                size_t retained = 0) {
  const auto& state = *reference.original->Parameters();
  const size_t baseline = transfer_memory::Add(reference.original->ResidentBytes(), retained);
  if (baseline >= ceiling) throw std::length_error("Curved mobility setup exceeds storage admission.");
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_bytes = ceiling;
  budget.caller_bytes = baseline;
  budget.maximum_integer_bits = transfer_memory::Mul(ceiling - baseline, 8);
  budget.maximum_work = transfer_memory::Mul(
      transfer_memory::Add(state.Geometry().Reference().Facets().size(), state.Surface().size()),
      SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  if (budget.maximum_work == SIZE_MAX || budget.maximum_integer_bits == SIZE_MAX)
    throw std::length_error("Curved mobility setup reservation overflows.");
  auto mobility = state.WithMobilityPolicy(60, {{0, 2}, {1, 0}, {2, 2}}, budget, ceiling, baseline);
  auto entity =
      mobility->WithEntityGeometry(Tolerance, budget, ceiling, transfer_memory::Add(baseline, mobility->ResidentBytes()));
  reference.original = reference.original->WithParameterState(entity);
  return budget;
}
}  // namespace native_fp2_fixture
