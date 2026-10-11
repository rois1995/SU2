// Diagnostic-only event trace: the production engine and gates remain unchanged.
#include <fstream>
#include <iomanip>
namespace SU2Native3D {
static void TraceAttempt(MetricChange intent, size_t round, int rank, bool chosen, bool approved,
                         bool repairing, Edge edge, const Imported& imported, const Patch& fresh, const std::string& reason) {
  if (!chosen || intent!=MetricChange::Coarsen) return;
  std::ofstream out("attempts_rank"+std::to_string(rank)+".jsonl",std::ios::app);
  out << std::setprecision(17) << "{\"round\":" << round << ",\"rank\":" << rank
      << ",\"approved\":" << (approved ? "true" : "false") << ",\"repairing\":" << (repairing ? "true" : "false")
      << ",\"edge\":[" << edge[0] << ',' << edge[1] << "],\"reason\":" << std::quoted(reason) << ",\"old\":[";
  bool first=true;
  auto dump=[&](const OwnedCell& row) {
    if (!first) out << ','; first=false;
    out << "{\"id\":" << row.cell.id << ",\"version\":" << row.version << ",\"nodes\":[";
    for (size_t i=0;i<4;++i) { if (i) out << ','; auto n=row.cell.v[i];
      out << '[' << n.id << ',' << n.p.x << ',' << n.p.y << ',' << n.p.z << ']'; }
    out << "],\"physical\":[";
    for (size_t i=0;i<4;++i) { if (i) out << ','; out << '[' << (row.physical[i].present ? 1 : 0) << ',' << row.physical[i].facet << ']'; }
    out << "]}";
  };
  for (const auto& row:imported.cells) dump(row);
  out << "],\"fresh\":["; first=true;
  if (approved) for (const auto& cell:fresh.cells) { OwnedCell row; row.cell=cell; dump(row); }
  out << "]}\n";
  if (!out) throw std::runtime_error("Diagnostic attempt trace write failed.");
}
static void TraceFinish(MetricChange intent, int rank, bool valid, const Owned& cells, const PlanarReference& geometry) {
  if (rank!=0) return;
  std::map<Id,Node> nodes;
  std::map<int,std::vector<Face>> surface;
  for (const auto& row:cells) {
    for (auto node:row.second.cell.v) nodes.emplace(node.id,node);
    const auto& v=row.second.cell.v;
    const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for (size_t f=0;f<4;++f) if (row.second.physical[f].present)
      surface[geometry.Get(row.second.physical[f].facet).marker].push_back(faces[f]);
  }
  const std::string name=intent==MetricChange::Coarsen ? "coarsen_rejected.su2" : "refine_private.su2";
  std::ofstream out(name); out << std::setprecision(17);
  std::map<Id,size_t> local; size_t index=0;
  for (const auto& row:nodes) local.emplace(row.first,index++);
  out << "NDIME= 3\nNELEM= " << cells.size() << '\n'; index=0;
  for (const auto& row:cells) { out << "10"; for (auto node:row.second.cell.v) out << ' ' << local.at(node.id); out << ' ' << index++ << '\n'; }
  out << "NPOIN= " << nodes.size() << '\n'; index=0;
  for (const auto& row:nodes) { auto p=row.second.p; out << p.x << ' ' << p.y << ' ' << p.z << ' ' << index++ << '\n'; }
  out << "NMARK= " << surface.size() << '\n';
  for (const auto& group:surface) { out << "MARKER_TAG= reference_marker_" << group.first << "\nMARKER_ELEMS= " << group.second.size() << '\n';
    for (auto face:group.second) out << "5 " << local.at(face[0]) << ' ' << local.at(face[1]) << ' ' << local.at(face[2]) << '\n'; }
  if (!out) throw std::runtime_error("Diagnostic private mesh write failed.");
  std::ofstream meta("private_mesh_status.jsonl",std::ios::app);
  meta << "{\"intent\":" << int(intent) << ",\"valid_cached_exit\":" << (valid ? "true" : "false")
       << ",\"file\":" << std::quoted(name) << ",\"cells\":" << cells.size() << ",\"nodes\":" << nodes.size()
       << ",\"scope\":\"private engine output only, not accepted CFD replacement or independent mesh certificate\"}\n";
  if (!meta) throw std::runtime_error("Diagnostic private mesh status write failed.");
}
}
