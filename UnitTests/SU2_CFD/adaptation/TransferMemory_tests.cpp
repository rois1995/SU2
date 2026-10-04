/*!
 * \file TransferMemory_tests.cpp
 * \brief Memory ceiling of the distributed solution transfers (REVIEW5_FIX_PLAN.md S1-S4, F2, F3), measured with the
 *        allocation probe of the test_memory executable: in every gated phase (point location, conservative import,
 *        coverage protocol) the requested bytes alive since the test started (residents, the caller's data and the
 *        phase's own allocations) stay within the transfer's prediction, which stays within the ceiling.
 *        Valid for any number of ranks: mpirun -n 3 test_memory.
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
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>

#include "TransferTestCase.hpp"
#include "../../Common/memory/AllocationProbe.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CDistributedLocator.hpp"
#include "../../../SU2_CFD/include/adaptation/CDistributedProjection.hpp"

using namespace transfer_test;
using transfer_memory::TransferPhase;

namespace {

/*--- Number of ranks where a check failed (native MPI, independent of the code under test). ---*/
unsigned long FailedRanks(bool ok) {
  unsigned long bad = ok ? 0 : 1, total = bad;
#ifdef HAVE_MPI
  MPI_Allreduce(&bad, &total, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
#endif
  return total;
}
unsigned long WorldMax(unsigned long value) {
  unsigned long result = value;
#ifdef HAVE_MPI
  MPI_Allreduce(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
#endif
  return result;
}

/*--- Deterministic pseudo-random numbers. ---*/
struct Random {
  uint64_t state;
  explicit Random(uint64_t seed) : state(seed * 6364136223846793005ull + 1442695040888963407ull) {}
  uint64_t Next() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
  double Uniform() { return (Next() >> 11) * (1.0 / 9007199254740992.0); }
};

/*--- The gated phases seen by the probe: per phase the largest measured total (requested bytes alive above the
 *    test's base, at the phase's peak) and the transfer's prediction for it. ---*/
struct PhaseRecord {
  size_t measured = 0, predicted = 0, measuredUsable = 0;
  unsigned long count = 0;
  bool violated = false;
};
std::array<PhaseRecord, 3> records;
size_t base0 = 0, base0Usable = 0;

void Hook(TransferPhase phase, bool begin, size_t predicted) {
  if (begin) {
    alloc_probe::ResetPeak();
    return;
  }
  auto& record = records[static_cast<int>(phase)];
  const size_t peak = alloc_probe::Peak(), peakUsable = alloc_probe::PeakUsable();
  const size_t measured = peak > base0 ? peak - base0 : 0;
  record.measured = std::max(record.measured, measured);
  record.measuredUsable = std::max(record.measuredUsable, peakUsable > base0Usable ? peakUsable - base0Usable : 0);
  record.predicted = std::max(record.predicted, predicted);
  record.count++;
  record.violated |= measured > predicted;
}

/*--- Start a measurement: everything allocated from now on belongs to the transfer (or is declared to it). ---*/
void StartMeasure() {
  records = {};
  transfer_memory::SetTransferMemoryHook(Hook);
  base0 = alloc_probe::Live();
  base0Usable = alloc_probe::LiveUsable();
}
void StopMeasure() { transfer_memory::SetTransferMemoryHook(nullptr); }

/*--- Every phase that ran: measured <= predicted <= ceiling on every rank; a table row on rank 0. ---*/
void CheckPhases(const std::string& name, size_t ceiling) {
  const char* phaseName[3] = {"LOCATE", "IMPORT", "SLIVERS"};
  for (int p = 0; p < 3; ++p) {
    const auto& r = records[p];
    const unsigned long ran = WorldMax(r.count);
    if (ran == 0) continue;
    const bool ok = !r.violated && r.predicted <= ceiling;
    INFO(name << " " << phaseName[p] << ": measured " << r.measured << ", predicted " << r.predicted << ", ceiling "
              << ceiling);
    CHECK(FailedRanks(ok) == 0);
    const unsigned long measured = WorldMax(r.measured), predicted = WorldMax(r.predicted),
                        usable = WorldMax(r.measuredUsable);
    if (SU2_MPI::GetRank() == MASTER_NODE) {
      std::ostringstream row;
      row << "[memtable] P=" << SU2_MPI::GetSize() << " | " << std::left << std::setw(58) << name << " | "
          << std::setw(7) << phaseName[p] << " | measured " << std::setw(9) << measured << " | usable " << std::setw(9)
          << (alloc_probe::HasUsable() ? std::to_string(usable) : "n/a") << " | predicted " << std::setw(9) << predicted
          << " | ratio " << std::fixed << std::setprecision(2) << (measured > 0 ? double(predicted) / measured : 0.0)
          << " | ceiling " << (ceiling >> 20 > 1024 ? std::string("default") : std::to_string(ceiling));
      std::cout << row.str() << std::endl;
    }
  }
}

/*--- Ceiling for a scope. ---*/
struct CeilingScope {
  size_t saved = GetTransferMemoryCeiling();
  explicit CeilingScope(size_t bytes) { SetTransferMemoryCeiling(bytes); }
  ~CeilingScope() { SetTransferMemoryCeiling(saved); }
};

/*--- Every triangle contains the origin, forcing a collect-all search to traverse the broadest fronts. ---*/
CSimplexMesh FanMesh(unsigned long nElem) {
  CSimplexMesh mesh;
  mesh.nDim = 2;
  mesh.coord.resize(2 * (nElem + 1), 0.0);
  mesh.elemRef.assign(nElem, 1);
  const char* names[] = {"right", "upper", "left", "lower_a", "lower_b"};
  for (const auto* name : names) {
    CSimplexMesh::Marker marker;
    marker.name = name;
    marker.ref = mesh.markers.size() + 1;
    mesh.markers.push_back(marker);
  }
  const double pi = std::acos(-1.0);
  for (auto e = 0ul; e < nElem; ++e) {
    const double angle = 2.0 * pi * e / nElem;
    mesh.coord[2 * (e + 1)] = std::cos(angle);
    mesh.coord[2 * (e + 1) + 1] = std::sin(angle);
    const auto next = 1 + (e + 1) % nElem;
    mesh.elem.insert(mesh.elem.end(), {0ul, e + 1, next});
    auto& edges = mesh.markers[e * mesh.markers.size() / nElem].elem;
    edges.insert(edges.end(), {e + 1, next});
  }
  return mesh;
}

/*--- Points of the location fixtures (this rank's queries). ---*/
enum class Queries { INTERLEAVED, CORNER, ASYMMETRIC, EMPTY_RANK_1, VERTICES };
std::vector<passivedouble> MakeQueries(Queries kind, unsigned short nDim, const CGeometry& mesh, unsigned long n) {
  const int rank = SU2_MPI::GetRank();
  std::vector<passivedouble> coord;
  Random random(31 + rank);
  const passivedouble extent[3] = {passivedouble(nDim == 2 ? 2.0 : 1.0), 1.0, 1.0};
  if (kind == Queries::VERTICES) {
    /*--- The donor's own points (on vertices: every element around them contains them). ---*/
    for (auto iPoint = 0ul; iPoint < mesh.GetnPointDomain(); ++iPoint)
      for (unsigned short d = 0; d < nDim; ++d) coord.push_back(SU2_TYPE::GetValue(mesh.nodes->GetCoord(iPoint, d)));
    return coord;
  }
  if ((kind == Queries::ASYMMETRIC && rank != 0) || (kind == Queries::EMPTY_RANK_1 && rank == 1)) return coord;
  const unsigned long count = kind == Queries::ASYMMETRIC ? n * SU2_MPI::GetSize() : n;
  for (unsigned long q = 0; q < count; ++q)
    for (unsigned short d = 0; d < nDim; ++d)
      coord.push_back(kind == Queries::CORNER ? 0.2 * random.Uniform() : extent[d] * random.Uniform());
  return coord;
}

/*--- Values of the projection fixtures. ---*/
void ProjectionValues(const CGeometry& mesh, unsigned short nDim, unsigned short nField,
                      std::vector<su2double>& values) {
  values.assign(mesh.GetnPointDomain() * nField, 0.0);
  for (auto iPoint = 0ul; iPoint < mesh.GetnPointDomain(); ++iPoint) {
    const auto* x = mesh.nodes->GetCoord(iPoint);
    su2double U[MAXVAR] = {};
    AffineFlow(nDim)(x, U);
    for (unsigned short f = 0; f < nDim + 2; ++f) values[iPoint * nField + f] = U[f];
    values[iPoint * nField + nDim + 2] = sin(3.0 * x[0]) * cos(2.0 * x[1]);
  }
}

/*--- Target box scaled by sx in x about the origin (a strip of the donor outside the new domain for sx < 1). ---*/
CSimplexMesh ScaledX(CSimplexMesh mesh, passivedouble sx) {
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) mesh.coord[iPoint * mesh.nDim] *= sx;
  return mesh;
}

struct ProjectionCase {
  std::string name;
  unsigned short nDim;
  unsigned long nDonor, nTarget;
  passivedouble scaleX;
};

const std::vector<ProjectionCase> kProjectionCases = {
    {"2D box, interleaved partitions", 2, 8, 10, 1.0},
    {"3D box, interleaved partitions", 3, 4, 5, 1.0},
    {"2D coarse target over a fine donor (fan-out)", 2, 16, 1, 1.0},
    {"3D coarse target over a fine donor (fan-out)", 3, 6, 1, 1.0},
    {"2D partial coverage (slivers with moments)", 2, 4, 6, 0.99875},
};

}  // namespace

TEST_CASE("Transfer memory: allocation probe", "[TransferMemory]") {
  const auto live = alloc_probe::Live();
  {
    std::vector<char> v(1000);
    CHECK(alloc_probe::Live() - live == 1000);
    struct alignas(64) Wide {
      char c[64];
    };
    auto* w = new Wide[3];
    CHECK(reinterpret_cast<uintptr_t>(w) % 64 == 0);
    CHECK(alloc_probe::Live() - live == 1000 + 3 * 64);
    delete[] w;
    auto* n = new (std::nothrow) double[5];
    CHECK(alloc_probe::Live() - live == 1000 + 40);
    delete[] n;
  }
  CHECK(alloc_probe::Live() == live);
  alloc_probe::ResetPeak();
  { std::vector<double> big(1 << 16); }
  CHECK(alloc_probe::Peak() - live == (1u << 16) * sizeof(double));
}

TEST_CASE("Transfer memory: models", "[TransferMemory]") {
  using namespace transfer_memory;
  SECTION("reviewer's numbers: 3000 B, four peers of 100 2D queries") {
    /*--- The old formula took Q = 9 (each buffer alone fits 3000 B): 48 queries per chunk, replies 3072 B, queries
     *    and replies together 4224 B. The model rejects Q = 9 and takes the smallest Q whose bytes fit. ---*/
    CLocatorChunkModel model;
    model.nRank = 4;
    model.queryBytes = 8 + 2 * 8;
    model.replyBytes = 8 * 4 + 8 * 4;
    model.roundBytes = size_t(1) << 30;
    model.out = model.in = {100, 100, 100, 100};
    const size_t available = 3000;
    const size_t oldQ =
        std::max(CeilDiv(400 * model.queryBytes, available), CeilDiv(400 * model.replyBytes, available));
    CHECK(oldQ == 9);
    CHECK(48 * (model.queryBytes + model.replyBytes) == 4224);
    CHECK(model.Peak(9) > available);
    const auto q = model.SmallestChunks(available);
    CHECK(q > 9);
    CHECK(model.Peak(q) <= available);
    CHECK(model.Peak(q - 1) > available);
  }
  SECTION("bounds do not increase with the chunk / sub-round count, constant beyond the largest list") {
    Random random(7);
    for (int trial = 0; trial < 50; ++trial) {
      CLocatorChunkModel locator;
      CImportRoundModel import;
      const size_t nRank = 1 + random.Next() % 6;
      locator.nRank = import.nRank = nRank;
      locator.rank = import.rank = static_cast<int>(random.Next() % nRank);
      locator.queryBytes = 32;
      locator.replyBytes = 72;
      import.elemBytes = 40;
      import.nodeBytes = 64;
      import.nNode = 4;
      locator.roundBytes = import.roundBytes = 64 + random.Next() % 4000;
      locator.baseline = import.baseline = random.Next() % 1000;
      locator.out.resize(nRank);
      locator.in.resize(nRank);
      import.sendElems.resize(nRank);
      import.recvElems.resize(nRank);
      for (size_t p = 0; p < nRank; ++p) {
        locator.out[p] = random.Next() % 300;
        locator.in[p] = random.Next() % 300;
        import.sendElems[p] = random.Next() % 300;
        import.recvElems[p] = random.Next() % 300;
      }
      locator.in[locator.rank] = locator.out[locator.rank];
      import.recvElems[import.rank] = import.sendElems[import.rank];
      bool monotone = true;
      for (size_t q = 1; q < locator.MaxChunks() + 3; ++q) monotone &= locator.Peak(q + 1) <= locator.Peak(q);
      for (size_t k = 1; k < import.MaxRounds() + 3; ++k) monotone &= import.Peak(k + 1) <= import.Peak(k);
      CHECK(monotone);
      CHECK(locator.Peak(locator.MaxChunks() + 5) == locator.Peak(locator.MaxChunks()));
      CHECK(import.Peak(import.MaxRounds() + 5) == import.Peak(import.MaxRounds()));
      /*--- Smallest feasible: the minimum is the bound at the largest count. ---*/
      const size_t floor = locator.Peak(locator.MaxChunks());
      CHECK(locator.SmallestChunks(floor) > 0);
      CHECK(locator.SmallestChunks(floor - 1) == 0);
      const size_t floorImport = import.Peak(import.MaxRounds());
      CHECK(import.SmallestRounds(floorImport) > 0);
      CHECK(import.SmallestRounds(floorImport - 1) == 0);
    }
  }
  SECTION("saturating arithmetic") {
    CLocatorChunkModel model;
    model.nRank = 2;
    model.queryBytes = 32;
    model.replyBytes = 72;
    model.roundBytes = 1 << 20;
    const uint64_t huge = std::numeric_limits<size_t>::max() / 16;
    model.out = model.in = {huge, huge};
    CHECK(model.Peak(1) == std::numeric_limits<size_t>::max());
    const auto q = model.SmallestChunks(size_t(1) << 40);
    CHECK(q > 1);
    CHECK(model.Peak(q) <= (size_t(1) << 40));
    CHECK(model.Peak(q - 1) > (size_t(1) << 40));
    CHECK(Mul(size_t(1) << 40, size_t(1) << 40) == std::numeric_limits<size_t>::max());
    CHECK(Add(std::numeric_limits<size_t>::max() - 1, 5) == std::numeric_limits<size_t>::max());
  }
}

TEST_CASE("Transfer memory: ADT bytes", "[TransferMemory]") {
  /*--- The tree holds exactly the predicted bytes, and its constructor stays within the predicted peak. ---*/
  Random random(3);
  for (const unsigned short nDim : {2, 3}) {
    for (const unsigned long nElem : {1ul, 2ul, 3ul, 17ul, 500ul}) {
      const unsigned short nNode = nDim + 1;
      const unsigned long nPoint = nElem + nNode;
      std::vector<su2double> coord(nPoint * nDim);
      for (auto& x : coord) x = random.Uniform();
      std::vector<unsigned long> conn(nElem * nNode);
      for (unsigned long e = 0; e < nElem; ++e)
        for (unsigned short k = 0; k < nNode; ++k) conn[e * nNode + k] = e + k;
      std::vector<unsigned short> types(nElem, nDim == 2 ? TRIANGLE : TETRAHEDRON), markers(nElem, 0);
      std::vector<unsigned long> ids(nElem);
      for (unsigned long e = 0; e < nElem; ++e) ids[e] = e;
      size_t retained = 0, peak = 0;
      CADTElemClass::PredictBytes(nDim, nPoint, nElem, nElem * nNode, &retained, &peak);
      const auto before = alloc_probe::Live();
      alloc_probe::ResetPeak();
      auto* adt = new CADTElemClass(nDim, coord, conn, types, markers, ids, false);
      const auto held = alloc_probe::Live() - before;
      const auto constructorPeak = alloc_probe::Peak() - before;
      CHECK(held == sizeof(CADTElemClass) + adt->GetAllocatedBytes());
      CHECK(adt->GetAllocatedBytes() == retained);
      CHECK(constructorPeak <= sizeof(CADTElemClass) + peak);
      delete adt;
    }
  }
}

TEST_CASE("Transfer memory: ADT retained workspaces", "[TransferMemory]") {
  auto mesh = FanMesh(512);
  const auto nElem = mesh.GetnElem();
  std::vector<su2double> coord(mesh.coord.begin(), mesh.coord.end());
  std::vector<unsigned short> types(nElem, TRIANGLE), markers(nElem, 0);
  std::vector<unsigned long> ids(nElem);
  for (auto e = 0ul; e < nElem; ++e) ids[e] = e;
  CADTElemClass adt(2, coord, mesh.elem, types, markers, ids, false);
  const auto resident = adt.GetAllocatedBytes();
  const auto retained = adt.RetainedWorkspaceBound(1);
  const auto before = alloc_probe::Live();
  su2double x[2] = {0.0, 0.0};
  size_t found = 0;
  alloc_probe::ResetPeak();
  {
    std::vector<unsigned long> candidates;
    std::vector<su2double> weights;
    adt.DetermineContainingElements(x, candidates, weights);
    found = candidates.size();
  }
  const auto searchPeak = alloc_probe::Peak() - before;
  const auto growth = alloc_probe::Live() - before;
  CHECK(found == nElem);
  CHECK(growth > 0);
  CHECK(growth == adt.GetAllocatedBytes() - resident);
  CHECK(growth <= retained);
  CHECK(searchPeak <=
        transfer_memory::Add(retained, transfer_memory::ContainmentQueryTransientBound(nElem, sizeof(su2double))));

  /*--- A nearest-element search also keeps its box targets, within the same original retained envelope. ---*/
  su2double distance = 0.0;
  unsigned short marker = 0;
  unsigned long element = 0;
  int rank = 0;
  const auto nearestBefore = alloc_probe::Live();
  const auto treeBefore = adt.GetAllocatedBytes();
  adt.DetermineNearestElement(x, distance, marker, element, rank);
  const auto nearestGrowth = alloc_probe::Live() - nearestBefore;
  CHECK(nearestGrowth == adt.GetAllocatedBytes() - treeBefore);
  CHECK(transfer_memory::Add(growth, nearestGrowth) <= retained);
}

TEST_CASE("Transfer memory: resident structures", "[AdaptationMPI][TransferMemory]") {
  /*--- The bytes the locator and the projection keep after their constructors are their GetMemory() (4 kB slack for
   *    the objects themselves). ---*/
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    MeshSolution donor(config.get(), BoxMesh(nDim, nDim == 2 ? 8 : 4, true), 0);
    MeshSolution target(config.get(), BoxMesh(nDim, nDim == 2 ? 10 : 5, true), 0);
    Mute mute;
    {
      const auto before = alloc_probe::Live();
      auto* locator = new CDistributedLocator(donor.Fine(), donor.markerTags, *config, 0.0);
      const size_t measured = alloc_probe::Live() - before, predicted = locator->GetMemory();
      INFO("locator " << nDim << "D: measured " << measured << ", GetMemory " << predicted);
      CHECK(FailedRanks(measured <= predicted + sizeof(CDistributedLocator) && predicted <= measured + 4096) == 0);
      delete locator;
    }
    {
      CConservativeProjection::Options options;
      const auto before = alloc_probe::Live();
      auto* projection = new CDistributedProjection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags,
                                                    *config, options);
      const size_t measured = alloc_probe::Live() - before, predicted = projection->GetMemory();
      INFO("projection " << nDim << "D: measured " << measured << ", GetMemory " << predicted);
      CHECK(FailedRanks(measured <= predicted + sizeof(CDistributedProjection) && predicted <= measured + 4096) == 0);
      delete projection;
    }
  }
}

TEST_CASE("Transfer memory: point location", "[AdaptationMPI][TransferMemory]") {
  /*--- Every fixture at the default ceiling and at the smallest admitted ceiling (many chunks): the measured bytes
   *    within the prediction within the ceiling, the same results. ---*/
  const std::vector<std::pair<Queries, std::string>> kinds = {{Queries::INTERLEAVED, "interleaved queries"},
                                                              {Queries::CORNER, "all queries in one corner"},
                                                              {Queries::ASYMMETRIC, "all queries from rank 0"},
                                                              {Queries::EMPTY_RANK_1, "no queries on rank 1"},
                                                              {Queries::VERTICES, "queries on the donor vertices"}};
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    MeshSolution donor(config.get(), BoxMesh(nDim, nDim == 2 ? 8 : 4, true), 0);
    for (const auto& kind : kinds) {
      const auto name = std::to_string(nDim) + "D " + kind.second;
      const auto queries = MakeQueries(kind.first, nDim, donor.Fine(), 400);
      std::vector<CElementHit> reference, chunked;
      size_t minimum = 0;
      unsigned long nChunk = 0;
      {
        Mute mute;
        StartMeasure();
        CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
        const auto coord = queries;
        reference = locator.LocateElements(coord);
        minimum = locator.GetLastMinimumCeiling();
        StopMeasure();
      }
      CheckPhases(name, GetTransferMemoryCeiling());
      {
        CeilingScope ceiling(minimum);
        Mute mute;
        StartMeasure();
        CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
        const auto coord = queries;
        chunked = locator.LocateElements(coord);
        nChunk = locator.GetLastChunks();
        StopMeasure();
      }
      CheckPhases(name + ", smallest ceiling", minimum);
      bool same = reference.size() == chunked.size();
      for (auto i = 0ul; same && i < reference.size(); ++i) {
        same = reference[i].found == chunked[i].found && reference[i].key == chunked[i].key &&
               reference[i].minWeight == chunked[i].minWeight;
        for (int k = 0; k < 4; ++k) same &= reference[i].weight[k] == chunked[i].weight[k];
      }
      CHECK(FailedRanks(same) == 0);
      CHECK(nChunk >= 1);
    }
  }
}

TEST_CASE("Transfer memory: retained search workspaces", "[AdaptationMPI][TransferMemory]") {
  /*--- Fresh locators: broad traversal grows the retained fronts, and enough queries make the unchunked reply
   *    exchange dominate the search. Check both one source rank and queries split over all ranks. ---*/
  const unsigned long nElem = 4096, nQuery = 65536;
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), FanMesh(nElem), 0);
  CHECK(nQuery * (24 + 64) + transfer_memory::ContainmentQueryTransientBound(nElem, sizeof(su2double)) <
        nQuery * 2 * 64);
  for (const bool split : {false, true}) {
    const auto name = std::string("2D shared-vertex fan, ") + (split ? "split queries" : "queries from rank 0");
    const unsigned long count =
        split ? nQuery / size + (rank < static_cast<int>(nQuery % size) ? 1 : 0) : (rank == 0 ? nQuery : 0);
    const std::vector<passivedouble> queries(count * 2, 0.0);
    std::vector<CElementHit> reference, chunked;
    size_t minimum = 0, growth = 0;
    unsigned long chunks = 0;
    {
      Mute mute;
      StartMeasure();
      CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
      const auto resident = locator.GetMemory();
      const auto coord = queries;
      reference = locator.LocateElements(coord);
      minimum = locator.GetLastMinimumCeiling();
      chunks = locator.GetLastChunks();
      growth = locator.GetMemory() - resident;
      StopMeasure();
    }
    CheckPhases(name, GetTransferMemoryCeiling());
    CHECK(chunks == 1);
    CHECK(WorldMax(growth) > 0);
    {
      CeilingScope ceiling(minimum);
      Mute mute;
      StartMeasure();
      CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
      const auto coord = queries;
      chunked = locator.LocateElements(coord);
      StopMeasure();
    }
    CheckPhases(name + ", smallest ceiling", minimum);
    bool same = reference.size() == count && chunked.size() == count;
    for (auto i = 0ul; same && i < reference.size(); ++i) {
      same = reference[i].found && chunked[i].found && reference[i].key == chunked[i].key &&
             reference[i].minWeight == chunked[i].minWeight;
      for (int k = 0; k < 4; ++k) same &= reference[i].weight[k] == chunked[i].weight[k];
    }
    CHECK(FailedRanks(same) == 0);
  }
}

namespace {

struct ProjectionRun {
  std::map<uint64_t, std::vector<passivedouble>> values;
  CConservativeProjection::Summary summary;
  size_t minimum = 0;
  unsigned long subRounds = 0, duplicates = 0;
};

ProjectionRun RunProjection(const ProjectionCase& test, const std::string& label, bool quiet = true) {
  ProjectionRun run;
  auto config = MakeConfig(test.nDim, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(test.nDim, test.nDonor, true), 0);
  MeshSolution target(config.get(), ScaledX(BoxMesh(test.nDim, test.nTarget, true), test.scaleX), 0);
  const unsigned short nField = test.nDim + 3;
  std::vector<su2double> donorValues, newValues;
  ProjectionValues(donor.Fine(), test.nDim, nField, donorValues);
  CConservativeProjection::Options options;
  options.sliverRule = CConservativeProjection::SliverRule::GLOBAL;
  {
    std::unique_ptr<Mute> mute(quiet ? new Mute : nullptr);
    StartMeasure();
    CDistributedProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, *config,
                                      options);
    projection.Project(nField, donorValues, newValues);
    StopMeasure();
    run.summary = projection.GetSummary();
    run.minimum = projection.GetLastMinimumCeiling();
    run.subRounds = projection.GetLastSubRounds();
    run.duplicates = projection.GetLastDuplicateNodes();
  }
  CheckPhases(test.name + label, GetTransferMemoryCeiling());
  for (auto iPoint = 0ul; iPoint < target.Fine().GetnPointDomain(); ++iPoint) {
    auto& row = run.values[target.Fine().nodes->GetGlobalIndex(iPoint)];
    for (unsigned short f = 0; f < nField; ++f) row.push_back(SU2_TYPE::GetValue(newValues[iPoint * nField + f]));
  }
  return run;
}

bool SameValues(const ProjectionRun& a, const ProjectionRun& b) {
  if (a.values.size() != b.values.size()) return false;
  for (const auto& entry : a.values) {
    const auto it = b.values.find(entry.first);
    if (it == b.values.end() || it->second != entry.second) return false;
  }
  return true;
}

}  // namespace

TEST_CASE("Transfer memory: conservative import and coverage", "[AdaptationMPI][TransferMemory]") {
  /*--- Every fixture at the default ceiling, at the smallest admitted ceiling (one region box per group, many
   *    sub-rounds) and with three forced sub-rounds (nodes resent): the measured bytes within the prediction within
   *    the ceiling; sub-rounds and groups change nothing (bitwise for the sub-rounds, the order of the right-hand side
   *    sums for the groups), totals exact. ---*/
  for (const auto& test : kProjectionCases) {
    const auto reference = RunProjection(test, "");
    CHECK(reference.subRounds == 1);
    if (SU2_MPI::GetSize() == 1) CHECK(reference.duplicates == 0);
    if (test.scaleX < 1.0) CHECK(reference.summary.nSliverElems > 0);

    CDistributedProjection::SetMinimumSubRounds(3);
    const auto forced = RunProjection(test, ", 3 forced sub-rounds");
    CDistributedProjection::SetMinimumSubRounds(1);
    CHECK(forced.subRounds == 3);
    CHECK(forced.duplicates > reference.duplicates);  // nodes resent in later sub-rounds
    CHECK(FailedRanks(SameValues(forced, reference)) == 0);

    ProjectionRun smallest;
    {
      CeilingScope ceiling(reference.minimum);
      smallest = RunProjection(test, ", smallest ceiling");
    }
    CHECK(smallest.minimum == reference.minimum);
    CHECK(smallest.summary.nPairs == reference.summary.nPairs);
    CHECK(smallest.summary.nSliverElems == reference.summary.nSliverElems);
    passivedouble worst = 0.0;
    for (const auto& entry : smallest.values) {
      const auto& other = reference.values.at(entry.first);
      for (size_t f = 0; f < entry.second.size(); ++f)
        worst = std::max(worst, std::fabs(entry.second[f] - other[f]) / std::max(1.0, std::fabs(other[f])));
    }
    CHECK(FailedRanks(worst <= TransferTol(1e-10, 1e-4)) == 0);
    for (size_t f = 0; f < smallest.summary.newTotal.size(); ++f)
      CHECK(std::fabs(smallest.summary.newTotal[f] - smallest.summary.targetTotal[f]) <=
            TransferTol(1e-12, 1e-5) * smallest.summary.scale[f]);
  }
}

TEST_CASE("Transfer memory: complete transfers", "[AdaptationMPI][TransferMemory]") {
  /*--- The transfers as the driver calls them: the transfer's own data (directories, names, values) are declared to
   *    the gated phases, the measured bytes stay within the prediction. ---*/
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    MeshSolution donor(config.get(), BoxMesh(nDim, nDim == 2 ? 6 : 3, true), 0);
    donor.SetField(FLOW_SOL, AffineFlow(nDim));
    MeshSolution target(config.get(), BoxMesh(nDim, nDim == 2 ? 8 : 4, true), 0);
    {
      CBarycentricTransfer transfer(true, false);
      Mute mute;
      StartMeasure();
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      StopMeasure();
    }
    CheckPhases(std::to_string(nDim) + "D barycentric transfer", GetTransferMemoryCeiling());
    CHECK(WorldMax(records[0].count) == 2);
    {
      CConservativeTransfer transfer;
      Mute mute;
      StartMeasure();
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      StopMeasure();
    }
    CheckPhases(std::to_string(nDim) + "D conservative transfer", GetTransferMemoryCeiling());
  }
}

/*--- Hidden: a ceiling one byte below the smallest admitted one stops the run with one collective error (run by hand
 *    with SU2_COLLECTIVE_ERROR_TESTS=1: mpirun -n 3 test_memory "[locator-minimum]", exit 1, one message). ---*/
TEST_CASE("Collective error: point location below the smallest ceiling", "[.CollectiveError][locator-minimum]") {
  if (std::getenv("SU2_COLLECTIVE_ERROR_TESTS") == nullptr) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 8, true), 0);
  const auto queries = MakeQueries(Queries::CORNER, 2, donor.Fine(), 400);
  CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
  locator.LocateElements(queries);
  SetTransferMemoryCeiling(locator.GetLastMinimumCeiling() - 1);
  locator.LocateElements(queries);
}

TEST_CASE("Collective error: conservative import below the smallest ceiling", "[.CollectiveError][import-minimum]") {
  if (std::getenv("SU2_COLLECTIVE_ERROR_TESTS") == nullptr) return;
  const auto& test = kProjectionCases[2];
  const auto run = RunProjection(test, "");
  SetTransferMemoryCeiling(run.minimum - 1);
  RunProjection(test, "", false);
}
