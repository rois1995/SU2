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
  for (size_t n : {size_t(4), size_t(16)}) {
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
