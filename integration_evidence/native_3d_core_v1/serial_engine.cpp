#include "Common/include/adaptation/CNativeEngine3D.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <chrono>
#include <iomanip>
using namespace SU2Native3D;
namespace {
Face Key(Face face) { std::sort(face.begin(), face.end()); return face; }
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id}, {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id}, {v[0].id, v[2].id, v[1].id}}};
}
struct Fixture { Patch patch; std::vector<Facet> facets; };
Fixture Box() {
  const std::array<Node, 8> nodes{{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {1, 1, 0}}, {3, {0, 1, 0}},
                                 {4, {0, 0, 1}}, {5, {1, 0, 1}}, {6, {1, 1, 1}}, {7, {0, 1, 1}}}};
  Fixture result;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < ring.size(); ++i) {
    Cell c{Id(i), {nodes[0], nodes[6], nodes[ring[i]], nodes[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    result.patch.cells.push_back(c);
  }
  for (auto face : Incidence(result.patch.cells).Boundary()) {
    const auto id = Id(result.facets.size());
    result.facets.push_back({id, int(id) + 10, {nodes[face[0]], nodes[face[1]], nodes[face[2]]}});
    result.patch.surface.push_back({face, id});
  }
  return result;
}
std::vector<OwnedCell> Rows(const Patch& patch) {
  std::map<Face, Id> physical;
  for (auto face : patch.surface)
    if (!physical.emplace(Key(face.v), face.facet).second) throw std::runtime_error("Duplicate fixture physical face.");
  std::vector<OwnedCell> rows;
  for (auto cell : patch.cells) {
    OwnedCell row;
    row.cell = cell;
    row.version = 17 + cell.id;
    const auto faces = Faces(cell);
    for (size_t i = 0; i < faces.size(); ++i) {
      const auto found = physical.find(Key(faces[i]));
      if (found != physical.end()) row.physical[i] = {true, found->second};
    }
    rows.push_back(row);
  }
  return rows;
}

std::vector<char> Bytes(const Owned& cells) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (auto row : cells) stream(row.second);
  return bytes;
}
}
int main() {
  World world;
  assert(world.size == 1 && world.rank == 0);
  const auto fixture = Box();
  const PlanarReference reference(fixture.facets);
  Owned input;
  std::vector<DonorCell> original;
  for (auto row : Rows(fixture.patch)) {
    input.emplace(row.cell.id, row);
    DonorCell donor;
    donor.cell = row.cell;
    donor.sensor.fill({4,0,0,4,0,4});
    original.push_back(donor);
  }
  const auto before = Bytes(input);
  const auto refined = Refine(world, input, original, reference);
  if (!refined.valid) std::cerr << refined.reason << '\n';
  assert(refined.valid && refined.cells.size() == 48 && refined.splits.size() == 19);
  assert(Bytes(input) == before);
  std::vector<Cell> cells;
  std::map<Face,Id> physical;
  for (const auto& row : refined.cells) {
    cells.push_back(row.second.cell);
    const auto faces = Faces(row.second.cell);
    for (size_t i=0; i<4; ++i) if (row.second.physical[i].present) {
      const auto facet = row.second.physical[i].facet;
      assert(physical.emplace(Key(faces[i]), facet).second);
      for (auto node : row.second.cell.v)
        if (std::find(faces[i].begin(), faces[i].end(), node.id) != faces[i].end())
          assert(reference.Contains(facet, node.p));
    }
  }
  assert(physical.size() == 48 && Incidence(cells).Boundary().size() == physical.size());
  FrozenField field(original);
  PatchMetric measured;
  std::string reason;
  assert(ValidateMetricPatch(cells, field, measured, reason));
  EngineOptions options; options.maximum_rounds = 1;
  const auto rejected = Refine(world, input, original, reference, {}, options);
  assert(!rejected.valid && rejected.cells.size() == 12 && Bytes(input) == before);
  const auto composed = Refine(world, input, original, reference, [](Point, Tensor m){ return m; });
  assert(!composed.valid && composed.stats.accepted == 0 && Bytes(input) == before);
  std::vector<DonorCell> coarse_source;
  for(const auto& row:refined.cells) { DonorCell d;d.cell=row.second.cell;d.sensor.fill({});coarse_source.push_back(d); }
  const auto saved_refined=Bytes(refined.cells);
  const auto coarsened=Coarsen(world,refined.cells,coarse_source,reference);
  if(!coarsened.valid) std::cerr<<coarsened.reason<<'\n';
  assert(coarsened.valid && coarsened.cells.size()==6 && coarsened.collapses.size()==19 && coarsened.splits.empty());
  assert(Bytes(refined.cells)==saved_refined);
  cells.clear();for(const auto& row:coarsened.cells) cells.push_back(row.second.cell);
  FrozenField coarse_field(coarse_source);
  assert(ValidateMetricPatch(cells,coarse_field,measured,reason));
  options.maximum_rounds=1;
  const auto limited_coarsening=Coarsen(world,refined.cells,coarse_source,reference,{},options);
  assert(!limited_coarsening.valid && Bytes(refined.cells)==saved_refined);
  const auto recoarsen=Coarsen(world,input, [&](){
    auto source=original;for(auto& d:source)d.sensor.fill({});return source;
  }(),reference);
  assert(recoarsen.valid && Bytes(recoarsen.cells)==before && recoarsen.collapses.empty());
  std::vector<DonorCell> repeated_source;
  for(const auto& row:coarsened.cells) { DonorCell d;d.cell=row.second.cell;d.sensor.fill({4,0,0,4,0,4});repeated_source.push_back(d); }
  const auto rerefined=Refine(world,coarsened.cells,repeated_source,reference);
  assert(rerefined.valid && rerefined.cells.size()==48 && rerefined.splits.size()==19);
  Patch edge;
  for(auto cell:fixture.patch.cells) {
    bool a=false,b=false;for(auto n:cell.v){a=a || n.id==0;b=b || n.id==1;}
    if(a && b) edge.cells.push_back(cell);
  }
  const auto edge_skin=Incidence(edge.cells).Boundary();
  for(auto f:fixture.patch.surface) if(std::find(edge_skin.begin(),edge_skin.end(),f.v)!=edge_skin.end()) edge.surface.push_back(f);
  Patch distorted,moved;
  assert(SplitEdge(edge,{0,1},{8,{.125,0,0}},1000,reference,distorted,reason));
  assert(MoveVertex(distorted,8,{9,{.5,0,0}},2000,reference,moved,reason));
  std::vector<DonorCell> motion_source;
  for(auto cell:distorted.cells) {
    DonorCell donor;donor.cell=cell;
    for(size_t i=0;i<4;++i){const double factor=1+.1*cell.v[i].p.x;donor.sensor[i]={factor,0,0,factor,0,factor};}
    motion_source.push_back(donor);
  }
  FrozenField motion_field(motion_source);
  assert(ValidateMetricChange(distorted.cells,moved.cells,MetricChange::Repair,motion_field,measured,reason));
  assert(ValidateMetricPatch(moved.cells,motion_field,measured,reason));
  const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{.5,std::sqrt(3.)/2,0}},
      d{3,{.5,std::sqrt(3.)/6,.25}},e{4,{.5,std::sqrt(3.)/6,-.25}};
  Patch bipyramid;bipyramid.cells={{0,{a,b,c,d}},{1,{a,c,b,e}}};
  std::map<Id,Node> corners;for(auto cell:bipyramid.cells)for(auto n:cell.v)corners.emplace(n.id,n);
  std::vector<Facet> authority;
  for(auto face:Incidence(bipyramid.cells).Boundary()) {
    const Id id=authority.size();authority.push_back({id,int(id)+10,{corners.at(face[0]),corners.at(face[1]),corners.at(face[2])}});
    bipyramid.surface.push_back({face,id});
  }
  const PlanarReference planes(authority);Patch reconnected;
  assert(Reconnect(bipyramid,3,100,planes,reconnected,reason));assert(reconnected.cells.size()==3);
  std::vector<DonorCell> reconnection_source;
  for(auto cell:bipyramid.cells){DonorCell donor;donor.cell=cell;donor.sensor.fill({});reconnection_source.push_back(donor);}
  FrozenField reconnect_field(reconnection_source);
  assert(ValidateMetricChange(bipyramid.cells,reconnected.cells,MetricChange::Repair,reconnect_field,measured,reason));
  assert(ValidateMetricPatch(reconnected.cells,reconnect_field,measured,reason));
  Owned bad;std::vector<DonorCell> repair_source;
  for(auto row:Rows(reconnected)) {bad.emplace(row.cell.id,row);DonorCell donor;donor.cell=row.cell;donor.sensor.fill({1,0,0,1,0,16});repair_source.push_back(donor);}
  const auto bad_saved=Bytes(bad);const auto automatic=Repair(world,bad,repair_source,planes);
  if(!automatic.valid)std::cerr<<automatic.reason<<'\n';
  assert(automatic.valid && automatic.cells.size()==2 && automatic.stats.reconnected==1 && Bytes(bad)==bad_saved);
  std::vector<Cell> repaired_cells;for(auto row:automatic.cells)repaired_cells.push_back(row.second.cell);
  FrozenField automatic_field(repair_source);assert(ValidateMetricPatch(repaired_cells,automatic_field,measured,reason));
  for(int trial=0;trial<3;++trial) for(int order=0;order<2;++order) {
    EngineOptions profile;profile.timings=bool(order^(trial%2));const auto start=std::chrono::steady_clock::now();
    const auto repeated=Repair(world,bad,repair_source,planes,{},profile);
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    assert(repeated.valid && Bytes(repeated.cells)==Bytes(automatic.cells));
    assert(repeated.stats.accepted==automatic.stats.accepted && repeated.stats.field_edges==automatic.stats.field_edges);
    assert(profile.timings ? repeated.stats.other_seconds>=-1e-12 : repeated.stats.total_seconds==0);
    std::cout<<std::setprecision(17)<<"REPAIR_PROFILE trial="<<trial<<" detail="<<profile.timings
        <<" external_s="<<elapsed<<" engine_s="<<repeated.stats.total_seconds<<" accepted="<<repeated.stats.accepted
        <<" rejected="<<repeated.stats.rejected<<" field_edges="<<repeated.stats.field_edges
        <<" reconstruction_s="<<repeated.stats.reconstruction_seconds<<" metric_s="<<repeated.stats.metric_seconds<<'\n';
  }
  for (int trial=0; trial<3; ++trial) for (int order=0; order<2; ++order) {
    EngineOptions profile;
    profile.timings = bool(order ^ (trial % 2));
    const auto start=std::chrono::steady_clock::now();
    const auto repeated=Refine(world,input,original,reference,{},profile);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    assert(repeated.valid && Bytes(repeated.cells)==Bytes(refined.cells));
    assert(repeated.stats.accepted==refined.stats.accepted && repeated.stats.field_edges==refined.stats.field_edges);
    assert(profile.timings ? repeated.stats.other_seconds>=-1e-12 : repeated.stats.total_seconds==0);
    std::cout << std::setprecision(17) << "PROFILE trial=" << trial << " detail=" << profile.timings
              << " external_s=" << elapsed << " engine_s=" << repeated.stats.total_seconds
              << " accepted=" << repeated.stats.accepted << " field_edges=" << repeated.stats.field_edges
              << " trace_nested_s=" << repeated.stats.edge_trace_seconds << '\n';
  }
  for(int trial=0;trial<3;++trial) for(int order=0;order<2;++order) {
    EngineOptions profile;profile.timings=bool(order^(trial%2));
    const auto start=std::chrono::steady_clock::now();
    const auto repeated=Coarsen(world,refined.cells,coarse_source,reference,{},profile);
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    assert(repeated.valid && Bytes(repeated.cells)==Bytes(coarsened.cells));
    assert(repeated.stats.accepted==coarsened.stats.accepted && repeated.stats.field_edges==coarsened.stats.field_edges);
    assert(profile.timings ? repeated.stats.other_seconds>=-1e-12 : repeated.stats.total_seconds==0);
    std::cout<<std::setprecision(17)<<"COARSEN_PROFILE trial="<<trial<<" detail="<<profile.timings
        <<" external_s="<<elapsed<<" engine_s="<<repeated.stats.total_seconds<<" accepted="<<repeated.stats.accepted
        <<" rejected="<<repeated.stats.rejected<<" field_edges="<<repeated.stats.field_edges
        <<" trace_nested_s="<<repeated.stats.edge_trace_seconds<<'\n';
  }
  std::cout << "PASS actual non-MPI engine: 6->48->6->48 tetrahedra, general collapse, automatic and private movement/reconnection, full frozen metric gates, rejection and composed-target refusal\n";
}
