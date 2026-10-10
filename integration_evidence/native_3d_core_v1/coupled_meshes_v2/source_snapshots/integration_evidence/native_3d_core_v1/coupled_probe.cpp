/* Bounded serial full-mesh probe of production private operators; not an SU2 driver or MPI engine. */
#include "Common/include/adaptation/CNativeBoundary3D.hpp"
#include "Common/include/adaptation/CNativeMetric3D.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <set>
using namespace SU2Native3D;
namespace {
using Clock = std::chrono::steady_clock;
double Seconds(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }
struct Phases {
  double field_build = 0, selection = 0, geometry = 0, metric = 0, commit = 0, final_gate = 0, output = 0;
};
struct History {
  Edge endpoints;
  Node point;
};
std::ofstream Open(const std::string& path) {
  std::ofstream out;
  out.exceptions(std::ios::failbit | std::ios::badbit);
  out.open(path);
  out << std::setprecision(21);
  return out;
}
Patch Cube(double height, bool shear) {
  std::array<Node, 8> v{{{0, {0, 0, 0}},
                         {1, {1, 0, 0}},
                         {2, {1, 1, 0}},
                         {3, {0, 1, 0}},
                         {4, {0, 0, height}},
                         {5, {1, 0, height}},
                         {6, {1, 1, height}},
                         {7, {0, 1, height}}}};
  if (shear)
    for (auto& n : v) n.p.x += n.p.y;
  Patch patch;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < 6; ++i) {
    Cell c{Id(i), {v[0], v[6], v[ring[i]], v[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    patch.cells.push_back(c);
  }
  const auto skin = Incidence(patch.cells).Boundary();
  for (size_t i = 0; i < skin.size(); ++i) patch.surface.push_back({skin[i], Id(i)});
  return patch;
}
std::map<Id, Node> Nodes(const Patch& patch) {
  std::map<Id, Node> nodes;
  for (const auto& c : patch.cells)
    for (auto n : c.v) nodes.emplace(n.id, n);
  return nodes;
}
std::vector<Facet> Facets(const Patch& patch) {
  const auto nodes = Nodes(patch);
  std::vector<Facet> facets;
  for (const auto& f : patch.surface) {
    int marker = -1;
    const auto a = nodes.at(f.v[0]).p, b = nodes.at(f.v[1]).p, c = nodes.at(f.v[2]).p;
    if (a.z == b.z && b.z == c.z)
      marker = a.z == 0 ? 0 : 1;
    else if (a.y == b.y && b.y == c.y)
      marker = a.y == 0 ? 2 : 3;
    else
      marker = 4 + (a.x - a.y > 0 || b.x - b.y > 0 || c.x - c.y > 0);
    facets.push_back({f.facet, marker, {nodes.at(f.v[0]), nodes.at(f.v[1]), nodes.at(f.v[2])}});
  }
  return facets;
}
Patch Star(const Patch& patch, Edge edge, Id vertex = std::numeric_limits<Id>::max()) {
  Patch result;
  for (const auto& c : patch.cells) {
    size_t ends = 0;
    bool has_vertex = false;
    for (auto n : c.v) {
      ends += n.id == edge[0] || n.id == edge[1];
      has_vertex = has_vertex || n.id == vertex;
    }
    if (vertex == std::numeric_limits<Id>::max() ? ends == 2 : has_vertex) result.cells.push_back(c);
  }
  const auto skin = Incidence(result.cells).Boundary();
  for (const auto& f : patch.surface)
    if (std::find(skin.begin(), skin.end(), f.v) != skin.end()) result.surface.push_back(f);
  return result;
}
void Replace(Patch& mesh, const Patch& old, const Patch& fresh) {
  std::set<Id> cells;
  std::set<Face> faces;
  for (const auto& c : old.cells) cells.insert(c.id);
  for (const auto& f : old.surface) faces.insert(f.v);
  Patch next;
  for (const auto& c : mesh.cells)
    if (!cells.count(c.id)) next.cells.push_back(c);
  for (const auto& f : mesh.surface)
    if (!faces.count(f.v)) next.surface.push_back(f);
  next.cells.insert(next.cells.end(), fresh.cells.begin(), fresh.cells.end());
  next.surface.insert(next.surface.end(), fresh.surface.begin(), fresh.surface.end());
  const Incidence checked(next.cells);
  (void)checked;
  mesh.cells.swap(next.cells);
  mesh.surface.swap(next.surface);
}
Tensor Target(Point p, double height, bool shear, bool spatial, bool coarse) {
  const double level = coarse ? 1 : 4;
  if (shear) return {level, -level, 0, 2 * level, 0, level / (height * height)};
  if (!spatial) return {level, 0, 0, level, 0, level / (height * height)};
  const double c = std::cos(.37), s = std::sin(.37), x = 4 + .4 * p.x, y = 2 + .4 * p.y, z = 4 + .4 * p.z;
  return {x * c * c + y * s * s, (x - y) * c * s, 0, x * s * s + y * c * c, 0, z};
}
std::vector<DonorCell> Donors(const Patch& patch, double height, bool shear, bool spatial, bool coarse) {
  std::vector<DonorCell> donors;
  for (const auto& c : patch.cells) {
    DonorCell d;
    d.cell = c;
    for (size_t i = 0; i < 4; ++i) d.sensor[i] = Target(c.v[i].p, height, shear, spatial && !coarse, coarse);
    donors.push_back(d);
  }
  return donors;
}
void TensorOut(std::ostream& out, Tensor m) {
  out << '[' << m.xx << ',' << m.xy << ',' << m.xz << ',' << m.yy << ',' << m.yz << ',' << m.zz << ']';
}
void Write(const std::string& path, const Patch& patch, const std::vector<Facet>& facets,
           const std::vector<DonorCell>& original) {
  auto out = Open(path);
  const auto nodes = Nodes(patch);
  out << "{\"nodes\":[";
  bool comma = false;
  for (auto n : nodes) {
    if (comma) out << ',';
    comma = true;
    out << '[' << n.first << ',' << n.second.p.x << ',' << n.second.p.y << ',' << n.second.p.z << ']';
  }
  out << "],\"cells\":[";
  comma = false;
  for (const auto& c : patch.cells) {
    if (comma) out << ',';
    comma = true;
    out << "{\"id\":" << c.id << ",\"nodes\":[";
    for (size_t i = 0; i < 4; ++i) {
      if (i) out << ',';
      out << c.v[i].id;
    }
    out << "]}";
  }
  out << "],\"surface\":[";
  comma = false;
  for (const auto& f : patch.surface) {
    if (comma) out << ',';
    comma = true;
    out << "{\"facet\":" << f.facet << ",\"nodes\":[" << f.v[0] << ',' << f.v[1] << ',' << f.v[2] << "]}";
  }
  out << "],\"facets\":[";
  comma = false;
  for (const auto& f : facets) {
    if (comma) out << ',';
    comma = true;
    out << "{\"id\":" << f.id << ",\"marker\":" << f.marker << ",\"vertices\":[";
    for (size_t i = 0; i < 3; ++i) {
      if (i) out << ',';
      auto n = f.v[i];
      out << '[' << n.id << ',' << n.p.x << ',' << n.p.y << ',' << n.p.z << ']';
    }
    out << "]}";
  }
  out << "],\"original\":[";
  comma = false;
  for (const auto& d : original) {
    if (comma) out << ',';
    comma = true;
    out << "{\"vertices\":[";
    for (size_t i = 0; i < 4; ++i) {
      if (i) out << ',';
      auto n = d.cell.v[i];
      out << '[' << n.id << ',' << n.p.x << ',' << n.p.y << ',' << n.p.z << ',';
      TensorOut(out, d.sensor[i]);
      out << ']';
    }
    out << "]}";
  }
  out << "]}\n";
}
void Case(const std::string& folder, const std::string& name, double height, bool shear, bool spatial,
          std::ostream& summary, bool comma) {
  const auto parent = Clock::now();
  Phases phases;
  const Patch original = Cube(height, shear);
  Patch mesh = original;
  const auto facets = Facets(original);
  const PlanarReference reference(facets);
  auto tick = Clock::now();
  const auto fine_donors = Donors(original, height, shear, spatial, false);
  FrozenField fine(fine_donors, {}, true);
  phases.field_build += Seconds(tick);
  tick = Clock::now();
  Write(folder + '/' + name + "_initial.json", mesh, facets, fine_donors);
  phases.output += Seconds(tick);
  std::vector<History> history;
  Id next_point = 8, next_cell = 100;
  size_t rejections = 0, attempts = 0;
  MetricStats stats;
  KernelStats geometry;
  std::string reason;
  PatchMetric global, local;
  bool completed = false;
  for (size_t sweep = 0; sweep < 96; ++sweep) {
    tick = Clock::now();
    const bool valid = ValidateMetricPatch(mesh.cells, fine, global, reason, &stats);
    phases.selection += Seconds(tick);
    if (valid) {
      completed = true;
      break;
    }
    if (mesh.cells.size() > MaximumReplacementCells)
      throw std::runtime_error(name + ": whole-probe cell budget exceeded");
    std::vector<std::pair<Edge, EdgeMeasure>> requests(global.edges.begin(), global.edges.end());
    std::sort(requests.begin(), requests.end(), [](const auto& a, const auto& b) {
      return a.second.length == b.second.length ? a.first < b.first : a.second.length > b.second.length;
    });
    bool changed = false;
    for (const auto& request : requests) {
      if (request.second.length <= MaximumMetricEdge) break;
      tick = Clock::now();
      const auto old = Star(mesh, request.first);
      phases.selection += Seconds(tick);
      const auto& a = request.second.a;
      const auto& b = request.second.b;
      const Node point{
          next_point,
          {double((static_cast<long double>(a.x) + b.x) / 2), double((static_cast<long double>(a.y) + b.y) / 2),
           double((static_cast<long double>(a.z) + b.z) / 2)}};
      Patch fresh;
      ++attempts;
      tick = Clock::now();
      bool accepted = SplitEdge(old, request.first, point, next_cell, reference, fresh, reason, &geometry);
      phases.geometry += Seconds(tick);
      if (accepted) {
        tick = Clock::now();
        accepted = ValidateMetricChange(old.cells, fresh.cells, MetricChange::Refine, fine, local, reason, &stats);
        phases.metric += Seconds(tick);
      }
      if (!accepted) {
        ++rejections;
        continue;
      }
      next_cell += fresh.cells.size();
      ++next_point;
      tick = Clock::now();
      Replace(mesh, old, fresh);
      history.push_back({request.first, point});
      phases.commit += Seconds(tick);
      changed = true;
      break;
    }
    if (!changed) {
      Write(folder + '/' + name + "_rejected.json", mesh, facets, fine_donors);
      throw std::runtime_error(name + ": stagnation, last rejection: " + reason);
    }
  }
  if (!completed) throw std::runtime_error(name + ": sweep budget exceeded");
  const auto final_cells = mesh.cells.size(), final_points = Nodes(mesh).size(), final_faces = mesh.surface.size();
  tick = Clock::now();
  Write(folder + '/' + name + "_adapted.json", mesh, facets, fine_donors);
  phases.output += Seconds(tick);
  tick = Clock::now();
  const auto coarse_donors = Donors(mesh, height, shear, spatial, true);
  FrozenField coarse(coarse_donors, {}, true);
  phases.field_build += Seconds(tick);
  for (auto it = history.rbegin(); it != history.rend(); ++it) {
    tick = Clock::now();
    const auto old = Star(mesh, it->endpoints, it->point.id);
    phases.selection += Seconds(tick);
    Patch fresh;
    tick = Clock::now();
    const bool geometry_ok =
        UndoEdgeSplit(old, it->point.id, it->endpoints, next_cell, reference, fresh, reason, &geometry);
    phases.geometry += Seconds(tick);
    if (!geometry_ok) throw std::runtime_error(name + ": inverse geometry: " + reason);
    tick = Clock::now();
    const bool metric_ok =
        ValidateMetricChange(old.cells, fresh.cells, MetricChange::Coarsen, coarse, local, reason, &stats);
    phases.metric += Seconds(tick);
    if (!metric_ok) throw std::runtime_error(name + ": inverse metric: " + reason);
    next_cell += fresh.cells.size();
    tick = Clock::now();
    Replace(mesh, old, fresh);
    phases.commit += Seconds(tick);
  }
  tick = Clock::now();
  const bool final_ok = ValidateMetricPatch(mesh.cells, coarse, local, reason, &stats);
  phases.final_gate += Seconds(tick);
  if (!final_ok) throw std::runtime_error(name + ": coarsened final: " + reason);
  if (mesh.cells.size() != original.cells.size() || Nodes(mesh).size() != 8 || mesh.surface.size() != 12)
    throw std::runtime_error(name + ": inverse did not restore mesh counts");
  tick = Clock::now();
  Write(folder + '/' + name + "_coarsened.json", mesh, facets, coarse_donors);
  phases.output += Seconds(tick);
  const double elapsed = Seconds(parent), exclusive = phases.field_build + phases.selection + phases.geometry +
                                                      phases.metric + phases.commit + phases.final_gate + phases.output;
  if (exclusive > elapsed) throw std::runtime_error("Probe phase accounting does not close");
  if (comma) summary << ',';
  summary << std::setprecision(17) << "{\"case\":\"" << name << "\",\"height\":" << height
          << ",\"initial_cells\":6,\"adapted_cells\":" << final_cells << ",\"adapted_points\":" << final_points
          << ",\"adapted_physical_faces\":" << final_faces << ",\"accepted_splits\":" << history.size()
          << ",\"accepted_coarsenings\":" << history.size() << ",\"attempts\":" << attempts
          << ",\"rejections\":" << rejections << ",\"final_q\":" << global.minimum_mean_ratio
          << ",\"final_J\":" << global.minimum_scaled_jacobian << ",\"final_L\":" << global.maximum_length
          << ",\"metric_edge_evaluations\":" << stats.edge_evaluations
          << ",\"metric_edge_reused\":" << stats.edge_reused << ",\"max_patch_cells\":" << stats.maximum_cells
          << ",\"max_patch_nodes\":" << stats.maximum_nodes << ",\"max_patch_edges\":" << stats.maximum_edges
          << ",\"parent_seconds\":" << elapsed << ",\"field_build_seconds\":" << phases.field_build
          << ",\"selection_seconds\":" << phases.selection << ",\"geometry_seconds\":" << phases.geometry
          << ",\"metric_seconds\":" << phases.metric << ",\"commit_seconds\":" << phases.commit
          << ",\"final_gate_seconds\":" << phases.final_gate << ",\"output_seconds\":" << phases.output
          << ",\"other_seconds\":" << elapsed - exclusive
          << ",\"fine_query_seconds\":" << fine.Statistics().query_seconds
          << ",\"fine_edge_seconds\":" << fine.Statistics().edge_seconds
          << ",\"fine_edge_trace_seconds\":" << fine.Statistics().edge_trace_seconds
          << ",\"coarse_edge_seconds\":" << coarse.Statistics().edge_seconds << '}';
  std::cout << name << ": 6 -> " << final_cells << " -> 6 tetrahedra, 12 -> " << final_faces
            << " -> 12 wall triangles\n";
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    auto summary = Open(std::string(argv[1]) + "/probe_summary.json");
    summary << '[';
    Case(argv[1], "box", 1, false, false, summary, false);
    Case(argv[1], "thin_1e4", 1e-4, false, false, summary, true);
    Case(argv[1], "thin_1e8", 1e-8, false, false, summary, true);
    Case(argv[1], "sheared", 1, true, false, summary, true);
    Case(argv[1], "rotated_spatial", 1, false, true, summary, true);
    summary << "]\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
