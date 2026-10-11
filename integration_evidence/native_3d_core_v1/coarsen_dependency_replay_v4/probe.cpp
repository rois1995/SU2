// Source-matched local dependency reproduction; not a replacement engine.
#include "../../../Common/src/adaptation/CNativeEngine3D.cpp"
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace SU2Native3D;
void Check(bool ok, const std::string& why) { if (!ok) throw std::runtime_error(why); }
void Dump(const std::string& name, const Patch& patch, bool strict, FrozenField& field) {
  std::ofstream out(name); out << std::setprecision(17);
  std::map<Id,Node> nodes;
  for (auto cell:patch.cells) for (auto node:cell.v) nodes.emplace(node.id,node);
  out << "{\"sensor_peak_vertex\":4,\"sensor_peak_xx\":7,\"metric\":[0.25,0,0,0.25,0,0.25],\"metric_gates_required\":" << (strict ? "true" : "false") << ",\"nodes\":[";
  bool first=true;
  for (auto row:nodes) { auto p=row.second.p; if (!first) out << ','; first=false;
    out << '[' << row.first << ',' << p.x << ',' << p.y << ',' << p.z << ']'; }
  out << "],\"cells\":["; first=true;
  for (auto cell:patch.cells) {
    auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},field.evaluate(Centroid(cell)));
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
int main(int argc, char** argv) {
 SU2_MPI::Init(&argc,&argv);
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
  for (auto& donor:donors) for (size_t i=0;i<4;++i) if (donor.cell.v[i].id==4) donor.sensor[i].xx=7;
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
  Patch collapse_star;
  for (auto cell:initial.cells) if (std::any_of(cell.v.begin(),cell.v.end(),[](Node n){return n.id==4;})) collapse_star.cells.push_back(cell);
  Patch blocked_fresh;
  Check(CollapseVertex(collapse_star,4,0,2000,geometry,blocked_fresh,reason),"Initial collapse geometry: "+reason);
  PatchMetric blocked_metric;
  Check(!ValidateMetricChange(collapse_star.cells,blocked_fresh.cells,MetricChange::Coarsen,field,blocked_metric,reason),"Initial collapse should fail the frozen metric gate.");
  blocked_metric=MeasureMetricCells(blocked_fresh.cells,field);
  Check(blocked_metric.minimum_scaled_jacobian<MinimumScaledJacobian,"Expected a real frozen sliver rejection.");
  std::cout << "INITIAL_REJECTION " << reason << '\n';
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
  Patch updated_star;
  for (auto cell:final.cells) if (std::any_of(cell.v.begin(),cell.v.end(),[](Node n){return n.id==4;})) updated_star.cells.push_back(cell);
  Patch successful_fresh;
  Check(CollapseVertex(updated_star,4,0,3000,geometry,successful_fresh,reason),"Updated collapse geometry: "+reason);
  PatchMetric successful_metric;
  Check(ValidateMetricChange(updated_star.cells,successful_fresh.cells,MetricChange::Coarsen,field,successful_metric,reason),"Updated collapse target: "+reason);
  Patch collapsed;
  for (auto cell:final.cells) if (std::none_of(cell.v.begin(),cell.v.end(),[](Node n){return n.id==4;})) collapsed.cells.push_back(cell);
  collapsed.cells.insert(collapsed.cells.end(),successful_fresh.cells.begin(),successful_fresh.cells.end());
  collapsed.surface=initial.surface;
  Check(ValidateFixedInterface(initial.cells,collapsed.cells,reason),"Collapsed global skin: "+reason);
  Check(ValidateMetricPatch(collapsed.cells,field,successful_metric,reason),"Collapsed global metric: "+reason);
  // A replacement touching the candidate edge does reactivate it with current code.
  selection.Drop(initial.cells[0].id);
  selection.Add(owned(initial.cells[0]),MeasureMetricCells({initial.cells[0]},field),true);
  Check(active(),"Edge-touching positive control did not reactivate candidate.");
  Dump("dependency_initial.json",initial,true,field); Dump("dependency_candidate.json",final,true,field);
  Dump("dependency_collapsed.json",collapsed,true,field);
  Dump("collapse_rejected_patch.json",blocked_fresh,false,field);
  std::cout << "{\"status\":\"PASS\",\"classification\":\"real sliver-rejected collapse becomes admissible after full-star change but remains inactive\","
            << "\"initial_rejected_sliver\":" << blocked_metric.minimum_scaled_jacobian << ",\"collapsed_cells\":3,\"initial_cells\":5,\"final_cells\":6,\"initial_vertex_star\":4,\"final_vertex_star\":6,"
            << "\"edge_star_unchanged\":true,\"unrelated_control\":true,\"edge_touching_control\":true,"
            << "\"minimum_quality\":" << measured.minimum_mean_ratio << ",\"minimum_scaled_jacobian\":" << measured.minimum_scaled_jacobian
            << ",\"maximum_metric_length\":" << double(measured.maximum_length) << "}\n";
  SU2_MPI::Finalize();
  return 0;
 } catch (const std::exception& e) { std::cerr << e.what() << '\n'; SU2_MPI::Finalize(); return 1; }
}
