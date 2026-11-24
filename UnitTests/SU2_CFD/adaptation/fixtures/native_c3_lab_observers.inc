/*!
 * \file native_c3_lab_observers.inc
 * \brief Report-only collapse-bound and leftover distance diagnostics for the FP2-C3 lab.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

std::pair<double, double> WingPlanform(Point p) {
  const auto pi = std::acos(-1.);
  const double leading = p.y * std::tan(pi / 6), trailing = .8059 + p.y * std::tan(15.8 * pi / 180);
  return {p.y / 1.1963, (p.x - leading) / (trailing - leading)};
}

std::optional<size_t> WingDistanceBin(double eta, double chord) {
  if (eta >= .98) return 4;
  if (chord < .005) return 0;
  if (chord < .02) return 1;
  if (chord < .10) return 2;
  if (chord > .90) return 3;
  return std::nullopt;
}

class LabC3Observers {
 public:
  LabC3Observers(const local::Mesh& mesh, const local::StateSurfaceGeometry& surface, bool regions)
      : mesh_(mesh), surface_(surface), state_(surface.State()), regions_(regions) {}

  void Sample(const SurfaceFace& face, const local::SurfaceSamples& samples) noexcept {
    if (distance_finished_) return;
    distance_.Record([&] { samples_.insert_or_assign(face.v, std::make_pair(face.facet, samples)); });
  }

  void Refusal(const char* step, local::SurfaceEdge edge, local::SurfaceRefusal reason) noexcept {
    if (std::strcmp(step, "surface_distance") != 0) return;
    distance_.Record([&] {
      std::sort(edge.begin(), edge.end());
      last_refusal_ = edge;
      auto& row = refusals_[edge];
      row.ordinal = ++refusal_ordinal_;
      row.reason = reason;
    });
  }

  void Near(char step, const std::string& row) noexcept {
    if (step != 'D' || row.rfind("outcome=refused reason=metric_floor ", 0) != 0) return;
    distance_.Record([&] {
      auto& refusal = refusals_.at(last_refusal_);
      if (refusal.ordinal != refusal_ordinal_ || refusal.reason != local::SurfaceRefusal::Near)
        throw std::logic_error("Metric-floor row has no preceding distance Near refusal.");
      refusal.metric_floor = true;
    });
  }

  void RefusedFace(const Face& face) noexcept {
    ++total_refused_;
    if (captured_ == refused_faces_.size()) return;
    distance_.Record([&] {
      auto row = DescribeFace(face);
      for (size_t k = 0; k < face.size(); ++k) {
        local::SurfaceEdge edge{face[k], face[(k + 1) % face.size()]};
        std::sort(edge.begin(), edge.end());
        if (const auto found = refusals_.find(edge); found != refusals_.end()) row.refusals[k] = found->second;
      }
      if (regions_) {
        const auto a = row.xyz[0], b = row.xyz[1], c = row.xyz[2];
        const auto [eta, chord] = WingPlanform({(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3,
                                               (a.z + b.z + c.z) / 3});
        const auto bin = std::get<2>(state_.Entity(row.facet)) == 2 ? WingDistanceBin(eta, chord) : std::nullopt;
        ++refused_bins_[bin.value_or(refused_bins_.size() - 1)];
      }
      refused_faces_[captured_++] = row;
    });
  }

  template <class RecordRegion>
  void ShortFace(std::ostream& out, const Face& face, int rank, unsigned pass, RecordRegion record_region) noexcept {
    if (short_count_ >= 50) return;
    distance_.Record([&] {
      const auto row = DescribeFace(face);
      PrintDistanceFace(out, "SHORT_FACE", rank, pass, row);
      out << '\n';
      const auto a = row.xyz[0], b = row.xyz[1], c = row.xyz[2];
      record_region(row.facet, Point{(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3, (a.z + b.z + c.z) / 3});
      ++short_count_;
    });
  }

  void EndDistance() noexcept {
    if (distance_finished_) return;
    const auto start = Clock::now();
    distance_finished_ = true;
    distance_.scratch = transfer_memory::Add(sizeof(samples_), sizeof(refusals_), sizeof(distance_slots_),
                                              sizeof(refused_faces_), MapBytes(samples_), MapBytes(refusals_),
                                              MapBytes(distance_slots_));
    samples_.clear();
    refusals_.clear();
    distance_slots_.clear();
    distance_.seconds += Seconds(start);
  }

  size_t BoundWorkBefore() noexcept {
    size_t work = 0;
    bound_.Record([&] { work = state_.Geometry().GeometryStatsSnapshot().classification_work; });
    return work;
  }

  void Bound(const SurfaceFace& face, double seconds, bool answer, size_t work_before) noexcept {
    bound_.Record([&] {
      if (!bound_slots_ready_) {
        FillSlots(bound_slots_);
        bound_slots_ready_ = true;
      }
      const auto& geometry = state_.Geometry();
      const auto work = geometry.GeometryStatsSnapshot().classification_work - work_before;
      const auto canonical = geometry.Canonical(face.facet);
      const auto entity = state_.Entity(canonical);
      std::array<Point, 3> xyz;
      bool direct = true;
      for (size_t k = 0; k < xyz.size(); ++k) {
        xyz[k] = mesh_.Points()[bound_slots_.at(face.v[k])].node.p;
        const auto* point = surface_.Find(face.v[k]);
        if (!point) throw std::logic_error("Bound call has no live surface binding.");
        direct = direct && geometry.Supports(canonical, point->binding);
      }
      double longest = 0;
      for (size_t k = 0; k < xyz.size(); ++k) longest = std::max(longest, Space(xyz[k], xyz[(k + 1) % xyz.size()]));
      const auto tbin = std::upper_bound(TimeLimits.begin(), TimeLimits.end(), seconds) - TimeLimits.begin();
      const auto lbin = std::upper_bound(LengthLimits.begin(), LengthLimits.end(), longest) - LengthLimits.begin();
      time_bins_[tbin].Add(seconds);
      length_bins_[lbin].Add(seconds);
      direct_bins_[direct].Add(seconds);
      direct_work_[direct] += work;

      const auto key = Key(face);
      auto& entry = bound_faces_[key];
      entry.total.Add(seconds);
      entry.last = seconds;
      entry.last_committed = false;
      auto& group = entities_[entity];
      group.calls.Add(seconds);
      group.longest_sum += longest;

      const auto at = std::find_if(slowest_.begin(), slowest_.end(),
                                   [seconds](const auto& call) { return seconds > call.seconds; });
      if (at != slowest_.end()) {
        std::move_backward(at, slowest_.end() - 1, slowest_.end());
        *at = {face, canonical, entity, xyz, seconds, longest, answer, direct, work};
      }
    });
  }

  void BoundUnavailable() noexcept { bound_.available = false; }

  void Commit(const SurfaceFace& face) noexcept {
    bound_.Record([&] {
      auto& row = bound_faces_.at(Key(face));
      if (!row.last_committed) {
        row.committed.Add(row.last);
        row.last_committed = true;
      }
    });
  }

  void Report(std::ostream& out, int rank, unsigned pass) noexcept {
    EndDistance();
    // Format each report off-stream so a failed observer cannot leave half a successful table.
    Emit(out, distance_, "FP2_D REFUSED_OBSERVER", rank, pass, [&](auto& row) {
      row << " total_refused=" << total_refused_ << " captured=" << captured_;
    }, [&](auto& row) { ReportDistance(row, rank, pass); });
    std::array<Totals, 3> fates{};
    bound_.Record([&] {
      local::VisitLivePhysicalFaces(mesh_, [&](const SurfaceFace& face, const auto&) {
        const auto found = bound_faces_.find(Key(face));
        if (found != bound_faces_.end()) found->second.live = true;
      });
      for (const auto& item : bound_faces_) {
        const auto& face = item.second;
        if (face.live) {
          fates[0].Merge(face.total);
        } else {
          fates[1].Merge(face.committed);
          fates[2].Merge({face.total.calls - face.committed.calls,
                          std::max(0., face.total.seconds - face.committed.seconds)});
        }
      }
    });
    bound_.scratch = transfer_memory::Add(sizeof(bound_faces_), sizeof(bound_slots_), sizeof(entities_),
                                           sizeof(slowest_), sizeof(time_bins_), sizeof(length_bins_),
                                           sizeof(direct_bins_), sizeof(direct_work_), sizeof(fates),
                                           MapBytes(bound_faces_), MapBytes(bound_slots_), MapBytes(entities_));
    Emit(out, bound_, "FP2_C3 BOUND_OBSERVER", rank, pass, [&](auto& row) {
      Totals total;
      for (const auto& bin : time_bins_) total.Merge(bin);
      row << " calls=" << total.calls << " seconds=" << total.seconds << " work_units=unavailable";
    }, [&](auto& row) { ReportBound(row, rank, pass, fates); });
  }

 private:
  struct Measurement {
    bool available = true;
    double seconds = 0;
    size_t scratch = 0;

    template <class Observe>
    void Record(Observe observe) noexcept {
      if (!available) return;
      const auto start = Clock::now();
      try {
        observe();
      } catch (...) {
        available = false;
      }
      seconds += Seconds(start);
    }
  };

  struct Totals {
    size_t calls = 0;
    double seconds = 0.;
    void Add(double elapsed) { Merge({1, elapsed}); }
    void Merge(Totals other) {
      calls += other.calls;
      seconds += other.seconds;
    }
  };

  struct RefusedEdge {
    local::SurfaceRefusal reason = local::SurfaceRefusal::None;
    size_t ordinal = 0;
    bool metric_floor = false;
  };

  struct DistanceFace {
    Face corners{};
    Id facet = 0;
    std::array<Point, 3> xyz{};
    std::array<double, 3> space{}, metric{};
    std::optional<local::SurfaceSamples> samples;
    std::array<RefusedEdge, 3> refusals{};
  };

  struct BoundFace {
    Totals total, committed;
    double last = 0;
    bool last_committed = false, live = false;
  };

  struct BoundEntity {
    Totals calls;
    double longest_sum = 0;
  };

  struct BoundCall {
    SurfaceFace face;
    Id canonical = 0;
    BoundaryParameterState::EntityKey entity{};
    std::array<Point, 3> xyz{};
    double seconds = -1, longest = 0;
    bool answer = false, direct = false;
    size_t classification_work = 0;
  };

  using Slots = std::map<Id, local::Slot>;
  using FaceBinding = std::pair<Face, Id>;
  static constexpr std::array<double, 3> TimeLimits{.001, .01, .1};
  static constexpr std::array<double, 4> LengthLimits{.002, .005, .01, .02};

  template <class Map>
  static size_t MapBytes(const Map& map) {
    return transfer_memory::Mul(map.size(), sizeof(typename Map::value_type) + 4 * sizeof(void*));
  }

  static FaceBinding Key(const SurfaceFace& face) { return {CollapseFaceKey(face.v), face.facet}; }
  static double Space(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); }

  void FillSlots(Slots& slots) const {
    for (local::Slot slot = 0; slot < mesh_.Points().size(); ++slot)
      if (!mesh_.Points()[slot].removed) slots.emplace(mesh_.Points()[slot].node.id, slot);
  }

  DistanceFace DescribeFace(const Face& face) {
    DistanceFace result;
    result.corners = face;
    if (const auto found = samples_.find(face); found != samples_.end()) {
      result.facet = found->second.first;
      result.samples = found->second.second;
    } else {
      std::optional<Id> facet;
      local::VisitLivePhysicalFaces(mesh_, [&](const SurfaceFace& live, const auto&) {
        if (live.v == face) facet = live.facet;
      });
      if (!facet) throw std::logic_error("Distance census has no live oriented physical face.");
      result.facet = *facet;
    }
    if (distance_slots_.empty()) FillSlots(distance_slots_);
    std::array<local::Slot, 3> slots;
    for (size_t k = 0; k < slots.size(); ++k) {
      slots[k] = distance_slots_.at(face[k]);
      result.xyz[k] = mesh_.Points()[slots[k]].node.p;
    }
    for (size_t k = 0; k < slots.size(); ++k) {
      const auto j = (k + 1) % slots.size();
      result.space[k] = Space(result.xyz[k], result.xyz[j]);
      result.metric[k] = mesh_.Length({slots[k], slots[j]});
    }
    return result;
  }

  static void Prefix(std::ostream& out, const char* name, int rank, unsigned pass) {
    out << name << " rank=" << rank << " pass=" << pass;
  }

  static void PrintDistanceFace(std::ostream& out, const char* kind, int rank, unsigned pass,
                                const DistanceFace& row) {
    out << "FP2_D ";
    Prefix(out, kind, rank, pass);
    out << " facet=" << row.facet << " vertices=";
    PrintLabTriple(out, row.corners);
    for (size_t k = 0; k < row.xyz.size(); ++k) {
      out << " xyz" << k << '=';
      PrintLabPoint(out, row.xyz[k]);
      out << " space" << k << '=' << row.space[k] << " metric" << k << '=' << row.metric[k];
    }
    if (row.samples) {
      const auto& distances = row.samples->distances;
      out << " samples=" << distances[0] << ',' << distances[1] << ',' << distances[2] << ',' << distances[3]
          << " unconverged=" << row.samples->unconverged;
    } else {
      out << " samples=unavailable unconverged=unavailable";
    }
  }

  void ReportDistance(std::ostream& out, int rank, unsigned pass) const {
    constexpr std::array<const char*, static_cast<size_t>(local::SurfaceRefusal::Near) + 1> labels{
        "none", "border", "ineligible", "walk", "protected", "feature", "trace", "duplicate",
        "outward", "size", "validity", "topology", "memory", "entity", "near"};
    for (size_t i = 0; i < captured_; ++i) {
      const auto& row = refused_faces_[i];
      PrintDistanceFace(out, "REFUSED_FACE", rank, pass, row);
      for (size_t k = 0; k < row.refusals.size(); ++k) {
        const auto& refusal = row.refusals[k];
        out << " refusal" << k << '=' << labels[static_cast<size_t>(refusal.reason)] << " ordinal" << k << '=';
        if (refusal.ordinal) out << refusal.ordinal;
        else out << "none";
        out << " metric_floor" << k << '=' << refusal.metric_floor;
      }
      out << '\n';
    }
    if (!regions_) return;
    for (size_t k = 0; k < refused_bins_.size(); ++k) {
      Prefix(out, "FP2_D REFUSED_REGION", rank, pass);
      out << " bin=";
      if (k < refused_bins_.size() - 1) out << k;
      else out << "outside";
      out << " captured=" << refused_bins_[k] << " total_refused=" << total_refused_ << '\n';
    }
  }

  void ReportBound(std::ostream& out, int rank, unsigned pass, const std::array<Totals, 3>& fates) const {
    const auto bins = [&](const char* kind, const auto& rows, size_t first_bin = 0) {
      for (size_t k = 0; k < rows.size(); ++k) {
        Prefix(out, "FP2_C3 BOUND_BIN", rank, pass);
        out << " kind=" << kind << " bin=" << k + first_bin << " calls=" << rows[k].calls
            << " seconds=" << rows[k].seconds;
        if (std::strcmp(kind, "direct") == 0) out << " classification_work=" << direct_work_[k];
        out << '\n';
      }
    };
    bins("time", time_bins_);
    bins("length", length_bins_);
    bins("fate", fates, 1);
    bins("direct", direct_bins_);

    using EntityRow = decltype(entities_)::value_type;
    std::array<const EntityRow*, 5> entities{};
    for (const auto& row : entities_) {
      const auto at = std::find_if(entities.begin(), entities.end(), [&](const auto* current) {
        return !current || row.second.calls.seconds > current->second.calls.seconds;
      });
      if (at == entities.end()) continue;
      std::move_backward(at, entities.end() - 1, entities.end());
      *at = &row;
    }
    size_t order = 0;
    for (const auto* row : entities) {
      if (!row) break;
      Prefix(out, "FP2_C3 BOUND_ENTITY", rank, pass);
      out << " order=" << ++order;
      PrintLabEntity(out, row->first);
      const auto& group = row->second;
      out << " calls=" << group.calls.calls << " seconds=" << group.calls.seconds
          << " mean_longest_edge=" << group.longest_sum / group.calls.calls << '\n';
    }
    order = 0;
    for (const auto& call : slowest_) {
      if (call.seconds < 0) break;
      Prefix(out, "FP2_C3 BOUND_SLOW", rank, pass);
      out << " order=" << ++order;
      PrintLabEntity(out, call.entity);
      out << " facet=" << call.face.facet << " canonical=" << call.canonical << " vertices=";
      PrintLabTriple(out, call.face.v);
      for (size_t k = 0; k < call.xyz.size(); ++k) {
        out << " xyz" << k << '=';
        PrintLabPoint(out, call.xyz[k]);
      }
      out << " seconds=" << call.seconds << " answer=" << call.answer << " longest_edge=" << call.longest
          << " direct=" << call.direct << " classification_work=" << call.classification_work << '\n';
    }
  }

  template <class Summary, class Details>
  static void Emit(std::ostream& out, Measurement& measurement, const char* type, int rank, unsigned pass,
                   Summary summary, Details details) noexcept {
    std::string formatted;
    measurement.Record([&] {
      std::ostringstream row;
      row << std::setprecision(17);
      details(row);
      formatted = row.str();
      // The formatting stream and its rendered copy coexist before the stream is destroyed.
      measurement.scratch = transfer_memory::Add(measurement.scratch, transfer_memory::Mul(formatted.size(), 3));
    });
    try {
      Prefix(out, type, rank, pass);
      out << " status=" << (measurement.available ? "ok" : "unavailable")
          << " seconds_observer=" << measurement.seconds << " scratch_bytes=" << measurement.scratch;
      if (measurement.available) summary(out);
      out << '\n';
      if (measurement.available) out << formatted;
    } catch (...) {
      measurement.available = false;
    }
  }

  const local::Mesh& mesh_;
  const local::StateSurfaceGeometry& surface_;
  const BoundaryParameterState& state_;
  const bool regions_;
  Measurement bound_, distance_;
  bool distance_finished_ = false, bound_slots_ready_ = false;
  size_t total_refused_ = 0, captured_ = 0, short_count_ = 0, refusal_ordinal_ = 0;
  local::SurfaceEdge last_refusal_{};
  std::map<local::SurfaceEdge, RefusedEdge> refusals_;
  std::map<Face, std::pair<Id, local::SurfaceSamples>> samples_;
  Slots distance_slots_, bound_slots_;
  std::array<DistanceFace, 100> refused_faces_{};
  std::array<size_t, 6> refused_bins_{};
  std::map<FaceBinding, BoundFace> bound_faces_;
  std::map<BoundaryParameterState::EntityKey, BoundEntity> entities_;
  std::array<BoundCall, 10> slowest_{};
  std::array<Totals, TimeLimits.size() + 1> time_bins_{};
  std::array<Totals, LengthLimits.size() + 1> length_bins_{};
  std::array<Totals, 2> direct_bins_{};
  std::array<size_t, 2> direct_work_{};
};
