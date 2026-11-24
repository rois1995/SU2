/*!
 * \file native_m2_tables.inc
 * \brief FP2-M2 serial hand-over exports after the inherited lab checks.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

unsigned M2Bin(int marker, Point point) {
  if (marker != 2) return 6;
  const auto [eta, chord] = WingPlanform(point);
  return static_cast<unsigned>(WingDistanceBin(eta, chord).value_or(5));
}

Point M2Midpoint(Point a, Point b) { return {(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2}; }
Point M2Difference(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
double M2Norm(Point p) { return std::hypot(p.x, p.y, p.z); }
double M2Distance(Point a, Point b) { return M2Norm(M2Difference(a, b)); }

Point M2Cross(Point a, Point b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Point M2Centroid(const std::array<Point, 3>& p) {
  return {(p[0].x + p[1].x + p[2].x) / 3, (p[0].y + p[1].y + p[2].y) / 3,
          (p[0].z + p[1].z + p[2].z) / 3};
}

std::optional<Point> M2Unit(Point p) {
  const double length = M2Norm(p);
  if (!(length > 0 && std::isfinite(length))) return std::nullopt;
  return Point{p.x / length, p.y / length, p.z / length};
}

std::optional<double> M2Angle(Point a, Point b) {
  const auto u = M2Unit(a), v = M2Unit(b);
  if (!u || !v) return std::nullopt;
  const auto dot = u->x * v->x + u->y * v->y + u->z * v->z;
  return std::atan2(M2Norm(M2Cross(*u, *v)), dot) * 180 / std::acos(-1.);
}

Edge M2Edge(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }
void M2CsvPoint(std::ostream& out, Point p) { out << ',' << p.x << ',' << p.y << ',' << p.z; }

void M2CsvEntity(std::ostream& out, const BoundaryParameterState::EntityKey& key) {
  const auto& [body, sheet, marker] = key;
  out << ',' << body << ',' << sheet << ',' << marker;
}

template <class T>
void M2CsvValue(std::ostream& out, const std::optional<T>& value) {
  out << ',';
  if (value) out << *value;
}

template <class Map>
size_t M2MapBytes(size_t rows) {
  return transfer_memory::Mul(rows, sizeof(typename Map::value_type) + 4 * sizeof(void*));
}

void M2CheckStorage(size_t ceiling, size_t bytes) {
  if (bytes == SIZE_MAX || bytes > ceiling) throw std::length_error("FP2-M2 geometry table storage exhausted.");
}

void M2CheckDeadline(Clock::time_point start) {
  if (Seconds(start) >= 5400.) throw std::length_error("FP2-M2 geometry table deadline exhausted.");
}

void M2Output(std::ofstream& out, const std::string& path) {
  out.exceptions(std::ios::failbit | std::ios::badbit);
  out.imbue(std::locale::classic());
  out.open(path);
  out << std::setprecision(17);
}

class LabM2Tables {
 public:
  explicit LabM2Tables(Clock::time_point start) noexcept : start_(start) {}

  double Capture(const local::Mesh& mesh, size_t ceiling, size_t retained_bytes) noexcept {
    const auto begin = Clock::now();
    Observe([&] {
      size_t physical_faces = 0, live_surface_points = 0;
      local::VisitLivePhysicalFaces(mesh, [&](const auto&, const auto&) { ++physical_faces; });
      for (const auto& point : mesh.Points()) live_surface_points += point.surface && !point.removed;
      std::map<Id, local::Slot> slots;
      M2CheckStorage(ceiling, transfer_memory::Add(retained_bytes,
            M2MapBytes<MetricMap>(transfer_memory::Mul(physical_faces, 3)),
            M2MapBytes<decltype(slots)>(live_surface_points)));
      for (local::Slot slot = 0; slot < mesh.Points().size(); ++slot) {
        const auto& point = mesh.Points()[slot];
        if (point.surface && !point.removed) slots.emplace(point.node.id, slot);
      }
      local::VisitLivePhysicalFaces(mesh, [&](const SurfaceFace& face, const auto&) {
        M2CheckDeadline(start_);
        for (size_t i = 0; i < 3; ++i) {
          const auto edge = M2Edge(face.v[i], face.v[(i + 1) % 3]);
          auto [it, inserted] = metric_.try_emplace(edge, 0);
          if (inserted) it->second = mesh.Length({slots.at(edge[0]), slots.at(edge[1])});
        }
      });
      captured_ = true;
    });
    return Seconds(begin);
  }

  void Report(const ReferenceState& reference, size_t cells, size_t ceiling, size_t retained_bytes, int rank) noexcept {
    const auto begin = Clock::now();
    Observe([&] {
      if (!captured_ || !rebuilt) throw std::logic_error("FP2-M2 hand-over metric lengths or rebuilt state missing.");
      const auto& state = *rebuilt;
      const auto n = state.Surface().size();
      const auto model_facets = state.Geometry().Reference().Facets().size();
      const auto corners = state.Records().size();
      using EdgeSet = std::set<Edge>;
      EdgeSet edges;
      FaceIndex faces;
      const auto kept = transfer_memory::Add(retained_bytes, rebuilt->ResidentBytes(),
          M2MapBytes<MetricMap>(metric_.size()), M2MapBytes<decltype(bounds)>(bounds.size()),
          M2MapBytes<decltype(answers)>(answers.size()));
      const auto table_bytes = transfer_memory::Add(M2MapBytes<FaceIndex>(n),
          M2MapBytes<EdgeSet>(transfer_memory::Mul(3, n)), transfer_memory::GrowthBound(model_facets, sizeof(Id)),
          size_t{1} << 20);
      const auto ledger = transfer_memory::Add(kept, table_bytes);
      // Pre-admit the stencil and transient adjacency maps, and the two point-key indexes.
      const auto construction = transfer_memory::Add(transfer_memory::Mul(1024, model_facets),
                                                     transfer_memory::Mul(256, corners));
      M2CheckStorage(ceiling, transfer_memory::Add(ledger, construction));
      M2CheckDeadline(start_);
      const auto view = reference.original->WithParameterState(rebuilt);
      auto budget = native_fp2_fixture::Budget(cells, ledger, start_, 5400., ceiling);
      local::StateSurfaceGeometry sampler(view, std::move(budget));
      M2CheckStorage(ceiling, transfer_memory::Add(ledger, sampler.ResidentBytes()));
      const auto tag = ToleranceTag(state.GeometryTolerance());
      std::ofstream face_file, edge_file;
      M2Output(face_file, "handover_faces_" + tag + ".csv");
      M2Output(edge_file, "handover_edges_" + tag + ".csv");
      FaceHeader(face_file);
      EdgeHeader(edge_file);
      for (const auto& [key, face] : state.Surface()) {
        M2CheckDeadline(start_);
        const auto triangle = LabTriangle(state, face);
        const auto entity = state.Entity(face.facet);
        const auto cross = M2Cross(M2Difference(triangle[1], triangle[0]), M2Difference(triangle[2], triangle[0]));
        const auto normal = M2Unit(cross);
        faces.emplace(key, FaceRecord{faces_written_, normal, entity});
        for (size_t k = 0; k < 3; ++k) edges.insert(M2Edge(face.v[k], face.v[(k + 1) % 3]));
        FaceRow(face_file, state, sampler, key, face, triangle, entity, normal, M2Norm(cross) / 2);
        ++faces_written_;
      }
      for (const auto& edge : edges) {
        M2CheckDeadline(start_);
        EdgeRow(edge_file, state, faces, edge);
        ++edges_written_;
      }
      face_file.close();
      edge_file.close();
    });
    try {
      std::cout << "FP2_M2 TABLE rank=" << rank << " status=" << (status_ ? "complete" : "incomplete")
                << " faces=" << faces_written_ << " edges=" << edges_written_ << " corners_ok=" << corners_ok_
                << " corners_none=" << corners_none_ << " edges_without_metric=" << edges_without_metric_
                << " seconds=" << std::setprecision(17) << Seconds(begin);
      if (!status_) {
        std::cout << " reason=";
        for (const unsigned char c : reason_) {
          if (!c) break;
          std::cout << (std::isspace(c) || std::iscntrl(c) || c == '=' ? '_' : static_cast<char>(c));
        }
      }
      std::cout << '\n' << std::flush;
    } catch (...) {
    }
  }

  std::shared_ptr<BoundaryParameterState> rebuilt;
  std::map<Face, double> bounds;
  LabSurfaceAnswers answers;

 private:
  using MetricMap = std::map<Edge, double>;
  using Eval = SU2NativeGeometry::MappedFacetReference::SurfaceEvaluation;
  using Entity = BoundaryParameterState::EntityKey;
  struct FaceRecord {
    size_t ordinal;
    std::optional<Point> normal;
    Entity entity;
  };
  using FaceIndex = std::map<Face, FaceRecord>;

  template <class Work>
  void Observe(Work work) noexcept {
    if (!status_) return;
    try {
      work();
    } catch (const std::exception& error) {
      Fail(error.what());
    } catch (...) {
      Fail("Unknown geometry table error");
    }
  }

  void Fail(const char* message) noexcept {
    status_ = false;
    const auto n = std::min(std::strlen(message), reason_.size() - 1);
    std::memcpy(reason_.data(), message, n);
  }

  static std::string ToleranceTag(double tolerance) {
    if (tolerance == 1e-4) return "1E-4";
    if (tolerance == 2e-4) return "2E-4";
    if (tolerance == 5e-4) return "5E-4";
    if (tolerance == 1e-3) return "1E-3";
    throw std::logic_error("FP2-M2 unexpected tolerance tag");
  }

  static void FaceHeader(std::ostream& out) {
    out << "ordinal,v0,v1,v2,body,sheet,marker,facet,bin,centroid_x,centroid_y,centroid_z,area,normal_x,normal_y,normal_z"
           ",length0,length1,length2,metric0,metric1,metric2,m_max,sample0,sample1,sample2,sample3"
           ",finite0,finite1,finite2,finite3,unconverged,sample_max,strict_within,rebuilt_classification_bound";
    for (size_t i = 0; i < 3; ++i)
      out << ",side" << i << ",eval" << i << ",model_normal" << i << "_x,model_normal" << i << "_y,model_normal" << i
          << "_z,curvature1_" << i << ",curvature2_" << i << ",direction1_" << i << "_x,direction1_" << i
          << "_y,direction1_" << i << "_z";
    out << ",normal_dev_deg\n";
  }

  static void EdgeHeader(std::ostream& out) {
    out << "v0,v1,face0,body0,sheet0,marker0,face1,body1,sheet1,marker1,feature,class,length,metric,turn_deg,bin\n";
  }

  std::optional<double> Metric(Edge edge) const {
    const auto it = metric_.find(edge);
    return it == metric_.end() ? std::nullopt : std::optional<double>{it->second};
  }

  std::optional<std::pair<Id, Eval>> Corner(const BoundaryParameterState& state, const SurfaceFace& face,
                                            const Entity& entity, Id point) {
    const auto& geometry = state.Geometry();
    const auto& binding = state.Find(point)->binding;
    std::vector<Id> candidates;
    geometry.ForFacets(binding, [&](Id facet) {
      if (state.Entity(facet) == entity) candidates.push_back(facet);
    });
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    const auto inherited = std::lower_bound(candidates.begin(), candidates.end(), face.facet);
    if (inherited != candidates.end() && *inherited == face.facet)
      std::rotate(candidates.begin(), inherited, inherited + 1);
    for (const Id candidate : candidates) {
      M2CheckDeadline(start_);
      Eval value;
      try {
        value = geometry.EvaluateSurface(candidate, binding);
      } catch (const std::bad_alloc&) {
        throw;
      } catch (const std::exception&) {
        continue;
      }
      M2CheckDeadline(start_);
      return std::make_pair(candidate, value);
    }
    M2CheckDeadline(start_);
    return std::nullopt;
  }

  void FaceRow(std::ostream& out, const BoundaryParameterState& state, local::StateSurfaceGeometry& sampler,
               const Face& key, const SurfaceFace& face, const std::array<Point, 3>& triangle, const Entity& entity,
               const std::optional<Point>& normal, double area) {
    const auto centroid = M2Centroid(triangle);
    out << faces_written_;
    for (Id vertex : face.v) out << ',' << vertex;
    M2CsvEntity(out, entity);
    out << ',' << face.facet << ',' << M2Bin(std::get<2>(entity), centroid);
    M2CsvPoint(out, centroid);
    out << ',' << area;
    if (normal) M2CsvPoint(out, *normal);
    else out << ",,,";
    for (size_t i = 0; i < 3; ++i) out << ',' << M2Distance(triangle[i], triangle[(i + 1) % 3]);
    std::array<std::optional<double>, 3> metric;
    for (size_t i = 0; i < 3; ++i) {
      metric[i] = Metric(M2Edge(face.v[i], face.v[(i + 1) % 3]));
      M2CsvValue(out, metric[i]);
    }
    if (metric[0] && metric[1] && metric[2]) out << ',' << std::max({*metric[0], *metric[1], *metric[2]});
    else out << ',';

    const auto samples = sampler.Samples(face);
    std::optional<double> sample_max;
    for (double sample : samples.distances) {
      out << ',';
      if (!std::isfinite(sample)) continue;
      out << sample;
      sample_max = sample_max ? std::max(*sample_max, sample) : sample;
    }
    for (double sample : samples.distances) out << ',' << std::isfinite(sample);
    out << ',' << samples.unconverged;
    M2CsvValue(out, sample_max);
    const auto answer = answers.find(key);
    out << ',';
    if (answer != answers.end()) out << answer->second;
    const auto bound = bounds.find(key);
    out << ',';
    if (bound != bounds.end()) out << bound->second;

    std::optional<double> normal_dev;
    for (Id point : face.v) {
      const auto corner = Corner(state, face, entity, point);
      if (!corner) {
        ++corners_none_;
        out << ",,none,,,,,,,,";
        continue;
      }
      ++corners_ok_;
      const auto& [side, value] = *corner;
      out << ',' << side << ",ok";
      M2CsvPoint(out, value.normal);
      out << ',' << value.curvature1 << ',' << value.curvature2;
      M2CsvPoint(out, value.direction1);
      if (normal) {
        const auto angle = M2Angle(*normal, value.normal);
        if (angle) normal_dev = normal_dev ? std::max(*normal_dev, *angle) : *angle;
      }
    }
    M2CsvValue(out, normal_dev);
    out << '\n';
  }

  void EdgeRow(std::ostream& out, const BoundaryParameterState& state, const FaceIndex& faces, Edge edge) {
    const auto incident = state.IncidentFaces(edge);
    std::array<const FaceRecord*, 2> records{};
    out << edge[0] << ',' << edge[1];
    for (size_t i = 0; i < 2; ++i) {
      if (!incident[i]) {
        out << ",,,,";
        continue;
      }
      records[i] = &faces.at(CollapseFaceKey(incident[i]->v));
      out << ',' << records[i]->ordinal;
      M2CsvEntity(out, records[i]->entity);
    }
    const bool open = !records[0] || !records[1];
    const bool feature = state.Coverage().traces.count(edge) != 0;
    const char* kind = open ? "open" : feature ? "feature" :
                       records[0]->entity != records[1]->entity ? "entity_border" : "interior";
    const Point a = state.Find(edge[0])->node.p, b = state.Find(edge[1])->node.p;
    out << ',' << feature << ',' << kind << ',' << M2Distance(a, b);
    const auto metric = Metric(edge);
    edges_without_metric_ += !metric;
    M2CsvValue(out, metric);
    std::optional<double> turn;
    if (!open && records[0]->normal && records[1]->normal)
      turn = M2Angle(*records[0]->normal, *records[1]->normal);
    M2CsvValue(out, turn);
    unsigned bin = 6;
    if (!open && std::get<2>(records[0]->entity) == std::get<2>(records[1]->entity))
      bin = M2Bin(std::get<2>(records[0]->entity), M2Midpoint(a, b));
    out << ',' << bin << '\n';
  }

  Clock::time_point start_;
  MetricMap metric_;
  std::array<char, 200> reason_{};
  size_t faces_written_ = 0, edges_written_ = 0, corners_ok_ = 0, corners_none_ = 0, edges_without_metric_ = 0;
  bool status_ = true, captured_ = false;
};
