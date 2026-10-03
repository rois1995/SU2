/*!
 * \file CDistributedProjection.hpp
 * \brief Distributed conservative P1 projection (MPI_TRANSFER_PLAN.md, milestone M2): target-centric import of donor
 *        elements, local supermesh with exhaustive enumeration, coverage protocol, distributed guarded CG and limiter.
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "CConservativeTransfer.hpp"
#include "CDistributedLocator.hpp"
#include "../../../Common/include/adaptation/CAccurateSum.hpp"
#include "ConservativeKernels.hpp"

class CConfig;
class CGeometry;

/*!
 * \class CDistributedProjection
 * \brief The conservative P1 projection of CConservativeProjection on partitioned meshes, every rank with its owned
 *        rows of the new mesh (MPI_TRANSFER_PLAN.md 5.2-5.9, 5.17).
 * \note - Region boxes of each rank (recursive bisection of its local target elements with an owned vertex, union of
 *         their bounding boxes), replicated (CRankBoxTree). Every owned donor element whose inflated box (the ADT's)
 *         intersects a region box of rank t is sent to t with the coordinates and fields of its nodes (the donor
 *         fields are halo-exchanged first), in import groups that keep each rank's planned import under the memory
 *         ceiling (one group by default; a single region box over the ceiling is a collective error).
 *       - Each rank clips its local target elements with an owned vertex, in key order, against every imported element
 *         whose inflated box intersects the element's box (exhaustive enumeration, the serial kernels), in key order,
 *         and accumulates only the dual pieces of its owned vertices: complete owned rows, no reverse communication.
 *       - S_n fill: canonical stencils (same-name canonical nearest face on the replicated donor boundary, else
 *         collect-all containment in the imported elements, else the canonical nearest face of any marker), values from
 *         the donor directory.
 *       - S_d: the importers send their partial coverage of each imported element (Neumaier pair, or FULL) to its owner,
 *         which decides the slivers and asks for the moments of the partly covered ones; the owner adds the content.
 *       - Totals by accurate sums, distributed guarded CG (conservative::MassSolver), limiter with global
 *         redistribution (conservative::BoundedRedistribute).
 *       All collective calls are made by every rank, also without rows or imports.
 */
class CDistributedProjection {
 public:
  using Options = CConservativeProjection::Options;
  using Summary = CConservativeProjection::Summary;

  /*!
   * \brief Collective: the geometry-dependent structures (ownership, region boxes, mass matrix, donor boundary).
   * \param[in] donorTags - Marker names of the donor by iMarker.
   * \param[in] targetTags - Marker names of the new mesh by iMarker.
   */
  CDistributedProjection(const CGeometry& donor, const std::vector<std::string>& donorTags, const CGeometry& target,
                         const std::vector<std::string>& targetTags, const CConfig& config, const Options& options);
  ~CDistributedProjection();

  /*!
   * \brief Collective: project the fields.
   * \param[in] donorValues - Values of the owned donor points (nPointDomain x nField).
   * \param[out] newValues - Values of the owned points of the new mesh (nPointDomain x nField), limited, exact totals.
   */
  void Project(unsigned short nField, const std::vector<su2double>& donorValues, std::vector<su2double>& newValues);

  /*!
   * \brief Collective: restore the target total of a field after values were changed (frozen rows kept), within the
   *        limiter bounds (values outside them are not pulled back). False if the bounds were relaxed.
   */
  bool Redistribute(unsigned short iField, std::vector<su2double>& newValues, const std::vector<bool>& frozen) const;

  /*! \brief Collective: total of a field on the new mesh (accurate sum of value x control volume). */
  su2double NewTotal(unsigned short iField, const std::vector<su2double>& newValues) const;

  const Summary& GetSummary() const { return summary; }

  /*! \brief Imported donor elements of this rank (all groups) and clipped pairs, for the tests. */
  unsigned long GetLocalImports() const { return localImports; }

 private:
  struct TargetElem {
    unsigned long elem = 0;      /*!< \brief Local element index. */
    CSimplexKey key = {};
    unsigned long nodes[4] = {}; /*!< \brief Local point indices (element order). */
    bool owner = false;          /*!< \brief This rank owns the element (counters). */
    unsigned long group = 0;     /*!< \brief Region box (then import group). */
    passivedouble volume = 0.0;
  };
  struct FillEntry {
    unsigned long row = 0;
    passivedouble missing = 0.0;
    unsigned short nPoint = 0;
    uint64_t gid[4] = {};
    passivedouble weight[4] = {};
    bool open = false, named = false;
  };
  struct Coverage {
    int rank = 0;
    uint64_t index = 0;          /*!< \brief Owner-local index of the donor element. */
    passivedouble volume = 0.0;
    bool contributed = false;
    CNeumaierSum cover, moment[3];
  };

  void Supermesh(std::vector<FillEntry>& fills);
  void Slivers(const std::vector<su2double>& localDonor);
  void Fill(const std::vector<FillEntry>& fills);
  void Totals(const std::vector<su2double>& donorValues);
  void Solve(std::vector<su2double>& newValues);
  void SetBounds(unsigned short iField, std::vector<su2double>& lo, std::vector<su2double>& hi) const;
  passivedouble CountTolerance(unsigned short iField) const;
  passivedouble DistanceLimit(passivedouble faceSize) const;
  static void AccumulatePiece(void* context, unsigned short i, passivedouble volume, const passivedouble* centroid,
                              const passivedouble* mu);

  const CGeometry& donor;
  const CGeometry& target;
  const CConfig& config;
  Options options;
  unsigned short nDim = 0, nNode = 0, nField = 0;
  size_t recordBytes = 0;

  /*--- Donor. ---*/
  COwnedSimplices ownedD;
  std::vector<passivedouble> volumeD;   /*!< \brief Measure of each owned donor element. */
  std::vector<double> inflatedD;        /*!< \brief Inflated box of each owned donor element. */
  std::unique_ptr<CCanonicalBoundary> boundary;
  std::vector<uint32_t> namedMarkers;   /*!< \brief Donor marker ids with faces and a name. */
  std::vector<uint32_t> openIds;        /*!< \brief Config ids of the open markers. */
  passivedouble domainSize = 0.0;
  std::vector<char> localRecords;       /*!< \brief Packed fields of the local donor points (halos exchanged). */
  CPointDirectory directory;

  /*--- Target. ---*/
  unsigned long nRow = 0;               /*!< \brief Owned points of the new mesh. */
  std::vector<TargetElem> elems;        /*!< \brief Local elements with an owned vertex, by key. */
  std::vector<passivedouble> cvT, cvD;
  std::vector<std::vector<uint32_t>> rowMarkers; /*!< \brief Marker ids of each owned point. */
  conservative::MassMatrix mass;
  CRankBoxTree regions;
  std::vector<double> regionBoxes;
  unsigned long nGroup = 1;

  /*--- State of a projection. ---*/
  const std::vector<su2double>* subValues = nullptr; /*!< \brief Values of the sub-mesh nodes (AccumulatePiece). */
  const std::vector<unsigned long>* subElem = nullptr;
  const std::vector<passivedouble>* subCoord = nullptr;
  unsigned long pairT = 0, pairE = 0;
  std::vector<Coverage> coverage;
  std::vector<unsigned long> pairCoverage;           /*!< \brief Coverage record of each sub element. */
  std::vector<su2double> rhs, lower, upper, range;
  std::vector<passivedouble> covT, momT;
  std::vector<su2double> fillA, fillOpenA, sliverA, sliverOpenA, targetTotalA;
  unsigned long localImports = 0;
  passivedouble localTime[4] = {};

  Summary summary;
};
