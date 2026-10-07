/*!
 * \file computeHessiansQuadratic.hpp
 * \brief Direct quadratic reconstruction for primal adaptation sensors.
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

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <unordered_map>
#include <vector>
#include "Eigen/Dense"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"

/*!
 * \brief Fit sensor differences directly using SVD coordinate whitening and pivoted QR.
 * \note Primal only. One-sided wall stencils are allowed; periodic/symmetry markers are rejected by CConfig.
 *       Input sensors must be communicated first. Failed fits retain the caller's WLS derivatives.
 *       Complete donor neighborhoods are exchanged, so two-ring stencils do not depend on partition boundaries.
 *       Call outside OpenMP regions; the caller communicates the resulting gradients and Hessians.
 */
template <class FieldType, class GradientType, class HessianType>
void computeHessiansQuadratic(CGeometry& geometry, unsigned short nSensor, const FieldType& field,
                              GradientType& gradient, HessianType& hessian) {
  const unsigned short dim = geometry.GetnDim(), terms = dim + dim * (dim + 1) / 2;
  struct Sample {
    unsigned long center, id;
    passivedouble coord[3];
  };
  auto sample = [&](unsigned long point, unsigned long center) {
    Sample result{};
    result.center = center;
    result.id = geometry.nodes->GetGlobalIndex(point);
    for (unsigned short d = 0; d < dim; ++d) result.coord[d] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(point, d));
    return result;
  };

  /*--- Exchange complete neighborhoods of exported owners, not the truncated connectivity of halo nodes.
   *    Use the existing bounded passive transport; global IDs remain integer values. ---*/
  const auto ranks = SU2_MPI::GetSize();
  std::vector<std::vector<Sample>> byRank(ranks);
  std::vector<std::vector<passivedouble>> valuesByRank(ranks);
  for (int message = 0; message < geometry.nP2PSend; ++message) {
    const auto peer = geometry.Neighbors_P2PSend[message];
    for (int offset = geometry.nPoint_P2PSend[message]; offset < geometry.nPoint_P2PSend[message + 1]; ++offset) {
      const auto point = geometry.Local_Point_P2PSend[offset];
      const auto center = geometry.nodes->GetGlobalIndex(point);
      auto append = [&](unsigned long neighbor) {
        byRank[peer].push_back(sample(neighbor, center));
        for (unsigned short v = 0; v < nSensor; ++v)
          valuesByRank[peer].push_back(SU2_TYPE::GetValue(field(neighbor, v)));
      };
      append(point);
      for (const auto neighbor : geometry.nodes->GetPoints(point)) append(neighbor);
    }
  }
  std::vector<Sample> send;
  std::vector<passivedouble> sendValues;
  std::vector<size_t> counts(ranks), valueCounts(ranks), receivedCounts;
  for (int peer = 0; peer < ranks; ++peer) {
    counts[peer] = byRank[peer].size();
    valueCounts[peer] = valuesByRank[peer].size();
    send.insert(send.end(), byRank[peer].begin(), byRank[peer].end());
    sendValues.insert(sendValues.end(), valuesByRank[peer].begin(), valuesByRank[peer].end());
  }
  const auto received = CPassiveComm::Alltoallv(send, counts, receivedCounts);
  const auto receivedValues = CPassiveComm::Alltoallv(sendValues, valueCounts, receivedCounts);
  std::unordered_map<unsigned long, std::vector<size_t>> neighborhoods;
  for (size_t k = 0; k < received.size(); ++k) neighborhoods[received[k].center].push_back(k);

  unsigned long local[3] = {}, global[3] = {};  // first-ring fits, grown fits, fallback point-sensor pairs
  for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
    struct Entry {
      Sample sample;
      std::vector<passivedouble> values;
    };
    std::map<unsigned long, Entry> cloud;  // deterministic QR row order across partitions
    const auto center = sample(point, 0);
    auto appendLocal = [&](unsigned long neighbor) {
      const auto item = sample(neighbor, center.id);
      if (item.id == center.id || cloud.count(item.id)) return;
      Entry entry{item, std::vector<passivedouble>(nSensor)};
      for (unsigned short v = 0; v < nSensor; ++v) entry.values[v] = SU2_TYPE::GetValue(field(neighbor, v));
      cloud.emplace(item.id, std::move(entry));
    };
    for (const auto neighbor : geometry.nodes->GetPoints(point)) appendLocal(neighbor);
    std::vector<bool> accepted(nSensor, false);
    // ponytail: grow at most two rings; extend only if reported fallback counts justify the extra communication.
    for (unsigned short ring = 1; ring <= 2; ++ring) {
      if (ring == 2) {
        for (const auto neighbor : geometry.nodes->GetPoints(point)) {
          if (neighbor < geometry.GetnPointDomain()) {
            for (const auto other : geometry.nodes->GetPoints(neighbor)) appendLocal(other);
          } else {
            const auto donor = neighborhoods.find(geometry.nodes->GetGlobalIndex(neighbor));
            if (donor == neighborhoods.end()) continue;
            for (const auto k : donor->second) {
              const auto& item = received[k];
              if (item.id == center.id || cloud.count(item.id)) continue;
              const auto begin = receivedValues.begin() + k * nSensor;
              cloud.emplace(item.id, Entry{item, std::vector<passivedouble>(begin, begin + nSensor)});
            }
          }
        }
      }
      if (cloud.size() < terms) continue;
      Eigen::MatrixXd displacement(cloud.size(), dim), rhs(cloud.size(), nSensor);
      size_t row = 0;
      for (const auto& item : cloud) {
        for (unsigned short d = 0; d < dim; ++d) displacement(row, d) = item.second.sample.coord[d] - center.coord[d];
        for (unsigned short v = 0; v < nSensor; ++v)
          rhs(row, v) = item.second.values[v] - SU2_TYPE::GetValue(field(point, v));
        ++row;
      }
      if (!displacement.allFinite()) continue;
      Eigen::JacobiSVD<Eigen::MatrixXd> svd(displacement, Eigen::ComputeThinV);
      const auto& singular = svd.singularValues();
      if (!(singular(dim - 1) > 64 * std::numeric_limits<passivedouble>::epsilon() * singular(0))) continue;
      const Eigen::MatrixXd whitening =
          svd.matrixV() * (sqrt(passivedouble(cloud.size())) * singular.cwiseInverse()).asDiagonal();
      const Eigen::MatrixXd coordinate = displacement * whitening;
      Eigen::MatrixXd design(cloud.size(), terms);
      for (Eigen::Index k = 0; k < coordinate.rows(); ++k) {
        const auto weight = 1.0 / std::max(coordinate.row(k).norm(), passivedouble(1e-12));
        design.row(k).head(dim) = coordinate.row(k);
        unsigned short column = dim;
        for (unsigned short i = 0; i < dim; ++i)
          for (unsigned short j = i; j < dim; ++j)
            design(k, column++) = coordinate(k, i) * coordinate(k, j) * (i == j ? 0.5 : 1.0);
        design.row(k) *= weight;
        rhs.row(k) *= weight;
      }
      Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(design);
      qr.setThreshold(1e-10);
      if (qr.rank() != terms) continue;
      for (unsigned short v = 0; v < nSensor; ++v) {
        if (accepted[v] || !rhs.col(v).allFinite()) continue;
        const Eigen::VectorXd coefficient = qr.solve(rhs.col(v));
        Eigen::MatrixXd scaledHessian(dim, dim);
        unsigned short column = dim;
        for (unsigned short i = 0; i < dim; ++i)
          for (unsigned short j = i; j < dim; ++j) scaledHessian(i, j) = scaledHessian(j, i) = coefficient(column++);
        /*--- An affine sensor should not acquire curvature from subtraction/solve roundoff. Estimate the
         *    input resolution in the scaled fit, amplified by the smallest QR pivot. This does not remove
         *    resolved CFD noise or filter shocks; it only discards curvature below floating-point resolution. ---*/
        passivedouble amplitude = fabs(SU2_TYPE::GetValue(field(point, v)));
        for (const auto& item : cloud) amplitude = std::max(amplitude, fabs(item.second.values[v]));
        const auto pivot = qr.matrixR().topLeftCorner(terms, terms).diagonal().cwiseAbs().minCoeff();
        const auto resolution = 64 * std::numeric_limits<passivedouble>::epsilon() * amplitude / pivot;
        if (scaledHessian.cwiseAbs().maxCoeff() <= resolution) scaledHessian.setZero();
        const Eigen::VectorXd physicalGradient = whitening * coefficient.head(dim);
        const Eigen::MatrixXd physicalHessian = whitening * scaledHessian * whitening.transpose();
        if (!physicalGradient.allFinite() || !physicalHessian.allFinite()) continue;
        for (unsigned short i = 0; i < dim; ++i) gradient(point, v, i) = physicalGradient(i);
        column = 0;
        for (unsigned short i = 0; i < dim; ++i)
          for (unsigned short j = i; j < dim; ++j) hessian(point, v, column++) = physicalHessian(i, j);
        accepted[v] = true;
        ++local[ring - 1];
      }
      if (std::all_of(accepted.begin(), accepted.end(), [](bool value) { return value; })) break;
    }
    local[2] += std::count(accepted.begin(), accepted.end(), false);
  }
  CPassiveComm::Allreduce(local, global, 3, CPassiveComm::Op::SUM);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::cout << "Quadratic Hessian fits (owned point-sensor pairs): " << global[0] << " first ring, " << global[1]
              << " grown stencil, " << global[2] << " WLS fallbacks." << std::endl;
    if (global[2])
      std::cout << "WARNING: Quadratic Hessian reconstruction retained WLS for failed fits. "
                << "Check non-finite sensor values and stencil rank." << std::endl;
  }
}
