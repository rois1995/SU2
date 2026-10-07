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

  /*--- Request complete owner neighborhoods only for points needing a second ring. Halo connectivity
   *    is truncated; exporting it locally would make reconstruction depend on the partition. ---*/
  const auto ranks = SU2_MPI::GetSize();
  std::vector<int> owner(geometry.GetnPoint(), -1);
  for (int message = 0; message < geometry.nP2PRecv; ++message)
    for (int k = geometry.nPoint_P2PRecv[message]; k < geometry.nPoint_P2PRecv[message + 1]; ++k)
      owner[geometry.Local_Point_P2PRecv[k]] = geometry.Neighbors_P2PRecv[message];
  std::vector<Sample> received;
  std::vector<passivedouble> receivedValues;
  std::unordered_map<unsigned long, std::vector<size_t>> neighborhoods;
  std::vector<bool> redundant(geometry.GetnPointDomain(), false);
  std::vector<bool> grow(geometry.GetnPointDomain(), false), valid(geometry.GetnPointDomain() * nSensor, false);
  std::vector<passivedouble> residual(valid.size(), std::numeric_limits<passivedouble>::infinity());
  std::vector<passivedouble> fitCondition(valid.size(), 0);
  std::vector<size_t> fitDof(valid.size(), 0);
  unsigned long requested = 0, exported = 0;
  auto exchange = [&]() {
    std::vector<std::vector<unsigned long>> request(ranks);
    for (auto point = 0ul; point < grow.size(); ++point) {
      if (!grow[point]) continue;
      for (const auto neighbor : geometry.nodes->GetPoints(point)) {
        if (neighbor < geometry.GetnPointDomain()) continue;
        if (owner[neighbor] < 0) SU2_MPI::Error("Quadratic Hessian halo has no owner.", CURRENT_FUNCTION);
        request[owner[neighbor]].push_back(geometry.nodes->GetGlobalIndex(neighbor));
      }
    }
    std::vector<size_t> counts(ranks), incomingCounts;
    std::vector<unsigned long> ids;
    for (int peer = 0; peer < ranks; ++peer) {
      auto& list = request[peer];
      std::sort(list.begin(), list.end());
      list.erase(std::unique(list.begin(), list.end()), list.end());
      counts[peer] = list.size();
      ids.insert(ids.end(), list.begin(), list.end());
    }
    requested = ids.size();
    const auto incoming = CPassiveComm::Alltoallv(ids, counts, incomingCounts);
    std::unordered_map<unsigned long, unsigned long> owned;
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point)
      owned.emplace(geometry.nodes->GetGlobalIndex(point), point);
    std::vector<Sample> send;
    std::vector<passivedouble> values;
    std::vector<size_t> valueCounts(ranks);
    size_t offset = 0;
    for (int peer = 0; peer < ranks; ++peer) {
      const auto before = send.size();
      for (size_t k = 0; k < incomingCounts[peer]; ++k) {
        const auto center = incoming[offset++];
        const auto found = owned.find(center);
        if (found == owned.end()) SU2_MPI::Error("Quadratic Hessian requested a non-owned point.", CURRENT_FUNCTION);
        auto append = [&](unsigned long point) {
          send.push_back(sample(point, center));
          for (unsigned short v = 0; v < nSensor; ++v) values.push_back(SU2_TYPE::GetValue(field(point, v)));
        };
        append(found->second);
        for (const auto neighbor : geometry.nodes->GetPoints(found->second)) append(neighbor);
      }
      counts[peer] = send.size() - before;
      valueCounts[peer] = counts[peer] * nSensor;
    }
    exported = send.size();
    received = CPassiveComm::Alltoallv(send, counts, incomingCounts);
    receivedValues = CPassiveComm::Alltoallv(values, valueCounts, incomingCounts);
    for (size_t k = 0; k < received.size(); ++k) neighborhoods[received[k].center].push_back(k);
  };

  unsigned long local[3] = {}, global[3] = {};  // first-ring fits, grown fits, fallback point-sensor pairs
  constexpr passivedouble poorFit =
      0.05;  // Relative weighted fit residual; triggers stencil comparison, not smoothing.
  for (unsigned short ring = 1; ring <= 2; ++ring) {
    if (ring == 2) {
      unsigned long pending = std::count(grow.begin(), grow.end(), true), globalPending = 0;
      CPassiveComm::Allreduce(&pending, &globalPending, 1, CPassiveComm::Op::SUM);
      if (!globalPending) break;
      exchange();
    }
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
      if (ring == 2 && !grow[point]) continue;
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
      // ponytail: compare at most two rings; extend only if failed-fit statistics justify more communication.
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
      if (ring == 1) {
        redundant[point] = cloud.size() >= terms + 2;
        grow[point] = !redundant[point];  // An exactly determined fit has no residual degrees of freedom.
      }
      if (cloud.size() < terms) {
        if (ring == 1) grow[point] = true;
        continue;
      }
      Eigen::MatrixXd displacement(cloud.size(), dim), rhs(cloud.size(), nSensor);
      size_t row = 0;
      for (const auto& item : cloud) {
        for (unsigned short d = 0; d < dim; ++d) displacement(row, d) = item.second.sample.coord[d] - center.coord[d];
        for (unsigned short v = 0; v < nSensor; ++v)
          rhs(row, v) = item.second.values[v] - SU2_TYPE::GetValue(field(point, v));
        ++row;
      }
      if (!displacement.allFinite()) {
        if (ring == 1) grow[point] = true;
        continue;
      }
      Eigen::JacobiSVD<Eigen::MatrixXd> svd(displacement, Eigen::ComputeThinV);
      const auto& singular = svd.singularValues();
      if (!(singular(dim - 1) > 64 * std::numeric_limits<passivedouble>::epsilon() * singular(0))) {
        if (ring == 1) grow[point] = true;
        continue;
      }
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
      if (qr.rank() != terms) {
        if (ring == 1) grow[point] = true;
        continue;
      }
      for (unsigned short v = 0; v < nSensor; ++v) {
        const auto slot = point * nSensor + v;
        if ((ring == 2 && redundant[point] && residual[slot] <= poorFit) || !rhs.col(v).allFinite()) continue;
        const Eigen::VectorXd coefficient = qr.solve(rhs.col(v));
        const auto norm = rhs.col(v).stableNorm();
        const auto error = (design * coefficient - rhs.col(v)).stableNorm() /
                           std::max(norm, std::numeric_limits<passivedouble>::min());
        if (!std::isfinite(error) || (valid[slot] && redundant[point] && error >= residual[slot])) continue;
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
        if (valid[slot]) --local[0];
        valid[slot] = true;
        residual[slot] = error;
        const auto diagonal = qr.matrixR().topLeftCorner(terms, terms).diagonal().cwiseAbs();
        fitCondition[slot] = diagonal.minCoeff() / diagonal.maxCoeff();
        fitDof[slot] = cloud.size() - terms;
        ++local[ring - 1];
      }
      if (ring == 1)
        for (unsigned short v = 0; v < nSensor; ++v)
          grow[point] = grow[point] || !valid[point * nSensor + v] || residual[point * nSensor + v] > poorFit;
    }
  }
  local[2] = std::count(valid.begin(), valid.end(), false);
  passivedouble residualSum = 0, residualMax = 0, residualGlobal[2] = {}, localCondition = 1, globalCondition = 1;
  unsigned long lowDof = 0, globalLowDof = 0;
  unsigned long poor = 0, globalPoor = 0, traffic[2] = {requested, exported}, globalTraffic[2] = {};
  for (size_t k = 0; k < valid.size(); ++k) {
    if (!valid[k]) continue;
    localCondition = std::min(localCondition, fitCondition[k]);
    residualSum += residual[k];
    residualMax = std::max(residualMax, residual[k]);
    poor += residual[k] > poorFit;
    lowDof += fitDof[k] < 2;
  }
  CPassiveComm::Allreduce(&localCondition, &globalCondition, 1, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(&residualSum, &residualGlobal[0], 1, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(&residualMax, &residualGlobal[1], 1, CPassiveComm::Op::MAX);
  CPassiveComm::Allreduce(&lowDof, &globalLowDof, 1, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(&poor, &globalPoor, 1, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(traffic, globalTraffic, 2, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(local, global, 3, CPassiveComm::Op::SUM);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::cout << "Quadratic Hessian fits (owned point-sensor pairs): " << global[0] << " first ring, " << global[1]
              << " grown stencil, " << global[2] << " WLS fallbacks." << std::endl;
    std::cout << "Quadratic Hessian relative weighted fit residual: mean "
              << residualGlobal[0] / std::max(1ul, global[0] + global[1]) << ", maximum " << residualGlobal[1] << "; "
              << globalPoor << " fits above " << poorFit << "; minimum QR pivot ratio " << globalCondition
              << ". Second-ring MPI: " << globalTraffic[0] << " requested owner neighborhoods, " << globalTraffic[1]
              << " exported samples." << std::endl;
    if (globalLowDof)
      std::cout << "WARNING: " << globalLowDof
                << " quadratic fits have fewer than two residual degrees of freedom; "
                   "a small residual alone does not establish their reliability."
                << std::endl;
    if (globalPoor)
      std::cout << "WARNING: Large quadratic fit residuals remain after stencil comparison; inspect sensor noise "
                   "and nonsmooth flow features. No resolved curvature was discarded."
                << std::endl;
    if (global[2])
      std::cout << "WARNING: Quadratic Hessian reconstruction retained WLS for failed fits. "
                << "Check non-finite sensor values and stencil rank." << std::endl;
  }
}
