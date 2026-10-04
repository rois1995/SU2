/*!
 * \file CDistributedProjection.cpp
 * \brief Distributed conservative P1 projection (target-centric import, local supermesh, coverage protocol,
 *        distributed CG and limiter).
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

#include "../../include/adaptation/CDistributedProjection.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"

using namespace conservative;

namespace {

constexpr passivedouble kInf = std::numeric_limits<passivedouble>::infinity();
constexpr uint64_t kFullBit = uint64_t(1) << 63;

std::string PointText(unsigned short nDim, const passivedouble* x) {
  std::ostringstream text;
  text << std::setprecision(10) << "(";
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) text << (iDim ? ", " : "") << x[iDim];
  text << ")";
  return text.str();
}

/*--- Coordinates of a local point as passive doubles. ---*/
void PointCoord(const CGeometry& geometry, unsigned long iPoint, unsigned short nDim, passivedouble* x) {
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
}

}  // namespace

CDistributedProjection::CDistributedProjection(const CGeometry& donorIn, const std::vector<std::string>& donorTags,
                                               const CGeometry& targetIn, const std::vector<std::string>& targetTags,
                                               const CConfig& configIn, const Options& optionsIn)
    : donor(donorIn), target(targetIn), config(configIn), options(optionsIn) {
  const auto start = SU2_MPI::Wtime();
  const int rank = SU2_MPI::GetRank();
  nDim = target.GetnDim();
  nNode = nDim + 1;
  if (donor.GetnDim() != nDim) SU2_MPI::Error("The two meshes have different dimensions.", CURRENT_FUNCTION);

  /*--- Donor: owned elements, their measures and inflated boxes; the replicated canonical boundary. ---*/
  ownedD = OwnedSimplices(donor);
  const auto nOwnedD = ownedD.size();
  volumeD.resize(nOwnedD);
  inflatedD.resize(nOwnedD * 2 * nDim);
  for (auto e = 0ul; e < nOwnedD; ++e) {
    passivedouble x[4][3] = {};
    const passivedouble* nodes[4] = {};
    double* lo = &inflatedD[e * 2 * nDim];
    double* hi = lo + nDim;
    for (unsigned short k = 0; k < nNode; ++k) {
      PointCoord(donor, ownedD.nodes[e * nNode + k], nDim, x[k]);
      nodes[k] = x[k];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        lo[iDim] = (k == 0) ? x[k][iDim] : std::min(lo[iDim], x[k][iDim]);
        hi[iDim] = (k == 0) ? x[k][iDim] : std::max(hi[iDim], x[k][iDim]);
      }
    }
    volumeD[e] = SimplexMeasure(nDim, nodes);
    CADTElemClass::InflateBox(nDim, lo, hi);
  }
  double xMin[3], xMax[3], diagonal = 0.0;
  GlobalBoundingBox(donor, xMin, xMax, &diagonal);
  domainSize = diagonal;
  boundary = std::make_unique<CCanonicalBoundary>(CBoundaryFaces::Gather(donor, donorTags, config), domainSize);
  for (const auto id : boundary->GetMarkers())
    if (id < config.GetnMarker_CfgFile()) namedMarkers.push_back(id);
  for (const auto& name : options.openMarkers) openIds.push_back(MarkerConfigId(config, name));
  std::sort(openIds.begin(), openIds.end());
  cvD.resize(donor.GetnPointDomain());
  for (auto iPoint = 0ul; iPoint < cvD.size(); ++iPoint) cvD[iPoint] = SU2_TYPE::GetValue(donor.nodes->GetVolume(iPoint));

  /*--- Target: owned rows, local elements with an owned vertex (by key), control volumes, markers. ---*/
  nRow = target.GetnPointDomain();
  const unsigned short simplex = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  std::vector<unsigned long> allNodes;
  std::vector<passivedouble> allVolumes;
  for (auto iElem = 0ul; iElem < target.GetnElem(); ++iElem) {
    const auto* element = target.elem[iElem];
    if (element->GetVTK_Type() != simplex) SU2_MPI::Error("The new mesh must consist of simplices.", CURRENT_FUNCTION);
    TargetElem elem;
    elem.elem = iElem;
    uint64_t gids[4] = {};
    bool anyOwned = false;
    uint64_t smallest = UINT64_MAX;
    unsigned long owner = 0;
    passivedouble x[4][3] = {};
    const passivedouble* nodes[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto iPoint = element->GetNode(k);
      elem.nodes[k] = iPoint;
      gids[k] = target.nodes->GetGlobalIndex(iPoint);
      anyOwned |= iPoint < nRow;
      if (gids[k] < smallest) {
        smallest = gids[k];
        owner = iPoint;
      }
      PointCoord(target, iPoint, nDim, x[k]);
      nodes[k] = x[k];
      allNodes.push_back(iPoint);
    }
    elem.volume = SimplexMeasure(nDim, nodes);
    allVolumes.push_back(elem.volume);
    if (!anyOwned) continue;
    elem.key = MakeSimplexKey(gids, nNode);
    elem.owner = owner < nRow;
    elems.push_back(elem);
  }
  std::sort(elems.begin(), elems.end(), [](const TargetElem& a, const TargetElem& b) { return a.key < b.key; });
  cvT.resize(nRow);
  for (auto iPoint = 0ul; iPoint < nRow; ++iPoint) cvT[iPoint] = SU2_TYPE::GetValue(target.nodes->GetVolume(iPoint));
  rowMarkers = PointMarkerIds(target, targetTags, config);
  rowMarkers.resize(nRow);

  /*--- Mass matrix of the owned rows (all their elements are local). ---*/
  mass.Assemble(nDim, nRow, target.GetnPoint(), allNodes, allVolumes);
  CLocalFailure failure;
  for (auto iRow = 0ul; iRow < nRow; ++iRow) {
    if (!(mass.diag[iRow] > 0.0)) {
      failure.Set(1, target.nodes->GetGlobalIndex(iRow),
                  "A point of the new mesh has no control volume (unused point or degenerate elements).");
      break;
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);

  /*--- Region boxes: recursive bisection of the elements with an owned vertex, union of their plain boxes. ---*/
  std::vector<double> boxes(elems.size() * 2 * nDim), centroids(elems.size() * nDim, 0.0);
  for (auto i = 0ul; i < elems.size(); ++i) {
    double* lo = &boxes[i * 2 * nDim];
    double* hi = lo + nDim;
    for (unsigned short k = 0; k < nNode; ++k) {
      passivedouble x[3];
      PointCoord(target, elems[i].nodes[k], nDim, x);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        lo[iDim] = (k == 0) ? x[iDim] : std::min(lo[iDim], x[iDim]);
        hi[iDim] = (k == 0) ? x[iDim] : std::max(hi[iDim], x[iDim]);
        centroids[i * nDim + iDim] += x[iDim] / nNode;
      }
    }
    /*--- Inflated as the ADT inflates boxes: a fill point that round-off puts just outside its element (the centroid of
     *    a missing part, 5.5) still lies in the region box, so every donor element the ADT could accept for it is
     *    imported (a superset of the plan's plain boxes, no effect on the results). ---*/
    CADTElemClass::InflateBox(nDim, lo, hi);
  }
  const unsigned long size = SU2_MPI::GetSize();
  unsigned long maxBoxes = (nDim == 2) ? 32 : 64;
  while (maxBoxes > 1 && size * maxBoxes > (1ul << 20)) maxBoxes /= 2;
  std::vector<unsigned long> boxOfElem;
  regionBoxes = BisectionBoxes(nDim, boxes, centroids, maxBoxes, &boxOfElem);
  for (auto i = 0ul; i < elems.size(); ++i) elems[i].group = boxOfElem[i];
  regions.Build(nDim, regionBoxes);
  summary.timeSetup = SU2_TYPE::GetValue(SU2_MPI::Wtime() - start);
  (void)rank;
}

CDistributedProjection::~CDistributedProjection() = default;

unsigned long CDistributedProjection::minimumSubRounds = 1;

passivedouble CDistributedProjection::DistanceLimit(passivedouble faceSize) const {
  return std::max(faceSize, std::max(options.absoluteLimit, passivedouble(1e-3) * domainSize));
}

void CDistributedProjection::AccumulatePiece(void* context, unsigned short i, passivedouble volume,
                                             const passivedouble* centroid, const passivedouble* mu) {
  auto& self = *static_cast<CDistributedProjection*>(context);
  const auto& elem = self.elems[self.pairT];
  const auto row = elem.nodes[i];
  if (row >= self.nRow) return;  // the piece of a vertex owned by another rank
  const auto nNode = self.nNode, nDim = self.nDim, nField = self.nField;
  const auto* subNodes = &(*self.subElem)[self.pairE * nNode];
  const auto& U = *self.subValues;
  for (unsigned short f = 0; f < nField; ++f) {
    su2double value = 0.0;
    for (unsigned short k = 0; k < nNode; ++k) value += mu[k] * U[subNodes[k] * nField + f];
    self.rhs[row * nField + f] += volume * value;
  }
  const auto piece = self.pairT * nNode + i;
  self.covT[piece] += volume;
  passivedouble origin[3];
  PointCoord(self.target, elem.nodes[0], nDim, origin);
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) self.momT[piece * 3 + iDim] += volume * centroid[iDim];
  auto& cover = self.coverage[self.pairCoverage[self.pairE]];
  cover.contributed = true;
  cover.cover.Add(volume);
  const auto* x0 = &(*self.subCoord)[subNodes[0] * nDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim)
    cover.moment[iDim].Add(volume * (centroid[iDim] + origin[iDim] - x0[iDim]));
  for (unsigned short k = 0; k < nNode; ++k) {
    for (unsigned short f = 0; f < nField; ++f) {
      const su2double& value = U[subNodes[k] * nField + f];
      su2double& lo = self.lower[row * nField + f];
      su2double& hi = self.upper[row * nField + f];
      if (value < lo) lo = value;
      if (value > hi) hi = value;
    }
  }
}

size_t CDistributedProjection::GetMemory() const {
  using transfer_memory::Bytes;
  size_t bytes = ownedD.GetMemory() + Bytes(volumeD) + Bytes(inflatedD) + Bytes(namedMarkers) + Bytes(openIds) +
                 Bytes(localRecords) + directory.GetMemory() + Bytes(elems) + Bytes(cvT) + Bytes(cvD) +
                 Bytes(rowMarkers) + regions.GetMemory() + Bytes(regionBoxes);
  if (boundary) bytes += sizeof(CCanonicalBoundary) + boundary->GetMemory();
  bytes += Bytes(mass.rowPtr) + Bytes(mass.col) + Bytes(mass.value) + Bytes(mass.diag) + Bytes(mass.rowVolume);
  bytes += Bytes(coverage) + Bytes(coverageKeys) + Bytes(pairCoverage) + Bytes(rhs) + Bytes(lower) + Bytes(upper) +
           Bytes(range) + Bytes(covT) + Bytes(momT) + Bytes(fillA) + Bytes(fillOpenA) + Bytes(sliverA) +
           Bytes(sliverOpenA) + Bytes(targetTotalA);
  bytes += Bytes(summary.scale) + Bytes(summary.donorTotal) + Bytes(summary.targetTotal) + Bytes(summary.fill) +
           Bytes(summary.sliver) + Bytes(summary.fillOpen) + Bytes(summary.sliverOpen) + Bytes(summary.correction) +
           Bytes(summary.supermeshDefect) + Bytes(summary.newTotal) + Bytes(summary.iterations) +
           Bytes(summary.residual) + Bytes(summary.nLimited) + Bytes(summary.infeasible) + Bytes(summary.nViolations) +
           Bytes(summary.maxViolation);
  return bytes;
}

size_t CDistributedProjection::SubMeshBytes(uint64_t nElem, uint64_t nRecord) const {
  using namespace transfer_memory;
  const size_t sp = sizeof(passivedouble), sa = sizeof(su2double), ul = sizeof(unsigned long);
  const size_t sub = Mul(nElem, sizeof(SubElem));
  const size_t records = Mul(nRecord, Add(sizeof(uint64_t), Mul(nDim, sp), recordBytes));
  /*--- Deduplication of the node records: the order, the sorted gids, coordinates and values. ---*/
  const size_t nodes = Mul(nRecord, Add(sizeof(uint64_t), Mul(nDim, sp), Mul(nField, sa)));
  const size_t dedup = Add(sub, records, Mul(nRecord, ul), nodes);
  /*--- Connectivity, measures, coverage slots; the ADT with the caller's copies and its constructor. ---*/
  const size_t elements = Mul(nElem, Add(Mul(nNode, ul), sp, ul, sizeof(CoverageKey)));
  size_t adtRetained = 0, adtPeak = 0;
  CADTElemClass::PredictBytes(nDim, nRecord, nElem, nElem * nNode, &adtRetained, &adtPeak);
  const size_t adtCopies = Add(Mul(Mul(nRecord, nDim), sa), Mul(nElem, Add(Mul(nNode, ul), 4, ul)));
  const size_t build = Add(sub, nodes, elements, adtCopies, adtPeak, sizeof(CADTElemClass));
  /*--- Clipping: the sub-mesh without the gids, the ADT, the candidates of a box query and of a containment query
   *    (one vector for the ids), the nearest-face queries and the marker names of a fill stencil. ---*/
  size_t maxMarkers = 0;
  for (const auto& list : rowMarkers) maxMarkers = std::max(maxMarkers, list.size());
  const size_t clip = Add(sub, Mul(nRecord, Add(Mul(nDim, sp), Mul(nField, sa))), elements, adtRetained,
                          sizeof(CADTElemClass), IntersectionQueryBound(nElem), ContainmentQueryBound(nElem, sa),
                          boundary->QueryBytes(), GrowthBound(maxMarkers, sizeof(uint32_t)));
  return std::max({dedup, build, clip});
}

void CDistributedProjection::Project(unsigned short nFieldIn, const std::vector<su2double>& donorValues,
                                     std::vector<su2double>& newValues, size_t callerBytesIn) {
  SU2_ZONE_SCOPED
  using transfer_memory::Bytes;
  nField = nFieldIn;
  recordBytes = nField * FieldValueBytes();
  const auto timeSetup = summary.timeSetup;
  summary = Summary();
  summary.timeSetup = timeSetup;
  summary.nField = nField;
  summary.nTargetElem = target.GetGlobal_nElemDomain();
  summary.nDonorElem = donor.GetGlobal_nElemDomain();
  const auto nOwnedD = donor.GetnPointDomain();
  if (donorValues.size() != nOwnedD * nField) SU2_MPI::Error("Wrong size of the donor values.", CURRENT_FUNCTION);

  /*--- Donor fields of the local points (owned, then halos from their owners), and the directory. ---*/
  auto start = SU2_MPI::Wtime();
  localRecords.assign(donor.GetnPoint() * recordBytes, 0);
  for (auto iPoint = 0ul; iPoint < nOwnedD; ++iPoint)
    PackFieldValues(&localRecords[iPoint * recordBytes], &donorValues[iPoint * nField], nField);
  CPassiveComm::ExchangeHalo(donor, localRecords.data(), recordBytes);
  {
    std::vector<uint64_t> gids(nOwnedD);
    for (auto iPoint = 0ul; iPoint < nOwnedD; ++iPoint) gids[iPoint] = donor.nodes->GetGlobalIndex(iPoint);
    directory.Build(gids, std::vector<char>(localRecords.begin(), localRecords.begin() + nOwnedD * recordBytes),
                    recordBytes, "donor");
  }
  std::vector<su2double> localDonor(donor.GetnPoint() * nField);
  for (auto iPoint = 0ul; iPoint < donor.GetnPoint(); ++iPoint)
    UnpackFieldValues(&localRecords[iPoint * recordBytes], &localDonor[iPoint * nField], nField);
  localTime[0] = SU2_MPI::Wtime() - start;

  rhs.assign(nRow * nField, 0.0);
  lower.assign(nRow * nField, kInf);
  upper.assign(nRow * nField, -kInf);
  covT.assign(elems.size() * nNode, 0.0);
  momT.assign(elems.size() * nNode * 3, 0.0);
  coverage.clear();
  coverageKeys.clear();

  /*--- The caller's arrays and this call's local donor values are live in the gated phases (memory ceiling). ---*/
  const size_t callerBytes = callerBytesIn + Bytes(donorValues) + Bytes(newValues) + Bytes(localDonor);
  std::vector<FillEntry> fills;
  Supermesh(fills, callerBytes);
  start = SU2_MPI::Wtime();
  Slivers(localDonor, fills, callerBytes);
  localTime[2] = SU2_MPI::Wtime() - start;
  Fill(fills);
  Totals(donorValues);
  Solve(newValues);
  localRecords = std::vector<char>();
  coverage = std::vector<Coverage>();
  coverageKeys = std::vector<CoverageKey>();
  pairCoverage = std::vector<unsigned long>();
}

void CDistributedProjection::Supermesh(std::vector<FillEntry>& fills, size_t callerBytes) {
  using namespace transfer_memory;
  TransferMemoryEvent(TransferPhase::IMPORT, true, 0);
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  const auto nOwnedD = ownedD.size();
  auto start = SU2_MPI::Wtime();
  const size_t ceiling = GetTransferMemoryCeiling();
  const size_t roundBytes = CPassiveComm::GetRoundBytes();
  size_t predicted = 0;

  /*--- Import plan, count pass (nothing stored per element yet): the region boxes that each owned donor element's
   *    inflated box intersects; per box of every rank the number of elements it will import, per rank the distinct
   *    elements sent to it, the (owned element, importer rank) pairs. ---*/
  const size_t nBoxAll = regions.GetnBox();
  std::vector<uint64_t> boxCount(nBoxAll, 0), distinct(size, 0);
  size_t nHit = 0;
  unsigned long pairsOwned = 0;
  {
    std::vector<unsigned long> ids;
    for (auto e = 0ul; e < nOwnedD; ++e) {
      const double* lo = &inflatedD[e * 2 * nDim];
      regions.BoxesIntersecting(lo, lo + nDim, ids);
      nHit += ids.size();
      int last = -1;
      for (const auto b : ids) {
        boxCount[b]++;
        if (regions.BoxRank(b) != last) {
          distinct[regions.BoxRank(b)]++;
          pairsOwned++;
        }
        last = regions.BoxRank(b);
      }
    }
  }
  ownerPairs = pairsOwned;
  const auto nLocalBox = regionBoxes.size() / (2 * nDim);
  std::vector<uint64_t> importCount(nLocalBox, 0);
  uint64_t distinctImport = 0;
  size_t planBytes = 0;
  {
    /*--- Per destination rank: the counts of its boxes, then the number of distinct elements sent to it. ---*/
    std::vector<uint64_t> send;
    send.reserve(nBoxAll + size);
    std::vector<size_t> sendCount(size), recvCount;
    for (int t = 0; t < size; ++t) {
      send.insert(send.end(), boxCount.begin() + regions.FirstBox(t), boxCount.begin() + regions.FirstBox(t + 1));
      send.push_back(distinct[t]);
      sendCount[t] = regions.FirstBox(t + 1) - regions.FirstBox(t) + 1;
    }
    const auto received = CPassiveComm::Alltoallv(send, sendCount, recvCount);
    /*--- Typed exchange: the sent array, the received bytes and their typed copy, staging and counts. ---*/
    planBytes = Add(Mul(Add(Mul(2, nBoxAll + size), Mul(3, Mul(size, nLocalBox + 1))), sizeof(uint64_t)),
                    TransportBytes(size), Mul(3 * size, sizeof(size_t)));
    for (size_t i = 0; i < received.size(); ++i) {
      const auto j = i % (nLocalBox + 1);
      if (j < nLocalBox) {
        importCount[j] += received[i];
      } else {
        distinctImport += received[i];
      }
    }
  }
  boxCount = std::vector<uint64_t>();
  distinct = std::vector<uint64_t>();

  /*--- Memory ceiling (5.17), admission before the import data is allocated. Live during the whole import: resident
   *    structures (with the target arrays of this projection), the caller's arrays, the fill stencils (bound: every
   *    owned dual piece, grown by push_back), the target volume terms, the persistent coverage records and their index
   *    (reserved for the distinct imported elements), the routing of the owned elements (CSR), the plan arrays, the
   *    node stamps, the per-group element lists, the per-peer counts. ---*/
  unsigned long nOwnedPieces = 0, nOwnerElems = 0;
  for (const auto& elem : elems) {
    nOwnerElems += elem.owner;
    for (unsigned short k = 0; k < nNode; ++k) nOwnedPieces += elem.nodes[k] < nRow;
  }
  const size_t fillBound = GrowthBound(nOwnedPieces, sizeof(FillEntry));
  const size_t persistent = Mul(distinctImport, sizeof(Coverage) + sizeof(CoverageKey));
  const size_t routingBytes = Add(Mul(nHit, sizeof(unsigned long)), Mul(nOwnedD + 1, sizeof(size_t)));
  const size_t listBytes = Add(Mul(pairsOwned, sizeof(unsigned long)), Mul(size + 1, sizeof(size_t)));
  const size_t fixedBytes = Add(Mul(nLocalBox, sizeof(uint64_t) + sizeof(uint32_t)), Mul(4 * nBoxAll, sizeof(uint32_t)),
                                TransportBytes(size), Mul(donor.GetnPoint(), sizeof(unsigned long)),
                                Mul(nOwnerElems, sizeof(double)), Mul(8 * size, 8), regions.QueryBytes(), 4096);
  const size_t sliverBound = Add(SliversBytes(distinctImport, pairsOwned, nOwnedD), persistent, fillBound);
  auto liveBaseline = [&]() { return Add(GetMemory(), callerBytes, fillBound, routingBytes, listBytes, fixedBytes); };
  const size_t planPeak = Add(liveBaseline(), persistent, planBytes);
  predicted = planPeak;

  /*--- Elements imported per group (upper bound from the per-box counts) and the bound of a group's import: the
   *    sub-mesh phases, or the smallest sub-rounds (one element per peer and round). ---*/
  const size_t perElemWire = 8 * (1 + nNode) + nNode * (8 + 8 * nDim + recordBytes);
  const size_t reservedPerElem =
      sizeof(SubElem) + nNode * (sizeof(uint64_t) + nDim * sizeof(passivedouble) + recordBytes);
  const size_t groupExtra = Mul(4 * size, sizeof(uint64_t));
  auto groupNeed = [&](uint64_t nElem) {
    const size_t smallest = Mul(size, 2 * sizeof(uint64_t) + perElemWire);
    const size_t rounds = Add(Mul(nElem, reservedPerElem), Mul(2, smallest), Staging(roundBytes, smallest, smallest),
                              TransportBytes(size), Mul(32, size), nNode * sizeof(unsigned long));
    return Add(groupExtra, std::max(SubMeshBytes(nElem, nElem * nNode), rounds));
  };
  CLocalFailure failure;
  size_t budget = 0;
  const size_t base = Add(liveBaseline(), persistent);
  {
    /*--- Smallest ceiling that admits this projection (every region box its own group, K at its largest). ---*/
    size_t minimum = std::max(planPeak, Add(GetMemory(), callerBytes, sliverBound));
    for (auto b = 0ul; b < nLocalBox; ++b) minimum = std::max(minimum, Add(base, groupNeed(importCount[b])));
    lastMinimumCeiling = CPassiveComm::AllreduceMax(static_cast<unsigned long>(minimum));
  }
  if (std::max(planPeak, Add(GetMemory(), callerBytes, sliverBound)) > ceiling) {
    const bool slivers = planPeak <= ceiling;
    failure.Set(1, rank,
                "Strong local coarsening: the target-centric conservative transfer would need " +
                    std::to_string(slivers ? Add(GetMemory(), callerBytes, sliverBound) : planPeak) + " bytes of " +
                    (slivers ? "owner coverage data" : "resident and coverage data") + " on rank " +
                    std::to_string(rank) + ", more than the memory ceiling of the transfer (" +
                    std::to_string(ceiling) + " bytes).");
  } else {
    budget = ceiling - base;
  }
  std::vector<uint32_t> localGroup(nLocalBox, 0);
  unsigned long nLocalGroup = nLocalBox > 0 ? 1 : 0;
  uint64_t used = 0;
  /*--- One group if the whole distinct import fits (the per-box counts count an element once per box). ---*/
  const bool oneGroup = !failure.Failed() && groupNeed(distinctImport) <= budget;
  for (auto b = 0ul; b < nLocalBox && !failure.Failed() && !oneGroup; ++b) {
    if (groupNeed(importCount[b]) > budget) {
      failure.Set(1, rank,
                  "Strong local coarsening: the target-centric conservative transfer would need " +
                      std::to_string(Add(base, groupNeed(importCount[b]))) +
                      " bytes to import the donor elements of one region box on rank " + std::to_string(rank) +
                      ", more than the memory ceiling of the transfer (" + std::to_string(ceiling) + " bytes).");
      break;
    }
    if (groupNeed(used + importCount[b]) > budget) {
      nLocalGroup++;
      used = 0;
    }
    used += importCount[b];
    localGroup[b] = nLocalGroup - 1;
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  {
    unsigned long planned[3] = {base, 0, 0}, largest[3];
    for (auto b = 0ul; b < nLocalBox; ++b) planned[1] = std::max<unsigned long>(planned[1], groupNeed(importCount[b]));
    planned[2] = groupNeed(distinctImport);
    CPassiveComm::Allreduce(planned, largest, 3, CPassiveComm::Op::MAX);
    summary.memoryResident = largest[0];
    summary.memoryLargestBox = largest[1];
    summary.memoryImport = largest[2];
  }
  nGroup = std::max(1ul, CPassiveComm::AllreduceMax(nLocalGroup));
  const auto groupOfBox = CPassiveComm::Allgatherv(localGroup, nullptr);
  summary.nImportGroups = nGroup;

  /*--- Routing of the owned donor elements (CSR: the boxes each one intersects), the persistent records. ---*/
  std::vector<size_t> hitStart(nOwnedD + 1, 0);
  std::vector<unsigned long> hitBox(nHit);
  {
    std::vector<unsigned long> ids;
    for (auto e = 0ul; e < nOwnedD; ++e) {
      const double* lo = &inflatedD[e * 2 * nDim];
      regions.BoxesIntersecting(lo, lo + nDim, ids);
      std::copy(ids.begin(), ids.end(), hitBox.begin() + hitStart[e]);
      hitStart[e + 1] = hitStart[e] + ids.size();
    }
  }
  coverage.reserve(distinctImport);
  coverageKeys.reserve(distinctImport);
  std::vector<double> targetVolumeTerms;
  targetVolumeTerms.reserve(nOwnerElems);

  const size_t elemBytes = sizeof(uint64_t) * (1 + nNode);
  const size_t nodeBytes = sizeof(uint64_t) + nDim * sizeof(double) + recordBytes;
  std::vector<unsigned long> stamp(donor.GetnPoint(), 0);
  unsigned long tag = 0;
  unsigned long nTested = 0, nPairs = 0, nOutside = 0, nDegenerate = 0;
  passivedouble maxUncovered = 0.0;
  unsigned long nBeyond = 0;
  passivedouble worstRatio = 0.0, worstX[3] = {};
  uint64_t worstGid = UINT64_MAX;
  passivedouble maxFillDistance = 0.0, maxFillRelDistance = 0.0;
  localImports = 0;
  lastSubRounds = 0;
  lastDuplicateNodes = 0;
  passivedouble importTime = 0.0;

  for (unsigned long g = 0; g < nGroup; ++g) {
    auto importStart = SU2_MPI::Wtime();
    /*--- Element lists of the group per destination (CSR, element order), the counts of every pair. ---*/
    std::vector<size_t> listStart(size + 1, 0);
    for (auto e = 0ul; e < nOwnedD; ++e) {
      int last = -1;
      for (auto h = hitStart[e]; h < hitStart[e + 1]; ++h) {
        const auto b = hitBox[h];
        if (groupOfBox[b] != g || regions.BoxRank(b) == last) continue;
        last = regions.BoxRank(b);
        listStart[last + 1]++;
      }
    }
    for (int t = 0; t < size; ++t) listStart[t + 1] += listStart[t];
    std::vector<unsigned long> listElem(listStart[size]);
    {
      std::vector<size_t> fill(listStart.begin(), listStart.end() - 1);
      for (auto e = 0ul; e < nOwnedD; ++e) {
        int last = -1;
        for (auto h = hitStart[e]; h < hitStart[e + 1]; ++h) {
          const auto b = hitBox[h];
          if (groupOfBox[b] != g || regions.BoxRank(b) == last) continue;
          last = regions.BoxRank(b);
          listElem[fill[last]++] = e;
        }
      }
    }
    std::vector<uint64_t> sendElems(size), recvElems(size);
    for (int t = 0; t < size; ++t) sendElems[t] = listStart[t + 1] - listStart[t];
    CPassiveComm::Alltoall(sendElems.data(), recvElems.data(), sizeof(uint64_t));
    uint64_t nElemGroup = 0;
    for (int p = 0; p < size; ++p) nElemGroup += recvElems[p];
    const uint64_t nRecordGroup = nElemGroup * nNode;

    /*--- Sub-rounds K: smallest with the envelope (every element with all its nodes) under the ceiling, also the
     *    sub-mesh phases (independent of K), the largest over the ranks. ---*/
    CImportRoundModel model;
    model.nRank = size;
    model.rank = rank;
    model.elemBytes = elemBytes;
    model.nodeBytes = nodeBytes;
    model.nNode = nNode;
    model.roundBytes = roundBytes;
    model.baseline = Add(liveBaseline(), groupExtra, Mul(nElemGroup, reservedPerElem));
    model.sendElems = sendElems;
    model.recvElems = recvElems;
    const size_t subMesh = Add(liveBaseline(), groupExtra, SubMeshBytes(nElemGroup, nRecordGroup));
    size_t localRounds = subMesh <= ceiling ? model.SmallestRounds(ceiling) : 0;
    if (localRounds > 0) localRounds = std::max<size_t>(localRounds, minimumSubRounds);
    CLocalFailure groupFailure;
    if (localRounds == 0) {
      groupFailure.Set(1, rank,
                       "Strong local coarsening: the target-centric conservative transfer would need " +
                           std::to_string(std::max(subMesh, model.Peak(model.MaxRounds()))) +
                           " bytes to import or send the donor elements of import group " + std::to_string(g) +
                           " on rank " + std::to_string(rank) + ", more than the memory ceiling of the transfer (" +
                           std::to_string(ceiling) + " bytes).");
    }
    CollectiveFailure(groupFailure, CURRENT_FUNCTION);
    const unsigned long nRound = CPassiveComm::AllreduceMax(static_cast<unsigned long>(localRounds));
    if (model.Peak(nRound) > ceiling)
      SU2_MPI::Error("Inconsistent import sub-rounds (internal error).", CURRENT_FUNCTION);
    predicted = std::max({predicted, model.Peak(nRound), subMesh});
    lastSubRounds = std::max(lastSubRounds, nRound);

    /*--- Sub-mesh arrays reserved for the envelope; the sub-rounds append to them. ---*/
    std::vector<SubElem> subElems;
    std::vector<uint64_t> nodeGid;
    std::vector<passivedouble> nodeCoord;
    std::vector<char> nodeRecords;
    subElems.reserve(nElemGroup);
    nodeGid.reserve(nRecordGroup);
    nodeCoord.reserve(nRecordGroup * nDim);
    nodeRecords.reserve(nRecordGroup * recordBytes);
    for (unsigned long k = 0; k < nRound; ++k) {
      /*--- Exact stream sizes: element records, distinct nodes per destination and sub-round. ---*/
      std::vector<size_t> sendBytes(size, 0), first(size), last(size);
      size_t largest = 0;
      for (int t = 0; t < size; ++t) {
        const auto n = listStart[t + 1] - listStart[t];
        first[t] = listStart[t] + n * k / nRound;
        last[t] = listStart[t] + n * (k + 1) / nRound;
        largest = std::max(largest, last[t] - first[t]);
        tag++;
        size_t nodesOf = 0;
        for (auto i = first[t]; i < last[t]; ++i) {
          for (unsigned short j = 0; j < nNode; ++j) {
            const auto iPoint = ownedD.nodes[listElem[i] * nNode + j];
            if (stamp[iPoint] != tag) {
              stamp[iPoint] = tag;
              nodesOf++;
            }
          }
        }
        sendBytes[t] = 2 * sizeof(uint64_t) + (last[t] - first[t]) * elemBytes + nodesOf * nodeBytes;
      }
      std::vector<char> send;
      send.reserve(std::accumulate(sendBytes.begin(), sendBytes.end(), size_t(0)));
      {
        std::vector<unsigned long> nodes;
        nodes.reserve(largest * nNode);
        for (int t = 0; t < size; ++t) {
          tag++;
          nodes.clear();
          PutBytes(send, static_cast<uint64_t>(last[t] - first[t]));
          for (auto i = first[t]; i < last[t]; ++i) {
            const auto e = listElem[i];
            PutBytes(send, static_cast<uint64_t>(e));
            for (unsigned short j = 0; j < nNode; ++j) {
              const auto iPoint = ownedD.nodes[e * nNode + j];
              PutBytes(send, static_cast<uint64_t>(donor.nodes->GetGlobalIndex(iPoint)));
              if (stamp[iPoint] != tag) {
                stamp[iPoint] = tag;
                nodes.push_back(iPoint);
              }
            }
          }
          PutBytes(send, static_cast<uint64_t>(nodes.size()));
          for (const auto iPoint : nodes) {
            PutBytes(send, static_cast<uint64_t>(donor.nodes->GetGlobalIndex(iPoint)));
            for (unsigned short iDim = 0; iDim < nDim; ++iDim)
              PutBytes(send, static_cast<double>(SU2_TYPE::GetValue(donor.nodes->GetCoord(iPoint, iDim))));
            send.insert(send.end(), &localRecords[iPoint * recordBytes],
                        &localRecords[iPoint * recordBytes] + recordBytes);
          }
        }
      }
      std::vector<size_t> recvBytes;
      auto received = CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);
      send = std::vector<char>();

      /*--- Append to the sub-mesh arrays (within the reserved envelope). ---*/
      const char* src = received.data();
      for (int p = 0; p < size; ++p) {
        const char* end = src + recvBytes[p];
        if (src == end) continue;
        const auto nElem = GetBytes<uint64_t>(src);
        for (uint64_t i = 0; i < nElem; ++i) {
          SubElem elem;
          elem.rank = p;
          elem.index = GetBytes<uint64_t>(src);
          for (unsigned short j = 0; j < nNode; ++j) elem.gid[j] = GetBytes<uint64_t>(src);
          elem.key = MakeSimplexKey(elem.gid, nNode);
          subElems.push_back(elem);
        }
        const auto nNodeRecv = GetBytes<uint64_t>(src);
        for (uint64_t i = 0; i < nNodeRecv; ++i) {
          nodeGid.push_back(GetBytes<uint64_t>(src));
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) nodeCoord.push_back(GetBytes<double>(src));
          nodeRecords.insert(nodeRecords.end(), src, src + recordBytes);
          src += recordBytes;
        }
        if (src != end) SU2_MPI::Error("Malformed import stream.", CURRENT_FUNCTION);
      }
    }
    if (subElems.size() != nElemGroup || nodeGid.size() > nRecordGroup)
      SU2_MPI::Error("Import beyond its envelope (internal error).", CURRENT_FUNCTION);
    localImports += subElems.size();

    /*--- Sub-mesh: nodes by global index (duplicates of other senders and sub-rounds dropped), elements by key. ---*/
    std::vector<uint64_t> gidSorted;
    std::vector<passivedouble> coord;
    std::vector<su2double> values;
    {
      std::vector<unsigned long> order(nodeGid.size());
      std::iota(order.begin(), order.end(), 0ul);
      std::sort(order.begin(), order.end(), [&](unsigned long a, unsigned long b) { return nodeGid[a] < nodeGid[b]; });
      gidSorted.reserve(nodeGid.size());
      coord.reserve(nodeGid.size() * nDim);
      values.reserve(nodeGid.size() * nField);
      for (const auto i : order) {
        if (!gidSorted.empty() && gidSorted.back() == nodeGid[i]) {
          lastDuplicateNodes++;
          continue;
        }
        gidSorted.push_back(nodeGid[i]);
        coord.insert(coord.end(), &nodeCoord[i * nDim], &nodeCoord[i * nDim] + nDim);
        values.resize(values.size() + nField);
        UnpackFieldValues(&nodeRecords[i * recordBytes], &values[values.size() - nField], nField);
      }
    }
    nodeGid = std::vector<uint64_t>();
    nodeCoord = std::vector<passivedouble>();
    nodeRecords = std::vector<char>();
    std::sort(subElems.begin(), subElems.end(), [](const SubElem& a, const SubElem& b) { return a.key < b.key; });
    const auto nSub = subElems.size();
    std::vector<unsigned long> conn(nSub * nNode);
    std::vector<passivedouble> measure(nSub);
    pairCoverage.assign(nSub, 0);
    std::vector<CoverageKey> added;
    added.reserve(nSub);
    for (auto i = 0ul; i < nSub; ++i) {
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) {
        conn[i * nNode + k] =
            std::lower_bound(gidSorted.begin(), gidSorted.end(), subElems[i].gid[k]) - gidSorted.begin();
        nodes[k] = &coord[conn[i * nNode + k] * nDim];
      }
      measure[i] = SimplexMeasure(nDim, nodes);
      /*--- Coverage record of the element (one per element over the groups, in the order of first import): the
       *    sorted index of the earlier groups, else a new record (an element appears once per group). ---*/
      CoverageKey key;
      key.rank = subElems[i].rank;
      key.index = subElems[i].index;
      const auto it = std::lower_bound(coverageKeys.begin(), coverageKeys.end(), key);
      if (it != coverageKeys.end() && it->rank == key.rank && it->index == key.index) {
        pairCoverage[i] = it->slot;
      } else {
        if (coverage.size() == coverage.capacity()) SU2_MPI::Error("Coverage beyond the plan.", CURRENT_FUNCTION);
        key.slot = coverage.size();
        Coverage record;
        record.rank = subElems[i].rank;
        record.index = subElems[i].index;
        record.volume = measure[i];
        coverage.push_back(record);
        added.push_back(key);
        pairCoverage[i] = key.slot;
      }
    }
    /*--- Merge the new keys into the index (from the back, within the reserved capacity). ---*/
    std::sort(added.begin(), added.end());
    {
      size_t i = coverageKeys.size(), j = added.size(), k = i + j;
      coverageKeys.resize(k);
      while (j > 0) {
        if (i > 0 && added[j - 1] < coverageKeys[i - 1]) {
          coverageKeys[--k] = coverageKeys[--i];
        } else {
          coverageKeys[--k] = added[--j];
        }
      }
    }
    added = std::vector<CoverageKey>();
    gidSorted = std::vector<uint64_t>();
    std::unique_ptr<CADTElemClass> adt;
    if (nSub > 0) {
      std::vector<su2double> adtCoord(coord.begin(), coord.end());
      std::vector<unsigned long> adtConn = conn;
      std::vector<unsigned short> types(nSub, nDim == 2 ? TRIANGLE : TETRAHEDRON), markers(nSub, 0);
      std::vector<unsigned long> ids(nSub);
      std::iota(ids.begin(), ids.end(), 0ul);
      adt = std::make_unique<CADTElemClass>(nDim, adtCoord, adtConn, types, markers, ids, false);
    }
    importTime += SU2_MPI::Wtime() - importStart;

    /*--- Supermesh of the target elements of this group (their region box is in the group), in key order. ---*/
    subValues = &values;
    subElem = &conn;
    subCoord = &coord;
    Frame frame;
    std::vector<unsigned long> candidates;
    std::vector<su2double> containing;
    for (auto iT = 0ul; iT < elems.size(); ++iT) {
      const auto& elem = elems[iT];
      if (groupOfBox[regions.FirstBox(rank) + elem.group] != g) continue;
      passivedouble x[4][3] = {};
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) {
        PointCoord(target, elem.nodes[k], nDim, x[k]);
        nodes[k] = x[k];
      }
      SetFrame(nDim, nodes, frame);
      if (elem.owner) targetVolumeTerms.push_back(frame.volume);
      if (!(frame.volume > 0.0)) {
        nDegenerate += elem.owner;
        continue;
      }
      su2double bbMin[3] = {}, bbMax[3] = {};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        passivedouble lo = x[0][iDim], hi = x[0][iDim];
        for (unsigned short k = 1; k < nNode; ++k) {
          lo = std::min(lo, x[k][iDim]);
          hi = std::max(hi, x[k][iDim]);
        }
        bbMin[iDim] = lo;
        bbMax[iDim] = hi;
      }
      candidates.clear();
      if (adt) adt->DetermineIntersectingElements(bbMin, bbMax, candidates);
      std::sort(candidates.begin(), candidates.end());  // sub elements are sorted by key
      bool any = false;
      pairT = iT;
      for (const auto e : candidates) {
        const passivedouble* donorNodes[4] = {};
        for (unsigned short k = 0; k < nNode; ++k) donorNodes[k] = &coord[conn[e * nNode + k] * nDim];
        pairE = e;
        bool clipped = false;
        const bool overlapped = Overlap(nDim, frame, donorNodes, measure[e], AccumulatePiece, this, &clipped);
        if (elem.owner) {
          nTested += clipped;
          nPairs += overlapped;
        }
        any |= overlapped;
      }
      if (!any && elem.owner) nOutside++;

      /*--- Fill stencils of the owned pieces with a missing part (values fetched later). ---*/
      const passivedouble pieceVolume = frame.volume / nNode;
      for (unsigned short i = 0; i < nNode; ++i) {
        const auto row = elem.nodes[i];
        if (row >= nRow) continue;
        const auto piece = iT * nNode + i;
        const passivedouble missing = pieceVolume - covT[piece];
        maxUncovered = std::max(maxUncovered, missing / pieceVolume);
        if (!(missing > kGapFraction * pieceVolume)) continue;
        passivedouble volume = 0.0, centroid[3] = {}, xp[3] = {};
        PieceMoments(nDim, frame, i, volume, centroid);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          const passivedouble local = (missing > kCentroidFraction * pieceVolume)
                                          ? (volume * centroid[iDim] - momT[piece * 3 + iDim]) / missing
                                          : centroid[iDim];
          xp[iDim] = local + frame.origin[iDim];
        }
        FillEntry fill;
        fill.row = row;
        fill.missing = missing;
        std::vector<uint32_t> names;
        for (const auto id : rowMarkers[row])
          if (boundary->HasMarker(id) && id < config.GetnMarker_CfgFile()) names.push_back(id);
        fill.named = !names.empty();
        CFaceHit face;
        CElementHit element;
        if (!names.empty()) {
          face = boundary->Nearest(xp, names);
        } else {
          /*--- Canonical containing element among the imported ones (complete: 5.3), else nearest face. ---*/
          if (adt) {
            su2double xs[3] = {0.0, 0.0, 0.0};
            for (unsigned short iDim = 0; iDim < nDim; ++iDim) xs[iDim] = xp[iDim];
            adt->DetermineContainingElements(xs, candidates, containing);
            for (auto c = 0ul; c < candidates.size(); ++c) {
              CElementHit hit;
              hit.found = true;
              hit.minWeight = std::numeric_limits<passivedouble>::max();
              for (unsigned short k = 0; k < nNode; ++k) {
                hit.weight[k] = SU2_TYPE::GetValue(containing[8 * c + k]);
                hit.minWeight = std::min(hit.minWeight, hit.weight[k]);
                hit.gid[k] = subElems[candidates[c]].gid[k];
              }
              hit.key = subElems[candidates[c]].key;
              if (BetterElement(hit, element)) element = hit;
            }
          }
          if (!element.found) face = boundary->NearestAny(xp);
        }
        uint32_t marker = 0;
        bool haveMarker = false;
        if (element.found) {
          const auto stencil = CDistributedLocator::ElementStencil(element, nDim);
          fill.nPoint = stencil.nPoint;
          for (unsigned short k = 0; k < stencil.nPoint; ++k) {
            fill.gid[k] = stencil.gid[k];
            fill.weight[k] = stencil.weight[k];
          }
        } else {
          fill.nPoint = nDim;
          for (unsigned short k = 0; k < nDim; ++k) {
            fill.gid[k] = boundary->FaceNodeGid(face.face, k);
            fill.weight[k] = face.weight[k];
          }
          /*--- The marker of the stencil, if it is a named one. ---*/
          if (face.marker < config.GetnMarker_CfgFile()) {
            marker = face.marker;
            haveMarker = true;
          }
          const bool tiny = !(missing > kCentroidFraction * pieceVolume);
          if (!tiny) {
            maxFillDistance = std::max(maxFillDistance, face.distance);
            maxFillRelDistance = std::max(maxFillRelDistance, face.distance / face.faceSize);
            if (!(face.distance <= DistanceLimit(face.faceSize))) {
              nBeyond++;
              passivedouble ratio = face.distance / DistanceLimit(face.faceSize);
              if (!std::isfinite(ratio)) ratio = kInf;
              const auto gid = target.nodes->GetGlobalIndex(row);
              if (ratio > worstRatio || (ratio == worstRatio && gid < worstGid)) {
                worstRatio = ratio;
                worstGid = gid;
                std::copy(xp, xp + 3, worstX);
              }
            }
          }
        }
        /*--- The boundary this part lies at: the marker of its stencil, else that of the nearest named face. ---*/
        if (!haveMarker && !namedMarkers.empty()) {
          marker = boundary->Nearest(xp, namedMarkers).marker;
          haveMarker = true;
        }
        fill.open = haveMarker && std::binary_search(openIds.begin(), openIds.end(), marker);
        fills.push_back(fill);
      }
    }
    subValues = nullptr;
    subElem = nullptr;
    subCoord = nullptr;
    pairCoverage = std::vector<unsigned long>();
  }
  localTime[1] = SU2_MPI::Wtime() - start - importTime;
  localTime[3] = importTime;

  /*--- Beyond-limit fill parts: the worst (by ratio, ties by global index) stops the transfer on all ranks. ---*/
  const unsigned long nBeyondGlobal = CPassiveComm::AllreduceSum(nBeyond);
  if (nBeyondGlobal > 0) {
    const double worst = CPassiveComm::Allreduce(static_cast<double>(worstRatio), CPassiveComm::Op::MAX);
    CLocalFailure beyond;
    if (nBeyond > 0 && worstRatio == worst) {
      beyond.Set(1, worstGid,
                 std::to_string(nBeyondGlobal) +
                     " parts of control volumes of the new mesh are farther from the donor "
                     "mesh than accepted (max(face size, 2 ADAP_HAUSD, 1e-3 x domain size)), the farthest at " +
                     PointText(nDim, worstX) + ". The two meshes do not describe the same domain.");
    }
    CollectiveFailure(beyond, CURRENT_FUNCTION);
  }

  /*--- Global counters. ---*/
  unsigned long counts[5] = {nTested, nPairs, nOutside, nDegenerate, localImports}, sums[5], maxima[5];
  CPassiveComm::Allreduce(counts, sums, 5, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(counts, maxima, 5, CPassiveComm::Op::MAX);
  summary.nTested = sums[0];
  summary.nPairs = sums[1];
  summary.nElemOutside = sums[2];
  summary.nDegenerate = sums[3];
  summary.nImported = sums[4];
  summary.maxImported = maxima[4];
  summary.maxPairs = maxima[1];
  double maxima2[3] = {maxUncovered, maxFillDistance, maxFillRelDistance}, global2[3];
  CPassiveComm::Allreduce(maxima2, global2, 3, CPassiveComm::Op::MAX);
  summary.maxUncovered = global2[0];
  summary.maxFillDistance = global2[1];
  summary.maxFillRelDistance = global2[2];
  CAccurateSumBatch batch;
  batch.Add(targetVolumeTerms);
  batch.Reduce();
  summary.targetVolume = batch.Get(0);
  lastSubRounds = CPassiveComm::AllreduceMax(lastSubRounds);
  lastDuplicateNodes = CPassiveComm::AllreduceSum(lastDuplicateNodes);
  /*--- After the groups: the batch of the target volume terms (a copy as doubles, the triples of every rank). ---*/
  TransferMemoryEvent(TransferPhase::IMPORT, false, predicted);
}

size_t CDistributedProjection::SliversBytes(uint64_t nRecord, uint64_t nPair, uint64_t nOwned) const {
  using namespace transfer_memory;
  const size_t nRank = SU2_MPI::GetSize();
  const size_t roundBytes = CPassiveComm::GetRoundBytes();
  const size_t sp = sizeof(passivedouble), sa = sizeof(su2double);
  /*--- Round 1: per-peer counts and offsets, the importer's records (24 B, at most one per coverage record), the
   *    owner's receive (at most one per (owned element, importer) pair), staging. ---*/
  const size_t round1 = Add(Mul(24, nRecord), Mul(24, nPair), Staging(roundBytes, Mul(24, nRecord), Mul(24, nPair)),
                            TransportBytes(nRank), Mul(4 * nRank, sizeof(size_t)));
  /*--- Owner decisions: full and sliver flags, missing parts, the CSR of the partial pairs (start, cursor, hi/lo,
   *    importer). ---*/
  const size_t owner = Add(Mul(nOwned, 2 + sp), Mul(2 * (nOwned + 1), sizeof(size_t)), Mul(nPair, 16 + sizeof(int)));
  /*--- Round 2: requests (8 B per pair), the importer's received requests and answers (8 and 56 B per record), the
   *    owner's moments (56 B per pair, then a CSR of 3 hi/lo pairs per pair with start and cursor), staging. ---*/
  const size_t round2 =
      Add(Add(Mul(8, nPair), Mul(8, nRecord), Mul(56, nRecord), Mul(56, nPair)),
          Mul(2, Staging(roundBytes, Mul(56, nRecord), Mul(56, nPair))), Mul(2, TransportBytes(nRank)),
          Mul(2 * (nOwned + 1), sizeof(size_t)), Mul(48, nPair), Mul(8 * nRank, sizeof(size_t)));
  /*--- Content: terms of every field (all and open, at most one per owned element), sliver volumes, overlap terms
   *    (one per record), the batch (a double copy of the largest quantity with its derivative, the triples of every
   *    rank, received, copied and staged), one nearest-face query (open boundaries). ---*/
  const size_t nQuantity = 2 * nField + 3;
  const size_t batch =
      Add(Mul(2 * std::max(nOwned, nRecord), sizeof(double)), GrowthBound(3 * nQuantity, sizeof(double)),
          GrowthBound(nQuantity, 2 * sizeof(size_t)), Mul(Mul(9 * nQuantity, nRank), sizeof(double)),
          Mul(3 * (nRank + nQuantity), sizeof(double)), TransportBytes(nRank));
  const size_t content = Add(Mul(Mul(2 * nField, nOwned), sa), Mul(2 * nField, sizeof(std::vector<su2double>)),
                             Mul(nOwned + nRecord, sizeof(double)), batch, boundary->QueryBytes());
  return Add(round1, owner, round2, content, 4096);
}

void CDistributedProjection::Slivers(const std::vector<su2double>& localDonor, const std::vector<FillEntry>& fills,
                                     size_t callerBytes) {
  using namespace transfer_memory;
  TransferMemoryEvent(TransferPhase::SLIVERS, true, 0);
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  const auto nOwnedD = ownedD.size();
  const size_t recordSize = sizeof(uint64_t) + 2 * sizeof(double);
  const size_t answerSize = sizeof(uint64_t) + 6 * sizeof(double);

  /*--- Memory ceiling (5.17): the coverage protocol's own arrays on top of the resident data, admitted before any of
   *    them is allocated (owner side: one record per (owned element, importer) pair of the plan). ---*/
  const size_t ceiling = GetTransferMemoryCeiling();
  const size_t predicted =
      Add(GetMemory(), callerBytes, Bytes(fills), SliversBytes(coverage.size(), ownerPairs, nOwnedD));
  {
    CLocalFailure failure;
    if (predicted > ceiling) {
      failure.Set(1, rank,
                  "Strong local coarsening: the target-centric conservative transfer would need " +
                      std::to_string(predicted) + " bytes of owner coverage data on rank " + std::to_string(rank) +
                      ", more than the memory ceiling of the transfer (" + std::to_string(ceiling) + " bytes).");
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
  }
  auto put = [](char*& dst, const void* value, size_t bytes) {
    std::memcpy(dst, value, bytes);
    dst += bytes;
  };

  /*--- Round 1: every importer sends, per imported element with a contribution, FULL or its Neumaier pair (hi, lo)
   *    to the element's owner (also on one rank, as a self message), in the order of the records. ---*/
  std::vector<size_t> recvBytes;
  std::vector<char> received;
  {
    std::vector<size_t> sendBytes(size, 0), offset(size, 0);
    for (const auto& record : coverage)
      if (record.contributed) sendBytes[record.rank] += recordSize;
    for (int t = 1; t < size; ++t) offset[t] = offset[t - 1] + sendBytes[t - 1];
    std::vector<char> send(std::accumulate(sendBytes.begin(), sendBytes.end(), size_t(0)));
    for (const auto& record : coverage) {
      if (!record.contributed) continue;
      const passivedouble c = record.cover.Sum();
      char* dst = send.data() + offset[record.rank];
      const bool isFull = c >= record.volume * (1.0 - DecisionThreshold(1e-10));
      const uint64_t word = isFull ? (record.index | kFullBit) : record.index;
      const double hi = isFull ? 0.0 : record.cover.s, lo = isFull ? 0.0 : record.cover.c;
      put(dst, &word, sizeof(word));
      put(dst, &hi, sizeof(hi));
      put(dst, &lo, sizeof(lo));
      offset[record.rank] += recordSize;
    }
    received = CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);
  }

  /*--- Owner: decision per element; the pairs of the partly covered ones by importer (rank order, CSR). ---*/
  std::vector<char> full(nOwnedD, 0);
  std::vector<size_t> pairStart(nOwnedD + 1, 0);
  auto forEachRecord = [&](auto&& f) {
    const char* src = received.data();
    for (int p = 0; p < size; ++p) {
      const char* end = src + recvBytes[p];
      while (src < end) {
        const auto word = GetBytes<uint64_t>(src);
        const double hi = GetBytes<double>(src), lo = GetBytes<double>(src);
        const auto e = word & ~kFullBit;
        if (e >= nOwnedD) SU2_MPI::Error("Coverage record of an unknown element.", CURRENT_FUNCTION);
        f(p, e, (word & kFullBit) != 0, hi, lo);
      }
    }
  };
  forEachRecord([&](int, uint64_t e, bool isFull, double, double) {
    if (isFull) {
      full[e] = 1;
    } else {
      pairStart[e + 1]++;
    }
  });
  for (auto e = 0ul; e < nOwnedD; ++e) pairStart[e + 1] += pairStart[e];
  std::vector<double> pairValues(2 * pairStart[nOwnedD]);
  std::vector<int> pairRank(pairStart[nOwnedD]);
  {
    std::vector<size_t> cursor(pairStart.begin(), pairStart.end() - 1);
    forEachRecord([&](int p, uint64_t e, bool isFull, double hi, double lo) {
      if (isFull) return;
      const auto pos = cursor[e]++;
      pairValues[2 * pos] = hi;
      pairValues[2 * pos + 1] = lo;
      pairRank[pos] = p;
    });
  }
  received = std::vector<char>();
  std::vector<passivedouble> missing(nOwnedD, 0.0);
  std::vector<char> sliver(nOwnedD, 0);
  CLocalFailure failure;
  auto nonfinite = [&](unsigned long e) {
    uint64_t gids[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) gids[k] = donor.nodes->GetGlobalIndex(ownedD.nodes[e * nNode + k]);
    failure.Set(1, MakeSimplexKey(gids, nNode)[0],
                "The coverage of a donor element by the new mesh is not finite (conservative transfer).");
  };
  for (auto e = 0ul; e < nOwnedD; ++e) {
    const passivedouble volume = volumeD[e];
    if (full[e] || !(volume > 0.0)) continue;
    double covered = 0.0;
    const auto n = pairStart[e + 1] - pairStart[e];
    if (n > 0 && !CAccurateSum::MergePairs(&pairValues[2 * pairStart[e]], n, &covered)) nonfinite(e);
    missing[e] = volume - covered;
    sliver[e] = missing[e] > kGapFraction * volume;
  }
  pairValues = std::vector<double>();
  full = std::vector<char>();

  /*--- Round 2: the moments of the partly covered slivers from their importers. ---*/
  std::vector<size_t> askedBytes;
  std::vector<char> asked;
  {
    std::vector<size_t> sendBytes(size, 0), offset(size, 0);
    for (auto e = 0ul; e < nOwnedD; ++e)
      if (sliver[e])
        for (auto i = pairStart[e]; i < pairStart[e + 1]; ++i) sendBytes[pairRank[i]] += sizeof(uint64_t);
    for (int t = 1; t < size; ++t) offset[t] = offset[t - 1] + sendBytes[t - 1];
    std::vector<char> requests(std::accumulate(sendBytes.begin(), sendBytes.end(), size_t(0)));
    for (auto e = 0ul; e < nOwnedD; ++e) {
      if (!sliver[e]) continue;
      const uint64_t word = e;
      for (auto i = pairStart[e]; i < pairStart[e + 1]; ++i) {
        char* dst = requests.data() + offset[pairRank[i]];
        put(dst, &word, sizeof(word));
        offset[pairRank[i]] += sizeof(uint64_t);
      }
    }
    asked = CPassiveComm::AlltoallvRounds(requests.data(), sendBytes, askedBytes);
  }
  std::vector<size_t> momentBytes;
  std::vector<char> moments;
  {
    std::vector<size_t> sendBytes(size, 0);
    for (int p = 0; p < size; ++p) sendBytes[p] = askedBytes[p] / sizeof(uint64_t) * answerSize;
    std::vector<char> answers(std::accumulate(sendBytes.begin(), sendBytes.end(), size_t(0)));
    const char* src = asked.data();
    char* dst = answers.data();
    for (int p = 0; p < size; ++p) {
      const char* end = src + askedBytes[p];
      while (src < end) {
        const auto e = GetBytes<uint64_t>(src);
        CoverageKey key;
        key.rank = p;
        key.index = e;
        const auto it = std::lower_bound(coverageKeys.begin(), coverageKeys.end(), key);
        if (it == coverageKeys.end() || it->rank != p || it->index != e)
          SU2_MPI::Error("Moment request for an element not imported.", CURRENT_FUNCTION);
        const auto& record = coverage[it->slot];
        put(dst, &e, sizeof(e));
        for (unsigned short iDim = 0; iDim < 3; ++iDim) {
          const double hi = iDim < nDim ? record.moment[iDim].s : 0.0, lo = iDim < nDim ? record.moment[iDim].c : 0.0;
          put(dst, &hi, sizeof(hi));
          put(dst, &lo, sizeof(lo));
        }
      }
    }
    asked = std::vector<char>();
    moments = CPassiveComm::AlltoallvRounds(answers.data(), sendBytes, momentBytes);
  }
  pairStart = std::vector<size_t>();
  pairRank = std::vector<int>();

  /*--- Moments by element (CSR in arrival order: importers in rank order), per dimension the hi/lo pairs. ---*/
  const auto nMoment = moments.size() / answerSize;
  std::vector<size_t> momentStart(nOwnedD + 1, 0);
  for (size_t m = 0; m < nMoment; ++m) {
    uint64_t e = 0;
    std::memcpy(&e, &moments[m * answerSize], sizeof(e));
    if (e >= nOwnedD) SU2_MPI::Error("Moment of an unknown element.", CURRENT_FUNCTION);
    momentStart[e + 1]++;
  }
  for (auto e = 0ul; e < nOwnedD; ++e) momentStart[e + 1] += momentStart[e];
  std::vector<double> momentValues(6 * nMoment);
  {
    std::vector<size_t> cursor(momentStart.begin(), momentStart.end() - 1);
    const char* src = moments.data();
    for (size_t m = 0; m < nMoment; ++m) {
      const auto e = GetBytes<uint64_t>(src);
      const auto pos = cursor[e]++;
      for (unsigned short iDim = 0; iDim < 3; ++iDim) {
        momentValues[2 * (iDim * nMoment + pos)] = GetBytes<double>(src);
        momentValues[2 * (iDim * nMoment + pos) + 1] = GetBytes<double>(src);
      }
    }
  }
  moments = std::vector<char>();

  /*--- Content of every sliver on its owner. ---*/
  sliverA.assign(nField, 0.0);
  sliverOpenA.assign(nField, 0.0);
  const auto nSliverLocal = static_cast<unsigned long>(std::count(sliver.begin(), sliver.end(), 1));
  std::vector<std::vector<su2double>> sliverTerms(nField), sliverOpenTerms(nField);
  for (unsigned short f = 0; f < nField; ++f) {
    sliverTerms[f].reserve(nSliverLocal);
    sliverOpenTerms[f].reserve(nSliverLocal);
  }
  std::vector<double> sliverVolume;
  sliverVolume.reserve(nSliverLocal);
  unsigned long nSliver = 0;
  for (auto e = 0ul; e < nOwnedD; ++e) {
    if (!sliver[e]) continue;
    passivedouble moment[3] = {0.0, 0.0, 0.0};
    const auto n = momentStart[e + 1] - momentStart[e];
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      double m = 0.0;
      if (n > 0 && !CAccurateSum::MergePairs(&momentValues[2 * (iDim * nMoment + momentStart[e])], n, &m)) nonfinite(e);
      moment[iDim] = m;
    }
    passivedouble x[4][3] = {};
    const passivedouble* nodes[4] = {};
    for (unsigned short k = 0; k < nNode; ++k) {
      PointCoord(donor, ownedD.nodes[e * nNode + k], nDim, x[k]);
      nodes[k] = x[k];
    }
    passivedouble xc[3] = {}, mu[4] = {};
    SliverCentroid(nDim, nodes, volumeD[e], missing[e], moment, xc, mu);
    bool open = false;
    if (!namedMarkers.empty()) {
      const auto marker = boundary->Nearest(xc, namedMarkers).marker;
      open = std::binary_search(openIds.begin(), openIds.end(), marker);
    }
    for (unsigned short f = 0; f < nField; ++f) {
      su2double content = 0.0;
      for (unsigned short k = 0; k < nNode; ++k)
        content += missing[e] * mu[k] * localDonor[ownedD.nodes[e * nNode + k] * nField + f];
      sliverTerms[f].push_back(content);
      if (open) sliverOpenTerms[f].push_back(content);
    }
    sliverVolume.push_back(missing[e]);
    nSliver++;
  }
  momentStart = std::vector<size_t>();
  momentValues = std::vector<double>();
  CAccurateSumBatch batch;
  std::vector<size_t> q(nField), qOpen(nField);
  for (unsigned short f = 0; f < nField; ++f) {
    q[f] = batch.AddActive(sliverTerms[f]);
    qOpen[f] = batch.AddActive(sliverOpenTerms[f]);
  }
  const auto qVolume = batch.Add(sliverVolume);
  std::vector<double> overlapTerms;
  overlapTerms.reserve(coverage.size());
  for (const auto& record : coverage) overlapTerms.push_back(record.cover.Sum());
  const auto qOverlap = batch.Add(overlapTerms);
  const auto qDonorVolume = batch.Add(volumeD);
  batch.Reduce();
  for (unsigned short f = 0; f < nField; ++f) {
    sliverA[f] = batch.GetActive(q[f]);
    sliverOpenA[f] = batch.GetActive(qOpen[f]);
  }
  summary.sliverVolume = batch.Get(qVolume);
  summary.overlapVolume = batch.Get(qOverlap);
  summary.donorVolume = batch.Get(qDonorVolume);
  summary.nSliverElems = CPassiveComm::AllreduceSum(nSliver);
  CollectiveFailure(failure, CURRENT_FUNCTION);
  TransferMemoryEvent(TransferPhase::SLIVERS, false, predicted);
}

void CDistributedProjection::Fill(const std::vector<FillEntry>& fills) {
  /*--- Values of the fill stencils from the directory, then the contributions in the order of the pieces. ---*/
  std::vector<uint64_t> gids;
  for (const auto& fill : fills) gids.insert(gids.end(), fill.gid, fill.gid + fill.nPoint);
  std::sort(gids.begin(), gids.end());
  gids.erase(std::unique(gids.begin(), gids.end()), gids.end());
  const auto fetched = directory.Fetch(gids);
  std::vector<su2double> values(gids.size() * nField);
  for (auto i = 0ul; i < gids.size(); ++i) UnpackFieldValues(&fetched[i * recordBytes], &values[i * nField], nField);

  std::vector<std::vector<su2double>> fillTerms(nField), openTerms(nField);
  std::vector<double> fillVolume;
  std::vector<char> filled(nRow, 0);
  unsigned long nInterior = 0;
  for (const auto& fill : fills) {
    unsigned long index[4] = {};
    for (unsigned short k = 0; k < fill.nPoint; ++k)
      index[k] = std::lower_bound(gids.begin(), gids.end(), fill.gid[k]) - gids.begin();
    for (unsigned short f = 0; f < nField; ++f) {
      su2double value = 0.0;
      for (unsigned short k = 0; k < fill.nPoint; ++k) value += fill.weight[k] * values[index[k] * nField + f];
      rhs[fill.row * nField + f] += fill.missing * value;
      fillTerms[f].push_back(fill.missing * value);
      if (fill.open) openTerms[f].push_back(fill.missing * value);
      for (unsigned short k = 0; k < fill.nPoint; ++k) {
        const su2double& v = values[index[k] * nField + f];
        su2double& lo = lower[fill.row * nField + f];
        su2double& hi = upper[fill.row * nField + f];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
      }
    }
    fillVolume.push_back(fill.missing);
    filled[fill.row] = 1;
    nInterior += !fill.named;
  }
  fillA.assign(nField, 0.0);
  fillOpenA.assign(nField, 0.0);
  CAccurateSumBatch batch;
  std::vector<size_t> q(nField), qOpen(nField);
  for (unsigned short f = 0; f < nField; ++f) {
    q[f] = batch.AddActive(fillTerms[f]);
    qOpen[f] = batch.AddActive(openTerms[f]);
  }
  const auto qVolume = batch.Add(fillVolume);
  batch.Reduce();
  for (unsigned short f = 0; f < nField; ++f) {
    fillA[f] = batch.GetActive(q[f]);
    fillOpenA[f] = batch.GetActive(qOpen[f]);
  }
  summary.fillVolume = batch.Get(qVolume);
  unsigned long counts[3] = {fills.size(), nInterior, static_cast<unsigned long>(std::count(filled.begin(), filled.end(), 1))},
                global[3];
  CPassiveComm::Allreduce(counts, global, 3, CPassiveComm::Op::SUM);
  summary.nFillPieces = global[0];
  summary.nInteriorFill = global[1];
  summary.nFillNodes = global[2];
}

void CDistributedProjection::Totals(const std::vector<su2double>& donorValues) {
  const auto nOwnedD = donor.GetnPointDomain();
  summary.fill.assign(nField, 0.0);
  summary.sliver.assign(nField, 0.0);
  summary.fillOpen.assign(nField, 0.0);
  summary.sliverOpen.assign(nField, 0.0);
  summary.donorTotal.assign(nField, 0.0);
  summary.targetTotal.assign(nField, 0.0);
  summary.scale.assign(nField, 0.0);
  summary.correction.assign(nField, 0.0);
  summary.supermeshDefect.assign(nField, 0.0);
  range.assign(nField, 0.0);
  targetTotalA.assign(nField, 0.0);

  /*--- Accurate sums (one batch): volumes, donor totals, scales, right-hand side totals; ranges exact (MIN/MAX). ---*/
  CAccurateSumBatch batch;
  const auto qRow = batch.Add(mass.rowVolume);
  const auto qTarget = batch.Add(cvT);
  const auto qDonor = batch.Add(cvD);
  std::vector<size_t> qTotal(nField), qScale(nField), qRhs(nField);
  std::vector<su2double> terms;
  std::vector<double> absTerms;
  std::vector<double> lo(nField, kInf), hi(nField, -kInf);
  for (unsigned short f = 0; f < nField; ++f) {
    terms.resize(nOwnedD);
    absTerms.resize(nOwnedD);
    for (auto iPoint = 0ul; iPoint < nOwnedD; ++iPoint) {
      const su2double& value = donorValues[iPoint * nField + f];
      terms[iPoint] = value * cvD[iPoint];
      absTerms[iPoint] = std::fabs(SU2_TYPE::GetValue(value)) * cvD[iPoint];
      lo[f] = std::min(lo[f], SU2_TYPE::GetValue(value));
      hi[f] = std::max(hi[f], SU2_TYPE::GetValue(value));
    }
    qTotal[f] = batch.AddActive(terms);
    qScale[f] = batch.Add(absTerms);
    terms.resize(nRow);
    for (auto iRow = 0ul; iRow < nRow; ++iRow) terms[iRow] = rhs[iRow * nField + f];
    qRhs[f] = batch.AddActive(terms);
  }
  batch.Reduce();
  std::vector<double> globalLo(nField), globalHi(nField);
  CPassiveComm::Allreduce(lo.data(), globalLo.data(), nField, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(hi.data(), globalHi.data(), nField, CPassiveComm::Op::MAX);
  const passivedouble sumRowVolume = batch.Get(qRow);
  summary.targetCV = batch.Get(qTarget);
  summary.donorCV = batch.Get(qDonor);
  CLocalFailure failure;
  for (unsigned short f = 0; f < nField; ++f) {
    if (!batch.FiniteActive(qTotal[f]) || !batch.FiniteActive(qRhs[f])) {
      const auto bad = std::min(batch.LocalFirstBad(qTotal[f]), nOwnedD);
      const uint64_t gid = bad < nOwnedD ? donor.nodes->GetGlobalIndex(bad) : UINT64_MAX;
      failure.Set(1, gid,
                  "The totals of field " + std::to_string(f) + " of the conservative transfer are not finite" +
                      (bad < nOwnedD ? " (donor point with global index " + std::to_string(gid) + ")." : "."));
    }
    summary.fill[f] = SU2_TYPE::GetValue(fillA[f]);
    summary.sliver[f] = SU2_TYPE::GetValue(sliverA[f]);
    summary.fillOpen[f] = SU2_TYPE::GetValue(fillOpenA[f]);
    summary.sliverOpen[f] = SU2_TYPE::GetValue(sliverOpenA[f]);
    const su2double donorTotal = batch.GetActive(qTotal[f]);
    const su2double rhsTotal = batch.GetActive(qRhs[f]);
    summary.scale[f] = batch.Get(qScale[f]);
    range[f] = globalHi[f] - globalLo[f];
    targetTotalA[f] = donorTotal;
    if (options.sliverRule == CConservativeProjection::SliverRule::NONE) targetTotalA[f] += fillA[f] - sliverA[f];
    if (options.sliverRule == CConservativeProjection::SliverRule::CLOSED)
      targetTotalA[f] += fillOpenA[f] - sliverOpenA[f];
    const su2double correction = targetTotalA[f] - rhsTotal;
    summary.donorTotal[f] = SU2_TYPE::GetValue(donorTotal);
    summary.targetTotal[f] = SU2_TYPE::GetValue(targetTotalA[f]);
    summary.correction[f] = SU2_TYPE::GetValue(correction);
    passivedouble expected = 0.0;
    switch (options.sliverRule) {
      case CConservativeProjection::SliverRule::GLOBAL:
        expected = summary.sliver[f] - summary.fill[f];
        break;
      case CConservativeProjection::SliverRule::CLOSED:
        expected = (summary.sliver[f] - summary.sliverOpen[f]) - (summary.fill[f] - summary.fillOpen[f]);
        break;
      default:
        expected = 0.0;
        break;
    }
    const passivedouble scale = std::max(summary.scale[f], passivedouble(1e-300));
    summary.supermeshDefect[f] = (summary.correction[f] - expected) / scale;
    for (auto iRow = 0ul; iRow < nRow; ++iRow) rhs[iRow * nField + f] += correction * mass.rowVolume[iRow] / sumRowVolume;
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

void CDistributedProjection::SetBounds(unsigned short iField, std::vector<su2double>& lo,
                                       std::vector<su2double>& hi) const {
  const su2double widen = options.limiterTolerance * range[iField];
  lo.resize(nRow);
  hi.resize(nRow);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) {
    lo[iRow] = options.limiter ? su2double(lower[iRow * nField + iField] - widen) : su2double(-kInf);
    hi[iRow] = options.limiter ? su2double(upper[iRow * nField + iField] + widen) : su2double(kInf);
    if (std::isinf(SU2_TYPE::GetValue(lo[iRow])) || std::isinf(SU2_TYPE::GetValue(hi[iRow]))) {
      lo[iRow] = -kInf;
      hi[iRow] = kInf;
    }
  }
}

passivedouble CDistributedProjection::CountTolerance(unsigned short iField) const {
  const passivedouble typical = summary.scale[iField] / std::max(summary.donorCV, passivedouble(1e-300));
  return DiagnosticTol(1e-12) * std::max({SU2_TYPE::GetValue(range[iField]), typical, passivedouble(1e-300)});
}

void CDistributedProjection::Solve(std::vector<su2double>& newValues) {
  newValues.assign(nRow * nField, 0.0);
  summary.iterations.assign(nField, 0);
  summary.residual.assign(nField, 0.0);
  summary.nLimited.assign(nField, 0);
  summary.infeasible.assign(nField, false);
  summary.nViolations.assign(nField, 0);
  summary.maxViolation.assign(nField, 0.0);
  summary.newTotal.assign(nField, 0.0);
  const MassSolver solver(mass, &target, options.solverTolerance, options.maxSolverIter);
  std::vector<su2double> b(nRow), x(nRow), lo, hi;
  passivedouble solveTime = 0.0, limiterTime = 0.0;
  for (unsigned short f = 0; f < nField; ++f) {
    auto start = SU2_MPI::Wtime();
    for (auto iRow = 0ul; iRow < nRow; ++iRow) b[iRow] = rhs[iRow * nField + f];
    const auto solve = solver.SolveActive(b, x);
    summary.iterations[f] = solve.iterations;
    summary.residual[f] = solve.trueResidual;
    summary.nSolveWarnings += solve.warning;
    if (solve.failed) {
      /*--- The flags are reduced values: every rank fails here. ---*/
      SU2_MPI::Error("The mass-matrix solve of field " + std::to_string(f) + " failed (" +
                         (solve.nonfiniteRHS ? std::string("nonfinite right-hand side")
                                             : solve.breakdown ? std::string("breakdown")
                                                               : "true residual " + std::to_string(solve.trueResidual)) +
                         ", " + std::to_string(solve.iterations) + " iterations).",
                     CURRENT_FUNCTION);
    }
    solveTime += SU2_MPI::Wtime() - start;
    start = SU2_MPI::Wtime();
    SetBounds(f, lo, hi);
    const auto limited = BoundedRedistribute(x, cvT, targetTotalA[f], lo, hi, nullptr, CountTolerance(f),
                                             SU2_TYPE::GetValue(range[f]), true);
    if (limited.error) {
      SU2_MPI::Error("The limiter of field " + std::to_string(f) + ": " + limited.reason + ".", CURRENT_FUNCTION);
    }
    summary.nLimited[f] = limited.nClipped;
    summary.infeasible[f] = limited.relaxed;
    summary.nViolations[f] = limited.nViolations;
    summary.maxViolation[f] = limited.maxViolation;
    for (auto iRow = 0ul; iRow < nRow; ++iRow) newValues[iRow * nField + f] = x[iRow];
    summary.newTotal[f] = SU2_TYPE::GetValue(NewTotal(f, newValues));
    limiterTime += SU2_MPI::Wtime() - start;
  }
  double times[6] = {localTime[0], localTime[1], localTime[2], localTime[3], solveTime, limiterTime}, maxTimes[6];
  CPassiveComm::Allreduce(times, maxTimes, 6, CPassiveComm::Op::MAX);
  summary.timeSupermesh = maxTimes[1];
  summary.timeCoverage = maxTimes[2];
  summary.timeImport = maxTimes[0] + maxTimes[3];
  summary.timeSolve = maxTimes[4];
  summary.timeLimiter = maxTimes[5];
}

su2double CDistributedProjection::NewTotal(unsigned short iField, const std::vector<su2double>& values) const {
  std::vector<su2double> terms(nRow);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) terms[iRow] = values[iRow * nField + iField] * cvT[iRow];
  CAccurateSumBatch batch;
  batch.AddActive(terms);
  batch.Reduce();
  return batch.GetActive(0);
}

bool CDistributedProjection::Redistribute(unsigned short iField, std::vector<su2double>& newValues,
                                          const std::vector<bool>& frozen) const {
  std::vector<su2double> v(nRow), lo, hi;
  SetBounds(iField, lo, hi);
  for (auto iRow = 0ul; iRow < nRow; ++iRow) {
    v[iRow] = newValues[iRow * nField + iField];
    if (v[iRow] < lo[iRow]) lo[iRow] = v[iRow];
    if (v[iRow] > hi[iRow]) hi[iRow] = v[iRow];
  }
  const auto result = BoundedRedistribute(v, cvT, targetTotalA[iField], lo, hi, &frozen, CountTolerance(iField),
                                          SU2_TYPE::GetValue(range[iField]), true);
  if (result.error) {
    SU2_MPI::Error("The redistribution of field " + std::to_string(iField) + ": " + result.reason + ".",
                   CURRENT_FUNCTION);
  }
  for (auto iRow = 0ul; iRow < nRow; ++iRow) newValues[iRow * nField + iField] = v[iRow];
  return !result.relaxed;
}
