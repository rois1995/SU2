/*!
 * \file native_m2_stages.inc
 * \brief Lab-only stage C query anatomy and timing.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

class LabM2Phase {
 public:
  LabM2Phase(bool trace, const char* name) noexcept : name_(trace ? name : nullptr) {
    if (name_) std::fprintf(stderr, "FP2_M2 PHASE name=%s begin\n", name_);
  }
  ~LabM2Phase() noexcept {
    if (name_) std::fprintf(stderr, "FP2_M2 PHASE name=%s end\n", name_);
  }
  LabM2Phase(const LabM2Phase&) = delete;
  LabM2Phase& operator=(const LabM2Phase&) = delete;

 private:
  const char* name_;
};

class LabM2Stages {
 public:
  LabM2Stages(const local::StateSurfaceGeometry& surface, bool trace) : surface_(surface), trace_(trace) {}

  bool Forward(local::SurfaceGeometry& geometry, const SurfaceFace& face) {
    Call call;
    Before(face, call);
    try {
      const auto within = geometry.WithinTolerance(face);
      After(call, within ? "1" : "0");
      return within;
    } catch (...) {
      After(call, "threw");
      throw;
    }
  }

  double SecondsObserver() const { return observer_seconds_; }

  void Report(std::ostream& out, int rank, unsigned pass, double capture_seconds) noexcept {
    Observe([&] {
      const auto start = Clock::now();
      std::ostringstream table;
      table << std::setprecision(17);
      for (size_t bin = 0; bin < bins_.size(); ++bin)
        for (size_t direct = 0; direct < bins_[bin].size(); ++direct) {
          const auto& row = bins_[bin][direct];
          table << "FP2_M2 STAGE rank=" << rank << " pass=" << pass << " bin=" << bin
                << " direct_eligible=" << direct << " calls=" << row.calls << " seconds_forward=" << row.seconds;
          for (const auto& [name, member] : WorkFields) table << ' ' << name << '=' << row.stats.*member;
          for (const auto& [name, member] : TimeFields) table << ' ' << name << '=' << row.stats.*member;
          table << '\n';
        }
      const auto text = table.str();
      out << text << "FP2_M2 OBSERVER rank=" << rank << " pass=" << pass << " status=complete seconds="
          << observer_seconds_ + capture_seconds + Seconds(start) << '\n';
    });
    if (!complete_) {
      try {
        out << "FP2_M2 OBSERVER rank=" << rank << " pass=" << pass << " status=incomplete seconds="
            << observer_seconds_ + capture_seconds << '\n';
      } catch (...) {
      }
    }
  }

 private:
  using Stats = SU2NativeGeometry::MappedFacetReference::GeometryStats;
  using WorkField = std::pair<const char*, size_t Stats::*>;
  using TimeField = std::pair<const char*, double Stats::*>;
#define M2_STAT(field) {#field, &Stats::field}
  static constexpr WorkField WorkFields[]{
      M2_STAT(direct_calls), M2_STAT(upper_calls), M2_STAT(classification_calls), M2_STAT(classification_work),
      M2_STAT(facet_hits), M2_STAT(facet_misses), M2_STAT(scope_hits), M2_STAT(scope_misses),
      M2_STAT(scope_builds), M2_STAT(scope_build_work), M2_STAT(support_calls), M2_STAT(support_facets),
      M2_STAT(facet_build_work), M2_STAT(piece_selection_work), M2_STAT(traversal_work),
      M2_STAT(selected_pieces), M2_STAT(support_pieces)};
  static constexpr TimeField TimeFields[]{
      M2_STAT(direct_seconds), M2_STAT(upper_seconds), M2_STAT(scope_build_seconds), M2_STAT(support_seconds),
      M2_STAT(facet_build_seconds), M2_STAT(piece_selection_seconds), M2_STAT(traversal_seconds)};
#undef M2_STAT
  static constexpr std::array<double, 4> LengthBins{.002, .005, .01, .02};

  struct Call {
    Stats before{};
    Clock::time_point start;
    size_t ordinal = 0, bin = 0;
    bool direct = false, opened = false, ready = false;
  };

  struct Bin {
    Stats stats{};
    size_t calls = 0;
    double seconds = 0;
  };

  template<class Action>
  void Observe(Action action) noexcept {
    if (!complete_) return;
    const auto start = Clock::now();
    try {
      action();
    } catch (...) {
      complete_ = false;
    }
    observer_seconds_ += Seconds(start);
  }

  void Before(const SurfaceFace& face, Call& call) noexcept {
    Observe([&] {
      const auto& geometry = surface_.State().Geometry();
      const auto facet = geometry.Canonical(face.facet);
      std::array<Point, 3> points;
      call.direct = true;
      for (size_t i = 0; i < points.size(); ++i) {
        const auto* point = surface_.Find(face.v[i]);
        if (!point) throw std::logic_error("M2 query corner has no live binding.");
        points[i] = point->node.p;
        call.direct = call.direct && geometry.Supports(facet, point->binding);
      }
      double longest = 0;
      for (size_t i = 0; i < points.size(); ++i) {
        const auto p = points[i], q = points[(i + 1) % points.size()];
        longest = std::max(longest, std::hypot(p.x - q.x, p.y - q.y, p.z - q.z));
      }
      call.bin = std::upper_bound(LengthBins.begin(), LengthBins.end(), longest) - LengthBins.begin();
      call.ordinal = ++ordinal_;
      if (trace_) {
        const auto sheet = std::get<1>(surface_.State().Entity(facet));
        call.opened = true;
        std::fprintf(stderr, "FP2_M2 Q begin ordinal=%zu bin=%zu direct_eligible=%d longest_edge=%.17g sheet=%llu\n",
                     call.ordinal, call.bin, int(call.direct), longest, static_cast<unsigned long long>(sheet));
      }
      call.before = geometry.GeometryStatsSnapshot();
      call.ready = true;
    });
    call.start = Clock::now();
  }

  void After(const Call& call, const char* answer) noexcept {
    const auto seconds = Seconds(call.start);
    Stats after{};
    if (call.ready) Observe([&] { after = surface_.State().Geometry().GeometryStatsSnapshot(); });
    const auto start = Clock::now();
    if (call.opened)
      std::fprintf(stderr, "FP2_M2 Q end ordinal=%zu answer=%s seconds=%.17g\n", call.ordinal, answer, seconds);
    observer_seconds_ += Seconds(start);
    if (!call.ready) return;
    Observe([&] {
      auto& bin = bins_[call.bin][call.direct];
      ++bin.calls;
      bin.seconds += seconds;
      for (const auto& field : WorkFields) bin.stats.*field.second += after.*field.second - call.before.*field.second;
      for (const auto& field : TimeFields) bin.stats.*field.second += after.*field.second - call.before.*field.second;
    });
  }

  const local::StateSurfaceGeometry& surface_;
  const bool trace_;
  std::array<std::array<Bin, 2>, 5> bins_{};
  bool complete_ = true;
  size_t ordinal_ = 0;
  double observer_seconds_ = 0;
};
