/*!
 * \file CEPMFieldSites.hpp
 * \brief Per-site zero-valued AD inputs and records for EPM field sensitivities.
 * \author SU2 Contributors
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

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basic_types/datatype_structure.hpp"
#include "CEPMExtraction.hpp"

/*!
 * \brief Rank-local registry of EPM field sites (edges, boundary faces, nodes).
 * \note Each site owns zero-valued AD inputs added to the stress tensor and to the k/omega productions.
 *       Sites are discovered in a passive pass, then the store is frozen and the inputs are registered
 *       in the real recording, so the input addresses stay stable.
 */
class CEPMFieldSites {
 public:
  using SiteKind = CEPMExtraction::SiteKind;

  /*! \brief Full 3x3 stress block; only the nDim x nDim part is active (tau_ij and tau_ji are separate inputs). */
  static constexpr int kTau = 9;
  /*! \brief Number of EPM cases stored in the d* columns. */
  static constexpr int kCases = 5;
  /*! \brief Returned by Site() for an unknown key after Freeze(). */
  static constexpr size_t npos = size_t(-1);

  /*! \brief Per-site output row. Member order is the file column order. */
  struct Record {
    static constexpr passivedouble nan = std::numeric_limits<passivedouble>::quiet_NaN();

    passivedouble kind = nan, key_a = nan, key_b = nan;
    passivedouble coord_a[3] = {nan, nan, nan}, coord_b[3] = {nan, nan, nan};
    passivedouble rho = nan, mu_t = nan, k = nan;
    /*! \brief Velocity gradient, row-major: grad[i*3+j] = du_i/dx_j. */
    passivedouble grad[9] = {nan, nan, nan, nan, nan, nan, nan, nan, nan};
    passivedouble normal[3] = {nan, nan, nan}, area = nan, volume = nan;
    passivedouble prod_limit = nan, pk0 = nan, pw0 = nan, p_raw0 = nan, alfa_blended = nan;
    passivedouble floor_active = nan, cap_active = nan;
    passivedouble S_tau[kTau] = {}, S_pk = 0, S_pw = 0;
    passivedouble dtau[kCases][kTau] = {}, dpk[kCases] = {}, dpw[kCases] = {};
    passivedouble uses = 0;
  };
  static_assert(std::is_standard_layout<Record>::value, "Record is written raw.");
  static_assert(std::is_trivially_copyable<Record>::value, "Record is written raw.");
  static_assert(sizeof(Record) == 100 * sizeof(passivedouble), "Record must hold only passivedouble columns.");

  /*! \brief Empty the registry and reset the AD indices of all inputs. */
  void Clear() {
    for (auto& in : inputs_) {
      for (auto& t : in.tau) AD::ResetInput(t);
      AD::ResetInput(in.pk);
      AD::ResetInput(in.pw);
    }
    index_.clear();
    inputs_.clear();
    records_.clear();
    frozen_ = false;
  }

  /*!
   * \brief Index of the site with this key; a new site is appended if the key is unknown.
   * \note EDGE keys are stored as (min, max). After Freeze(), an unknown key returns npos and nothing is added.
   */
  size_t Site(SiteKind kind, unsigned long a, unsigned long b) {
    if (kind == SiteKind::EDGE && b < a) std::swap(a, b);
    const Key key{static_cast<int>(kind), a, b};
    const auto it = index_.find(key);
    if (it != index_.end()) return it->second;
    if (frozen_) return npos;

    const size_t site = records_.size();
    index_.emplace(key, site);
    inputs_.emplace_back();
    records_.emplace_back();
    auto& record = records_.back();
    record.kind = static_cast<int>(kind);
    record.key_a = static_cast<passivedouble>(a);
    record.key_b = static_cast<passivedouble>(b);
    return site;
  }

  /*! \brief 9 contiguous stress inputs of a site (row-major 3x3). Addresses are stable. */
  su2double* TauInput(size_t site) { return inputs_[site].tau.data(); }
  /*! \brief k-production input of a site. */
  su2double& PkInput(size_t site) { return inputs_[site].pk; }
  /*! \brief omega-production input of a site. */
  su2double& PwInput(size_t site) { return inputs_[site].pw; }

  /*! \brief No new sites after this call. */
  void Freeze() { frozen_ = true; }
  bool Frozen() const { return frozen_; }

  /*!
   * \brief Reset every record to its unused state (uses, adjoints and corrections zero, state NaN).
   * \note Keys and inputs are kept. Call after Freeze() and before the recording: the hooks fill the
   *       state of a site only on its first use.
   */
  void ResetRecords() {
    for (auto& record : records_) {
      const auto kind = record.kind, a = record.key_a, b = record.key_b;
      record = Record();
      record.kind = kind;
      record.key_a = a;
      record.key_b = b;
    }
  }

  /*!
   * \brief Drop the AD indices of all inputs and set their values to zero. Records and keys are kept.
   * \note Call when the recording is over, so that no stale tape index stays in the inputs.
   */
  void ResetInputs() {
    for (auto& in : inputs_) {
      for (auto& t : in.tau) {
        AD::ResetInput(t);
        t = 0.0;
      }
      AD::ResetInput(in.pk);
      in.pk = 0.0;
      AD::ResetInput(in.pw);
      in.pw = 0.0;
    }
  }

  /*! \brief Set every input to zero and register it as an AD input of the current recording. */
  void RegisterInputs() {
    for (auto& in : inputs_) {
      for (auto& t : in.tau) Register(t);
      Register(in.pk);
      Register(in.pw);
    }
  }

  /*! \brief Copy the adjoints of the inputs into S_tau, S_pk and S_pw. */
  void ExtractAdjoints() {
    for (size_t site = 0; site < records_.size(); ++site) {
      const auto& in = inputs_[site];
      auto& record = records_[site];
      for (int i = 0; i < kTau; ++i) record.S_tau[i] = SU2_TYPE::GetDerivative(in.tau[i]);
      record.S_pk = SU2_TYPE::GetDerivative(in.pk);
      record.S_pw = SU2_TYPE::GetDerivative(in.pw);
    }
  }

  Record& Data(size_t site) { return records_[site]; }
  const Record& Data(size_t site) const { return records_[site]; }
  size_t Size() const { return records_.size(); }

 private:
  using Key = std::tuple<int, unsigned long, unsigned long>;
  struct KeyHash {
    size_t operator()(const Key& key) const {
      size_t h = std::hash<int>()(std::get<0>(key));
      const auto mix = [&h](unsigned long v) { h ^= std::hash<unsigned long>()(v) + 0x9e3779b9 + (h << 6) + (h >> 2); };
      mix(std::get<1>(key));
      mix(std::get<2>(key));
      return h;
    }
  };
  struct Inputs {
    std::array<su2double, kTau> tau{};
    su2double pk = 0, pw = 0;
  };

  static void Register(su2double& x) {
    x = 0.0;
    AD::ResetInput(x);
    AD::RegisterInput(x);
  }

  std::unordered_map<Key, size_t, KeyHash> index_;
  std::deque<Inputs> inputs_; /*!< deque: push_back never moves existing elements. */
  std::vector<Record> records_;
  bool frozen_ = false;
};
