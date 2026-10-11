// Source-matched local dependency reproduction; not a replacement engine.
#include "../../../Common/src/adaptation/CNativeEngine3D.cpp"
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace SU2Native3D;
void Check(bool ok, const std::string& why) { if (!ok) throw std::runtime_error(why); }
void Dump(const std::string& name, const Patch& patch, bool strict) {
  std::ofstream out(name); out << std::setprecision(17);
  std::map<Id,Node> nodes;
  for (auto cell:patch.cells) for (auto node:cell.v) nodes.emplace(node.id,node);
  out << "{\"metric\":[0.25,0,0,0.25,0,0.25],\"metric_gates_required\":" << (strict ? "true" : "false") << ",\"nodes\":[";
  bool first=true;
  for (auto row:nodes) { auto p=row.second.p; if (!first) out << ','; first=false;
    out << '[' << row.first << ',' << p.x << ',' << p.y << ',' << p.z << ']'; }
  out << "],\"cells\":["; first=true;
  for (auto cell:patch.cells) {
    auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},{.25,0,0,.25,0,.25});
    if (!first) out << ','; first=false;
    out << "{\"id\":" << cell.id << ",\"nodes\":[";
    for (size_t i=0;i<4;++i) { if (i) out << ','; out << cell.v[i].id; }
    out << "],\"measures\":[" << m.mean_ratio << ',' << m.minimum_scaled_jacobian << ',' << m.rms_edge << ',' << m.maximum_edge << "]}";
  }
  out << "],\"surface\":["; first=true;
  for (auto face:patch.surface) { if (!first) out << ','; first=false;
    out << '[' << face.facet << ',' << face.v[0] << ',' << face.v[1] << ',' << face.v[2] << ']'; }
  out << "]}\n"; Check(bool(out),"Mesh snapshot write failed.");
}
int main() {
 try {
  const std::array<Node,6> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}},{4,{.25,.25,.25}},{5,{1,1,1}}}};
  Patch initial;
  const std::array<std::array<Id,4>,5> layout{{{4,0,1,2},{4,1,2,3},{4,0,2,3},{4,0,3,1},{5,1,2,3}}};
  for (size_t i=0;i<layout.size();++i) {
    Cell cell; cell.id=10+i;
    for (size_t j=0;j<4;++j) cell.v[j]=nodes[layout[i][j]];
    if (Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0) std::swap(cell.v[0],cell.v[1]);
    initial.cells.push_back(cell);
  }
  std::vector<Facet> facets;
  for (auto face:Incidence(initial.cells).Boundary()) {
    const Id id=500+facets.size();
    facets.push_back({id,int(id),{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});
    initial.surface.push_back({face,id});
  }
  PlanarReference geometry(facets);
  std::vector<DonorCell> donors;
  for (auto cell:initial.cells) donors.push_back({cell,{{{.25,0,0,.25,0,.25},{.25,0,0,.25,0,.25},{.25,0,0,.25,0,.25},{.25,0,0,.25,0,.25}}}});
  FrozenField field(donors);
  PatchMetric measured; std::string reason;
  Check(ValidateMetricPatch(initial.cells,field,measured,reason),"Initial metric: "+reason);
  auto owned=[&](Cell cell) {
    OwnedCell row; row.cell=cell;
    auto faces=Faces(cell);
    for (size_t i=0;i<4;++i) for (auto face:initial.surface)
      if (Key(face.v)==Key(faces[i])) row.physical[i]={true,face.facet};
    return row;
  };
  const Edge collapse{0,4};
  EngineStats stats;
  Selection selection(true,false,geometry,stats,false);
  for (auto cell:initial.cells) selection.Add(owned(cell),MeasureMetricCells({cell},field),true);
  const auto length=selection.edges.at(collapse).measure.length;
  const auto users=selection.edges.at(collapse).users;
  auto active=[&]() { return selection.queue.count({selection.Priority(length),collapse})!=0; };
  Check(active(),"Missing initial collapse candidate.");
  selection.Block(collapse,{},false);
  Check(!active(),"Candidate was not blocked.");
  // Unrelated replacement changes no cell of the removed-vertex star.
  selection.Drop(initial.cells[4].id);
  selection.Add(owned(initial.cells[4]),MeasureMetricCells({initial.cells[4]},field),true);
  Check(!active(),"Unrelated cell replacement spuriously reactivated candidate.");
  Patch cavity; cavity.cells={initial.cells[1],initial.cells[4]};
  auto skin=Incidence(cavity.cells).Boundary();
  for (auto face:initial.surface)
    if (std::any_of(skin.begin(),skin.end(),[&](Face f){return Key(f)==Key(face.v);})) cavity.surface.push_back(face);
  Patch fresh;
  Check(Reconnect(cavity,4,1000,geometry,fresh,reason),"Reconnection: "+reason);
  Check(fresh.cells.size()==3,"Expected genuine face 2-to-3 reconnection.");
  Patch final=initial;
  final.cells.erase(std::remove_if(final.cells.begin(),final.cells.end(),[](Cell c){return c.id==11 || c.id==14;}),final.cells.end());
  final.cells.insert(final.cells.end(),fresh.cells.begin(),fresh.cells.end());
  Check(ValidateFixedInterface(initial.cells,final.cells,reason),"Global skin: "+reason);
  Check(ValidateMetricPatch(final.cells,field,measured,reason),"Reconnected metric: "+reason);
  Check(Incidence(initial.cells).EdgeCells(collapse)==Incidence(final.cells).EdgeCells(collapse),"Candidate edge star unexpectedly changed.");
  Check(Incidence(initial.cells).VertexCells(4)!=Incidence(final.cells).VertexCells(4),"Removed vertex dependency star did not change.");
  selection.Drop(11); selection.Drop(14);
  for (auto cell:fresh.cells) selection.Add(owned(cell),MeasureMetricCells({cell},field),true);
  Check(selection.edges.at(collapse).users==users && selection.edges.at(collapse).measure.length==length,"Candidate measurement changed.");
  Check(!active(),"Existing code no longer reproduces the missing full-star retry.");
  // A replacement touching the candidate edge does reactivate it with current code.
  selection.Drop(initial.cells[0].id);
  selection.Add(owned(initial.cells[0]),MeasureMetricCells({initial.cells[0]},field),true);
  Check(active(),"Edge-touching positive control did not reactivate candidate.");
  Dump("dependency_initial.json",initial,true); Dump("dependency_candidate.json",final,true);
  std::cout << "{\"status\":\"PASS\",\"classification\":\"full-vertex-star change without edge change leaves blocked candidate inactive\","
            << "\"initial_cells\":5,\"final_cells\":6,\"initial_vertex_star\":4,\"final_vertex_star\":6,"
            << "\"edge_star_unchanged\":true,\"unrelated_control\":true,\"edge_touching_control\":true,"
            << "\"minimum_quality\":" << measured.minimum_mean_ratio << ",\"minimum_scaled_jacobian\":" << measured.minimum_scaled_jacobian
            << ",\"maximum_metric_length\":" << double(measured.maximum_length) << "}\n";
  return 0;
 } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
