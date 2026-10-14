/*!
 * \file CNativeBoundary3D_tests.cpp
 * \brief Coupled private planar boundary refinement/coarsening and incomplete-star rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include "../../../Common/include/adaptation/CNativeField3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../Common/include/parallelization/mpi_structure.hpp"
using namespace SU2Native3D;
namespace {
Patch Cube(double height = 1) {
  std::array<Node, 8> v{{{0, {0, 0, 0}},
                         {1, {1, 0, 0}},
                         {2, {1, 1, 0}},
                         {3, {0, 1, 0}},
                         {4, {0, 0, height}},
                         {5, {1, 0, height}},
                         {6, {1, 1, height}},
                         {7, {0, 1, height}}}};
  Patch patch;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < 6; ++i) {
    Cell c{Id(i), {v[0], v[6], v[ring[i]], v[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    patch.cells.push_back(c);
  }
  const auto faces = Incidence(patch.cells).Boundary();
  for (size_t i = 0; i < faces.size(); ++i) patch.surface.push_back({faces[i], Id(i)});
  return patch;
}
std::vector<Facet> Facets(const Patch& patch) {
  std::map<Id, Node> nodes;
  for (const auto& c : patch.cells)
    for (auto n : c.v) nodes.emplace(n.id, n);
  std::vector<Facet> facets;
  for (const auto& f : patch.surface)
    facets.push_back({f.facet, int(f.facet + 10), {nodes.at(f.v[0]), nodes.at(f.v[1]), nodes.at(f.v[2])}});
  return facets;
}
Patch EdgeStar(const Patch& patch, Edge edge) {
  Patch star;
  for (const auto& c : patch.cells) {
    size_t found = 0;
    for (auto n : c.v) found += n.id == edge[0] || n.id == edge[1];
    if (found == 2) star.cells.push_back(c);
  }
  const auto skin = Incidence(star.cells).Boundary();
  for (const auto& f : patch.surface)
    if (std::find(skin.begin(), skin.end(), f.v) != skin.end()) star.surface.push_back(f);
  return star;
}
std::set<std::array<Id, 4>> Connectivity(const Patch& patch) {
  std::set<std::array<Id, 4>> result;
  for (const auto& c : patch.cells) {
    std::array<Id, 4> key;
    for (size_t i = 0; i < 4; ++i) key[i] = c.v[i].id;
    std::sort(key.begin(), key.end());
    result.insert(key);
  }
  return result;
}
}  // namespace
TEST_CASE("Native 3D complete internal edge star splits and reverses with fixed skin", "[NativeMesh3D]") {
  const auto old = Cube();
  const PlanarReference reference(Facets(old));
  Patch split, coarsened;
  std::string reason;
  REQUIRE(SplitEdge(old, {0, 6}, {8, {.5, .5, .5}}, 100, reference, split, reason));
  CHECK(split.cells.size() == 12);
  CHECK(split.surface.size() == 12);
  CHECK(ValidateFixedInterface(old.cells, split.cells, reason));
  REQUIRE(UndoEdgeSplit(split, 8, {6, 0}, 200, reference, coarsened, reason));
  CHECK(Connectivity(coarsened) == Connectivity(old));
  CHECK(ValidateFixedInterface(old.cells, coarsened.cells, reason));
}
TEST_CASE("Native 3D planar physical edge edits retain facets and marker junctions", "[NativeMesh3D]") {
  for (double height : {1., 1e-4, 1e-8}) {
    const auto cube = Cube(height);
    const PlanarReference reference(Facets(cube));
    const auto old = EdgeStar(cube, {0, 1});
    Patch current = old;
    std::string reason;
    for (Id step = 0; step < 3; ++step) {
      Patch split, coarsened;
      REQUIRE(SplitEdge(current, {1, 0}, {8, {.5, 0, 0}}, 100 + 40 * step, reference, split, reason));
      CHECK(split.cells.size() == 4);
      CHECK(split.surface.size() == old.surface.size() + 2);
      CHECK_FALSE(ValidateFixedInterface(old.cells, split.cells, reason));
      std::map<Id, size_t> counts;
      for (const auto& f : split.surface) {
        ++counts[f.facet];
        CHECK(reference.Get(f.facet).marker == int(f.facet + 10));
      }
      size_t changed = 0;
      for (const auto& count : counts) changed += count.second == 2;
      CHECK(changed == 2);  // Two original physical facets/markers meet at the edited feature edge.
      REQUIRE(UndoEdgeSplit(split, 8, {0, 1}, 120 + 40 * step, reference, coarsened, reason));
      CHECK(Connectivity(coarsened) == Connectivity(old));
      CHECK(coarsened.surface.size() == old.surface.size());
      for (size_t i = 0; i < old.surface.size(); ++i) {
        CHECK(coarsened.surface[i].v == old.surface[i].v);
        CHECK(coarsened.surface[i].facet == old.surface[i].facet);
      }
      current = std::move(coarsened);
    }
  }
}
TEST_CASE("Native 3D surface edits reject missing dependencies and preserve both outputs", "[NativeMesh3D]") {
  const auto cube = Cube();
  const PlanarReference reference(Facets(cube));
  const auto old = EdgeStar(cube, {0, 1});
  Patch output = old;
  std::string reason;
  auto incomplete = old;
  incomplete.cells.pop_back();
  const auto skin = Incidence(incomplete.cells).Boundary();
  incomplete.surface.erase(
      std::remove_if(incomplete.surface.begin(), incomplete.surface.end(),
                     [&](SurfaceFace f) { return std::find(skin.begin(), skin.end(), f.v) == skin.end(); }),
      incomplete.surface.end());
  CHECK_FALSE(SplitEdge(incomplete, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK(reason.find("incomplete") != std::string::npos);
  CHECK(Connectivity(output) == Connectivity(old));
  CHECK(output.surface.size() == old.surface.size());
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {.5, 1e-16, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {0, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {0, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 0}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {.5, 0, 0}}, std::numeric_limits<Id>::max(), reference, output, reason));
  CHECK_FALSE(SplitEdge(cube, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  auto unmarked = old;
  unmarked.surface.clear();
  CHECK_FALSE(SplitEdge(unmarked, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  auto wrong = old;
  wrong.surface[0].facet = 999;
  CHECK_FALSE(SplitEdge(wrong, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  Patch split;
  REQUIRE(SplitEdge(old, {0, 1}, {8, {.5, 0, 0}}, 100, reference, split, reason));
  auto missing = split;
  missing.cells.pop_back();
  CHECK_FALSE(UndoEdgeSplit(missing, 8, {0, 1}, 200, reference, output, reason));
  CHECK_FALSE(UndoEdgeSplit(split, 8, {0, 6}, 200, reference, output, reason));
  CHECK_FALSE(UndoEdgeSplit(split, 8, {0, 8}, 200, reference, output, reason));
  CHECK(Connectivity(output) == Connectivity(old));
  CHECK(output.surface.size() == old.surface.size());
}
TEST_CASE("Native 3D immutable planar facet authority checks exact membership and invalid input", "[NativeMesh3D]") {
  auto facets = Facets(Cube());
  PlanarReference reference(facets);
  const auto f = facets.front();
  for (auto n : f.v) CHECK(reference.Contains(f.id, n.p));
  const Point center{(f.v[0].p.x + f.v[1].p.x + f.v[2].p.x) / 3, (f.v[0].p.y + f.v[1].p.y + f.v[2].p.y) / 3,
                     (f.v[0].p.z + f.v[1].p.z + f.v[2].p.z) / 3};
  CHECK(reference.Contains(f.id, center));
  CHECK_FALSE(reference.Contains(f.id, {2, 2, 2}));
  facets.front().marker = 999;
  CHECK(reference.Get(f.id).marker == f.marker);
  facets = Facets(Cube());
  facets.push_back(facets.front());
  CHECK_THROWS_AS(PlanarReference(facets), std::invalid_argument);
  facets = {f};
  facets.front().v[2] = facets.front().v[1];
  CHECK_THROWS_AS(PlanarReference(facets), std::invalid_argument);
  CHECK_THROWS_AS(PlanarReference(std::vector<Facet>(4 * MaximumCavityCells + 1, f)), std::invalid_argument);
}

namespace {
Patch Single(double height) {
  Patch patch;
  patch.cells.push_back({0, {{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,height}}}}});
  const auto skin=Incidence(patch.cells).Boundary();
  for(size_t i=0;i<skin.size();++i) patch.surface.push_back({skin[i],Id(i)});
  return patch;
}
Patch WallPoint(const Patch& original,const PlanarReference& geometry) {
  const auto v=original.cells.front().v;const Node p{4,{.25,.25,0}};
  Patch star;
  star.cells={{10,{p,v[1],v[2],v[3]}},{11,{v[0],p,v[2],v[3]}},{12,{v[0],v[1],p,v[3]}}};
  std::map<Id,Point> nodes;
  for(auto cell:star.cells) for(auto node:cell.v) nodes.emplace(node.id,node.p);
  for(auto face:Incidence(star.cells).Boundary()) {
    bool found=false;
    for(auto facet:original.surface) {
      bool contains=true;
      for(auto id:face) contains=contains && geometry.Contains(facet.facet,nodes.at(id));
      if(contains) {star.surface.push_back({face,facet.facet});found=true;break;}
    }
    REQUIRE(found);
  }
  return star;
}
}
TEST_CASE("Native general collapse removes a boundary point without bisection history", "[NativeMesh3D]") {
  for(double height:{1.,1e-4,1e-8}) {
    const auto original=Single(height);const PlanarReference reference(Facets(original));
    const auto star=WallPoint(original,reference);
    for(Id neighbor:{0,1,2}) {
      Patch fresh;std::string reason;
      REQUIRE(CollapseVertex(star,4,neighbor,100,reference,fresh,reason));
      CHECK(Connectivity(fresh)==Connectivity(original));
      CHECK(fresh.surface.size()==original.surface.size());
      CHECK(fresh.cells.size()==1);
      for(const auto& f:fresh.surface) CHECK(reference.Get(f.facet).marker==int(f.facet+10));
    }
    Patch retained=star;std::string reason;
    CHECK_FALSE(CollapseVertex(star,4,3,100,reference,retained,reason));
    CHECK(reason.find("incident reference facet")!=std::string::npos);
    CHECK(Connectivity(retained)==Connectivity(star));
    CHECK(retained.surface.size()==star.surface.size());
  }
}
TEST_CASE("Native general collapse removes an interior point and preserves corner and interface constraints", "[NativeMesh3D]") {
  const auto original=Single(1);const PlanarReference reference(Facets(original));
  Patch star;std::string reason;
  REQUIRE(Cone(original.cells,{4,{.25,.25,.25}},10,star.cells,reason));
  star.surface=original.surface;
  Patch fresh;
  REQUIRE(CollapseVertex(star,4,0,100,reference,fresh,reason));
  CHECK(Connectivity(fresh)==Connectivity(original));
  CHECK(ValidateFixedInterface(star.cells,fresh.cells,reason));
  auto incomplete=star;incomplete.cells.pop_back();
  const auto skin=Incidence(incomplete.cells).Boundary();
  incomplete.surface.erase(std::remove_if(incomplete.surface.begin(),incomplete.surface.end(),
      [&](SurfaceFace f){return std::find(skin.begin(),skin.end(),f.v)==skin.end();}),incomplete.surface.end());
  const auto saved=Connectivity(fresh);const auto faces=fresh.surface.size();
  CHECK_FALSE(CollapseVertex(incomplete,4,0,200,reference,fresh,reason));
  CHECK(reason.find("incomplete")!=std::string::npos);
  CHECK_FALSE(CollapseVertex(original,0,1,200,reference,fresh,reason));
  CHECK_FALSE(CollapseVertex(star,4,4,200,reference,fresh,reason));
  CHECK_FALSE(CollapseVertex(star,4,999,200,reference,fresh,reason));
  CHECK(Connectivity(fresh)==saved);CHECK(fresh.surface.size()==faces);
}
TEST_CASE("Native planar movement keeps finite facets and rejects incomplete stars atomically", "[NativeMesh3D]") {
  for(double height:{1.,1e-4,1e-8}) {
    const auto original=Single(height);const PlanarReference reference(Facets(original));
    const auto old=WallPoint(original,reference);Patch fresh;std::string reason;
    REQUIRE(MoveVertex(old,4,{5,{.375,.125,0}},100,reference,fresh,reason));
    CHECK(fresh.cells.size()==old.cells.size());CHECK(fresh.surface.size()==old.surface.size());
    for(auto cell:fresh.cells) for(auto node:cell.v) CHECK(node.id!=4);
    for(auto face:fresh.surface) CHECK(reference.Get(face.facet).marker==int(face.facet)+10);
    const auto saved=Connectivity(fresh);const auto count=fresh.surface.size();
    CHECK_FALSE(MoveVertex(old,4,{5,{.25,.25,.125*height}},200,reference,fresh,reason));
    CHECK_FALSE(MoveVertex(old,4,{4,{.375,.125,0}},200,reference,fresh,reason));
    CHECK_FALSE(MoveVertex(old,4,{5,{.25,.25,0}},200,reference,fresh,reason));
    CHECK_FALSE(MoveVertex(old,4,{5,{0,0,0}},200,reference,fresh,reason));
    CHECK_FALSE(MoveVertex(old,4,{5,{.375,.125,0}},UINT64_MAX,reference,fresh,reason));
    auto incomplete=old;incomplete.cells.pop_back();
    const auto skin=Incidence(incomplete.cells).Boundary();
    incomplete.surface.erase(std::remove_if(incomplete.surface.begin(),incomplete.surface.end(),
        [&](SurfaceFace f){return std::find(skin.begin(),skin.end(),f.v)==skin.end();}),incomplete.surface.end());
    CHECK_FALSE(MoveVertex(incomplete,4,{5,{.375,.125,0}},200,reference,fresh,reason));
    CHECK(reason.find("incomplete")!=std::string::npos);
    CHECK(Connectivity(fresh)==saved);CHECK(fresh.surface.size()==count);
  }
}
TEST_CASE("Native movement preserves the interior skin and protects original corners", "[NativeMesh3D]") {
  const auto original=Single(1);const PlanarReference reference(Facets(original));Patch star,fresh;std::string reason;
  REQUIRE(Cone(original.cells,{4,{.25,.25,.25}},10,star.cells,reason));star.surface=original.surface;
  REQUIRE(MoveVertex(star,4,{5,{.125,.25,.25}},100,reference,fresh,reason));
  CHECK(ValidateFixedInterface(original.cells,fresh.cells,reason));
  CHECK(fresh.cells.size()==4);CHECK(fresh.surface.size()==4);
  CHECK_FALSE(MoveVertex(original,0,{9,{.0625,0,0}},200,reference,fresh,reason));
  CHECK(reason.find("incident reference facet")!=std::string::npos);
  CHECK_FALSE(MoveVertex(star,999,{5,{.125,.25,.25}},200,reference,fresh,reason));
  CHECK(fresh.cells.front().id==100);
}
TEST_CASE("Native boundary-aware reconnection preserves markers for face and edge swaps", "[NativeMesh3D]") {
  const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{.5,.75,0}},d{3,{.5,.25,.25}},e{4,{.5,.25,-.25}};
  Patch old;old.cells={{0,{a,b,c,d}},{1,{a,c,b,e}}};
  for(auto face:Incidence(old.cells).Boundary()) old.surface.push_back({face,Id(old.surface.size())});
  const PlanarReference reference(Facets(old));Patch fresh,back;std::string reason;
  REQUIRE(Reconnect(old,3,100,reference,fresh,reason));CHECK(fresh.cells.size()==3);
  CHECK(fresh.surface.size()==old.surface.size());CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
  REQUIRE(Reconnect(fresh,0,200,reference,back,reason));CHECK(Connectivity(back)==Connectivity(old));
  CHECK(back.surface.size()==old.surface.size());CHECK(ValidateFixedInterface(old.cells,back.cells,reason));
  for(auto face:back.surface) CHECK(reference.Get(face.facet).marker==int(face.facet)+10);
  const auto saved=Connectivity(back);
  CHECK_FALSE(Reconnect(old,999,300,reference,back,reason));CHECK(Connectivity(back)==saved);
}

namespace {
Patch Pyramid() {
  const std::array<Node, 6> v{{{0,{0,0,0}}, {1,{1,0,0}}, {2,{1,1,0}}, {3,{0,1,0}},
                              {4,{.5,.5,0}}, {5,{.5,.5,1}}}};
  Patch result;
  for (size_t i=0;i<4;++i) {
    Cell cell{Id(i),{v[i],v[(i+1)%4],v[4],v[5]}};
    if (Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0) std::swap(cell.v[0],cell.v[1]);
    result.cells.push_back(cell);
  }
  for (auto f:Incidence(result.cells).Boundary()) result.surface.push_back({f,Id(result.surface.size())});
  return result;
}
std::vector<Facet> PyramidFacets(const Patch& patch) {
  auto result=Facets(patch);
  for (auto& f:result) if (f.v[0].p.z==0 && f.v[1].p.z==0 && f.v[2].p.z==0) f.marker=70;
  return result;
}
}
TEST_CASE("Native coplanar same-marker wall seams allow finite patch movement and collapse", "[NativeMesh3D]") {
  const auto old=Pyramid();const auto facets=PyramidFacets(old);const PlanarReference reference(facets);
  Id root=UINT64_MAX;
  for (auto f:facets) if (f.marker==70) root=std::min(root,f.id);
  for (auto f:facets) if (f.marker==70) {
    CHECK(reference.Canonical(f.id)==root);
    CHECK(reference.Contains(f.id,{.875,.875,0}));
    CHECK(reference.Contains(f.id,{0,0,0}));
    CHECK_FALSE(reference.Contains(f.id,{1.01,.5,0}));
    CHECK_FALSE(reference.Contains(f.id,{.5,.5,std::numeric_limits<double>::denorm_min()}));
  }
  Patch moved,coarse;std::string reason;
  REQUIRE(MoveVertex(old,4,{8,{.75,.5,0}},100,reference,moved,reason));
  REQUIRE(CollapseVertex(old,4,0,200,reference,coarse,reason));
  CHECK(coarse.cells.size()==2);CHECK(coarse.surface.size()==6);
  Patch inverse;
  REQUIRE(UndoEdgeSplit(old,4,{0,2},250,reference,inverse,reason));
  CHECK(inverse.cells.size()==2);CHECK(inverse.surface.size()==6);
  for (auto f:coarse.surface) if (reference.Get(f.facet).marker==70) CHECK(f.facet==root);
  const auto saved=Connectivity(coarse);
  CHECK_FALSE(MoveVertex(old,4,{9,{1.25,.5,0}},300,reference,coarse,reason));
  CHECK(Connectivity(coarse)==saved);CHECK(coarse.surface.size()==6);
  auto junction=facets;
  for (auto& f:junction) if (f.marker==70) f.marker=int(f.id)+100;
  CHECK_FALSE(CollapseVertex(old,4,0,300,PlanarReference(junction),coarse,reason));
  CHECK(Connectivity(coarse)==saved);
}
TEST_CASE("Native finite patch certification preserves marker creases disconnected and nonconvex surfaces", "[NativeMesh3D]") {
  const std::array<Node,6> v{{{0,{0,0,0}}, {1,{2,0,0}}, {2,{2,1,0}}, {3,{1,1,0}},
                             {4,{1,2,0}}, {5,{0,2,0}}}};
  const std::vector<Facet> concave{{0,7,{v[0],v[1],v[3]}},{1,7,{v[1],v[2],v[3]}},
                                  {2,7,{v[0],v[3],v[5]}},{3,7,{v[3],v[4],v[5]}}};
  const PlanarReference retained(concave);
  for (auto f:concave) {
    CHECK(retained.Canonical(f.id)==f.id);
    CHECK_FALSE(retained.Contains(f.id,{1.5,1.5,0}));
  }
  const Node a{10,{0,0,0}},b{11,{1,0,0}},c{12,{1,1,0}},d{13,{0,1,0}};
  const Facet first{10,9,{a,b,c}},second{20,9,{a,c,d}};
  const PlanarReference square({second,first});
  CHECK(square.Canonical(20)==10);CHECK(square.Contains(10,{.125,.875,0}));
  auto changed=second;changed.marker=10;
  CHECK(PlanarReference({first,changed}).Canonical(20)==20);
  changed=second;changed.v[2].p.z=std::numeric_limits<double>::denorm_min();
  CHECK(PlanarReference({first,changed}).Canonical(20)==20);
  changed=second;std::swap(changed.v[1],changed.v[2]);
  CHECK(PlanarReference({first,changed}).Canonical(20)==20);
  changed=second;for (auto& n:changed.v) { n.id+=100;n.p.x+=3; }
  CHECK(PlanarReference({first,changed}).Canonical(20)==20);
  auto tilted=std::vector<Facet>{first,second};
  for (auto& f:tilted) for (auto& n:f.v) n.p.z=n.p.x+n.p.y;
  const PlanarReference oblique(tilted);
  CHECK(oblique.Canonical(20)==10);CHECK(oblique.Contains(10,{.125,.875,1}));
  CHECK_FALSE(oblique.Contains(10,{.125,.875,std::nextafter(1.,2.)}));
  auto reordered=std::vector<Facet>{second,first};std::reverse(reordered.begin(),reordered.end());
  CHECK(PlanarReference(reordered).Canonical(20)==10);
  // A triangulated annulus cannot be certified as a filled square.
  const std::array<Node,8> ring{{{0,{0,0,0}}, {1,{3,0,0}}, {2,{3,3,0}}, {3,{0,3,0}},
                                {4,{1,1,0}}, {5,{2,1,0}}, {6,{2,2,0}}, {7,{1,2,0}}}};
  std::vector<Facet> hole;
  for (size_t i=0;i<4;++i) {
    const size_t j=(i+1)%4;
    hole.push_back({Id(2*i),9,{ring[i],ring[j],ring[4+j]}});
    hole.push_back({Id(2*i+1),9,{ring[i],ring[4+j],ring[4+i]}});
  }
  const PlanarReference annulus(hole);
  for (auto f:hole) { CHECK(annulus.Canonical(f.id)==f.id);CHECK_FALSE(annulus.Contains(f.id,{1.5,1.5,0})); }
}

TEST_CASE("Native insertion removes an exact-placement obstacle with certified cavity skin", "[NativeMesh3D][NativeInsertion3D]") {
  for (double height : {1., 1e-8}) {
    const auto old=Cube(height);const PlanarReference reference(Facets(old));
    const Node point{8,{.5,std::nextafter(.5,1.),.5*height}};
    Patch fresh;std::string reason;
    CHECK_FALSE(SplitEdge(old,{0,6},point,100,reference,fresh,reason));
    REQUIRE(InsertEdgeCavity(old,{0,6},point,100,reference,fresh,reason));
    CHECK(fresh.cells.size()==2*old.cells.size());
    CHECK(fresh.surface.size()==old.surface.size());
    CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
    for (const auto& cell:fresh.cells) CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    const auto saved=Connectivity(fresh);const auto faces=fresh.surface.size();
    CHECK_FALSE(InsertEdgeCavity(old,{0,6},{8,{1.5,.5,.5*height}},200,reference,fresh,reason));
    CHECK_FALSE(InsertEdgeCavity(old,{0,6},{0,point.p},200,reference,fresh,reason));
    CHECK_FALSE(InsertEdgeCavity(old,{0,6},point,UINT64_MAX,reference,fresh,reason));
    auto incomplete=old;incomplete.cells.pop_back();
    CHECK_FALSE(InsertEdgeCavity(incomplete,{0,6},point,200,reference,fresh,reason));
    CHECK(Connectivity(fresh)==saved);CHECK(fresh.surface.size()==faces);
  }
}
TEST_CASE("Native insertion coordinates a planar surface fan and volume without moving marker junctions", "[NativeMesh3D][NativeInsertion3D]") {
  for (double height : {1.,1e-8}) {
    const auto cube=Cube(height);const auto old=EdgeStar(cube,{0,2});auto facets=Facets(cube);
    for (auto& facet:facets) if (facet.v[0].p.z==0 && facet.v[1].p.z==0 && facet.v[2].p.z==0) facet.marker=70;
    const PlanarReference reference(facets);
    const Node point{8,{.5,std::nextafter(.5,1.),0}};Patch fresh;std::string reason;
    CHECK_FALSE(SplitEdge(old,{0,2},point,100,reference,fresh,reason));
    REQUIRE(InsertEdgeCavity(old,{0,2},point,100,reference,fresh,reason));
    CHECK(fresh.cells.size()==2*old.cells.size());CHECK(fresh.surface.size()==old.surface.size()+2);
    unsigned wall=0;
    std::map<Id,Point> nodes;
    for (const auto& cell:fresh.cells) for (auto node:cell.v) nodes.emplace(node.id,node.p);
    for (const auto& face:fresh.surface) {
      CHECK(reference.ContainsFace(face.facet,{nodes.at(face.v[0]),nodes.at(face.v[1]),nodes.at(face.v[2])}));
      if(reference.Get(face.facet).marker==70)++wall;
    }
    CHECK(wall==4);
    const auto saved=Connectivity(fresh);const auto faces=fresh.surface.size();
    const PlanarReference junction(Facets(cube));
    CHECK_FALSE(InsertEdgeCavity(old,{0,2},point,200,junction,fresh,reason));
    CHECK(reason.find("junction")!=std::string::npos);
    CHECK_FALSE(InsertEdgeCavity(old,{0,2},{8,{1.25,.5,0}},200,reference,fresh,reason));
    CHECK_FALSE(InsertEdgeCavity(old,{0,2},{8,{.5,.5,1e-16*height}},200,reference,fresh,reason));
    auto incomplete=old;incomplete.cells.pop_back();
    CHECK_FALSE(InsertEdgeCavity(incomplete,{0,2},point,200,reference,fresh,reason));
    CHECK(Connectivity(fresh)==saved);CHECK(fresh.surface.size()==faces);
  }
}

TEST_CASE("Native indexed reference binding matches exhaustive finite oriented canonical patches", "[NativeMesh3D][NativeReferenceSearch3D]") {
  std::vector<Facet> facets;
  constexpr size_t n=8;
  auto node=[](size_t i,size_t j){return Node{Id(j*(n+1)+i),{double(i)/n,double(j)/n,0}};};
  for(size_t j=0;j<n;++j)for(size_t i=0;i<n;++i) {
    const auto a=node(i,j),b=node(i+1,j),c=node(i+1,j+1),d=node(i,j+1);
    facets.push_back({Id(facets.size()),7,{a,b,c}});facets.push_back({Id(facets.size()),7,{a,c,d}});
  }
  const Node a{1000,{3,0,0}},b{1001,{4,0,0}},c{1002,{4,1,0}},d{1003,{3,1,0}};
  facets.push_back({1000,7,{a,b,c}});facets.push_back({1001,7,{a,c,d}});
  auto upper=facets.back();upper.id=1100;upper.marker=8;
  for(auto& v:upper.v){v.id+=100;v.p.z=1;}facets.push_back(upper);
  std::reverse(facets.begin(),facets.end());
  const PlanarReference reference(facets);
  CHECK(reference.Canonical(127)==0);CHECK(reference.Canonical(1001)==1000);
  CHECK(reference.ResidentBytes()>0);CHECK(reference.SearchWorkspaceBytes()>0);
  CHECK(PlanarReference::ConstructionBytes(facets.size())>=reference.ResidentBytes()+reference.SearchWorkspaceBytes());
  CHECK(PlanarReference::ConstructionBytes(SIZE_MAX)==SIZE_MAX);
  std::vector<std::array<Point,3>> queries;
  for(const auto& f:facets)queries.push_back({f.v[0].p,f.v[1].p,f.v[2].p});
  queries.push_back({{{.125,.125,0},{.875,.125,0},{.125,.875,0}}});
  queries.push_back({{{.5,.25,0},{3.5,.25,0},{.5,.75,0}}});
  queries.push_back({{{1.25,.25,0},{1.75,.25,0},{1.25,.75,0}}});
  auto off=queries.front();for(auto& v:off)v.z=std::nextafter(v.z,2.);queries.push_back(off);
  auto reverse=queries[1];std::swap(reverse[0],reverse[1]);queries.push_back(reverse);
  for(int marker:{7,8,999})for(const auto& triangle:queries) {
    Id expected=UINT64_MAX;
    for(const auto& f:facets)if(f.marker==marker && reference.ContainsFace(f.id,triangle))expected=std::min(expected,reference.Canonical(f.id));
    Id actual=UINT64_MAX-1;
    const bool found=reference.FindFace(marker,triangle,actual);
    REQUIRE(found==(expected!=UINT64_MAX));CHECK(actual==(found?expected:UINT64_MAX-1));
  }
  auto nonfinite=queries.front();nonfinite[0].x=std::numeric_limits<double>::infinity();Id saved=42;
  CHECK_THROWS_AS(reference.FindFace(7,nonfinite,saved),std::invalid_argument);CHECK(saved==42);
  const PlanarReference single({facets.front()});
  CHECK(single.SearchWorkspaceBytes()==0);
  Id id=42;CHECK(single.FindFace(facets.front().marker,queries.front(),id));CHECK(id==facets.front().id);
}

TEST_CASE("Native planar authority admits large tiled patches with constant border query work", "[NativeMesh3D][NativeReferenceSearch3D]") {
  for (size_t n : {size_t(4), size_t(32)}) {
    std::vector<Facet> facets;
    auto node=[&](size_t i,size_t j){return Node{Id(j*(n+1)+i),{double(i)/n,double(j)/n,0}};};
    for(size_t j=0;j<n;++j)for(size_t i=0;i<n;++i) {
      const auto a=node(i,j),b=node(i+1,j),c=node(i+1,j+1),d=node(i,j+1);
      facets.push_back({Id(facets.size()),7,{a,b,c}});facets.push_back({Id(facets.size()),7,{a,c,d}});
    }
    const auto budget=PlanarReference::ConstructionBytes(facets.size());
    CHECK_THROWS_AS(PlanarReference(facets,budget-1),std::invalid_argument);
    const PlanarReference reference(facets,budget);
    CHECK(reference.Canonical(facets.back().id)==0);
    KernelStats stats;
    CHECK(reference.Contains(0,{.5,.5,0},&stats));
    CHECK(stats.filtered_orientations+stats.exact_orientations==8);
    for(Point point : {Point{0,0,0},Point{1,1,0},Point{.125,.875,0}})CHECK(reference.Contains(0,point));
    CHECK_FALSE(reference.Contains(0,{std::nextafter(1.,2.),.5,0}));
    CHECK_FALSE(reference.Contains(0,{.5,.5,std::numeric_limits<double>::denorm_min()}));
    CHECK(reference.ContainsFace(0,{{{0,0,0},{1,0,0},{0,1,0}}}));
    CHECK_FALSE(reference.ContainsFace(0,{{{0,0,0},{0,1,0},{1,0,0}}}));
    auto tilted=facets;
    for(auto& f:tilted)for(auto& v:f.v)v.p.z=v.p.x+v.p.y;
    const PlanarReference oblique(tilted,budget);
    CHECK(oblique.Canonical(tilted.back().id)==0);
    CHECK(oblique.ContainsFace(0,{{{0,0,0},{1,0,1},{0,1,1}}}));
    CHECK_FALSE(oblique.Contains(0,{.5,.5,std::nextafter(1.,2.)}));
  }
}

namespace {
Patch CurvedSeed(bool sphere) {
  Patch p;
  std::array<Node,4> v=sphere ?
      std::array<Node,4>{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}} :
      std::array<Node,4>{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,-1}}}};
  if (Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0) std::swap(v[0],v[1]);
  p.cells.push_back({0,v});
  const auto skin=Incidence(p.cells).Boundary();
  for (size_t i=0;i<skin.size();++i) p.surface.push_back({skin[i],Id(i)});
  return p;
}
Patch CurvedVolumeSeed(bool sphere) {
  Patch p;
  if (sphere) {
    const double q=1/std::sqrt(3.);
    std::array<Node,4> v{{{0,{q,q,q}},{1,{-q,-q,q}},{2,{-q,q,-q}},{3,{q,-q,-q}}}};
    if (Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0) std::swap(v[0],v[1]);
    p.cells.push_back({0,v});
  } else {
    const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{0,1,0}},d{5,{1,1,0}},tip{3,{.5,.5,-1}};
    for (auto v:{std::array<Node,4>{a,b,c,tip},std::array<Node,4>{b,d,c,tip}}) {
      if (Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0) std::swap(v[0],v[1]);
      p.cells.push_back({Id(p.cells.size()),v});
    }
  }
  const auto skin=Incidence(p.cells).Boundary();
  for(size_t i=0;i<skin.size();++i)p.surface.push_back({skin[i],Id(i)});
  return p;
}
Id CurvedRoof(const Patch& p,bool sphere) {
  for (auto f:p.surface) {
    const Id excluded=sphere ? 0 : 3;
    if (std::find(f.v.begin(),f.v.end(),excluded)==f.v.end()) return f.facet;
  }
  throw std::logic_error("Missing curved fixture roof.");
}
Point Position(const Patch& p,Id id) {
  for (auto c:p.cells) for (auto n:c.v) if (n.id==id) return n.p;
  throw std::logic_error("Missing curved fixture vertex.");
}
void SaveCurved(const Patch& p,const PlanarReference& reference,const std::string& name) {
  if (!std::getenv("SU2_NATIVE_CURVED_EVIDENCE") || SU2_MPI::GetRank()) return;
  std::ofstream out(name+".json");
  REQUIRE(out.good());
  out<<std::setprecision(17)<<"{\"nodes\":[";
  std::map<Id,Point> nodes;
  for(auto c:p.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
  bool comma=false;
  for(auto n:nodes){if(comma)out<<',';comma=true;out<<'['<<n.first<<','<<n.second.x<<','<<n.second.y<<','<<n.second.z<<']';}
  out<<"],\"cells\":[";comma=false;
  for(auto c:p.cells){if(comma)out<<',';comma=true;out<<'['<<c.v[0].id<<','<<c.v[1].id<<','<<c.v[2].id<<','<<c.v[3].id<<']';}
  out<<"],\"surface\":[";comma=false;
  for(auto f:p.surface){if(comma)out<<',';comma=true;out<<'['<<f.v[0]<<','<<f.v[1]<<','<<f.v[2]<<','<<f.facet<<','<<reference.Get(f.facet).marker<<']';}
  out<<"]}\n";out.close();REQUIRE(out.good());
}
long double Volume(const Patch& p) {
  long double v=0;
  for (auto c:p.cells) v+=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)/6;
  return v;
}
}
TEST_CASE("Analytic sphere and bump splits rebuild positive edge stars and reverse changed volumes",
          "[NativeCurvedGeometry3D]") {
  for (bool sphere:{true,false}) {
    const auto old=CurvedVolumeSeed(sphere);
    AnalyticSurface g;
    g.kind=sphere ? AnalyticSurface::Kind::Sphere : AnalyticSurface::Kind::Bump;
    g.extent=sphere ? Point{1,0,0} : Point{1,1,1};
    std::map<Id,AnalyticSurface> curves;
    for(auto f:old.surface)
      if(sphere || std::find(f.v.begin(),f.v.end(),3)==f.v.end())curves.emplace(f.facet,g);
    const auto roof=curves.begin()->first;
    const PlanarReference reference(Facets(old),SIZE_MAX,curves);
    const auto a=Position(old,1),b=Position(old,2);
    const Point chord{(a.x+b.x)/2,(a.y+b.y)/2,(a.z+b.z)/2};
    Patch split,undone;
    std::string reason;
    const bool valid=SplitEdge(old,{1,2},{4,chord},100,reference,split,reason);
    INFO(reason);REQUIRE(valid);
    const auto p=Position(split,4);
    if (sphere) {
      CHECK(p.x==-1);
      CHECK(p.y==0);
      CHECK(p.z==0);
    } else { CHECK(p.x==.5);CHECK(p.y==.5);CHECK(p.z==1./16); }
    CHECK(Volume(split)>Volume(old)); // Physical faceting changes; equal old/new volume is the wrong geometry gate.
    SaveCurved(old,reference,sphere?"sphere_original":"bump_original");
    SaveCurved(split,reference,sphere?"sphere_split":"bump_split");
    CHECK(split.cells.size()==2*old.cells.size());
    CHECK(split.surface.size()==old.surface.size()+2);
    for (auto c:split.cells) CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
    for (auto c:old.cells) for (auto n:c.v) {
      const auto kept=Position(split,n.id);
      CHECK(kept.x==n.p.x);CHECK(kept.y==n.p.y);CHECK(kept.z==n.p.z);
    }
    for (auto f:split.surface)
      CHECK(reference.ContainsFace(f.facet,{Position(split,f.v[0]),Position(split,f.v[1]),Position(split,f.v[2])}));
    for(auto f:old.surface)
      if(curves.count(f.facet) && std::find(f.v.begin(),f.v.end(),1)!=f.v.end() && std::find(f.v.begin(),f.v.end(),2)!=f.v.end())
        CHECK(reference.OnProjectedEdge(f.facet,a,b,p));
    REQUIRE(UndoEdgeSplit(split,4,{1,2},200,reference,undone,reason));
    SaveCurved(undone,reference,sphere?"sphere_coarsened":"bump_coarsened");
    CHECK(Connectivity(undone)==Connectivity(old));
    CHECK(Volume(undone)==Volume(old));
    CHECK(undone.surface.size()==old.surface.size());
    for (int cycle=0;cycle<3;++cycle) {
      REQUIRE(SplitEdge(undone,{1,2},{4,chord},300+20*cycle,reference,split,reason));
      REQUIRE(UndoEdgeSplit(split,4,{1,2},310+20*cycle,reference,undone,reason));
      CHECK(Volume(undone)==Volume(old));
    }
  }
}
TEST_CASE("Analytic reference binds complete finite oriented faces beyond original facet boxes",
          "[NativeCurvedGeometry3D]") {
  const auto old=CurvedSeed(true);
  auto facets=Facets(old);
  for (auto& f:facets) f.marker=7; // Multiple same-marker boxes exercise the existing indexed binding path.
  const auto roof=CurvedRoof(old,true);
  const PlanarReference reference(facets,SIZE_MAX,{{roof,AnalyticSurface{}}});
  const auto p=reference.Project(roof,{.5,.5,0});
  Id found=999;
  const auto original=reference.Get(roof);
  std::array<Point,3> face;
  for (size_t i=0;i<3;++i) face[i]=original.v[i].id==2 ? p : original.v[i].p;
  CHECK(reference.FindFace(7,face,found));CHECK(found==roof);
  CHECK_FALSE(reference.FindFace(8,face,found));CHECK(found==roof);
  std::swap(face[0],face[1]);CHECK_FALSE(reference.ContainsFace(roof,face));
  CHECK_FALSE(reference.Contains(roof,{-1,0,0}));
  CHECK_FALSE(reference.Contains(roof,{.5,.5,0}));
  CHECK_THROWS(reference.Project(roof,{0,0,0}));
  CHECK_THROWS(reference.Project(roof,{-1,0,0}));
  CHECK_THROWS(reference.Project(roof,{INFINITY,0,0}));
  CHECK_FALSE(reference.OnProjectedEdge(roof,{1,0,0},{0,1,0},{0,0,1}));
  CHECK(reference.ResidentBytes()>PlanarReference(facets).ResidentBytes());
  CHECK_THROWS(PlanarReference(facets,PlanarReference::ConstructionBytes(facets.size()),{{roof,AnalyticSurface{}}}));
  auto bad=AnalyticSurface{};bad.extent.x=0;
  CHECK_THROWS(PlanarReference(facets,SIZE_MAX,{{roof,bad}}));
  bad=AnalyticSurface{};bad.kind=static_cast<AnalyticSurface::Kind>(99);
  CHECK_THROWS(PlanarReference(facets,SIZE_MAX,{{roof,bad}}));
  bad=AnalyticSurface{};bad.tolerance=0;
  CHECK_THROWS(PlanarReference(facets,SIZE_MAX,{{roof,bad}}));
  CHECK_THROWS(PlanarReference(facets,SIZE_MAX,{{999,AnalyticSurface{}}}));
  auto wrong=facets;
  for(auto& f:wrong) for(auto& n:f.v) if(n.id==1)n.p.x=1.01;
  CHECK_THROWS(PlanarReference(wrong,SIZE_MAX,{{roof,AnalyticSurface{}}}));
}
TEST_CASE("Projected sphere split rejects inversion without reorienting children or changing outputs",
          "[NativeCurvedGeometry3D]") {
  const Node a{0,{1,0,0}},b{1,{0,1,0}},c{2,{0,0,1}},d{3,{0,0,-1}},tip{4,{.55,.55,0}};
  Patch old;
  for (auto v:{std::array<Node,4>{a,b,c,tip},std::array<Node,4>{a,b,d,tip}}) {
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);
    old.cells.push_back({Id(old.cells.size()),v});
  }
  for (auto f:Incidence(old.cells).Boundary())
    if(std::find(f.begin(),f.end(),0)!=f.end()&&std::find(f.begin(),f.end(),1)!=f.end())
      old.surface.push_back({f,Id(old.surface.size())});
  REQUIRE(old.surface.size()==2);
  std::map<Id,AnalyticSurface> curves;
  for(auto f:old.surface)curves.emplace(f.facet,AnalyticSurface{});
  const PlanarReference reference(Facets(old),SIZE_MAX,curves);
  Patch retained=old;std::string reason;
  CHECK_FALSE(SplitEdge(old,{0,1},{5,{.5,.5,0}},100,reference,retained,reason));
  CHECK(reason.find("inverts inherited")!=std::string::npos);
  CHECK(Connectivity(retained)==Connectivity(old));CHECK(Volume(retained)==Volume(old));
  CHECK(retained.surface.size()==old.surface.size());
  auto incomplete=old;incomplete.surface.pop_back();
  CHECK_FALSE(SplitEdge(incomplete,{0,1},{5,{.5,.5,0}},100,reference,retained,reason));
  CHECK(Connectivity(retained)==Connectivity(old));
}

TEST_CASE("Curved projection cannot silently enlarge an adjoining finite planar reference patch",
          "[NativeCurvedGeometry3D]") {
  for(bool sphere:{true,false}) {
    const auto old=CurvedSeed(sphere);
    AnalyticSurface g;g.kind=sphere?AnalyticSurface::Kind::Sphere:AnalyticSurface::Kind::Bump;
    g.extent=sphere?Point{1,0,0}:Point{1,1,1};
    const auto roof=CurvedRoof(old,sphere);
    const PlanarReference reference(Facets(old),SIZE_MAX,{{roof,g}});
    Patch retained=old;std::string reason;
    CHECK_FALSE(SplitEdge(old,{1,2},{4,{.5,.5,0}},100,reference,retained,reason));
    CHECK(reason.find("incident physical feature")!=std::string::npos);
    CHECK(Connectivity(retained)==Connectivity(old));CHECK(Volume(retained)==Volume(old));
    CHECK(retained.surface.size()==old.surface.size());
  }
}

TEST_CASE("Analytic finite chart tolerance cannot escape an acute corner along infinite edge planes",
          "[NativeCurvedGeometry3D]") {
  AnalyticSurface bump;
  bump.kind=AnalyticSurface::Kind::Bump;bump.origin={-1,-1,0};bump.extent={2,2,1};
  auto height=[](Point p){const double u=(p.x+1)/2,v=(p.y+1)/2;p.z=u*(1-u)*v*(1-v);return p;};
  const Facet f{0,0,{{{0,height({0,0,0})},{1,height({.5,.5,0})},{2,height({.50000001,.5,0})}}}};
  const PlanarReference graph({f},SIZE_MAX,{{0,bump}});
  CHECK(graph.Contains(0,f.v[0].p));
  CHECK_FALSE(graph.Contains(0,height({-1e-6,-1e-6,0})));
  CHECK(graph.Contains(0,height({-1e-13,-1e-13,0})));
  AnalyticSurface sphere;
  auto unit=[](Point p){const auto r=std::hypot(p.x,p.y,p.z);return Point{p.x/r,p.y/r,p.z/r};};
  const Facet cone{0,0,{{{0,unit({1,0,0})},{1,unit({1,1e-8,1e-8})},{2,unit({1,1e-8,1e-8+1e-16})}}}};
  const PlanarReference radial({cone},SIZE_MAX,{{0,sphere}});
  CHECK(radial.Contains(0,cone.v[0].p));
  CHECK_FALSE(radial.Contains(0,unit({1,-1e-6,-1e-6})));
  CHECK(radial.Contains(0,unit({1,-1e-13,-1e-13})));
}

namespace {
PlanarReference BumpChart(const Patch& seed) {
  auto facets=Facets(seed);std::map<Id,AnalyticSurface> curves;
  AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,1};
  for(auto& f:facets) {
    const bool roof=std::none_of(f.v.begin(),f.v.end(),[](Node n){return n.id==3;});
    if(roof){f.marker=7;curves.emplace(f.id,g);}
  }
  return PlanarReference(facets,SIZE_MAX,curves);
}
void CheckCurvedPatch(const Patch& patch,const PlanarReference& reference) {
  for(auto c:patch.cells) {
    const Tetrahedron t{c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p};
    const auto m=Measure(t,{});
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);
  }
  for(auto f:patch.surface)
    CHECK(reference.ContainsFace(f.facet,{Position(patch,f.v[0]),Position(patch,f.v[1]),Position(patch,f.v[2])}));
}
}
TEST_CASE("Bump reconstruction movement and collapse cross harmless original triangle seams as one finite chart",
          "[NativeCurvedChart3D]") {
  const auto old=CurvedVolumeSeed(false);const auto reference=BumpChart(old);
  std::vector<Id> roof;
  for(auto f:old.surface)if(reference.Analytic().count(f.facet))roof.push_back(f.facet);
  REQUIRE(roof.size()==2);REQUIRE(reference.Canonical(roof[0])==reference.Canonical(roof[1]));
  const auto root=reference.Canonical(roof[0]);
  const auto p=reference.Project(root,{.8,.7,0});CHECK(p.z==Approx(.0336).epsilon(1e-14));
  CHECK(reference.Contains(roof[0],p));CHECK(reference.Contains(roof[1],p));
  Patch inserted,moved,collapsed;std::string reason;
  auto valid=InsertEdgeCavity(old,{1,2},{4,{.6,.3,0}},100,reference,inserted,reason);INFO(reason);REQUIRE(valid);
  CHECK(inserted.cells.size()==4);CHECK(Position(inserted,4).z==Approx(.0504).epsilon(1e-14));
  CheckCurvedPatch(inserted,reference);
  valid=MoveVertex(inserted,4,{7,{.8,.7,0}},200,reference,moved,reason);INFO(reason);REQUIRE(valid);
  CHECK(Position(moved,7).z==Approx(.0336).epsilon(1e-14));CHECK(moved.cells.size()==4);
  CheckCurvedPatch(moved,reference);
  valid=CollapseVertex(moved,7,0,300,reference,collapsed,reason);INFO(reason);REQUIRE(valid);
  CHECK(collapsed.cells.size()==2);CHECK(Volume(collapsed)==Volume(old));
  CHECK(Connectivity(collapsed)!=Connectivity(old)); // A different coarse diagonal is permitted across the original seam.
  CheckCurvedPatch(collapsed,reference);
  for(auto cell:old.cells)for(auto node:cell.v) {
    const auto kept=Position(moved,node.id);CHECK(kept.x==node.p.x);CHECK(kept.y==node.p.y);CHECK(kept.z==node.p.z);
  }
  SaveCurved(old,reference,"joint_bump_original");SaveCurved(inserted,reference,"joint_bump_inserted");
  SaveCurved(moved,reference,"joint_bump_moved");SaveCurved(collapsed,reference,"joint_bump_collapsed");
  for(int i=0;i<3;++i) {
    const Edge diagonal=i%2 ? Edge{1,2} : Edge{0,5};
    const auto next=EdgeStar(collapsed,diagonal);
    valid=InsertEdgeCavity(next,diagonal,{8,{.35,.55,0}},400+30*i,reference,inserted,reason);INFO(reason);REQUIRE(valid);
    valid=CollapseVertex(inserted,8,i%2 ? 0 : 1,410+30*i,reference,collapsed,reason);INFO(reason);REQUIRE(valid);
    CHECK(collapsed.cells.size()==2);CheckCurvedPatch(collapsed,reference);
  }
}
TEST_CASE("Curved joint proposals reject missing interfaces invalid projections and marker junction crossing atomically",
          "[NativeCurvedChart3D]") {
  const auto old=CurvedVolumeSeed(false);const auto reference=BumpChart(old);
  Patch retained=old;std::string reason;
  CHECK_FALSE(InsertEdgeCavity(old,{1,2},{4,{1.5,.3,0}},100,reference,retained,reason));
  CHECK(Connectivity(retained)==Connectivity(old));CHECK(Volume(retained)==Volume(old));
  auto incomplete=old;incomplete.surface.clear();
  CHECK_FALSE(InsertEdgeCavity(incomplete,{1,2},{4,{.6,.3,0}},100,reference,retained,reason));
  CHECK(Connectivity(retained)==Connectivity(old));
  Patch inserted;REQUIRE(InsertEdgeCavity(old,{1,2},{4,{.6,.3,0}},100,reference,inserted,reason));
  const auto saved=Connectivity(retained);
  CHECK_FALSE(MoveVertex(inserted,4,{7,{.6,.3,1}},200,reference,retained,reason));
  CHECK(reason.find("projects back")!=std::string::npos);CHECK(Connectivity(retained)==saved);
  CHECK_FALSE(MoveVertex(inserted,4,{7,{INFINITY,.3,0}},200,reference,retained,reason));CHECK(Connectivity(retained)==saved);
  CHECK_FALSE(CollapseVertex(inserted,4,3,200,reference,retained,reason));CHECK(Connectivity(retained)==saved);
  auto f=Facets(old);std::map<Id,AnalyticSurface> curves;
  AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,1};
  std::vector<Id> roof;
  for(auto& face:f)if(std::none_of(face.v.begin(),face.v.end(),[](Node n){return n.id==3;})){roof.push_back(face.id);face.marker=7;curves.emplace(face.id,g);}
  f.at(roof[1]).marker=8;
  const PlanarReference junction(f,SIZE_MAX,curves);
  CHECK(junction.Canonical(roof[0])!=junction.Canonical(roof[1]));
  CHECK_FALSE(InsertEdgeCavity(old,{1,2},{4,{.6,.3,0}},100,junction,retained,reason));CHECK(Connectivity(retained)==saved);
  f.at(roof[1]).marker=7;curves.at(roof[1]).extent.z=2;
  const PlanarReference different(f,SIZE_MAX,curves);
  CHECK(different.Canonical(roof[0])!=different.Canonical(roof[1]));
}
TEST_CASE("Bump chart unions retain distinct disconnected components and original source authority",
          "[NativeCurvedChart3D]") {
  const auto old=CurvedVolumeSeed(false);auto f=Facets(old);
  f.erase(std::remove_if(f.begin(),f.end(),[](const Facet& face){return std::any_of(face.v.begin(),face.v.end(),[](Node n){return n.id==3;});}),f.end());
  REQUIRE(f.size()==2);
  AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={4,4,1};
  auto height=[](Point p){const double u=p.x/4,v=p.y/4;p.z=u*(1-u)*v*(1-v);return p;};
  for(auto& face:f){face.marker=7;for(auto& n:face.v)n.p=height(n.p);}
  const auto first=f;
  for(auto face:first){face.id+=100;for(auto& n:face.v){n.id+=100;n.p.x+=2;n.p=height(n.p);}f.push_back(face);}
  std::map<Id,AnalyticSurface> curves;for(auto face:f)curves.emplace(face.id,g);
  const PlanarReference reference(f,SIZE_MAX,curves);
  CHECK(reference.Canonical(f[0].id)==reference.Canonical(f[1].id));
  CHECK(reference.Canonical(f[2].id)==reference.Canonical(f[3].id));
  CHECK(reference.Canonical(f[0].id)!=reference.Canonical(f[2].id));
  CHECK_FALSE(reference.Contains(f[0].id,height({1.5,.5,0})));
  CHECK_FALSE(reference.Contains(f[2].id,height({1.5,.5,0})));
  CHECK_FALSE(reference.Contains(f[0].id,height({2.5,.5,0})));
  CHECK(reference.Contains(f[2].id,height({2.5,.5,0})));
  CHECK(reference.Get(f[1].id).v[0].p.x==f[1].v[0].p.x);
  CHECK_THROWS(PlanarReference(f,PlanarReference::ConstructionBytes(f.size()),curves));
  CHECK(reference.ResidentBytes()>PlanarReference(f).ResidentBytes());
}

TEST_CASE("Declared frozen sensor coverage evaluates actual curved replacement vertices centroids and edges",
          "[NativeCurvedLayered3D]") {
  const auto old=CurvedVolumeSeed(true);auto facets=Facets(old);std::map<Id,AnalyticSurface> curves;
  std::vector<Id> components;for(auto f:facets){curves.emplace(f.id,AnalyticSurface{});components.push_back(f.id);}
  const PlanarReference reference(facets,SIZE_MAX,curves);
  std::vector<DonorCell> donors;DonorCell original;original.cell=old.cells[0];
  for(auto& m:original.sensor)m={.5,0,0,.5,0,.5};donors.push_back(original);
  auto add=[&](std::array<Node,4> v,Id component) {
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);
    DonorCell d;d.cell={Id(donors.size()),v};d.extension_component=component;
    for(auto& m:d.sensor)m={.5,0,0,.5,0,.5};donors.push_back(d);
  };
  // Declared radial shell reaches factor4 before freezing; source target is sensor-only, never a nodal BL tensor.
  for(auto f:facets) {
    std::sort(f.v.begin(),f.v.end(),[](Node a,Node b){return a.id<b.id;});
    const auto a=f.v[0],b=f.v[1],c=f.v[2];auto aa=a,bb=b,cc=c;
    for(auto* n:{&aa,&bb,&cc}){n->id+=4;n->p.x*=4;n->p.y*=4;n->p.z*=4;}
    add({a,b,c,cc},f.id);add({a,b,bb,cc},f.id);add({a,aa,bb,cc},f.id);
  }
  FrozenField field(donors,{},false,components),original_only({original});
  Patch fresh;std::string reason;const auto a=Position(old,1),b=Position(old,2);
  REQUIRE(SplitEdge(old,{1,2},{8,{(a.x+b.x)/2,(a.y+b.y)/2,(a.z+b.z)/2}},100,reference,fresh,reason));
  const auto wall=Position(fresh,8);
  CHECK_THROWS(original_only.Query(wall));CHECK(field.Query(wall).extension_component!=UINT64_MAX);
  CHECK(field.Query({0,0,0}).sensor.xx==original_only.Query({0,0,0}).sensor.xx);
  for(auto cell:fresh.cells) {
    const auto m=field.Query(Centroid(cell));CHECK(m.sensor.xx==.5);
    const auto q=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},m.target);
    CHECK(q.mean_ratio>=MinimumMeanRatio);CHECK(q.minimum_scaled_jacobian>=MinimumScaledJacobian);
    for(size_t i=0;i<4;++i)for(size_t j=i+1;j<4;++j) {
      const auto length=field.TargetEdgeLength(cell.v[i].p,cell.v[j].p);
      CHECK(double(length)==Approx(double(MetricNorm(cell.v[i].p,cell.v[j].p,{.5,0,0,.5,0,.5}))).epsilon(1e-12));
      CHECK(length<=1.6L);
    }
  }
  CHECK_THROWS(field.Query({10,0,0}));
  SaveCurved(fresh,reference,"layered_sphere_split");
}

TEST_CASE("Native3D analytic surface bounds cover complete face interiors", "[NativeSurfaceError3D]") {
  const std::array<Point,3> sphere{{{1,0,0},{0,1,0},{0,0,1}}};
  const std::array<Point,3> bump{{{0,0,0},{1,0,0},{0,1,0}}};
  for(bool radial:{false,true}) {
    const auto points=radial?sphere:bump;
    Facet facet{0,0,{{{0,points[0]},{1,points[1]},{2,points[2]}}}};
    AnalyticSurface g;g.kind=radial?AnalyticSurface::Kind::Sphere:AnalyticSurface::Kind::Bump;g.extent=radial?Point{1,0,0}:Point{1,1,-.01};
    PlanarReference geometry({facet},SIZE_MAX,{{0,g}}),flat({facet});
    CHECK(flat.SurfaceError(0,points)==0);
    const auto error=geometry.SurfaceError(0,points);CHECK(error>0);
    for(int i=0;i<=20;++i)for(int j=0;j<=20-i;++j) {
      const double u=i/20.,v=j/20.;Point p{points[0].x*(1-u-v)+u*points[1].x+v*points[2].x,
        points[0].y*(1-u-v)+u*points[1].y+v*points[2].y,points[0].z*(1-u-v)+u*points[1].z+v*points[2].z};
      const auto projected=geometry.Project(0,p);
      CHECK(std::hypot(projected.x-p.x,projected.y-p.y,projected.z-p.z)<=error);
    }
  }
}

TEST_CASE("Sphere surface error uses the finite triangle when the plane foot lies outside", "[NativeSurfaceError3D]") {
  const double angle=.15;
  const std::array<Point,3> points{{{1,0,0},{std::cos(angle),std::sin(angle),0},
    {std::cos(.6)*std::cos(.2),std::sin(.6)*std::cos(.2),std::sin(.2)}}};
  const Facet facet{0,0,{{{0,points[0]},{1,points[1]},{2,points[2]}}}};
  AnalyticSurface sphere;
  const PlanarReference geometry({facet},SIZE_MAX,{{0,sphere}});
  const double error=geometry.SurfaceError(0,points);
  const double expected=1-std::sqrt((1+points[2].x)/2);
  CHECK(error>=expected-1e-16);
  CHECK(error<expected+1e-12);
  for(int i=0;i<=30;++i)for(int j=0;j<=30-i;++j) {
    const double u=i/30.,v=j/30.;
    const Point p{points[0].x*(1-u-v)+u*points[1].x+v*points[2].x,
                  points[0].y*(1-u-v)+u*points[1].y+v*points[2].y,
                  points[0].z*(1-u-v)+u*points[1].z+v*points[2].z};
    CHECK(1-std::hypot(p.x,p.y,p.z)<=error);
  }
}

namespace {
#include "fixtures/native_joint_obstruction.inc"
}
TEST_CASE("Joint curved split reconstructs a saved obstruction under unchanged frozen gates", "[NativeJointGeometry3D]") {
  const auto old=JointObstruction();const auto reference=JointReference();
  std::map<Id,Point> points;for(const auto& c:old.cells)for(auto n:c.v)points.emplace(n.id,n.p);
  const Edge edge{6,676};const auto a=points.at(edge[0]),b=points.at(edge[1]);
  const Node point{200000,{double((static_cast<long double>(a.x)+b.x)/2),double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};
  Patch fresh;Face obstruction{};std::string reason;
  const Tensor unit{.5,0,0,.5,0,.5};
  const bool rebuilt=ReconstructSplitPatch(old,edge,point,{200001,{}},unit,300000,reference,fresh,obstruction,reason);INFO(reason);REQUIRE(rebuilt);
  CHECK(fresh.cells.size()>old.cells.size());
  DonorCell donor;donor.cell={0,{{{0,{-10,-10,-10}},{1,{30,-10,-10}},{2,{-10,30,-10}},{3,{-10,-10,30}}}}};donor.sensor.fill(unit);
  FrozenField field({donor});PatchMetric metric;
  REQUIRE(ValidateMetricChange(old.cells,fresh.cells,MetricChange::GeometryRefine,field,metric,reason));
  auto excess=[&](const Patch& p){std::map<Id,Point> nodes;for(const auto& c:p.cells)for(auto n:c.v)nodes[n.id]=n.p;long double sum=0;for(const auto& f:p.surface){const std::array<Point,3> t{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};const auto e=std::max(0.,reference.SurfaceError(f.facet,t)-.04);const auto a=t[0],b=t[1],c=t[2];sum+=e*e*std::hypot((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y),(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z),(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));}return sum;};
  CHECK(excess(fresh)<excess(old));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)CHECK(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  auto incomplete=old;incomplete.surface.erase(std::remove_if(incomplete.surface.begin(),incomplete.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end();}),incomplete.surface.end());Patch retained=fresh;
  CHECK_FALSE(ReconstructSplitPatch(incomplete,edge,point,{200001,{}},unit,300000,reference,retained,obstruction,reason));
  CHECK(Connectivity(retained)==Connectivity(fresh));
}

namespace {
#include "fixtures/native_strip_obstruction.inc"
}
TEST_CASE("Coordinated surface strip preserves geometry while rejecting the old cap coverage gap", "[NativeSurfaceStrip3D]") {
  const auto old=StripObstruction();const auto reference=JointReference();
  const auto moves=StripMoves();const auto point=StripPoint();Patch fresh;std::string reason;
  const bool built=SplitSurfaceStrip(old,{12,1604},point,moves,300000,reference,fresh,reason);
  INFO(reason);REQUIRE(built);REQUIRE(fresh.cells.size()==30);
  const auto donors=StripSource();FrozenField field(donors,{},false,AssociatedExtensions(donors,reference));
  PatchMetric measured;
  const bool metric=ValidateMetricChange(old.cells,fresh.cells,MetricChange::GeometryRefine,field,measured,reason);
  INFO(reason);CHECK_FALSE(metric);
  CHECK(reason.find("Uncovered native3D original/extension source interval.")!=std::string::npos);
  auto excess=[&](const Patch& patch){long double sum=0;for(const auto& f:patch.surface){
    const auto a=Position(patch,f.v[0]),b=Position(patch,f.v[1]),c=Position(patch,f.v[2]);
    const auto e=std::max(0.,reference.SurfaceError(f.facet,{a,b,c})-.04);
    sum+=e*e*std::hypot((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y),
                       (b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z),
                       (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));}return sum;};
  CHECK(excess(fresh)<.94L*excess(old));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)
    REQUIRE(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  const auto outside=StripOutside();
  for(const auto& a:fresh.cells)for(const auto& b:outside)REQUIRE(ConformingDisjoint(a,b));
  SaveCurved(old,reference,"strip_original");SaveCurved(fresh,reference,"strip_repaired");
  SECTION("Missing dependency rejects the complete proposal atomically") {
    auto incomplete=old;incomplete.cells.erase(incomplete.cells.begin());
    const auto retained=fresh;
    CHECK_FALSE(SplitSurfaceStrip(incomplete,{12,1604},point,moves,400000,reference,fresh,reason));
    CHECK(Connectivity(fresh)==Connectivity(retained));
    for(const auto& m:moves){const auto a=Position(fresh,m.second.id),b=Position(retained,m.second.id);
      CHECK(a.x==b.x);CHECK(a.y==b.y);CHECK(a.z==b.z);}
  }
  SECTION("Duplicate movement identities reject atomically") {
    auto duplicate=moves;duplicate.back().second.id=duplicate.front().second.id;const auto retained=fresh;
    CHECK_FALSE(SplitSurfaceStrip(old,{12,1604},point,duplicate,400000,reference,fresh,reason));
    CHECK(Connectivity(fresh)==Connectivity(retained));
  }
  SECTION("Finite chart and feature departure rejects atomically") {
    auto bad=moves;bad.back().second.p={10,10,10};const auto retained=fresh;
    CHECK_FALSE(SplitSurfaceStrip(old,{12,1604},point,bad,400000,reference,fresh,reason));
    CHECK(Connectivity(fresh)==Connectivity(retained));
  }
}

TEST_CASE("Native3D joint first-height construction rebuilds wall children and transition", "[NativeFirstLayer3D]") {
  const auto old=Cube();const auto facets=Facets(old);const PlanarReference reference(facets);
  std::map<int,double> heights;
  for(const auto& face:facets)if(std::all_of(face.v.begin(),face.v.end(),[](Node n){return n.p.z==0;}))heights[face.marker]=.05;
  REQUIRE(heights.size()==2);
  FirstHeightReport before,after;std::string reason;Patch fresh;
  CHECK_FALSE(CheckFirstHeight(old,heights,reference,before,reason));CHECK(before.height_failures==2);
  REQUIRE(ConstructFirstLayer(old,{100,{.5,.5,.5}},101,200,heights,reference,fresh,after,reason));
  CHECK(fresh.cells.size()==18);CHECK(fresh.surface.size()==old.surface.size());
  CHECK(after.wall_faces==2);CHECK(after.height_failures==0);CHECK(after.maximum_tilt<1e-12);
  CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
  double volume=0;for(const auto& cell:fresh.cells) {
    const auto v=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p);
    REQUIRE(v>0);volume+=double(v/6);
  }
  CHECK(volume==Approx(1).epsilon(1e-14));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)
    REQUIRE(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  SaveCurved(old,reference,"first_height_initial");SaveCurved(fresh,reference,"first_height_constructed");
  for(auto& h:heights)h.second=.025;
  Patch updated;REQUIRE(ConstructFirstLayer(fresh,{300,{.5,.5,.5}},301,400,heights,reference,updated,after,reason));
  CHECK(updated.cells.size()==18);CHECK(after.wall_faces==2);CHECK(after.maximum_relative_error<1e-8);
  CHECK(ValidateFixedInterface(old.cells,updated.cells,reason));
  for(size_t i=0;i<updated.cells.size();++i)for(size_t j=i+1;j<updated.cells.size();++j)
    REQUIRE(ConformingDisjoint(updated.cells[i],updated.cells[j]));
  SaveCurved(updated,reference,"first_height_updated");
  FirstHeightReport released;CHECK(CheckFirstHeight(updated,{},reference,released,reason));CHECK(released.wall_faces==0);
}

TEST_CASE("Native3D first-layer obstruction rejects atomically without height relaxation", "[NativeFirstLayer3D]") {
  const auto old=Cube();const auto facets=Facets(old);const PlanarReference reference(facets);
  std::map<int,double> heights;
  for(const auto& f:facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.05;
  const auto expected=Connectivity(old);
  for(const auto background:std::vector<Node>{{100,{.5,.5,.01}},{0,{.5,.5,.5}},{100,{2,2,2}}}) {
    Patch output=old;FirstHeightReport report;std::string reason;
    CHECK_FALSE(ConstructFirstLayer(old,background,101,200,heights,reference,output,report,reason));
    CHECK_FALSE(reason.empty());CHECK(Connectivity(output)==expected);CHECK(output.surface.size()==old.surface.size());
    for(size_t i=0;i<old.cells.size();++i)for(unsigned k=0;k<4;++k) {
      CHECK(output.cells[i].v[k].id==old.cells[i].v[k].id);
      CHECK(output.cells[i].v[k].p.x==old.cells[i].v[k].p.x);
      CHECK(output.cells[i].v[k].p.y==old.cells[i].v[k].p.y);
      CHECK(output.cells[i].v[k].p.z==old.cells[i].v[k].p.z);
    }
  }
  Patch output=old;FirstHeightReport report;std::string reason;
  CHECK_FALSE(ConstructFirstLayer(old,{100,{.5,.5,.5}},0,200,heights,reference,output,report,reason));
  CHECK(Connectivity(output)==expected);
  for(auto& h:heights)h.second=std::numeric_limits<double>::quiet_NaN();
  CHECK_FALSE(ConstructFirstLayer(old,{100,{.5,.5,.5}},101,200,heights,reference,output,report,reason));
  CHECK(Connectivity(output)==expected);
}

TEST_CASE("Native3D first-height certificate reports tangential tilt independently", "[NativeFirstLayer3D]") {
  const auto old=Cube();const auto facets=Facets(old);const PlanarReference reference(facets);
  std::map<int,double> heights;
  for(const auto& f:facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.05;
  Patch fresh;FirstHeightReport report;std::string reason;
  REQUIRE(ConstructFirstLayer(old,{100,{.5,.5,.5}},101,200,heights,reference,fresh,report,reason));
  for(auto& cell:fresh.cells)for(auto& node:cell.v)if(node.id==101)node.p.x+=.001;
  CHECK(CheckFirstHeight(fresh,heights,reference,report,reason));CHECK(report.maximum_tilt==Approx(.02).epsilon(1e-10));
  CHECK(report.height_failures==0);CHECK(report.footprint_failures==0);
  // Isolate the positive wall tetrahedron so the footprint test does not invert its transition neighbours.
  Patch child;
  for(const auto& face:fresh.surface)for(const auto& cell:fresh.cells) {
    const bool wall=std::all_of(face.v.begin(),face.v.end(),[&](Id id) {
      return std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return n.id==id && n.p.z==0;});
    });
    if(wall && std::any_of(cell.v.begin(),cell.v.end(),[](Node n){return n.id==101;})) {
      child.cells.push_back(cell);child.surface.push_back(face);
    }
  }
  REQUIRE(child.cells.size()==1);fresh=std::move(child);
  // A normal ray through the triangle edge is admissible; moving beyond it is not.
  for(auto& cell:fresh.cells)for(auto& node:cell.v)if(node.id==101)node.p.x=node.p.y;
  CHECK(CheckFirstHeight(fresh,heights,reference,report,reason));
  CHECK(report.minimum_barycentric==Approx(0).margin(1e-15));
  for(auto& cell:fresh.cells)for(auto& node:cell.v)if(node.id==101)node.p.x=node.p.y-.1;
  CHECK_FALSE(CheckFirstHeight(fresh,heights,reference,report,reason));
  CHECK(report.height_failures==0);CHECK(report.footprint_failures==1);
  CHECK(report.minimum_barycentric==Approx(-.1));CHECK(reason.find("normal foot leaves")!=std::string::npos);
  FirstHeightReport released;CHECK(CheckFirstHeight(fresh,{},reference,released,reason));CHECK(released.footprint_failures==0);
}

TEST_CASE("Native3D first layer handles wall junctions and distinguishes local clearance failure", "[NativeFirstLayer3D]") {
  const auto old=Cube();const auto facets=Facets(old);const PlanarReference reference(facets);
  std::map<int,double> heights;
  for(const auto& f:facets) {
    const bool bottom=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;});
    const bool left=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.x==0;});
    if(bottom||left)heights[f.marker]=.05;
  }
  REQUIRE(heights.size()==4);Patch fresh;FirstHeightReport report;std::string reason;
  REQUIRE(ConstructFirstLayer(old,{100,{.5,.5,.5}},101,200,heights,reference,fresh,report,reason));
  CHECK(fresh.cells.size()==24);CHECK(report.wall_faces==4);CHECK(report.maximum_tilt<1e-12);
  CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)
    REQUIRE(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  Patch sentinel=old;
  CHECK_FALSE(ConstructFirstLayer(old,{100,{.5,.5,.01}},101,200,heights,reference,sentinel,report,reason));
  CHECK(reason.find("background obstruction")!=std::string::npos);CHECK(Connectivity(sentinel)==Connectivity(old));
  const auto thin=Cube(.1);const auto thinFacets=Facets(thin);const PlanarReference thinReference(thinFacets);
  std::map<int,double> excessive;
  for(const auto& f:thinFacets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))excessive[f.marker]=.2;
  sentinel=thin;
  CHECK_FALSE(ConstructFirstLayer(thin,{100,{.5,.5,.05}},101,200,excessive,thinReference,sentinel,report,reason));
  CHECK(reason.find("geometry-height incompatibility")!=std::string::npos);CHECK(Connectivity(sentinel)==Connectivity(thin));
}

TEST_CASE("Native3D first-row apex movement preserves altitude and boundary feature freedoms", "[NativeMetricMove3D]") {
  const auto old=Cube();const auto facets=Facets(old);const PlanarReference reference(facets);
  std::map<int,double> heights;for(const auto& f:facets)
    if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.05;
  Patch layer;FirstHeightReport report;std::string reason;
  REQUIRE(ConstructFirstLayer(old,{100,{.5,.5,.5}},101,200,heights,reference,layer,report,reason));
  Node apex;for(const auto& c:layer.cells)for(auto n:c.v)if(n.id==101)apex=n;
  const auto moved=ConstrainMovePoint(layer,101,{500,{apex.p.x+.01,apex.p.y-.01,apex.p.z+.02}},heights,reference);
  CHECK(moved.p.x==Approx(apex.p.x+.01));CHECK(moved.p.y==Approx(apex.p.y-.01));CHECK(moved.p.z==apex.p.z);
  auto fresh=layer;for(auto& c:fresh.cells)for(auto& n:c.v)if(n.id==101)n=moved;
  CHECK(CheckFirstHeight(fresh,heights,reference,report,reason));CHECK(report.maximum_tilt>0);
  const auto released=ConstrainMovePoint(layer,101,{500,{apex.p.x,apex.p.y,apex.p.z+.02}},{},reference);
  CHECK(released.p.z==Approx(apex.p.z+.02));
  for(const auto& n:old.cells.front().v) {
    const auto constrained=ConstrainMovePoint(old,n.id,{500,{.4,.4,.4}},heights,reference);
    CHECK(constrained.p.x==n.p.x);CHECK(constrained.p.y==n.p.y);CHECK(constrained.p.z==n.p.z);
  }
}

namespace {
#include "fixtures/native_bl_corner.inc"
}
namespace {
void CornerStripProbe(bool grouped) {
  const auto accepted=BLCornerObstruction();auto facets=BLCornerFacets();
  if(grouped)for(auto& f:facets)f.marker=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;})?10:11;
  const PlanarReference reference(facets);
  Patch old;for(const auto& c:accepted.cells) {
    unsigned endpoints=0;bool mover=false;for(auto n:c.v){endpoints+=n.id==0 || n.id==4;mover=mover || n.id==31 || n.id==47;}
    if(endpoints==2 || mover)old.cells.push_back(c);
  }
  const auto skin=Incidence(old.cells).Boundary();
  for(const auto& f:accepted.surface)if(std::find(skin.begin(),skin.end(),f.v)!=skin.end())old.surface.push_back(f);
  REQUIRE(old.cells.size()==14);
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};
  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};CBoundaryLayerMetric layers(3,{wall});
  const auto compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,1.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
      SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  std::vector<DonorCell> donors;for(auto c:Cube().cells){DonorCell d;d.cell=c;d.sensor.fill({1,0,0,1,0,1});donors.push_back(d);}
  FrozenField field(donors,compose,false,{},[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
  std::map<Id,Node> nodes;for(const auto& c:old.cells)for(auto n:c.v)nodes.emplace(n.id,n);
  std::map<int,double> heights;for(const auto& f:facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.1;
  Node split{200000,field.BalancedSplitPoint(nodes.at(0).p,nodes.at(4).p)};
  std::vector<std::pair<Id,Node>> moves;
  for(Id id:{31,47}) {
    std::set<Id> neighbors;for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),id)!=f.v.end())neighbors.insert(f.v.begin(),f.v.end());
    neighbors.erase(id);std::vector<std::pair<Point,Tensor>> values;const auto origin=nodes.at(id).p;
    for(auto n:neighbors){const auto q=nodes.at(n).p;const Point mid{double(((long double)origin.x+q.x)/2),double(((long double)origin.y+q.y)/2),double(((long double)origin.z+q.z)/2)};values.emplace_back(q,field.Query(mid).target);}
    auto point=ConstrainMovePoint(old,id,{200001+moves.size(),MetricMeanPoint(origin,values)},heights,reference);
    if(id==31){CHECK(point.p.y==0);if(!grouped)CHECK(point.p.x==point.p.z);}
    else {CHECK(point.p.x==0);if(!grouped)CHECK(point.p.y==point.p.z);}
    moves.emplace_back(id,point);
  }
  bool approved=false,builtAny=false;Patch fresh;PatchMetric metric;std::string reason;
  for(double fraction:grouped?std::vector<double>{.5,.55,.565}:std::vector<double>{.5}) {
   split.p=field.BalancedSplitPoint(nodes.at(0).p,nodes.at(4).p,fraction);
   for(double damping:grouped?std::vector<double>{0,1,3}:std::vector<double>{0}) {
    if(approved)break;
    for(double alpha:{1.,.5,.25,.125}) {
    auto trial=moves;for(auto& m:trial){const auto a=nodes.at(m.first).p;
      if(damping) {
        auto t=field.Query(a).target;const auto scale=std::max({std::abs(t.xx),std::abs(t.xy),std::abs(t.xz),std::abs(t.yy),std::abs(t.yz),std::abs(t.zz)});
        const auto factor=damping/scale;t={t.xx*factor,t.xy*factor,t.xz*factor,t.yy*factor,t.yz*factor,t.zz*factor};
        m.second.p=MetricMeanPoint(a,{{m.second.p,Tensor{}},{a,t}});
        m.second=ConstrainMovePoint(old,m.first,m.second,heights,reference);
      }
      const auto b=m.second.p;
      m.second.p={double(a.x+alpha*((long double)b.x-a.x)),double(a.y+alpha*((long double)b.y-a.y)),double(a.z+alpha*((long double)b.z-a.z))};}
    const bool built=SplitSurfaceStrip(old,{0,4},split,trial,300000,reference,fresh,reason);
    builtAny=builtAny || built;
    const bool passed=built && ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,field,metric,reason) &&
      (!grouped || metric.maximum_length<=MaximumMetricEdge);
    INFO("fraction="<<fraction<<" damping="<<damping<<" alpha="<<alpha<<" built="<<built<<" reason="<<reason<<" q="<<metric.minimum_mean_ratio<<" J="<<metric.minimum_scaled_jacobian<<" length="<<double(metric.maximum_length));
    if(passed){approved=true;break;}
   }
  }
   if(approved)break;
  }
  std::cout<<"CORNER_JOINT grouped="<<grouped<<" heuristic_pass="<<approved<<"\n";
  if(grouped && !approved) {
    split.p={0,0,.19584409919285048};
    moves={ {31,{200001,{.26373889197404216,0,.1746353513785195}}},
            {47,{200002,{0,.26373889197404216,.1746353513785195}}} };
    builtAny=SplitSurfaceStrip(old,{0,4},split,moves,300000,reference,fresh,reason);
    approved=builtAny && ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,field,metric,reason);
  }
  INFO(reason);REQUIRE(builtAny);if(grouped)REQUIRE(approved);else CHECK_FALSE(approved);
  FirstHeightReport height;CHECK(CheckFirstHeight(fresh,heights,reference,height,reason));
  auto whole=accepted;whole.cells.erase(std::remove_if(whole.cells.begin(),whole.cells.end(),[&](const auto& c){return std::any_of(old.cells.begin(),old.cells.end(),[&](const auto& d){return c.id==d.id;});}),whole.cells.end());
  for(const auto& c:fresh.cells){for(const auto& outside:whole.cells)CHECK(ConformingDisjoint(c,outside));}whole.cells.insert(whole.cells.end(),fresh.cells.begin(),fresh.cells.end());
  whole.surface.erase(std::remove_if(whole.surface.begin(),whole.surface.end(),[&](const auto& f){return std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& g){return f.v==g.v;});}),whole.surface.end());
  whole.surface.insert(whole.surface.end(),fresh.surface.begin(),fresh.surface.end());
  const bool full=ValidateMetricPatch(whole.cells,field,metric,reason);INFO(reason);if(grouped)CHECK(full);else CHECK_FALSE(full);
  CHECK(CheckFirstHeight(whole,heights,reference,height,reason));CHECK(double(Volume(whole))==Approx(1));
  if(grouped)SaveCurved(whole,reference,"grouped_joint_witness");
  auto incomplete=old;const auto missing=std::find_if(incomplete.surface.begin(),incomplete.surface.end(),[](const auto& f){return std::find(f.v.begin(),f.v.end(),31)!=f.v.end();});
  REQUIRE(missing!=incomplete.surface.end());incomplete.surface.erase(missing);Patch unchanged=old;
  CHECK_FALSE(SplitSurfaceStrip(incomplete,{0,4},split,moves,400000,reference,unchanged,reason));CHECK(Connectivity(unchanged)==Connectivity(old));
}
}
TEST_CASE("Coordinated planar corner movement preserves geometry but still rejects its frozen BL shape", "[NativeBLCorner3D]") {
  CornerStripProbe(false);
}
TEST_CASE("Saved ordinary-marker corner admits a complete joint replacement under unchanged frozen BL gates", "[NativeBLCornerGrouped3D]") {
  CornerStripProbe(true);
}

TEST_CASE("Atomic planar strip split retains marker junctions and complete interior movement", "[NativePlanarStrip3D]") {
  const auto initial=Cube();const PlanarReference reference(Facets(initial));Patch old,fresh;std::string reason;
  REQUIRE(SplitEdge(initial,{0,6},{8,{.5,.5,.5}},100,reference,old,reason));
  const std::vector<std::pair<Id,Node>> moves{{8,{10,{.45,.45,.5}}}};
  REQUIRE(SplitSurfaceStrip(old,{0,4},{9,{0,0,.5}},moves,200,reference,fresh,reason));
  CHECK(fresh.cells.size()==old.cells.size()+2);CHECK(fresh.surface.size()==old.surface.size()+2);
  std::vector<DonorCell> donors;for(auto c:initial.cells){DonorCell d;d.cell=c;d.sensor.fill({1,0,0,1,0,1});donors.push_back(d);}
  FrozenField field(donors);PatchMetric metric;CHECK(ValidateMetricPatch(fresh.cells,field,metric,reason));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)CHECK(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  Patch interior;
  REQUIRE(SplitSurfaceStrip(old,{0,8},{9,{.225,.225,.25}},moves,400,reference,interior,reason));
  CHECK(interior.cells.size()==old.cells.size()+6);CHECK(interior.surface.size()==old.surface.size());
  INFO(reason);CHECK(ValidateMetricPatch(interior.cells,field,metric,reason));
  CHECK(ValidateFixedInterface(old.cells,interior.cells,reason));
  for(size_t i=0;i<interior.cells.size();++i)for(size_t j=i+1;j<interior.cells.size();++j)CHECK(ConformingDisjoint(interior.cells[i],interior.cells[j]));
  auto sentinel=old;auto invalid=moves;invalid.front().second.p={2,2,2};
  CHECK_FALSE(SplitSurfaceStrip(old,{0,4},{9,{0,0,.5}},invalid,300,reference,sentinel,reason));CHECK(Connectivity(sentinel)==Connectivity(old));
  CHECK_FALSE(SplitSurfaceStrip(old,{0,4},{9,{.01,0,.5}},moves,300,reference,sentinel,reason));CHECK(Connectivity(sentinel)==Connectivity(old));
}


TEST_CASE("Native3D uneven wall split replaces apices and transition together", "[NativeWallReconstruction3D]") {
  const auto seed=Cube();const PlanarReference reference(Facets(seed));std::map<int,double> heights;
  for(const auto& f:Facets(seed))if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))
    heights[f.marker]=.1;
  Patch aligned;FirstHeightReport report;std::string reason;
  REQUIRE(ConstructFirstLayer(seed,{100,{.5,.5,.5}},101,200,heights,reference,aligned,report,reason));
  const Edge edge{0,2};const Node split{1000,{.4,.4,0}};Patch inherited;
  REQUIRE(SplitEdge(EdgeStar(aligned,edge),edge,split,2000,reference,inherited,reason));
  CHECK_FALSE(CheckFirstHeight(inherited,heights,reference,report,reason));
  CHECK(report.height_failures==0);CHECK(report.footprint_failures>0);
  Patch rebuilt;Face obstruction{};
  const bool valid=ReconstructSplitPatch(aligned,edge,split,{1001,{}},Tensor{},2000,reference,rebuilt,obstruction,reason,nullptr,heights);
  INFO(reason);REQUIRE(valid);REQUIRE(CheckFirstHeight(rebuilt,heights,reference,report,reason));
  CHECK(report.wall_faces==4);CHECK(report.maximum_tilt<1e-12);CHECK(report.minimum_barycentric>0);
  for(Id id:{100,101,102}) {const auto before=Position(aligned,id),after=Position(rebuilt,id);
    CHECK(after.x==before.x);CHECK(after.y==before.y);CHECK(after.z==before.z);
  }
  CHECK(double(Volume(rebuilt))==Approx(1).epsilon(1e-14));
  for(size_t i=0;i<rebuilt.cells.size();++i)for(size_t j=i+1;j<rebuilt.cells.size();++j)CHECK(ConformingDisjoint(rebuilt.cells[i],rebuilt.cells[j]));
  SaveCurved(aligned,reference,"wall_split_initial");SaveCurved(inherited,reference,"wall_split_inherited_rejected");SaveCurved(rebuilt,reference,"wall_split_joint");
  auto sentinel=rebuilt;const auto saved=Connectivity(sentinel);auto excessive=heights;for(auto& h:excessive)h.second=2.;
  CHECK_FALSE(ReconstructSplitPatch(aligned,edge,split,{1001,{}},Tensor{},2000,reference,sentinel,obstruction,reason,nullptr,excessive));
  CHECK(Connectivity(sentinel)==saved);
  CHECK_FALSE(ReconstructSplitPatch(aligned,edge,split,{UINT64_MAX,{}},Tensor{},2000,reference,sentinel,obstruction,reason,nullptr,heights));
  CHECK(Connectivity(sentinel)==saved);
}


TEST_CASE("Native3D private actual-field flips repair a slender internal edge", "[NativeConnectivity3D]") {
  const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{0,1,0}},top{3,{.01,.01,.5}},bottom{4,{.01,.01,-.5}};
  Patch pair;
  for(auto v:{std::array<Node,4>{a,b,c,top},std::array<Node,4>{a,b,c,bottom}}) {
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);pair.cells.push_back({Id(pair.cells.size()),v});
  }
  std::string reason;Patch old;REQUIRE(Cone(pair.cells,top,10,old.cells,reason));
  for(auto face:Incidence(old.cells).Boundary())old.surface.push_back({face,Id(old.surface.size())});
  const PlanarReference reference(Facets(old));
  DonorCell donor;donor.cell={0,{{{20,{-2,-2,-2}},{21,{8,-2,-2}},{22,{-2,8,-2}},{23,{-2,-2,8}}}}};donor.sensor.fill(Tensor{});
  FrozenField field({donor});Patch fresh;ConnectivityRepairReport report;
  REQUIRE(RepairMetricConnectivity(old,field,reference,{},100,fresh,report,reason));
  CHECK(report.flips>0);CHECK(report.complete_shape);CHECK(report.after_loss<report.before_loss);
  CHECK(fresh.cells.size()==2);CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
  PatchMetric metric;REQUIRE(ValidateMetricPatch(fresh.cells,field,metric,reason));
  auto unchanged=old;CHECK_FALSE(RepairMetricConnectivity(old,field,reference,{},100,unchanged,report,reason,0));
  CHECK(Connectivity(unchanged)==Connectivity(old));
}

namespace {
#include "fixtures/native_wall_joint.inc"
}
TEST_CASE("Native3D saved wall proposal replays bounded actual-field connectivity repair", "[NativeConnectivityReplay3D]") {
  const auto seed=Cube();const PlanarReference reference(Facets(seed));const auto old=SavedWallJoint();
  std::vector<DonorCell> source;
  for(const auto& c:seed.cells) {DonorCell d;d.cell=c;for(unsigned k=0;k<4;++k){const auto s=4+12*c.v[k].p.x;d.sensor[k]={s,0,0,s,0,s};}source.push_back(d);}
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  auto compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;const su2double x[3]={p.x,p.y,p.z};
    m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;layers.ApplyPoint(x,m,4.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
                  SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  FrozenField field(source,compose,false,{},[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
  std::map<int,double> heights;for(const auto& f:Facets(seed))if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.1;
  Patch fresh;ConnectivityRepairReport report;std::string reason;
  const bool progress=RepairMetricConnectivity(old,field,reference,heights,100000,fresh,report,reason,256);
  std::cout<<"CONNECTIVITY_REPLAY progress="<<progress<<" trials="<<report.trials<<" flips="<<report.flips
           <<" moves="<<report.moves<<" coordinated="<<report.coordinated<<" move_geometry_rejected="<<report.move_geometry_rejected<<" move_height_rejected="<<report.move_height_rejected<<" move_loss_rejected="<<report.move_loss_rejected<<" before_loss="<<report.before_loss<<" after_loss="<<report.after_loss<<" complete_shape="<<report.complete_shape<<" reason="<<reason<<"\n";
  REQUIRE(progress);CHECK(report.after_loss<report.before_loss);CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));
  CHECK(report.coordinated>0);CHECK(report.complete_shape);CHECK(report.after_loss==0);
  FirstHeightReport height;REQUIRE(CheckFirstHeight(fresh,heights,reference,height,reason));
  SaveCurved(old,reference,"connectivity_replay_before");SaveCurved(fresh,reference,"connectivity_replay_after");
  PatchMetric metric;const bool admissible=ValidateMetricPatch(fresh.cells,field,metric,reason);
  std::cout<<"CONNECTIVITY_REPLAY_GATE admissible="<<admissible<<" q="<<metric.minimum_mean_ratio<<" J="<<metric.minimum_scaled_jacobian
           <<" L="<<double(metric.maximum_length)<<" reason="<<reason<<"\n";
}

TEST_CASE("Native joint repair moves an interior sliver vertex without stale shape scores", "[NativeConnectivityMove3D]") {
  const auto seed=Cube();const PlanarReference reference(Facets(seed));std::vector<Cell> inserted;std::string reason;
  REQUIRE(Cone(seed.cells,{42,{.43,.37,.0001}},100,inserted,reason));Patch old{inserted,seed.surface};
  std::vector<DonorCell> source;for(const auto& c:seed.cells){DonorCell d;d.cell=c;d.sensor.fill(Tensor{});source.push_back(d);}
  FrozenField target(source);Patch fresh;ConnectivityRepairReport report;
  REQUIRE(RepairMetricConnectivity(old,target,reference,{},1000,fresh,report,reason,256));
  CHECK(report.moves>0);CHECK(report.complete_shape);CHECK(report.after_loss==0);
  CHECK(ValidateFixedInterface(old.cells,fresh.cells,reason));PatchMetric metric;
  REQUIRE(ValidateMetricPatch(fresh.cells,target,metric,reason));
  // Re-evaluation from coordinates must agree with the cached completion certificate.
  CHECK(metric.minimum_mean_ratio>=MinimumMeanRatio);CHECK(metric.minimum_scaled_jacobian>=MinimumScaledJacobian);
}

TEST_CASE("Native wall split inserts coordinated height points without a common background", "[NativeWallReconstruction3D]") {
  const auto seed=Cube();const PlanarReference reference(Facets(seed));std::map<int,double> heights;
  for(const auto& f:Facets(seed))if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.1;
  Patch aligned;FirstHeightReport height;std::string reason;
  REQUIRE(ConstructFirstLayer(seed,{100,{.5,.5,.5}},101,200,heights,reference,aligned,height,reason));
  Patch fresh;Face obstruction{};
  const bool built=ReconstructWallSplit(aligned,{0,2},{1000,{.4,.4,0}},1001,2000,heights,reference,fresh,obstruction,reason);
  INFO(reason);REQUIRE(built);REQUIRE(CheckFirstHeight(fresh,heights,reference,height,reason));
  CHECK(height.wall_faces==4);CHECK(height.maximum_tilt<1e-12);CHECK(height.minimum_barycentric>0);
  CHECK(double(Volume(fresh))==Approx(1).epsilon(1e-14));
  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)CHECK(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
  SaveCurved(fresh,reference,"wall_split_retained");
  auto sentinel=fresh;const auto before=Connectivity(sentinel);
  CHECK_FALSE(ReconstructWallSplit(aligned,{0,2},{1000,{.4,.4,0}},100,2000,heights,reference,sentinel,obstruction,reason));
  CHECK(Connectivity(sentinel)==before);
  CHECK_FALSE(ReconstructWallSplit(EdgeStar(aligned,{0,2}),{0,2},{1000,{.4,.4,0}},1001,2000,heights,reference,sentinel,obstruction,reason));
  CHECK(Connectivity(sentinel)==before);
}

TEST_CASE("Constructed private interface checks retain the accepted-cavity bound", "[NativeConnectivityMove3D]") {
  const auto seed=Cube();auto constructed=seed.cells;Id next_node=1000,next_cell=10000;std::string reason;
  for(unsigned i=0;i<20;++i) {
    const auto c=constructed.front();std::vector<Cell> children;
    REQUIRE(Cone({c},{next_node++,Centroid(c)},next_cell,children,reason));next_cell+=4;
    constructed.erase(constructed.begin());constructed.insert(constructed.end(),children.begin(),children.end());
  }
  REQUIRE(constructed.size()==66);
  CHECK(ValidateFixedInterface(seed.cells,constructed,reason));
  CHECK_FALSE(ValidateFixedInterface(constructed,constructed,reason));
  CHECK(ValidateFixedInterface(constructed,constructed,reason,nullptr,MaximumReplacementCells));
  CHECK_FALSE(ValidateFixedInterface(constructed,constructed,reason,nullptr,MaximumReplacementCells+1));
}

namespace {
#include "fixtures/native_wall_visibility.inc"
}
TEST_CASE("Replay actual wall seam blockage with retained and common-background construction", "[.][NativeWallVisibilityReplay3D]") {
  const auto full=WallVisibilityMesh();const PlanarReference reference(WallVisibilityFacets());const Edge edge{1,11543};
  const std::map<int,double> heights{{10,.1}};const auto all_nodes=[](const Patch& patch) {
    std::map<Id,Node> nodes;for(const auto& c:patch.cells)for(auto n:c.v)nodes.emplace(n.id,n);return nodes;
  };
  const auto nodes=all_nodes(full);std::set<Id> apices;
  for(const auto f:full.surface)if(reference.Get(f.facet).marker==10 &&
      std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end())
    for(const auto& c:full.cells) {
      unsigned matches=0;Id apex=0;for(auto n:c.v)if(std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end())++matches;else apex=n.id;
      if(matches==3)apices.insert(apex);
    }
  REQUIRE(apices.size()==1);std::set<Id> required;
  for(const auto& c:full.cells) {
    unsigned endpoints=0;bool apex=false;for(auto n:c.v){endpoints+=n.id==edge[0] || n.id==edge[1];apex=apex || apices.count(n.id);}
    if(endpoints==2 || apex)required.insert(c.id);
  }
  REQUIRE(required.size()==31);
  const auto seed=Cube();std::vector<DonorCell> source;
  for(const auto& c:seed.cells) {DonorCell d;d.cell=c;for(unsigned i=0;i<4;++i){const auto v=4+12*c.v[i].p.x;d.sensor[i]={v,0,0,v,0,v};}source.push_back(d);}
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  auto compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;const su2double x[3]={p.x,p.y,p.z};
    m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;layers.ApplyPoint(x,m,4.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  FrozenField field(source,compose,false,{},[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
  const Node point{1000000,field.BalancedSplitPoint(nodes.at(edge[0]).p,nodes.at(edge[1]).p)};
  const Incidence global(full.cells);
  for(unsigned strategy=0;strategy<2;++strategy) {
    auto included=required;
    for(unsigned growth=0;growth<12;++growth) {
      Patch old;for(const auto& c:full.cells)if(included.count(c.id))old.cells.push_back(c);
      const auto boundary=Incidence(old.cells).Boundary();
      for(auto f:full.surface)if(std::find(boundary.begin(),boundary.end(),f.v)!=boundary.end())old.surface.push_back(f);
      REQUIRE(std::count_if(old.surface.begin(),old.surface.end(),[&](const auto& f) {
        return std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end();
      })==2);
      FirstHeightReport height;std::string reason;REQUIRE(CheckFirstHeight(old,heights,reference,height,reason));REQUIRE(height.wall_faces>=2);
      const auto saved=Connectivity(old);Patch fresh;Face obstruction{};
      bool built=strategy==0 ? ReconstructWallSplit(old,edge,point,1000001,2000000,heights,reference,fresh,obstruction,reason) :
          ReconstructSplitPatch(old,edge,point,{1000001,{}},field.Query(Centroid(old.cells.front())).target,2000000,reference,fresh,obstruction,reason,nullptr,heights);
      std::cout<<"WALL_VISIBILITY strategy="<<strategy<<" growth="<<growth<<" old="<<old.cells.size()<<" built="<<built
               <<" obstruction="<<obstruction[0]<<","<<obstruction[1]<<","<<obstruction[2]<<" reason="<<reason<<"\n";
      CHECK(Connectivity(old)==saved);
      if(built) {
        Patch repaired;ConnectivityRepairReport report;
        const bool progress=RepairMetricConnectivity(fresh,field,reference,heights,2000000,repaired,report,reason,256,strategy==0 ? std::vector<Id>{1000010,1000011} : std::vector<Id>{});
        std::cout<<"WALL_VISIBILITY_REPAIR strategy="<<strategy<<" trials="<<report.trials<<" flips="<<report.flips<<" moves="<<report.moves
                 <<" before="<<report.before_loss<<" after="<<report.after_loss<<" insertions="<<report.insertions<<" complete="<<report.complete_shape<<" progress="<<progress<<" reason="<<reason<<"\n";
        if(progress)SaveCurved(repaired,reference,"visibility_partial_"+std::to_string(strategy));
        if(progress && report.complete_shape)fresh=std::move(repaired);
        PatchMetric metric;MetricDiagnosis diagnosis;
        const bool accepted=ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,field,metric,reason,nullptr,nullptr,&diagnosis);
        std::cout<<"WALL_VISIBILITY_GATE strategy="<<strategy<<" q="<<diagnosis.minimum_mean_ratio<<" J="<<diagnosis.minimum_scaled_jacobian
                 <<" removed_excess="<<double(diagnosis.removed_excess)<<" added_excess="<<double(diagnosis.added_excess)<<" accepted="<<accepted<<" reason="<<reason<<"\n";
        SaveCurved(old,reference,"visibility_old_"+std::to_string(strategy));SaveCurved(fresh,reference,"visibility_proposal_"+std::to_string(strategy));
        break;
      }
      if(std::adjacent_find(obstruction.begin(),obstruction.end())!=obstruction.end())break;
      const auto adjacent=global.FaceCells(obstruction);const auto before=included.size();for(Id id:adjacent)included.insert(id);
      if(included.size()==before || included.size()>MaximumCavityCells)break;
    }
  }
}
