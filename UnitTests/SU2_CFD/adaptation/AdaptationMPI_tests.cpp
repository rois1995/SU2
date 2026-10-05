/*!
 * \file AdaptationMPI_tests.cpp
 * \brief Mesh adaptation with MPI: gather of the partitioned mesh and of point values, scatter, broadcast, halo
 *        exchange; the reader slices of a complete mesh (the reader arrays and the geometry bitwise those of the
 *        complete mesh, the metric of the new points); the extracted and remeshed mesh, the solution transfers and the
 *        corner metric compared with the serial computation (every rank alone, MPI_COMM_SELF) in the same run. Valid
 *        with any number of ranks, e.g. mpirun -n 2 test_driver "[AdaptationMPI]".
 * \version 8.5.0 "Harrier"
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

#include "catch.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

#include "TransferTestCase.hpp"
#include "../../../SU2_CFD/include/variables/CPrimitiveIndices.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/adaptation/CReaderSlices.hpp"
#include "../../../Common/include/geometry/meshreader/CDistributedMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"

using namespace transfer_test;

/*--- Run explicitly in a subprocess with SU2_NATIVE_READER_FAULT set. These inputs must terminate collectively
 *    with the named diagnostic; they are hidden from normal unit runs because SU2_MPI::Error aborts MPI. ---*/
TEST_CASE("MPI adaptation: distributed reader rejects malformed ownership", "[.][NativeReaderFailure]") {
  const char* mode = std::getenv("SU2_NATIVE_READER_FAULT");
  REQUIRE(mode != nullptr);
  const std::string fault(mode);
  const auto input = BoxMesh(2, 2, false);
  const int rank = SU2_MPI::GetRank();
  CSimplexMesh local;
  local.nDim = 2;
  for (const auto& marker : input.markers) local.markers.push_back({marker.name, marker.ref, {}});
  std::vector<uint64_t> keys;
  if (rank == 0) {
    local = input;
    for (unsigned long p = 0; p < input.GetnPoint(); ++p) keys.push_back((uint64_t(1) << 40) + p * 97);
    if (fault == "duplicate-cell") local.elem.insert(local.elem.end(), input.elem.begin(), input.elem.begin() + 3);
    else if (fault == "duplicate-face") {
      const auto first = input.markers[0].elem;
      local.markers[1].elem.insert(local.markers[1].elem.end(), first.begin(), first.begin() + 2);
    } else if (fault == "bad-index") local.elem[0] = input.GetnPoint();
    else if (fault == "bad-shape") local.coord.push_back(0.0);
    else if (fault == "nonfinite") local.coord[0] = std::numeric_limits<passivedouble>::quiet_NaN();
    else if (fault == "unused-point") {
      local.coord.insert(local.coord.end(), {5.0, 5.0}); keys.push_back(uint64_t(1) << 60);
    } else if (fault == "duplicate-key") keys[1] = keys[0];
  }
  if (rank == 1) {
    if (fault == "shared-record") {
      keys.push_back((uint64_t(1) << 40));
      local.coord = {input.coord[0] + 0.001, input.coord[1]};
    } else if (fault == "marker-order") std::swap(local.markers[0], local.markers[1]);
    else if (fault == "dimension") local.nDim = 3;
  }
  CReaderSlices::FromDistributed(local, keys);
  FAIL("Malformed distributed reader input was accepted.");
}

TEST_CASE("MPI adaptation: distributed reader construction", "[AdaptationMPI][NativeReader]") {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  for (const unsigned short dim : {2, 3}) {
    for (const bool withMetric : {false, true}) {
      for (const bool emptyRank : {false, true}) {
        INFO("dimension=" << dim << " metric=" << withMetric << " empty=" << emptyRank);
        auto input = BoxMesh(dim, 2, true);
        const auto nMetric = CSimplexMesh::GetnMetric(dim);
        if (withMetric) {
          input.metric.resize(input.GetnPoint() * nMetric);
          for (unsigned long p = 0; p < input.GetnPoint(); ++p) {
            for (unsigned short m = 0; m < nMetric; ++m)
              input.metric[p * nMetric + m] = 0.125 * (1 + m) + input.coord[p * dim];
          }
        }
        CSimplexMesh local;
        local.nDim = dim;
        for (const auto& marker : input.markers) local.markers.push_back({marker.name, marker.ref, {}});
        std::vector<uint64_t> keys;
        std::map<unsigned long, unsigned long> localIndex;
        auto addPoint = [&](unsigned long original) {
          const auto found = localIndex.find(original);
          if (found != localIndex.end()) return found->second;
          const auto index = local.GetnPoint();
          localIndex[original] = index;
          /*--- Sparse identities far above the dense point count; unrelated to coordinates/ownership. ---*/
          keys.push_back((uint64_t(1) << 40) + original * 97);
          for (unsigned short d = 0; d < dim; ++d) local.coord.push_back(input.coord[original * dim + d]);
          if (withMetric) for (unsigned short m = 0; m < nMetric; ++m)
            local.metric.push_back(input.metric[original * nMetric + m]);
          return index;
        };
        const int active = emptyRank && size > 1 ? size - 1 : size;
        for (unsigned long e = 0; e < input.GetnElem(); ++e) {
          if ((3 * e + e / 3) % active != static_cast<unsigned long>(rank)) continue;
          for (unsigned short k = 0; k <= dim; ++k) local.elem.push_back(addPoint(input.elem[e * (dim + 1) + k]));
        }
        for (size_t m = 0; m < input.markers.size(); ++m) {
          const auto& marker = input.markers[m];
          for (unsigned long e = 0; e < marker.GetnElem(dim); ++e) {
            if ((e + m) % active != static_cast<unsigned long>(rank)) continue;
            for (unsigned short k = 0; k < dim; ++k)
              local.markers[m].elem.push_back(addPoint(marker.elem[e * dim + k]));
          }
        }
        const auto previousRound = CPassiveComm::GetRoundBytes();
        CPassiveComm::SetRoundBytes(127);
        const auto slices = CReaderSlices::FromDistributed(local, keys);
        CPassiveComm::SetRoundBytes(previousRound);
        REQUIRE(slices.nPointGlobal == input.GetnPoint());
        REQUIRE(slices.nElemGlobal == input.GetnElem());
        CHECK(slices.nMetric == (withMetric ? nMetric : 0));

        /*--- Audit-only gathering: identify returned points by their unique original coordinates, independently
         *    of the directory's hash, prefix numbering and local ownership. ---*/
        std::vector<passivedouble> coord;
        for (unsigned long p = 0; p < slices.nPointLocal; ++p)
          for (unsigned short d = 0; d < dim; ++d) coord.push_back(slices.coord[d][p]);
        const auto allCoord = CPassiveComm::Allgatherv(coord, nullptr);
        std::vector<unsigned long> original(slices.nPointGlobal);
        std::set<unsigned long> unique;
        bool correct = true;
        for (unsigned long p = 0; p < slices.nPointGlobal; ++p) {
          unsigned long match = input.GetnPoint();
          for (unsigned long q = 0; q < input.GetnPoint(); ++q) {
            bool same = true;
            for (unsigned short d = 0; d < dim; ++d) same &= allCoord[p * dim + d] == input.coord[q * dim + d];
            if (same) match = q;
          }
          correct &= match < input.GetnPoint();
          original[p] = match;
          unique.insert(match);
        }
        correct &= unique.size() == input.GetnPoint();
        REQUIRE(CPassiveComm::AllreduceMax(correct ? 0 : 1) == 0);
        for (unsigned long p = 0; p < slices.nPointLocal; ++p)
          if (withMetric) for (unsigned short m = 0; m < nMetric; ++m)
            CHECK(slices.metric[p * nMetric + m] == input.metric[original[slices.firstPoint + p] * nMetric + m]);

        auto key = [&](const unsigned long* nodes, unsigned short count, bool renumbered) {
          std::array<unsigned long, 4> result{};
          for (unsigned short k = 0; k < count; ++k) result[k] = renumbered ? original[nodes[k]] : nodes[k];
          std::sort(result.begin(), result.begin() + count);
          return result;
        };
        std::set<std::array<unsigned long, 4>> expected;
        for (unsigned long e = 0; e < input.GetnElem(); ++e) expected.insert(key(&input.elem[e * (dim + 1)], dim + 1, false));
        const auto rows = CPassiveComm::Allgatherv(slices.elemRows, nullptr);
        std::map<unsigned long, std::array<unsigned long, 4>> identities;
        std::set<std::array<unsigned long, 4>> actual;
        for (size_t r = 0; r < rows.size(); r += SU2_CONN_SIZE) {
          const auto shape = key(&rows[r + 2], dim + 1, true);
          const auto inserted = identities.emplace(rows[r], shape);
          if (!inserted.second) CHECK(inserted.first->second == shape);
          actual.insert(shape);
        }
        CHECK(actual == expected);
        CHECK(identities.size() == input.GetnElem());
        /*--- Each slice holds every element touching it, exactly once, and no other element. ---*/
        std::set<unsigned long> received;
        for (size_t r = 0; r < slices.elemRows.size(); r += SU2_CONN_SIZE) {
          bool touches = false;
          for (unsigned short k = 0; k <= dim; ++k) {
            const auto p = slices.elemRows[r + 2 + k];
            touches |= p >= slices.firstPoint && p < slices.firstPoint + slices.nPointLocal;
          }
          CHECK(touches);
          CHECK(received.insert(slices.elemRows[r]).second);
          if (r) CHECK(slices.elemRows[r - SU2_CONN_SIZE] < slices.elemRows[r]);
        }
        for (size_t r = 0; r < rows.size(); r += SU2_CONN_SIZE) {
          bool touches = false;
          for (unsigned short k = 0; k <= dim; ++k) {
            const auto p = rows[r + 2 + k];
            touches |= p >= slices.firstPoint && p < slices.firstPoint + slices.nPointLocal;
          }
          if (touches) CHECK(received.count(rows[r]) == 1);
        }
        REQUIRE(slices.markerNames.size() == input.markers.size());
        for (size_t m = 0; m < input.markers.size(); ++m) {
          CHECK(slices.markerNames[m] == input.markers[m].name);
          if (rank != MASTER_NODE) { CHECK(slices.boundaryRows[m].empty()); continue; }
          std::set<std::array<unsigned long, 4>> faces, reference;
          for (size_t r = 0; r < slices.boundaryRows[m].size(); r += SU2_CONN_SIZE)
            CHECK(faces.insert(key(&slices.boundaryRows[m][r + 2], dim, true)).second);
          for (unsigned long e = 0; e < input.markers[m].GetnElem(dim); ++e)
            reference.insert(key(&input.markers[m].elem[e * dim], dim, false));
          CHECK(faces == reference);
        }

        /*--- Exercise the actual SU2 reader and geometry reconstruction, not only exported row shapes. ---*/
        auto config = MakeConfig(dim, "SOLVER= EULER\n");
        CGeometry** geometry = nullptr;
        {
          Mute mute;
          config->SetMGLevels(0);
          CDistributedMemoryMeshReaderFVM reader(config.get(), slices, 0, 1);
          CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
        }
        CHECK(geometry[0]->GetGlobal_nPointDomain() == input.GetnPoint());
        const auto fetched = slices.FetchPointMetric(*geometry[0]);
        if (withMetric) {
          for (unsigned long p = 0; p < geometry[0]->GetnPoint(); ++p) {
            const auto id = geometry[0]->nodes->GetGlobalIndex(p);
            for (unsigned short m = 0; m < nMetric; ++m)
              CHECK(fetched[p * nMetric + m] == input.metric[original[id] * nMetric + m]);
          }
        } else CHECK(fetched.empty());
        delete geometry[0];
        delete[] geometry;
      }
    }
  }
}

namespace {

/*--- Run f with every rank alone (MPI_COMM_SELF): each rank does the serial computation of the whole problem. Objects
 *    built inside must be used and deleted inside. ---*/
template <class F>
void Serial(F f) {
#ifdef HAVE_MPI
  const auto world = SU2_MPI::GetComm();
  SU2_MPI::SetComm(MPI_COMM_SELF);
  f();
  SU2_MPI::SetComm(world);
#else
  f();
#endif
}

bool IsRoot() { return SU2_MPI::GetRank() == MASTER_NODE; }

/*--- Largest value over the ranks. ---*/
passivedouble GlobalMax(passivedouble value) {
  su2double local = value, global = 0.0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  return SU2_TYPE::GetValue(global);
}
su2double GlobalSum(su2double value) {
  su2double global = 0.0;
  SU2_MPI::Allreduce(&value, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  return global;
}

/*--- Sorted face keys of a marker. ---*/
std::vector<std::array<unsigned long, 3>> FaceKeys(const CSimplexMesh::Marker& marker, unsigned short nDim) {
  std::vector<std::array<unsigned long, 3>> keys;
  for (auto iFace = 0ul; iFace < marker.GetnElem(nDim); ++iFace) {
    std::array<unsigned long, 3> key = {0, 0, 0};
    for (unsigned short i = 0; i < nDim; ++i) key[i] = marker.elem[iFace * nDim + i];
    std::sort(key.begin(), key.begin() + nDim);
    keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());
  return keys;
}

/*--- Bitwise equality of two meshes (arrays and markers in order). ---*/
bool SameMesh(const CSimplexMesh& a, const CSimplexMesh& b) {
  if (a.nDim != b.nDim || a.coord != b.coord || a.metric != b.metric || a.elem != b.elem || a.elemRef != b.elemRef ||
      a.markers.size() != b.markers.size())
    return false;
  for (auto i = 0ul; i < a.markers.size(); ++i) {
    if (a.markers[i].name != b.markers[i].name || a.markers[i].ref != b.markers[i].ref ||
        a.markers[i].elem != b.markers[i].elem)
      return false;
  }
  return true;
}

/*--- Smooth anisotropic metric of the coordinates (sizes 0.08..0.2, rotated by an angle that varies in space). ---*/
void AnalyticMetric(unsigned short nDim, const su2double* x, su2double* metric) {
  const su2double h1 = 0.08 + 0.06 * x[0], h2 = 0.2 - 0.05 * x[1], angle = 0.4 + 0.3 * x[1];
  su2double R[3][3] = {{cos(angle), -sin(angle), 0.0}, {sin(angle), cos(angle), 0.0}, {0.0, 0.0, 1.0}};
  const su2double eig[3] = {1.0 / (h1 * h1), 1.0 / (h2 * h2), 1.0 / pow(0.15, 2)};
  for (unsigned short i = 0, iMet = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j, ++iMet) {
      metric[iMet] = 0.0;
      for (unsigned short k = 0; k < nDim; ++k) metric[iMet] += R[i][k] * eig[k] * R[j][k];
    }
}

/*--- The transferred arrays (solution, time n, time n-1 of the flow and turbulence solvers) of the domain points by
 *    global index: values[global index][...]; the rows of other points stay empty. ---*/
using PointValues = std::vector<std::vector<su2double>>;
PointValues TransferredValues(const MeshSolution& mesh) {
  PointValues values(mesh.Fine().GetGlobal_nPointDomain());
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPointDomain(); ++iPoint) {
    auto& row = values[mesh.Fine().nodes->GetGlobalIndex(iPoint)];
    for (const auto iSol : {FLOW_SOL, TURB_SOL}) {
      auto* solver = mesh.solver[MESH_0][iSol];
      if (solver == nullptr) continue;
      auto* nodes = solver->GetNodes();
      for (auto* array : {&nodes->GetSolution(), &nodes->GetSolution_time_n(), &nodes->GetSolution_time_n1()}) {
        if (array->rows() == 0) continue;
        for (unsigned long iVar = 0; iVar < array->cols(); ++iVar) row.push_back((*array)(iPoint, iVar));
      }
    }
  }
  return values;
}

/*--- Largest relative difference of the domain points of a partitioned mesh to the serial values (all ranks). ---*/
passivedouble DifferenceToSerial(const MeshSolution& parallel, const PointValues& serial) {
  const auto values = TransferredValues(parallel);
  passivedouble diff = 0.0;
  bool sizes = true;
  for (auto iPoint = 0ul; iPoint < parallel.Fine().GetnPointDomain(); ++iPoint) {
    const auto id = parallel.Fine().nodes->GetGlobalIndex(iPoint);
    sizes &= values[id].size() == serial[id].size() && !serial[id].empty();
    for (auto k = 0ul; k < std::min(values[id].size(), serial[id].size()); ++k)
      diff = std::max(diff, RelDiff(values[id][k], serial[id][k], 1e-300));
  }
  return sizes ? GlobalMax(diff) : 1e300;
}

/*--- Fields of the transfer tests: the solution, Solution_time_n (U^n) and Solution_time_n1 (U^(n-1), another field)
 *    of the flow (and SA) solver, smooth or affine functions of the coordinates (the same on every rank). ---*/
void SetDonorFields(MeshSolution& donor, unsigned short nDim, bool affine) {
  auto* flow = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  auto* turb = donor.solver[MESH_0][TURB_SOL] ? donor.solver[MESH_0][TURB_SOL]->GetNodes() : nullptr;
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
    const auto* x = donor.Fine().nodes->GetCoord(iPoint);
    su2double U[MAXVAR] = {}, Un1[MAXVAR] = {};
    if (affine) {
      AffineFlow(nDim)(x, U);
    } else {
      SmoothFlow(nDim, x, U);
    }
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) Un1[iVar] = U[iVar] * (1.0 + 0.01 * x[0]);
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
      flow->GetSolution()(iPoint, iVar) = U[iVar];
      if (flow->GetSolution_time_n().rows() > 0) flow->GetSolution_time_n()(iPoint, iVar) = U[iVar];
      if (flow->GetSolution_time_n1().rows() > 0) flow->GetSolution_time_n1()(iPoint, iVar) = Un1[iVar];
    }
    if (turb == nullptr) continue;
    const su2double nu = 1e-4 + 2e-5 * x[0] - 1e-5 * x[1];
    turb->GetSolution()(iPoint, 0) = nu;
    if (turb->GetSolution_time_n().rows() > 0) turb->GetSolution_time_n()(iPoint, 0) = nu;
    if (turb->GetSolution_time_n1().rows() > 0) turb->GetSolution_time_n1()(iPoint, 0) = 1.1 * nu;
  }
}

/*--- The arrays of two mesh readers are bitwise equal (points, volume rows, marker names, boundary rows). ---*/
bool SameReaderArrays(const CMeshReaderBase& a, const CMeshReaderBase& b) {
  bool same = a.GetDimension() == b.GetDimension() && a.GetNumberOfGlobalPoints() == b.GetNumberOfGlobalPoints() &&
              a.GetNumberOfLocalPoints() == b.GetNumberOfLocalPoints() &&
              a.GetLocalPointCoordinates() == b.GetLocalPointCoordinates() &&
              a.GetNumberOfGlobalElements() == b.GetNumberOfGlobalElements() &&
              a.GetNumberOfLocalElements() == b.GetNumberOfLocalElements() &&
              a.GetLocalVolumeElementConnectivity() == b.GetLocalVolumeElementConnectivity() &&
              a.GetNumberOfMarkers() == b.GetNumberOfMarkers() && a.GetMarkerNames() == b.GetMarkerNames();
  for (unsigned long iMarker = 0; same && iMarker < a.GetNumberOfMarkers(); ++iMarker) {
    same = a.GetSurfaceElementConnectivityForMarker(iMarker) == b.GetSurfaceElementConnectivityForMarker(iMarker);
  }
  return same;
}

/*--- The slices of this rank are those a reader of the complete mesh builds, with the metric of the slice's points
 *    and the markers that have elements. ---*/
bool SlicesOfComplete(const CConfig& config, const CReaderSlices& slices, const CSimplexMesh& complete) {
  const CMemoryMeshReaderFVM reference(&config, complete, 0, 1);
  const CDistributedMemoryMeshReaderFVM reader(&config, slices, 0, 1);
  bool same = SameReaderArrays(reader, reference);
  const auto nMet = complete.metric.empty() ? 0 : CSimplexMesh::GetnMetric(complete.nDim);
  same &= slices.nMetric == nMet;
  same &= slices.metric == std::vector<passivedouble>(complete.metric.begin() + slices.firstPoint * nMet,
                                                      complete.metric.begin() +
                                                          (slices.firstPoint + slices.nPointLocal) * nMet);
  std::vector<std::string> withElements;
  for (const auto& marker : complete.markers)
    if (!marker.elem.empty()) withElements.push_back(marker.name);
  return same && slices.markersWithElements == withElements;
}

/*--- Partitioned geometry (finest level only) from a reader. ---*/
CGeometry** BuildGeometry(CConfig& config, CMeshReaderBase& reader) {
  Mute mute;
  config.SetMGLevels(0);
  CGeometry** geometry = nullptr;
  CDriver::BuildGeometryFVM(&config, new CPhysicalGeometry(&config, reader, 1), geometry, true);
  return geometry;
}
void DeleteGeometry(const CConfig& config, CGeometry** geometry) {
  for (unsigned short iMesh = 0; iMesh <= config.GetnMGLevels(); ++iMesh) delete geometry[iMesh];
  delete[] geometry;
}

/*--- Two partitioned geometries are bitwise equal on this rank: points (global index, coordinates, domain flag),
 *    elements (type, global nodes), markers (boundary elements, global nodes). ---*/
bool SameGeometry(const CGeometry& a, const CGeometry& b) {
  bool same = a.GetnPoint() == b.GetnPoint() && a.GetnPointDomain() == b.GetnPointDomain() &&
              a.GetnElem() == b.GetnElem() && a.GetnMarker() == b.GetnMarker() && a.GetnDim() == b.GetnDim();
  for (auto iPoint = 0ul; same && iPoint < a.GetnPoint(); ++iPoint) {
    same = a.nodes->GetGlobalIndex(iPoint) == b.nodes->GetGlobalIndex(iPoint) &&
           a.nodes->GetDomain(iPoint) == b.nodes->GetDomain(iPoint);
    for (unsigned short iDim = 0; same && iDim < a.GetnDim(); ++iDim)
      same = a.nodes->GetCoord(iPoint, iDim) == b.nodes->GetCoord(iPoint, iDim);
  }
  auto sameElement = [&](const CPrimalGrid* x, const CPrimalGrid* y) {
    bool ok = x->GetVTK_Type() == y->GetVTK_Type() && x->GetnNodes() == y->GetnNodes();
    for (unsigned short iNode = 0; ok && iNode < x->GetnNodes(); ++iNode)
      ok = a.nodes->GetGlobalIndex(x->GetNode(iNode)) == b.nodes->GetGlobalIndex(y->GetNode(iNode));
    return ok;
  };
  for (auto iElem = 0ul; same && iElem < a.GetnElem(); ++iElem) same = sameElement(a.elem[iElem], b.elem[iElem]);
  for (unsigned short iMarker = 0; same && iMarker < a.GetnMarker(); ++iMarker) {
    same = a.GetnElem_Bound(iMarker) == b.GetnElem_Bound(iMarker);
    for (auto iElem = 0ul; same && iElem < a.GetnElem_Bound(iMarker); ++iElem)
      same = sameElement(a.bound[iMarker][iElem], b.bound[iMarker][iElem]);
  }
  return same;
}

}  // namespace

TEST_CASE("MPI adaptation: gather, scatter, broadcast and halo exchange", "[AdaptationMPI]") {
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      const auto input = BoxMesh(nDim, 4, true);
      MeshSolution mesh(config.get(), input, 0);
      const auto& geometry = mesh.Fine();

      const CMeshGather gather(geometry);
      CHECK(gather.GetnPointGlobal() == input.GetnPoint());
      auto gathered = gather.GatherMesh(*config, mesh.markerTags, true);
      const unsigned short nNode = nDim + 1;

      /*--- On the master rank: the input mesh in its point numbering (points bitwise, the same elements, ordered by
       *    their sorted nodes, each marker with its faces), and the control volumes fill the domain. ---*/
      if (IsRoot()) {
        CHECK(gathered.coord == input.coord);
        REQUIRE(gathered.GetnElem() == input.GetnElem());
        auto sortedElements = [nNode](const CSimplexMesh& mesh) {
          std::vector<std::vector<unsigned long>> elements;
          for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem) {
            std::vector<unsigned long> nodes(&mesh.elem[iElem * nNode], &mesh.elem[iElem * nNode] + nNode);
            std::sort(nodes.begin(), nodes.end());
            elements.push_back(nodes);
          }
          return elements;
        };
        const auto elements = sortedElements(gathered);
        auto reference = sortedElements(input);
        std::sort(reference.begin(), reference.end());
        CHECK(elements == reference);
        REQUIRE(gathered.markers.size() == input.markers.size());
        for (auto i = 1ul; i < gathered.markers.size(); ++i) {
          CHECK(config->GetMarker_CfgFile_TagBound(gathered.markers[i - 1].name) <
                config->GetMarker_CfgFile_TagBound(gathered.markers[i].name));
        }
        for (const auto& marker : gathered.markers) {
          const auto* reference = input.FindMarker(marker.name);
          REQUIRE(reference != nullptr);
          CHECK(FaceKeys(marker, nDim) == FaceKeys(*reference, nDim));
          CHECK(marker.ref == CMMGInterface::GetMarkerReference(*config, marker.name));
        }
        passivedouble volume = 0.0;
        for (const auto v : gathered.volume) volume += v;
        CHECK(volume == Approx(nDim == 2 ? 2.0 : 1.0).epsilon(1e-12));
      }

      /*--- Broadcast: every rank has the mesh; its domain points have these coordinates and control volumes. ---*/
      CMeshGather::Broadcast(gathered);
      REQUIRE(gathered.GetnPoint() == input.GetnPoint());
      CHECK(gathered.coord == input.coord);
      bool sameLocal = true;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        const auto id = geometry.nodes->GetGlobalIndex(iPoint);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          sameLocal &= SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim)) == gathered.coord[id * nDim + iDim];
        sameLocal &= SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint)) == gathered.volume[id];
      }
      CHECK(GlobalMax(sameLocal ? 0.0 : 1.0) == 0.0);

      /*--- Gather and scatter of point values. ---*/
      auto field = [nDim](const su2double* x, su2double* f) {
        f[0] = sin(3.0 * x[0]) + x[1];
        f[1] = x[nDim - 1] * x[0];
      };
      std::vector<su2double> local(geometry.GetnPointDomain() * 2);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint)
        field(geometry.nodes->GetCoord(iPoint), &local[iPoint * 2]);
      auto global = gather.Gather(local.data(), 2);
      if (IsRoot()) {
        REQUIRE(global.size() == 2 * input.GetnPoint());
        bool exact = true;
        for (auto i = 0ul; i < input.GetnPoint(); ++i) {
          su2double x[3] = {0.0}, f[2];
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = input.coord[i * nDim + iDim];
          field(x, f);
          exact &= global[2 * i] == f[0] && global[2 * i + 1] == f[1];
        }
        CHECK(exact);
        for (auto& v : global) v = 2.0 * v + 1.0;
      } else {
        CHECK(global.empty());
      }
      std::vector<su2double> back(geometry.GetnPointDomain() * 2);
      gather.Scatter(global, 2, back.data());
      bool scattered = true;
      for (auto i = 0ul; i < back.size(); ++i) scattered &= back[i] == 2.0 * local[i] + 1.0;
      CHECK(GlobalMax(scattered ? 0.0 : 1.0) == 0.0);

      /*--- Halo exchange: the halo points get the values of the ranks that own them. ---*/
      std::vector<su2double> ids(geometry.GetnPoint(), -1.0);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint)
        ids[iPoint] = geometry.nodes->GetGlobalIndex(iPoint);
      CMeshGather::ExchangeHalo(geometry, ids.data(), 1);
      bool halos = true;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
        halos &= ids[iPoint] == su2double(geometry.nodes->GetGlobalIndex(iPoint));
      CHECK(GlobalMax(halos ? 0.0 : 1.0) == 0.0);
    }
  }
}

#ifdef HAVE_MMG
TEST_CASE("MPI adaptation: extracted and remeshed mesh independent of the ranks", "[AdaptationMPI]") {
  /*--- The same mesh and analytic metric, extracted and remeshed by every rank alone (serial) and by all ranks (gather
   *    on the master rank, MMG there, reader slices to every rank): the same arrays bit for bit, including the
   *    numbering, and each rank's slices are those a reader of the serial mesh builds on that rank. ---*/
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      const auto input = BoxMesh(nDim, nDim == 2 ? 6 : 3, true);
      const auto nMet = CSimplexMesh::GetnMetric(nDim);
      auto makeConfig = [nDim]() { return MakeConfig(nDim, "SOLVER= EULER\nADAP_HMIN= 1e-3\nADAP_HMAX= 1\n"); };
      auto extract = [&](CSimplexMesh& extracted, CSimplexMesh* adapted, CRemeshResult* result) {
        auto config = makeConfig();
        MeshSolution mesh(config.get(), input, 0);
        su2activematrix metric(mesh.Fine().GetnPoint(), nMet);
        for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPoint(); ++iPoint)
          AnalyticMetric(nDim, mesh.Fine().nodes->GetCoord(iPoint), metric[iPoint]);
        Mute mute;
        extracted = CMMGInterface::ExtractMesh(*config, mesh.Fine(), metric);
        if (adapted != nullptr) *adapted = CMMGInterface(*config).Adapt(extracted);
        if (result != nullptr) {
          CMMGRemesher remesher;
          *result = remesher.Remesh(*config, mesh.Fine(), metric);
        }
      };
      CSimplexMesh serialMesh, serialAdapted, parallelMesh, unused;
      CRemeshResult serialResult, parallelResult;
      bool serialSlices = false;
      Serial([&]() {
        extract(serialMesh, &serialAdapted, &serialResult);
        /*--- One rank: the slices are the whole adapted mesh. ---*/
        serialSlices = SlicesOfComplete(*makeConfig(), serialResult.slices, serialAdapted);
      });
      extract(parallelMesh, nullptr, &parallelResult);

      if (IsRoot()) {
        CHECK(SameMesh(parallelMesh, serialMesh));
        CMMGInterface::ValidateMesh(parallelMesh, nullptr, "gathered mesh");
      }
      CHECK(serialAdapted.GetnPoint() > 0);
      CHECK(serialSlices);
      CHECK(serialResult.slices.nPointLocal == serialAdapted.GetnPoint());
      CHECK(GlobalMax(SlicesOfComplete(*makeConfig(), parallelResult.slices, serialAdapted) ? 0.0 : 1.0) == 0.0);
      CHECK(parallelResult.markers == serialResult.markers);
      CHECK(parallelResult.status == CRemeshResult::Status::COMPLETE);
    }
  }
}
#endif

TEST_CASE("MPI adaptation: barycentric transfer independent of the ranks", "[AdaptationMPI]") {
  struct Case {
    unsigned short nDim;
    string options;
    bool affine;
  };
  const std::vector<Case> cases = {
      {2, "SOLVER= EULER\n", true},
      {3, "SOLVER= EULER\n", true},
      {2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\nTIME_DOMAIN= YES\nTIME_STEP= 1e-3\n"
          "TIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n", false}};

  for (const auto& test : cases) {
    SECTION("nDim " + std::to_string(test.nDim) + ", " + test.options) {
      const auto nDim = test.nDim;
      PointValues serial;
      su2double serialIntegral = 0.0;
      auto run = [&](PointValues* values, su2double* integral, passivedouble* exactness, unsigned long* nPoint) {
        auto config = MakeConfig(nDim, test.options);
        MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
        SetDonorFields(donor, nDim, test.affine);
        MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        if (values) *values = TransferredValues(target);
        *integral = transfer.GetSummary().newIntegral[0];
        *nPoint = transfer.GetSummary().nPoint;
        if (exactness == nullptr) return;
        /*--- Affine flow: exact at the domain points of every rank. ---*/
        passivedouble maxDiff = 0.0;
        if (test.affine) {
          const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            su2double exact[MAXVAR] = {};
            AffineFlow(nDim)(target.Fine().nodes->GetCoord(iPoint), exact);
            for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
              maxDiff = max(maxDiff, RelDiff(nodes->GetSolution(iPoint, iVar), exact[iVar]));
          }
          CheckCoarseLevels(target, FLOW_SOL);
        }
        *exactness = GlobalMax(maxDiff);
        /*--- The difference to the serial transfer, while the target exists. ---*/
        exactness[1] = DifferenceToSerial(target, serial);
      };
      unsigned long nSerial = 0, nParallel = 0;
      Serial([&]() { run(&serial, &serialIntegral, nullptr, &nSerial); });
      su2double integral = 0.0;
      passivedouble checks[2] = {0.0, 0.0};
      run(nullptr, &integral, checks, &nParallel);

      CHECK(nParallel == nSerial);
      if (test.affine) CHECK(checks[0] < 1e-12);
      /*--- The same gathered meshes and donor values: the same values bit for bit. ---*/
      CHECK(checks[1] == 0.0);
      /*--- Integrals: the control volumes of the partitioned mesh may differ by round-off. ---*/
      CHECK(RelDiff(integral, serialIntegral) < 1e-13);
    }
  }
}

TEST_CASE("MPI adaptation: conservative transfer independent of the ranks", "[AdaptationMPI]") {
  struct Case {
    unsigned short nDim;
    string options;
    bool affine;
  };
  const std::vector<Case> cases = {
      {2, "SOLVER= EULER\n", true},
      {3, "SOLVER= EULER\n", false},
      {2, "SOLVER= EULER\nTIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\n"
          "TIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n", false}};

  for (const auto& test : cases) {
    SECTION("nDim " + std::to_string(test.nDim) + ", " + test.options) {
      const auto nDim = test.nDim;
      PointValues serial;
      std::vector<passivedouble> serialDefect;
      auto run = [&](bool parallel, std::vector<passivedouble>& defect, passivedouble* checks) {
        auto config = MakeConfig(nDim, test.options);
        MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
        SetDonorFields(donor, nDim, test.affine);
        MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
        CConservativeTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        defect = transfer.GetSummary().relativeDefect;
        if (!parallel) {
          serial = TransferredValues(target);
          return;
        }
        checks[0] = DifferenceToSerial(target, serial);

        /*--- The totals of the final state summed over the ranks (value x control volume of the domain points) are
         *    those of the donor. ---*/
        passivedouble worst = 0.0;
        for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
          su2double donorTotal = 0.0, newTotal = 0.0, scale = 0.0;
          for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPointDomain(); ++iPoint) {
            const auto u = donor.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution(iPoint, iVar);
            donorTotal += u * donor.Fine().nodes->GetVolume(iPoint);
            scale += fabs(u) * donor.Fine().nodes->GetVolume(iPoint);
          }
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPointDomain(); ++iPoint)
            newTotal += target.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution(iPoint, iVar) *
                        target.Fine().nodes->GetVolume(iPoint);
          donorTotal = GlobalSum(donorTotal);
          newTotal = GlobalSum(newTotal);
          scale = GlobalSum(scale);
          worst = max(worst, SU2_TYPE::GetValue(fabs(newTotal - donorTotal) / scale));
        }
        checks[1] = worst;
        /*--- Affine flow: exact at the domain points (where both domains coincide; the CG tolerance). ---*/
        passivedouble maxDiff = 0.0;
        if (test.affine) {
          const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            su2double exact[MAXVAR] = {};
            AffineFlow(nDim)(target.Fine().nodes->GetCoord(iPoint), exact);
            for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
              maxDiff = max(maxDiff, RelDiff(nodes->GetSolution(iPoint, iVar), exact[iVar]));
          }
        }
        checks[2] = GlobalMax(maxDiff);
      };
      Serial([&]() { run(false, serialDefect, nullptr); });
      std::vector<passivedouble> defect;
      passivedouble checks[3] = {0.0, 0.0, 0.0};
      run(true, defect, checks);

      /*--- Same meshes and donor values; the control volumes of a partitioned mesh can differ by round-off, so the
       *    projection differs at the level of the CG tolerance at most. ---*/
      CHECK(checks[0] < 1e-12);
      CHECK(checks[1] < 1e-13);
      if (test.affine) CHECK(checks[2] < 1e-11);
      REQUIRE(defect.size() == serialDefect.size());
      for (auto f = 0ul; f < defect.size(); ++f) CHECK(fabs(defect[f]) < 1e-13);
    }
  }
}

TEST_CASE("MPI adaptation: corner metric across the partitions", "[AdaptationMPI]") {
  /*--- Rectangle around a triangular obstacle (three sharp corners), uniform anisotropic Hessian: the isotropic corner
   *    sizes are shortest paths from the corners, which cross the partitions. The metric equals the serial one (up to
   *    the round-off of the global sums), and the largest size grows by at most log(ADAP_HGRAD) per edge length on
   *    every edge, also between the ranks. ---*/
  auto markerOf = [](const passivedouble* x) {
    const passivedouble tol = 1e-12;
    const bool outer = x[0] < tol || x[0] > 2.0 - tol || x[1] < tol || x[1] > 1.0 - tol;
    return std::string(outer ? "far" : "wall");
  };
  auto keepElem = [](const passivedouble* x) { return !(x[1] > 0.5 && x[0] < 1.0 && x[1] - 0.5 < x[0] - 0.75); };
  const auto input = simplex_test::MakeSimplexMesh(2, 16, markerOf, keepElem);
  const std::array<su2double, 3> hessian = {19.0, 10.392304845413264, 7.0};

  std::vector<su2double> serialMetric;
  auto run = [&](bool parallel, passivedouble* checks) {
    Mute mute;
    stringstream ss(
        "SOLVER= EULER\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\nMGLEVEL= 0\n"
        "COMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\nADAP_NORM= 2\nNUM_METHOD_HESS= GREEN_GAUSS\n"
        "MARKER_EULER= (wall)\nMARKER_FAR= (far)\nADAP_COMPLEXITY= 2000\nADAP_HMIN= 1e-4\nADAP_HMAX= 10\n"
        "ADAP_ARMAX= 1000\n");
    CConfig config(ss, SU2_COMPONENT::SU2_CFD, false);
    CGeometry** geometry = nullptr;
    CMemoryMeshReaderFVM reader(&config, input, 0, 1);
    CDriver::BuildGeometryFVM(&config, new CPhysicalGeometry(&config, reader, 1), geometry, true);
    auto** solver = CSolverFactory::CreateSolverContainer(config.GetKind_Solver(), &config, geometry[MESH_0], 0);
    auto* nodes = solver[FLOW_SOL]->GetNodes();
    for (auto iPoint = 0ul; iPoint < geometry[MESH_0]->GetnPoint(); ++iPoint)
      for (auto iMet = 0u; iMet < 3; ++iMet) nodes->GetHessian()(iPoint, 0, iMet) = hessian[iMet];
    solver[FLOW_SOL]->ComputeMetric(geometry[MESH_0], &config);

    const auto& fine = *geometry[MESH_0];
    if (!parallel) {
      serialMetric.assign(fine.GetGlobal_nPointDomain() * 3, 0.0);
      for (auto iPoint = 0ul; iPoint < fine.GetnPoint(); ++iPoint)
        for (auto iMet = 0u; iMet < 3; ++iMet)
          serialMetric[fine.nodes->GetGlobalIndex(iPoint) * 3 + iMet] = nodes->GetMetric(iPoint, iMet);
    } else {
      passivedouble diff = 0.0, worst = 0.0;
      for (auto iPoint = 0ul; iPoint < fine.GetnPointDomain(); ++iPoint)
        for (auto iMet = 0u; iMet < 3; ++iMet)
          diff = max(diff, RelDiff(nodes->GetMetric(iPoint, iMet),
                                   serialMetric[fine.nodes->GetGlobalIndex(iPoint) * 3 + iMet], 1.0));
      /*--- Largest size on every edge (the halo metric is the one of the owner). ---*/
      auto largestSize = [&](unsigned long iPoint) {
        su2double M[3][3] = {{0.0}}, vec[3][3], val[3], work[3];
        nodes->GetMetricMat(iPoint, M);
        CBlasStructure::EigenDecomposition(M, vec, val, 2, work);
        return 1.0 / sqrt(min(val[0], val[1]));
      };
      for (auto iEdge = 0ul; iEdge < fine.GetnEdge(); ++iEdge) {
        const auto iPoint = fine.edges->GetNode(iEdge, 0), jPoint = fine.edges->GetNode(iEdge, 1);
        const su2double length =
            GeometryToolbox::Distance(2, fine.nodes->GetCoord(iPoint), fine.nodes->GetCoord(jPoint));
        worst = max(worst, SU2_TYPE::GetValue(fabs(largestSize(iPoint) - largestSize(jPoint)) / (log(1.3) * length)));
      }
      checks[0] = GlobalMax(diff);
      checks[1] = GlobalMax(worst);
    }
    for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
      CSolverFactory::ClearSolverMeta(solver[iSol]);
      delete solver[iSol];
    }
    delete[] solver;
    delete geometry[MESH_0];
    delete[] geometry;
  };
  Serial([&]() { run(false, nullptr); });
  passivedouble checks[2] = {0.0, 0.0};
  run(true, checks);
  CHECK(checks[0] < 1e-10);
  CHECK(checks[1] <= 1 + 1e-6);
  CHECK(checks[1] > 0.5);
}

TEST_CASE("MPI adaptation: reader slices of a complete mesh", "[AdaptationMPI]") {
  /*--- A complete mesh with a metric given on the master rank only: every rank's slices are the arrays of a reader of
   *    the complete mesh (bitwise), the partitioned geometry built from them is bitwise the one built from the
   *    complete mesh, and each rank fetches the metric of its points (domain and halo) by global index. Also with
   *    rounds of a few hundred bytes (the slices sent in many chunks), and with a marker without elements. ---*/
  for (const unsigned short nDim : {2, 3}) {
    for (const size_t roundBytes : {CPassiveComm::DEFAULT_ROUND_BYTES, size_t(300)}) {
      SECTION("nDim " + std::to_string(nDim) + ", round size " + std::to_string(roundBytes)) {
        auto complete = BoxMesh(nDim, 4, true);
        const auto nMet = CSimplexMesh::GetnMetric(nDim);
        complete.metric.resize(complete.GetnPoint() * nMet);
        for (auto iPoint = 0ul; iPoint < complete.GetnPoint(); ++iPoint) {
          su2double x[3] = {0.0, 0.0, 0.0}, metric[6];
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = complete.coord[iPoint * nDim + iDim];
          AnalyticMetric(nDim, x, metric);
          for (unsigned short iMet = 0; iMet < nMet; ++iMet)
            complete.metric[iPoint * nMet + iMet] = SU2_TYPE::GetValue(metric[iMet]);
        }
        auto config = MakeConfig(nDim, "SOLVER= EULER\n");
        const auto saved = CPassiveComm::GetRoundBytes();
        CPassiveComm::SetRoundBytes(roundBytes);
        const auto slices = CReaderSlices::FromComplete(IsRoot() ? complete : CSimplexMesh(), MASTER_NODE);

        /*--- A named marker without elements: listed, but not with the markers that have elements. ---*/
        auto withEmpty = complete;
        withEmpty.markers.insert(withEmpty.markers.begin() + 1, CSimplexMesh::Marker());
        withEmpty.markers[1].name = "unused_marker";
        const auto emptySlices = CReaderSlices::FromComplete(IsRoot() ? withEmpty : CSimplexMesh(), MASTER_NODE);
        CPassiveComm::SetRoundBytes(saved);

        CHECK(GlobalMax(SlicesOfComplete(*config, slices, complete) ? 0.0 : 1.0) == 0.0);
        CHECK(GlobalMax(SlicesOfComplete(*config, emptySlices, withEmpty) ? 0.0 : 1.0) == 0.0);
        CHECK(emptySlices.markerNames.size() == withEmpty.markers.size());
        CHECK(emptySlices.markersWithElements.size() == withEmpty.markers.size() - 1);
        unsigned long nBoundaryRows = 0;
        for (const auto& rows : slices.boundaryRows) nBoundaryRows += rows.size();
        CHECK((IsRoot() || nBoundaryRows == 0));

        /*--- The geometry: bitwise equal; the metric of every local point. ---*/
        CMemoryMeshReaderFVM reference(config.get(), complete, 0, 1);
        auto** expected = BuildGeometry(*config, reference);
        CDistributedMemoryMeshReaderFVM reader(config.get(), slices, 0, 1);
        auto** geometry = BuildGeometry(*config, reader);
        CHECK(GlobalMax(SameGeometry(*geometry[MESH_0], *expected[MESH_0]) ? 0.0 : 1.0) == 0.0);

        const auto& fine = *geometry[MESH_0];
        const auto metric = slices.FetchPointMetric(fine);
        bool fetched = metric.size() == fine.GetnPoint() * nMet;
        for (auto iPoint = 0ul; fetched && iPoint < fine.GetnPoint(); ++iPoint)
          for (unsigned short iMet = 0; iMet < nMet; ++iMet)
            fetched &= metric[iPoint * nMet + iMet] == complete.metric[fine.nodes->GetGlobalIndex(iPoint) * nMet + iMet];
        CHECK(GlobalMax(fetched ? 0.0 : 1.0) == 0.0);

        /*--- Without a metric: nothing to fetch (all ranks). ---*/
        auto noMetric = complete;
        noMetric.metric.clear();
        const auto plain = CReaderSlices::FromComplete(IsRoot() ? noMetric : CSimplexMesh(), MASTER_NODE);
        CHECK(plain.nMetric == 0);
        CHECK(plain.FetchPointMetric(fine).empty());

        DeleteGeometry(*config, geometry);
        DeleteGeometry(*config, expected);
      }
    }
  }
}

TEST_CASE("MPI custom sensors, gradients, Hessians and metric by global point", "[Adaptation][CustomSensorsMPI]") {
  const auto input = simplex_test::MakeSimplexMesh(2, 12, [](const passivedouble* x) {
    if (x[0] < 1e-10) return std::string("left");
    if (x[0] > 2.0-1e-10) return std::string("right");
    if (x[1] > 1.0-1e-10) return std::string("upper");
    return std::string(x[0] < 1 ? "lower_a" : "lower_b");
  });
  std::vector<passivedouble> reference;
  auto run = [&](bool parallel) {
    auto config = transfer_test::MakeConfig(2,
        "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (S)\n"
        "ADAP_CUSTOM_SENSORS= 'G : GRAD_TEMPERATURE_X; S : PRESSURE+G*G'\n"
        "NUM_METHOD_GRAD= GREEN_GAUSS\nNUM_METHOD_HESS= WEIGHTED_LEAST_SQUARES\nADAP_NORM= 2\n"
        "ADAP_HMIN= 1e-4\nADAP_HMAX= 10\nADAP_COMPLEXITY= 1000\n");
    transfer_test::MeshSolution state(config.get(), input, 0);
    auto* flow = state.solver[MESH_0][FLOW_SOL];
    auto* nodes = flow->GetNodes();
    const auto idx = CPrimitiveIndices<unsigned short>(false, false, 2, 0);
    for (unsigned long point = 0; point < state.Fine().GetnPoint(); ++point) {
      const auto x = state.Fine().nodes->GetCoord(point);
      nodes->SetPrimitive(point, idx.Pressure(), 1+x[0]*x[0]+2*x[1]*x[1]);
      nodes->SetPrimitive(point, idx.Temperature(), x[0]*x[0]*x[0]);
    }
    flow->SetAuxVar_Adapt(&state.Fine(), config.get(), state.solver[MESH_0]);
    flow->SetHessian_Adapt(&state.Fine(), config.get());
    flow->ComputeMetric(&state.Fine(), config.get());
    if (!parallel) reference.resize(state.Fine().GetGlobal_nPointDomain()*9);
    passivedouble diff[4] = {};
    for (unsigned long point = 0; point < state.Fine().GetnPointDomain(); ++point) {
      const auto global = state.Fine().nodes->GetGlobalIndex(point);
      passivedouble values[9] = {SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0))};
      for (unsigned short dim = 0; dim < 2; ++dim)
        values[1+dim] = SU2_TYPE::GetValue(nodes->GetGradient_Adapt()(point, 0, dim));
      for (unsigned short comp = 0; comp < 3; ++comp) {
        values[3+comp] = SU2_TYPE::GetValue(nodes->GetHessian(point, 0, comp));
        values[6+comp] = SU2_TYPE::GetValue(nodes->GetMetric(point, comp));
      }
      for (unsigned short i = 0; i < 9; ++i) {
        if (!parallel) reference[global*9+i] = values[i];
        else {
          const auto group = i == 0 ? 0 : i < 3 ? 1 : i < 6 ? 2 : 3;
          diff[group] = max(diff[group], fabs(values[i]-reference[global*9+i]) / max(1.0, fabs(reference[global*9+i])));
        }
      }
    }
    if (parallel) {
      for (auto& error : diff) error = GlobalMax(error);
      if (IsRoot()) cout << "Custom sensor MPI maximum relative differences: values=" << diff[0]
                        << " gradients=" << diff[1] << " Hessians=" << diff[2] << " metric=" << diff[3] << endl;
      for (const auto error : diff) CHECK(error < 1e-10);
    }
  };
  Serial([&]() { run(false); });
  run(true);
}
