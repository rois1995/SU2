/* Small standalone private-cavity controls; not an SU2 remeshing driver. */
#include "Common/include/adaptation/CNativeCavity3D.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace SU2Native3D;
namespace {
std::ofstream Open(const std::string& path) {
  std::ofstream out;
  out.exceptions(std::ios::failbit | std::ios::badbit);
  out.open(path);
  out << std::setprecision(17);
  return out;
}
void Mesh(const std::string& file, const std::vector<Cell>& cells, Tensor metric, KernelStats& stats) {
  auto out = Open(file);
  std::map<Id, Node> nodes;
  for (const auto& cell : cells)
    for (const auto& v : cell.v) nodes[v.id] = v;
  out << "{\"metric\":[" << metric.xx << ',' << metric.xy << ',' << metric.xz << ',' << metric.yy << ',' << metric.yz
      << ',' << metric.zz << "],\"nodes\":[";
  bool first = true;
  for (const auto& entry : nodes) {
    const auto& n = entry.second;
    if (!first) out << ',';
    first = false;
    out << '[' << n.id << ',' << n.p.x << ',' << n.p.y << ',' << n.p.z << ']';
  }
  out << "],\"cells\":[";
  first = true;
  for (const auto& cell : cells) {
    const Tetrahedron geometry{cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p};
    const auto m = Measure(geometry, metric, &stats);
    if (!first) out << ',';
    first = false;
    out << "{\"id\":" << cell.id << ",\"nodes\":[" << cell.v[0].id << ',' << cell.v[1].id << ',' << cell.v[2].id << ','
        << cell.v[3].id << "],\"measures\":[" << m.mean_ratio << ',' << m.minimum_scaled_jacobian << ',' << m.rms_edge
        << ',' << m.maximum_edge << "]}";
  }
  out << "]}\n";
}
Cell Make(Id id, std::array<Node, 4> v) {
  if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
  return {id, v};
}
void Probe(const std::string& folder, const std::string& name, const std::vector<Cell>& old, Node apex, Tensor metric,
           std::ofstream& summary, bool first) {
  std::vector<Cell> fresh;
  std::string reason;
  KernelStats stats;
  const auto start = std::chrono::steady_clock::now();
  if (!Cone(old, apex, 100, fresh, reason, &stats)) throw std::runtime_error(name + ": " + reason);
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  Mesh(folder + '/' + name + "_initial.json", old, metric, stats);
  Mesh(folder + '/' + name + "_candidate.json", fresh, metric, stats);
  if (!first) summary << ',';
  summary << "{\"case\":\"" << name << "\",\"initial_cells\":" << old.size() << ",\"candidate_cells\":" << fresh.size()
          << ",\"private_reconstruction_seconds\":" << seconds
          << ",\"filtered_orientations\":" << stats.filtered_orientations
          << ",\"exact_orientations\":" << stats.exact_orientations << ",\"measured_cells\":" << stats.measured_cells
          << '}';
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    const std::string folder = argv[1];
    auto summary = Open(folder + "/probe_summary.json");
    summary << '[';
    const auto source = std::vector<Cell>{Make(0, {{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {0, 1, 0}}, {3, {0, 0, 1}}}})};
    const Node center{4, {.25, .25, .25}};
    Probe(folder, "insert", source, center, {}, summary, true);
    std::vector<Cell> refined;
    std::string reason;
    if (!Cone(source, center, 10, refined, reason)) throw std::runtime_error(reason);
    Probe(folder, "move", refined, {4, {.125, .25, .25}}, {}, summary, false);
    Probe(folder, "remove", refined, source.front().v[0], {}, summary, false);
    const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}}, top{3, {.25, .25, 1}}, bottom{4, {.25, .25, -1}};
    Probe(folder, "flip_2_to_3", {Make(0, {a, b, c, top}), Make(1, {a, b, c, bottom})}, top, {.5, 0, 0, .5, 0, .5},
          summary, false);
    auto thin = source;
    const double angle = .37, cosa = std::cos(angle), sina = std::sin(angle), aspect = 1e4;
    auto transform = [&](Point p) {
      return Point{cosa * p.x + sina * p.z / aspect, p.y, -sina * p.x + cosa * p.z / aspect};
    };
    for (auto& cell : thin)
      for (auto& node : cell.v) node.p = transform(node.p);
    const double squared = aspect * aspect;
    const Tensor metric{cosa * cosa + squared * sina * sina, 0, (squared - 1) * sina * cosa, 1, 0,
                        sina * sina + squared * cosa * cosa};
    Probe(folder, "rotated_aspect10000_insert", thin, {4, transform(center.p)}, metric, summary, false);
    summary << "]\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
