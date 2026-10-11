/*!
 * \file CNativeBoundary3D_tests.cpp
 * \brief Coupled private planar boundary refinement/coarsening and incomplete-star rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
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
