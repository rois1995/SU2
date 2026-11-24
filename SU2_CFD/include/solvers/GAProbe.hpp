#pragma once

#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/linear_algebra/CSysMatrix.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace GAProbe {

// Read once at process setup, shared by both solver hooks.
inline const char* const outputPrefix = [] {
  const auto* prefix = std::getenv("SU2_GA_DUMP");
  return prefix && *prefix ? prefix : nullptr;
}();

template <class T>
void Write(std::ofstream& out, const T* values, std::size_t count) {
  out.write(reinterpret_cast<const char*>(values), count * sizeof(T));
}

template <class Scalar>
void WriteDump(const std::string& path, const CGeometry& geometry, unsigned short nVar,
               const CSysVector<su2double>& residual, const CSysMatrix<Scalar>& matrix) {
  static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
  const auto nOwned = geometry.GetnPointDomain();
  std::uint64_t nBlocks = nOwned;  // The diagonal is stored separately from the CSR triangles.
  for (unsigned long row = 0; row < nOwned; ++row)
    matrix.ForEachOffDiagColumn(row, [&](unsigned long) { ++nBlocks; });

  std::ofstream out(path, std::ios::binary);
  if (!out) SU2_MPI::Error("Cannot open GA probe dump " + path, CURRENT_FUNCTION);

  // Native byte order, with no padding. Matrix scalar size describes the diagonal backing type.
  const std::uint64_t header[] = {1, nVar, nOwned, nBlocks, sizeof(Scalar)};
  Write(out, "SU2GADMP", 8);
  Write(out, header, 5);

  std::vector<double> values(std::size_t(nVar) * nVar);
  for (unsigned long row = 0; row < nOwned; ++row) {
    const std::uint64_t globalRow = geometry.nodes->GetGlobalIndex(row);
    for (unsigned short var = 0; var < nVar; ++var)
      values[var] = SU2_TYPE::PassiveValue(residual(row, var));
    Write(out, &globalRow, 1);
    Write(out, values.data(), nVar);
  }

  for (unsigned long row = 0; row < nOwned; ++row) {
    const std::uint64_t globalRow = geometry.nodes->GetGlobalIndex(row);
    const auto writeBlock = [&](unsigned long col) {
      const std::uint64_t indices[] = {globalRow, geometry.nodes->GetGlobalIndex(col)};
      const auto block = matrix.GetBlockView(row, col);
      for (unsigned short i = 0; i < nVar; ++i)
        for (unsigned short j = 0; j < nVar; ++j)
          values[std::size_t(i) * nVar + j] = static_cast<double>(SU2_TYPE::PassiveValue(block(i, j)));
      Write(out, indices, 2);
      Write(out, values.data(), values.size());
    };
    writeBlock(row);
    matrix.ForEachOffDiagColumn(row, writeBlock);
  }

  out.close();
  if (!out) SU2_MPI::Error("Cannot write GA probe dump " + path, CURRENT_FUNCTION);
}

template <bool Turbulence, class Scalar>
void Dump(const CGeometry& geometry, unsigned short nVar, const CSysVector<su2double>& residual,
          const CSysMatrix<Scalar>& matrix) {
  static bool written = false;
  if (written || geometry.GetMGLevel() != MESH_0) return;

  // Finish assembly on all threads, then keep the system intact until the master finishes writing.
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    const auto path = std::string(outputPrefix) + (Turbulence ? ".turb.rank" : ".flow.rank") +
                      std::to_string(SU2_MPI::GetRank()) + ".bin";
    WriteDump(path, geometry, nVar, residual, matrix);
    written = true;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

}  // namespace GAProbe
