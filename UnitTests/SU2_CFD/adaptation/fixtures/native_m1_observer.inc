/*!
 * \file native_m1_observer.inc
 * \brief Shadow-authority measurements for the FP2-M1 bound support lab.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

uint64_t M1Bits(double value) noexcept {
  uint64_t bits;
  std::memcpy(&bits, &value, sizeof bits);
  return bits;
}

bool M1SamePoint(Point a, Point b) {
  return M1Bits(a.x) == M1Bits(b.x) && M1Bits(a.y) == M1Bits(b.y) && M1Bits(a.z) == M1Bits(b.z);
}

template <class Map, class Equal>
bool M1SameMap(const Map& left, const Map& right, Equal equal) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin(), [&](const auto& a, const auto& b) {
           return a.first == b.first && equal(a.second, b.second);
         });
}

bool M1SameDefinition(const SU2NativeGeometry::MappedFacetReference& a,
                      const SU2NativeGeometry::MappedFacetReference& b) {
  if (&a == &b || a.Version() != b.Version() || a.ReferenceComponents() != b.ReferenceComponents()) return false;
  if (!M1SameMap(a.Errors(), b.Errors(), [](double x, double y) { return M1Bits(x) == M1Bits(y); })) return false;
  if (!M1SameMap(a.Curves(), b.Curves(), [](const auto& x, const auto& y) {
        return std::equal(x.begin(), x.end(), y.begin(), M1SamePoint);
      })) return false;
  const auto& ar = a.Reference();
  const auto& br = b.Reference();
  if (!M1SameMap(ar.Analytic(), br.Analytic(), [](const auto& x, const auto& y) {
        return x.kind == y.kind && M1SamePoint(x.origin, y.origin) && M1SamePoint(x.extent, y.extent) &&
               M1SamePoint(x.axis, y.axis) && M1Bits(x.tolerance) == M1Bits(y.tolerance);
      })) return false;
  if (!M1SameMap(ar.Affine(), br.Affine(), [](const auto& x, const auto& y) {
        return x.identity == y.identity && x.version == y.version && x.dimension == y.dimension &&
               M1SamePoint(x.origin, y.origin) && M1SamePoint(x.normal, y.normal) &&
               M1Bits(x.maximum_error) == M1Bits(y.maximum_error);
      })) return false;
  return M1SameMap(ar.Facets(), br.Facets(), [&](const auto& x, const auto& y) {
    return x.id == y.id && x.marker == y.marker && a.Canonical(x.id) == b.Canonical(y.id) &&
           a.Body(x.id) == b.Body(y.id) &&
           std::equal(x.v.begin(), x.v.end(), y.v.begin(), [](const auto& u, const auto& v) {
             return u.id == v.id && M1SamePoint(u.p, v.p);
           });
  });
}

void M1PrintToken(std::ostream& out, const char* text) {
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p)
    out << (std::iscntrl(*p) || std::isspace(*p) || *p == '=' ? '_' : static_cast<char>(*p));
}

struct LabM1Shadow {
  ReferenceState reference;
  std::string reason;
  size_t resident = 0, load_peak = 0, ceiling = 0;
  std::optional<size_t> rss_before, rss_after, rss_high_water;
  bool resource_refusal = false;

  bool Load(const std::string& path, const BoundaryParameterState& pass, size_t pass_ceiling,
            size_t observer_ceiling) {
    ceiling = std::min(pass_ceiling, observer_ceiling);
    rss_before = LinuxBytes("VmRSS:");
    bool loaded = false;
    const auto admitted = LoadReference(path, reference, loaded, reason, ceiling, 0, load_peak);
    rss_after = LinuxBytes("VmRSS:");
    rss_high_water = LinuxBytes("VmHWM:");
    if (!admitted) {
      resource_refusal = true;
      return false;
    }
    if (!loaded || !reference.original || !reference.original->Parameters()) {
      reason = "Shadow reference did not contain an authority";
      return false;
    }
    try {
      resident = ReferenceStorageBytes(reference);
      if (resident > ceiling || load_peak > ceiling) {
        resource_refusal = true;
        reason = "Shadow load or resident bytes exceed observer load ledger";
        return false;
      }
      if (M1SameDefinition(Geometry(), pass.Geometry())) return true;
      reason = "Shadow immutable geometry definition differs";
    } catch (const std::exception& error) {
      reason = error.what();
    } catch (...) {
      reason = "Unknown shadow definition comparison failure";
    }
    return false;
  }

  const SU2NativeGeometry::MappedFacetReference& Geometry() const {
    return reference.original->Parameters()->Geometry();
  }

  void ReportLoad(std::ostream& out, int rank, const char* status) const {
    out << "FP2_M1 SHADOW rank=" << rank << " status=" << status;
    PrintStorage(out);
    if (!reason.empty()) {
      out << " reason=";
      M1PrintToken(out, reason.c_str());
    }
    out << '\n';
  }

  void PrintStorage(std::ostream& out) const {
    out << " shadow_bytes=" << resident << " shadow_load_peak=" << load_peak << " shadow_load_ceiling=" << ceiling;
    PrintBytes(out, "rss_before", rss_before);
    PrintBytes(out, "rss_after", rss_after);
    PrintBytes(out, "rss_high_water", rss_high_water);
  }

 private:
  static std::optional<size_t> LinuxBytes(const char* key) noexcept {
    try {
      std::ifstream status("/proc/self/status");
      std::string line;
      while (std::getline(status, line)) {
        if (line.rfind(key, 0) != 0) continue;
        std::istringstream number(line.substr(std::strlen(key)));
        size_t kib;
        if (number >> kib) return transfer_memory::Mul(kib, 1024);
        break;
      }
    } catch (...) {
    }
    return std::nullopt;
  }

  static void PrintBytes(std::ostream& out, const char* key, std::optional<size_t> value) {
    out << ' ' << key << '=';
    if (value) out << *value;
    else out << "not_available";
  }
};

class LabM1Observer {
 public:
  using Stats = SU2NativeGeometry::MappedFacetReference::GeometryStats;
  using Binding = SU2NativeGeometry::SurfaceBinding;
  using Budget = SU2NativeGeometry::AtlasBudget;
  struct CopiedInput {
    Face ids{};
    Id facet = 0;
    BoundaryParameterState::EntityKey entity{};
    std::array<Binding, 3> bindings{};
    std::array<Point, 3> points{};
    Point anchor{};
    double tol = 0, radius = 0, pad1 = 0, pad2 = 0, longest = 0;
    bool direct = false;
    std::vector<Id> full, small1, small2;
  };
  struct ReplicaCounts {
    size_t attempted = 0, agree = 0, disagree = 0, resource = 0, threw = 0;
  };

  LabM1Observer(const local::Mesh& mesh, const local::StateSurfaceGeometry& surface, const LabM1Shadow& shadow,
                size_t cells, size_t ceiling, Clock::time_point trial_start, int rank, unsigned pass,
                bool trace_fixture = false) noexcept
      : mesh_(mesh), surface_(surface), shadow_(shadow), cells_(cells), ceiling_(ceiling), start_(trial_start),
        rank_(rank), pass_(pass), trace_fixture_(trace_fixture) {
    Observe([&] {
      if (SU2NativeTiming3D::current_frame || SU2NativeD9::active)
        throw std::logic_error("M1 observer requires inactive production timing and D9 sessions.");
      initial_pass_ = surface_.State().Geometry().GeometryStatsSnapshot();
      initial_shadow_ = shadow_.Geometry().GeometryStatsSnapshot();
    });
  }

  void BeforeForward(const SurfaceFace& face) noexcept {
    Observe([&] {
      input_ = {};
      selected_full_ = false;
      sample_ = {};
      stage2_ = false;
      ++ordinal_;
      active_facet_ = face.facet;
      Copy(face);
      if (input_.direct) {
        sample_[0].outcome = Outcome::Direct;
        return;
      }
      selected_full_ = Select(input_.radius, input_.full, sample_[0], 1, true);
      if (!selected_full_) return;

      auto& first = sample_[0];
      const auto selected_1 = Select(input_.pad1, input_.small1, first, 1, false);
      if (selected_1) {
        Nested(input_.small1, input_.full);
        if (input_.small1.empty()) first.outcome = Outcome::Empty;
        else if (input_.small1 == input_.full) first.outcome = Outcome::Same;
        else Query(input_.small1, first, 1, "bound_small");
      }
      if (first.outcome == Outcome::Proved) return;

      stage2_ = true;
      auto& second = sample_[1];
      if (input_.pad2 <= input_.pad1) {
        second.outcome = Outcome::NoLargerBox;
        return;
      }
      if (!Select(input_.pad2, input_.small2, second, 2, false)) return;
      Nested(input_.small2, input_.full);
      if (selected_1) Nested(input_.small1, input_.small2);
      if (selected_1 && input_.small2 == input_.small1) second.outcome = Outcome::SameAs1;
      else if (input_.small2 == input_.full) second.outcome = Outcome::SameAsFull;
      else if (input_.small2.empty()) second.outcome = Outcome::Empty;
      else Query(input_.small2, second, 2, "bound_small");
    });
  }

  void ForwardBegin() noexcept {
    Observe([&] {
      if (trace_fixture_ && selected_full_) {
        std::ostringstream trace;
        trace << "FP2_M1 EXPECT ordinal=" << ordinal_ << " tolerance=";
        PrintBits(trace, input_.tol);
        trace << " support=";
        for (auto id : input_.full) trace << id << ',';
        trace << " triangles=";
        for (auto p : input_.points)
          for (double x : {p.x, p.y, p.z}) {
            PrintBits(trace, x);
            trace << ',';
          }
        trace << ";\nFP2_M1 TRACE_BEGIN ordinal=" << ordinal_ << '\n';
        std::cerr << trace.str() << std::flush;
        trace_active_ = true;
      }
      forward_before_ = surface_.State().Geometry().GeometryStatsSnapshot();
    });
  }

  void AfterForward(const SurfaceFace& face, double seconds, bool answer) noexcept {
    Observe([&] {
      if (trace_active_) {
        std::cerr << "FP2_M1 TRACE_END ordinal=" << ordinal_ << '\n' << std::flush;
        trace_active_ = false;
      }
      const auto after = surface_.State().Geometry().GeometryStatsSnapshot();
      const auto work = after.classification_work - forward_before_.classification_work;
      const bool memo = after.memo_union_hits != forward_before_.memo_union_hits ||
                        after.memo_direct_hits != forward_before_.memo_direct_hits;
      forward_seconds_ = seconds;
      forward_work_ = work;
      AccumulateStage(0, answer);
      if (stage2_) AccumulateStage(1, answer);
      const auto group = input_.direct ? Group::Direct
                         : sample_[0].outcome == Outcome::Proved ? Group::Proved1
                         : stage2_ && sample_[1].outcome == Outcome::Proved ? Group::Proved2
                                                                        : Group::NeedsFull;
      auto& fallback = fallback_[static_cast<size_t>(group)];
      ++fallback.calls;
      fallback.seconds += seconds;
      fallback.work += work;
      fallback.hits += memo;
      if (selected_full_ || input_.direct) {
        ++fallback.replay_calls;
        fallback.replay_seconds += seconds;
      }
      AddBins(face, group, seconds);
      if (trace_fixture_ && selected_full_) RecordReplica(answer);
    });
  }

  void Commit(const SurfaceFace& face) noexcept {
    Observe([&] {
      auto& entry = fates_.at(Key(face));
      if (!entry.last_marked) {
        entry.committed.Add(entry.last);
        entry.last_marked = true;
      }
    });
  }

  void Report(std::ostream& out) noexcept {
    std::string formatted;
    Observe([&] {
      std::ostringstream row;
      row << std::setprecision(17);
      Format(row);
      formatted = row.str();
      scratch_peak_ = std::max(scratch_peak_, transfer_memory::Add(Scratch(),
                                                                 transfer_memory::GrowthBound(formatted.size(), 1)));
      Admit(transfer_memory::Add(shadow_.resident, scratch_peak_));
    });
    try {
      Prefix(out, "OBSERVER");
      out << " status=" << (available_ ? "ok" : "failed") << " seconds_observer=" << seconds_observer_
          << " scratch_bytes=" << scratch_peak_ << " ledger_peak=" << ledger_peak_
          << " selection_scratch_bytes=" << selection_scratch_;
      shadow_.PrintStorage(out);
      if (!available_) {
        out << " reason=";
        M1PrintToken(out, reason_.data());
      }
      out << '\n';
      if (available_) out << formatted;
    } catch (...) {
      available_ = false;
    }
  }

  const CopiedInput& Input() const noexcept { return input_; }
  size_t Proved1() const noexcept { return stages_[0].counts[static_cast<size_t>(Outcome::Proved)]; }
  size_t ReplicaAttempted() const noexcept { return replica_.attempted; }
  const ReplicaCounts& Replica() const noexcept { return replica_; }
  double SecondsObserver() const noexcept { return seconds_observer_; }

 private:
  enum class Outcome : unsigned {
    Direct, Empty, Same, NoLargerBox, SameAs1, SameAsFull, Proved, Fail, Resource, Threw, Count
  };
  enum class Group : unsigned { Proved1, Proved2, NeedsFull, Direct, Count };
  static constexpr std::array<const char*, static_cast<size_t>(Outcome::Count)> OutcomeNames{
      "direct_eligible", "empty", "same", "no_larger_box", "same_as_1", "same_as_full", "proved", "fail", "resource", "threw"};
  static constexpr std::array<const char*, static_cast<size_t>(Group::Count)> GroupNames{
      "proved_1", "proved_2", "needs_full", "direct_eligible"};
  static constexpr std::array<const char*, 4> AgreementNames{
      "full_yes_small_proved", "full_yes_small_other", "full_no_small_proved", "full_no_small_other"};
  static constexpr std::array<double, 4> LengthBins{.002, .005, .01, .02};

  struct Sample {
    Outcome outcome = Outcome::Threw;
    const char* where = "none";
    double seconds_select_full = 0, seconds_select_small = 0, seconds = 0;
    std::optional<double> result;
    size_t work = 0, classification = 0, selected_pieces = 0, support_pieces = 0;
    bool selected = false;
  };
  struct Stage {
    size_t calls = 0, k_full_sum = 0, k_full_calls = 0, k_small_sum = 0, k_small_calls = 0,
           radius_calls = 0, select_full_resource = 0, select_full_threw = 0;
    std::array<size_t, static_cast<size_t>(Outcome::Count)> counts{};
    std::array<size_t, 4> agreement{};
    double select_full = 0, select_small = 0, seconds = 0, r_over_tol_sum = 0;
    size_t work = 0, classification = 0, selected_pieces = 0, support_pieces = 0;
  };
  struct Fallback {
    size_t calls = 0, work = 0, hits = 0, replay_calls = 0;
    double seconds = 0, replay_seconds = 0;
  };
  struct Bin {
    size_t calls = 0, proved1 = 0, proved2 = 0, needs_full = 0, k_full = 0, k_small = 0;
    double seconds_small = 0, seconds_full = 0, seconds_needs = 0;
    void Add(const Bin& x) {
      calls += x.calls;
      proved1 += x.proved1;
      proved2 += x.proved2;
      needs_full += x.needs_full;
      k_full += x.k_full;
      k_small += x.k_small;
      seconds_small += x.seconds_small;
      seconds_full += x.seconds_full;
      seconds_needs += x.seconds_needs;
    }
    Bin Without(const Bin& x) const {
      return {calls - x.calls, proved1 - x.proved1, proved2 - x.proved2, needs_full - x.needs_full,
              k_full - x.k_full, k_small - x.k_small, std::max(0., seconds_small - x.seconds_small),
              std::max(0., seconds_full - x.seconds_full), std::max(0., seconds_needs - x.seconds_needs)};
    }
  };
  struct Fate {
    Bin all, committed, last;
    bool last_marked = false, live = false;
  };
  struct DifferenceRow {
    unsigned stage = 0;
    size_t agreement = 0, ordinal = 0, k_full = 0, k_small = 0, forward_work = 0;
    Id facet = 0, canonical = 0;
    Face vertices{};
    std::array<Point, 3> xyz{};
    BoundaryParameterState::EntityKey entity{};
    double seconds = 0, longest = 0, radius = 0, pad = 0, ratio = 0;
    bool answer = false;
    Sample sample;
  };
  struct ExceptionRow {
    std::string text;
    unsigned stage = 0;
    const char* where = nullptr;
    Outcome kind = Outcome::Threw;
    size_t count = 0;
  };

  template <class Action>
  void Observe(Action action) noexcept {
    if (!available_) return;
    const auto start = Clock::now();
    try {
      action();
    } catch (const std::exception& error) {
      Fail(error.what());
    } catch (...) {
      Fail("unknown observation exception");
    }
    seconds_observer_ += Seconds(start);
  }

  void Fail(const char* reason) noexcept {
    available_ = false;
    const auto length = std::min(std::strlen(reason), reason_.size() - 1);
    std::memcpy(reason_.data(), reason, length);
    reason_[length] = '\0';
  }

  static void PrintBits(std::ostream& out, double value) {
    out << std::hex << std::setw(16) << std::setfill('0') << M1Bits(value) << std::dec << std::setfill(' ');
  }

  auto Entity(Id facet) const {
    const auto& geometry = shadow_.Geometry();
    const auto& policy = surface_.State().Mobility();
    return BoundaryParameterState::EntityKey{geometry.Body(facet),
                                             policy.identity ? policy.sheets.at(facet) : geometry.Canonical(facet),
                                             geometry.Reference().Get(facet).marker};
  }

  void Copy(const SurfaceFace& face) {
    const auto& geometry = shadow_.Geometry();
    input_.tol = surface_.State().GeometryTolerance();
    input_.facet = geometry.Canonical(face.facet);
    input_.entity = Entity(input_.facet);
    input_.ids = face.v;
    std::rotate(input_.ids.begin(), std::min_element(input_.ids.begin(), input_.ids.end()), input_.ids.end());
    input_.direct = true;
    for (size_t k = 0; k < input_.ids.size(); ++k) {
      const auto* point = surface_.Find(input_.ids[k]);
      if (!point) throw std::logic_error("M1 face has no live surface binding.");
      input_.bindings[k] = point->binding;
      input_.points[k] = point->node.p;
      input_.direct = input_.direct && geometry.Supports(input_.facet, point->binding);
    }
    const auto distance = [](Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); };
    for (size_t k = 0; k < input_.points.size(); ++k)
      input_.longest = std::max(input_.longest, distance(input_.points[k], input_.points[(k + 1) % 3]));
    if (input_.direct) return;
    input_.anchor = geometry.Realize(input_.bindings[0]);
    for (auto point : input_.points) input_.radius = std::max(input_.radius, distance(point, input_.anchor));
    constexpr double up = std::numeric_limits<double>::infinity();
    input_.radius = std::nextafter(input_.radius + input_.tol, up);
    input_.pad1 = std::min(input_.radius, std::nextafter(2 * input_.tol, up));
    input_.pad2 = input_.radius / 4;
  }

  std::array<Point, 2> Box(double pad) const {
    Point lo = input_.points[0], hi = lo;
    for (auto p : input_.points) {
      lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
      hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    constexpr double up = std::numeric_limits<double>::infinity();
    return {Point{std::nextafter(lo.x - pad, -up), std::nextafter(lo.y - pad, -up), std::nextafter(lo.z - pad, -up)},
            Point{std::nextafter(hi.x + pad, up), std::nextafter(hi.y + pad, up), std::nextafter(hi.z + pad, up)}};
  }

  static void Nested(const std::vector<Id>& small, const std::vector<Id>& large) {
    if (!std::is_sorted(small.begin(), small.end()) || !std::is_sorted(large.begin(), large.end()) ||
        !std::includes(large.begin(), large.end(), small.begin(), small.end()))
      throw std::logic_error("M1 reference support sets are not nested in their ordered facet IDs.");
  }

  size_t Scratch() const {
    return transfer_memory::Add(sizeof(*this), transfer_memory::Bytes(input_.full), transfer_memory::Bytes(input_.small1),
                                transfer_memory::Bytes(input_.small2),
                                transfer_memory::Mul(fates_.size(), sizeof(decltype(fates_)::value_type) + 4 * sizeof(void*)),
                                exception_bytes_);
  }

  void Admit(size_t bytes) {
    ledger_peak_ = std::max(ledger_peak_, bytes);
    if (bytes == SIZE_MAX || bytes > ceiling_) throw std::length_error("M1 observer storage budget exhausted.");
  }

  Budget NewLedger(size_t extra = 0) {
    const auto scratch = transfer_memory::Add(Scratch(), extra);
    scratch_peak_ = std::max(scratch_peak_, scratch);
    const auto retained = transfer_memory::Add(shadow_.resident, scratch);
    Admit(retained);
    return native_fp2_fixture::Budget(cells_, retained, start_, 5400, ceiling_);
  }

  static bool Resource(const std::exception& error, const Budget& budget) {
    if (budget.work_exhausted || budget.time_exhausted || dynamic_cast<const std::bad_alloc*>(&error) ||
        dynamic_cast<const std::length_error*>(&error)) return true;
    const auto* message = error.what();
    return std::strstr(message, "Atlas arithmetic storage budget") ||
           std::strstr(message, "Atlas arithmetic work budget") ||
           std::strstr(message, "Atlas integer bit budget") ||
           std::strstr(message, "Curved atlas storage exhausted") ||
           std::strstr(message, "Curved atlas elapsed-time budget") ||
           std::strstr(message, "Curved atlas work exhausted");
  }

  void Exception(Sample& sample, unsigned stage, const char* where, Outcome kind, const char* text) {
    sample.outcome = kind;
    sample.where = where;
    for (size_t i = 0; i < exception_count_; ++i) {
      auto& row = exceptions_[i];
      if (row.text == text) {
        ++row.count;
        return;
      }
    }
    if (exception_count_ == exceptions_.size()) {
      ++exceptions_omitted_;
      return;
    }
    auto& row = exceptions_[exception_count_++];
    row = {text, stage, where, kind, 1};
    exception_bytes_ = transfer_memory::Add(exception_bytes_, transfer_memory::Bytes(row.text));
  }

  bool Select(double pad, std::vector<Id>& result, Sample& sample, unsigned stage, bool full) {
    const auto start = Clock::now();
    Budget budget;
    try {
      const auto& geometry = shadow_.Geometry();
      const auto n = geometry.Reference().Facets().size();
      // The house ADT keeps its own traversal workspace; two caller-side lists can coexist.
      const auto candidate = transfer_memory::Add(transfer_memory::GrowthBound(n, sizeof(unsigned long)),
                                                  transfer_memory::Mul(n, sizeof(Id)));
      selection_scratch_ = std::max(selection_scratch_, candidate);
      budget = NewLedger(candidate);
      budget.CheckTime();
      result = geometry.NearbySurfaceFacets(input_.facet, Box(pad), &budget);
      result.erase(std::remove_if(result.begin(), result.end(), [&](Id id) { return Entity(id) != input_.entity; }),
                   result.end());
      budget.CheckTime();
      sample.selected = !full;
    } catch (const std::exception& error) {
      result.clear();
      Exception(sample, stage, full ? "select_full" : "select_small",
                Resource(error, budget) ? Outcome::Resource : Outcome::Threw, error.what());
    } catch (...) {
      result.clear();
      Exception(sample, stage, full ? "select_full" : "select_small", Outcome::Threw, "unknown");
    }
    ledger_peak_ = std::max(ledger_peak_, budget.peak_bytes);
    (full ? sample.seconds_select_full : sample.seconds_select_small) += Seconds(start);
    return std::strcmp(sample.where, "none") == 0;
  }

  void Query(const std::vector<Id>& support, Sample& sample, unsigned stage, const char* where) {
    Budget budget;
    try {
      budget = NewLedger(transfer_memory::GrowthBound(1, sizeof(input_.points)));
      const auto& geometry = shadow_.Geometry();
      const auto before = geometry.GeometryStatsSnapshot();
      const auto start = Clock::now();
      try {
        sample.result = geometry.BoundPhysicalSurface(support, {input_.points}, input_.tol, budget,
            SU2NativeGeometry::MappedFacetReference::SurfaceBoundPurpose::Classification);
      } catch (...) {
        sample.seconds = Seconds(start);
        const auto after = geometry.GeometryStatsSnapshot();
        Work(sample, budget, before, after);
        throw;
      }
      sample.seconds = Seconds(start);
      Work(sample, budget, before, geometry.GeometryStatsSnapshot());
      sample.outcome = *sample.result <= input_.tol ? Outcome::Proved : Outcome::Fail;
      if (budget.work_exhausted || budget.time_exhausted)
        Exception(sample, stage, where, Outcome::Resource, "sticky atlas budget exhaustion");
    } catch (const std::exception& error) {
      Exception(sample, stage, where, Resource(error, budget) ? Outcome::Resource : Outcome::Threw, error.what());
    } catch (...) {
      Exception(sample, stage, where, Outcome::Threw, "unknown");
    }
    ledger_peak_ = std::max(ledger_peak_, budget.peak_bytes);
  }

  static void Work(Sample& sample, const Budget& budget, const Stats& before, const Stats& after) {
    sample.work = budget.work;
    sample.classification = after.classification_work - before.classification_work;
    sample.selected_pieces = after.selected_pieces - before.selected_pieces;
    sample.support_pieces = after.support_pieces - before.support_pieces;
  }

  void AccumulateStage(size_t k, bool answer) {
    auto& row = stages_[k];
    const auto& sample = sample_[k];
    ++row.calls;
    ++row.counts[static_cast<size_t>(sample.outcome)];
    row.select_full += sample.seconds_select_full;
    row.select_small += sample.seconds_select_small;
    row.seconds += sample.seconds;
    row.work += sample.work;
    row.classification += sample.classification;
    row.selected_pieces += sample.selected_pieces;
    row.support_pieces += sample.support_pieces;
    if (std::strcmp(sample.where, "select_full") == 0) {
      row.select_full_resource += sample.outcome == Outcome::Resource;
      row.select_full_threw += sample.outcome == Outcome::Threw;
    }
    if (input_.direct) return;
    const auto size = (k == 0 ? input_.small1 : input_.small2).size();
    if (selected_full_) {
      row.k_full_sum += input_.full.size();
      ++row.k_full_calls;
    }
    if (sample.selected) {
      row.k_small_sum += size;
      ++row.k_small_calls;
    }
    ++row.radius_calls;
    row.r_over_tol_sum += input_.radius / input_.tol;
    const size_t agreement = (answer ? 0 : 2) + (sample.outcome == Outcome::Proved ? 0 : 1);
    ++row.agreement[agreement];
    if (agreement == 3 || agreement == 0) return;
    const size_t bucket = agreement == 2 ? 0 : 1;
    const size_t limit = bucket == 0 ? 200 : 50;
    if (difference_count_[bucket] == limit) return;
    const size_t offset = bucket == 0 ? 0 : 200;
    auto& difference = differences_[offset + difference_count_[bucket]++];
    difference = {static_cast<unsigned>(k + 1), agreement, ordinal_, input_.full.size(), size, forward_work_,
                  active_facet_, input_.facet, input_.ids, input_.points, input_.entity, forward_seconds_, input_.longest,
                  input_.radius, k == 0 ? input_.pad1 : input_.pad2, input_.radius / input_.tol, answer, sample};
  }

  static std::pair<Face, Id> Key(const SurfaceFace& face) { return {CollapseFaceKey(face.v), face.facet}; }

  void AddBins(const SurfaceFace& face, Group group, double seconds) {
    const auto small = sample_[0].seconds + sample_[0].seconds_select_small +
                       (stage2_ ? sample_[1].seconds + sample_[1].seconds_select_small : 0.);
    const bool needs = group == Group::NeedsFull || group == Group::Direct;
    Bin bin{1, group == Group::Proved1, group == Group::Proved2, needs, input_.full.size(),
            input_.small1.size() + input_.small2.size(), small, seconds, needs ? seconds : 0.};
    const auto k = std::upper_bound(LengthBins.begin(), LengthBins.end(), input_.longest) - LengthBins.begin();
    length_[k].Add(bin);
    const auto key = Key(face);
    if (!fates_.count(key))
      Admit(transfer_memory::Add(shadow_.resident, Scratch(), sizeof(decltype(fates_)::value_type),
                                 4 * sizeof(void*)));
    auto& entry = fates_[key];
    entry.all.Add(bin);
    entry.last = bin;
    entry.last_marked = false;
    Admit(transfer_memory::Add(shadow_.resident, Scratch()));
  }

  void RecordReplica(bool answer) {
    ++replica_.attempted;
    Sample sample;
    Query(input_.full, sample, 0, "replica");
    if (sample.outcome == Outcome::Resource) ++replica_.resource;
    else if (sample.outcome == Outcome::Threw) ++replica_.threw;
    else if ((sample.outcome == Outcome::Proved) == answer) ++replica_.agree;
    else ++replica_.disagree;
  }

  void Prefix(std::ostream& out, const char* name) const {
    out << "FP2_M1 " << name << " rank=" << rank_ << " pass=" << pass_;
  }

  void Format(std::ostream& out) {
    std::array<Bin, 3> fate_totals{};
    local::VisitLivePhysicalFaces(mesh_, [&](const SurfaceFace& face, const auto&) {
      if (const auto it = fates_.find(Key(face)); it != fates_.end()) it->second.live = true;
    });
    for (const auto& item : fates_) {
      const auto& entry = item.second;
      if (entry.live) fate_totals[0].Add(entry.all);
      else {
        fate_totals[1].Add(entry.committed);
        fate_totals[2].Add(entry.all.Without(entry.committed));
      }
    }
    for (size_t k = 0; k < stages_.size(); ++k) PrintStage(out, k);
    for (size_t k = 0; k < fallback_.size(); ++k) PrintFallback(out, k);
    for (size_t k = 0; k < length_.size(); ++k) PrintBin(out, "length", k, length_[k]);
    for (size_t k = 0; k < fate_totals.size(); ++k) PrintBin(out, "fate", k + 1, fate_totals[k]);
    for (size_t k = 0; k < difference_count_[0]; ++k) PrintDifference(out, differences_[k]);
    for (size_t k = 0; k < difference_count_[1]; ++k) PrintDifference(out, differences_[200 + k]);
    for (size_t k = 0; k < exception_count_; ++k) PrintException(out, exceptions_[k]);
    if (exceptions_omitted_) {
      Prefix(out, "EXCEPTION");
      out << " kind=omitted count=" << exceptions_omitted_ << '\n';
    }
    PrintMemo(out, "start", "pass", initial_pass_);
    PrintMemo(out, "end", "pass", surface_.State().Geometry().GeometryStatsSnapshot());
    PrintMemo(out, "start", "shadow", initial_shadow_);
    PrintMemo(out, "end", "shadow", shadow_.Geometry().GeometryStatsSnapshot());
    if (trace_fixture_) {
      Prefix(out, "REPLICA");
      out << " attempted=" << replica_.attempted << " agree=" << replica_.agree << " disagree=" << replica_.disagree
          << " resource=" << replica_.resource << " threw=" << replica_.threw << '\n';
    }
  }

  void PrintStage(std::ostream& out, size_t k) const {
    const auto& row = stages_[k];
    Prefix(out, "SUPPORT");
    out << " stage=" << k + 1 << " interleaved=0 calls=" << row.calls;
    for (size_t i = 0; i < row.counts.size(); ++i) out << ' ' << OutcomeNames[i] << '=' << row.counts[i];
    out << " k_full_sum=" << row.k_full_sum << " k_full_calls=" << row.k_full_calls
        << " k_small_sum=" << row.k_small_sum << " k_small_calls=" << row.k_small_calls
        << " seconds_select_small=" << row.select_small << " seconds_select_full=" << row.select_full
        << " seconds_small=" << row.seconds << " work_small=" << row.work
        << " classification_work_small=" << row.classification << " selected_pieces_small=" << row.selected_pieces
        << " support_pieces_small=" << row.support_pieces << " radius_calls=" << row.radius_calls
        << " r_over_tol_sum=" << row.r_over_tol_sum << " select_full_resource=" << row.select_full_resource
        << " select_full_threw=" << row.select_full_threw;
    for (size_t i = 0; i < row.agreement.size(); ++i)
      out << ' ' << AgreementNames[i] << '=' << row.agreement[i];
    out << '\n';
    for (size_t i : {size_t{2}, size_t{1}}) {
      Prefix(out, "DIFF_TOTAL");
      out << " stage=" << k + 1 << " case=" << AgreementNames[i] << " count=" << row.agreement[i] << '\n';
    }
  }

  void PrintFallback(std::ostream& out, size_t k) const {
    const auto& row = fallback_[k];
    Prefix(out, "FALLBACK");
    out << " group=" << GroupNames[k] << " calls=" << row.calls << " seconds_full=" << row.seconds
        << " work_full=" << row.work << " full_memo_hits=" << row.hits << " replay_calls=" << row.replay_calls
        << " replay_seconds_full=" << row.replay_seconds << '\n';
  }

  void PrintBin(std::ostream& out, const char* kind, size_t k, const Bin& row) const {
    Prefix(out, "SUPPORT_BIN");
    out << " kind=" << kind << " bin=" << k << " calls=" << row.calls << " proved_1=" << row.proved1
        << " proved_2=" << row.proved2 << " needs_full=" << row.needs_full << " seconds_small=" << row.seconds_small
        << " seconds_full=" << row.seconds_full << " seconds_full_needs_full=" << row.seconds_needs
        << " k_full_sum=" << row.k_full << " k_small_sum=" << row.k_small << '\n';
  }

  void PrintDifference(std::ostream& out, const DifferenceRow& row) const {
    Prefix(out, "DIFF");
    out << " stage=" << row.stage << " case=" << AgreementNames[row.agreement] << " ordinal=" << row.ordinal;
    PrintLabEntity(out, row.entity);
    out << " facet=" << row.facet << " canonical=" << row.canonical << " vertices=";
    PrintLabTriple(out, row.vertices);
    for (size_t k = 0; k < row.xyz.size(); ++k) {
      out << " xyz" << k << '=';
      PrintLabPoint(out, row.xyz[k]);
    }
    out << " seconds=" << row.seconds << " answer=" << row.answer << " longest_edge=" << row.longest
        << " direct=0 classification_work=" << row.forward_work << " classification_work_small=" << row.sample.classification
        << " corner_order=canonical_cyclic k_full=" << row.k_full << " k_small=" << row.k_small << " result_small=";
    if (row.sample.result) out << *row.sample.result;
    else out << "not_available";
    out << " outcome=" << OutcomeNames[static_cast<size_t>(row.sample.outcome)] << " where=" << row.sample.where
        << " r=" << row.radius << " pad=" << row.pad << " r_over_tol=" << row.ratio << '\n';
  }

  void PrintException(std::ostream& out, const ExceptionRow& row) const {
    Prefix(out, "EXCEPTION");
    out << " stage=" << row.stage << " where=" << row.where << " kind=" << OutcomeNames[static_cast<size_t>(row.kind)]
        << " count=" << row.count << " text=";
    M1PrintToken(out, row.text.c_str());
    out << '\n';
  }

  void PrintMemo(std::ostream& out, const char* at, const char* authority, const Stats& stats) const {
    Prefix(out, "MEMO");
    out << " at=" << at << " authority=" << authority << " hits=" << stats.memo_union_hits + stats.memo_direct_hits
        << " misses=" << stats.memo_union_misses + stats.memo_direct_misses
        << " entries=" << stats.memo_union_entries + stats.memo_direct_entries << " evictions=" << stats.memo_evictions
        << " retention_refusals=" << stats.memo_retention_refusals << " bytes=" << stats.memo_bytes
        << " memo_limit=" << stats.memo_limit << " prepared_limit=" << stats.prepared_limit << '\n';
  }

  const local::Mesh& mesh_;
  const local::StateSurfaceGeometry& surface_;
  const LabM1Shadow& shadow_;
  const size_t cells_, ceiling_;
  const Clock::time_point start_;
  const int rank_;
  const unsigned pass_;
  const bool trace_fixture_;
  bool available_ = true, selected_full_ = false, stage2_ = false, trace_active_ = false;
  size_t ordinal_ = 0, scratch_peak_ = 0, ledger_peak_ = 0, selection_scratch_ = 0, exception_bytes_ = 0,
         forward_work_ = 0;
  std::array<char, 256> reason_{};
  double seconds_observer_ = 0, forward_seconds_ = 0;
  Id active_facet_ = 0;
  Stats initial_pass_, initial_shadow_, forward_before_;
  CopiedInput input_;
  std::array<Sample, 2> sample_{};
  std::array<Stage, 2> stages_{};
  std::array<Fallback, static_cast<size_t>(Group::Count)> fallback_{};
  std::array<Bin, LengthBins.size() + 1> length_{};
  std::map<std::pair<Face, Id>, Fate> fates_;
  std::array<DifferenceRow, 250> differences_{};
  std::array<size_t, 2> difference_count_{};
  std::array<ExceptionRow, 5> exceptions_{};
  size_t exception_count_ = 0, exceptions_omitted_ = 0;
  ReplicaCounts replica_;
};
