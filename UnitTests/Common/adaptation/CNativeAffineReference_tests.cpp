/*!
 * \file CNativeAffineReference_tests.cpp
 * \brief Rotated finite affine geometry, disconnected components and body-footprint barriers.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeAffineReference.hpp"
#include <fstream>
#include <iomanip>
using namespace SU2NativeGeometry;
namespace {
AffineAuthority Authority(unsigned dimension) {
  return {17,3,dimension,{2.1,-.7,dimension==2?0.:.8},dimension==2?Point{.37,.82,0}:Point{.37,-.61,.82},1e-13};
}
std::vector<AffineAuthority> Definitions() {
  auto a=Authority(2),b=a,c=Authority(3),d=c,e=c;
  b.identity=18;b.normal={-.92,.37,0};
  c.identity=19;d.identity=20;d.normal={-1.3,.27,.45};
  e.identity=21;e.normal={.12,-1.1,.4};return {a,b,c,d,e};
}
std::vector<SU2Native3D::Facet> MultipleFootprints(const AffineChart& chart) {
  std::map<std::pair<int,int>,SU2Native3D::Node> nodes;
  auto point=[&](int i,int j){const auto key=std::make_pair(i,j);return nodes.emplace(key,SU2Native3D::Node{Id(100*j+i),chart.Lift({double(i),double(j),0})}).first->second;};
  std::vector<SU2Native3D::Facet> faces;
  auto tile=[&](int i,int j){auto a=point(i,j),b=point(i+1,j),c=point(i+1,j+1),d=point(i,j+1);
    faces.push_back({Id(faces.size()),7,{a,b,c}});faces.push_back({Id(faces.size()),7,{a,c,d}});};
  for(int i=0;i<6;++i)for(int j=0;j<4;++j)if(!((j==1&&(i==1||i==4)) || (j==3&&i==2)))tile(i,j);
  tile(8,0);return faces;
}
}
TEST_CASE("Affine geometry charts bound rotated binary64 lifting without exact coplanarity", "[NativeAffineGeometry]") {
  std::ofstream samples("affine_lift_samples.json");samples<<std::setprecision(17)<<"[";bool first=true;
  for(const auto definition:Definitions()) {
    const auto dimension=definition.dimension;const AffineChart chart(definition);
    CHECK(chart.Authority().identity==definition.identity);CHECK(chart.Authority().version==3);
    for(unsigned i=0;i<64;++i) {
      const Point parameter{double(i)/17-.9,dimension==3?double((31*i)%67)/19-.8:0.,0};
      const auto p=chart.Lift(parameter);const auto q=chart.Coordinates(p);
      CHECK(q.x==parameter.x);CHECK(q.y==parameter.y);CHECK(q.z==0);
      const auto error=chart.ErrorBound(p);CHECK(error<=definition.maximum_error);
      const auto projected=chart.Project(p);CHECK(chart.ErrorBound(projected)<=definition.maximum_error);
      CHECK(projected.x==p.x);CHECK(projected.y==p.y);CHECK(projected.z==p.z);
      if(!first)samples<<',';first=false;
      samples<<"{\"dimension\":"<<dimension<<",\"normal\":["<<definition.normal.x<<','<<definition.normal.y<<','<<definition.normal.z
        <<"],\"origin\":["<<definition.origin.x<<','<<definition.origin.y<<','<<definition.origin.z
        <<"],\"point\":["<<p.x<<','<<p.y<<','<<p.z<<"],\"bound\":"<<error<<"}";
    }
    auto invalid=definition;invalid.maximum_error=0;CHECK_THROWS(AffineChart{invalid});
    invalid=definition;invalid.normal={};CHECK_THROWS(AffineChart{invalid});
    invalid=definition;invalid.version=0;CHECK_THROWS(AffineChart{invalid});
    CHECK_THROWS(chart.Lift({0,0,1}));
  }
  samples<<"]\n";REQUIRE(bool(samples));
  auto unrepresentable=Authority(3);unrepresentable.maximum_error=std::numeric_limits<double>::denorm_min();
  CHECK_THROWS(AffineChart(unrepresentable).Lift({.123,.456,0}));
}
TEST_CASE("Finite symmetry lines preserve separated body footprints in arbitrary charts", "[NativeAffineGeometry]") {
  const auto definition=Definitions().at(GENERATE(0u,1u));const AffineChart chart(definition);
  auto node=[&](Id id,double x){return SU2Native3D::Node{id,chart.Lift({x,0,0})};};
  const auto a=node(10,0),b=node(11,.5),c=node(12,1),d=node(13,2),e=node(14,3),f=node(15,4),g=node(16,5);
  const std::vector<LineEdge> edges{{20,a,b},{21,b,c},{22,d,e},{23,f,g}};
  const FiniteLineReference line(definition,edges);CHECK(line.Components()==3);CHECK(line.ResidentBytes()>sizeof(line));
  auto p=[&](double x){return chart.Lift({x,0,0});};
  for(double x:{0.,.5,1.,2.,3.,4.,5.})CHECK(line.Contains(p(x)));
  for(double x:{-.1,1.5,3.5,5.1})CHECK_FALSE(line.Contains(p(x)));
  CHECK(line.ContainsSegment(p(.1),p(.9)));CHECK(line.ContainsSegment(p(.9),p(.1)));
  CHECK_FALSE(line.ContainsSegment(p(.5),p(2.5)));CHECK_FALSE(line.ContainsSegment(p(2.5),p(4.5)));
  CHECK(line.FeaturesIntersecting(p(.5),p(4.5))==std::vector<Id>{12,13,14,15});
  CHECK_THROWS(line.Project(p(1.5)));
  auto offset=p(.2);offset.x+=.01*definition.normal.x;offset.y+=.01*definition.normal.y;
  CHECK(line.Contains(line.Project(offset)));
  auto reversed=edges;std::reverse(reversed.begin(),reversed.end());const FiniteLineReference reordered(definition,reversed);
  CHECK(reordered.FeaturesIntersecting(p(.5),p(4.5))==line.FeaturesIntersecting(p(.5),p(4.5)));
  auto overlap=edges;overlap.push_back({24,node(17,.75),node(18,1.25)});CHECK_THROWS(FiniteLineReference(definition,overlap));
  auto conflict=edges;conflict[1].a.p=chart.Lift({.6,0,0});CHECK_THROWS(FiniteLineReference(definition,conflict));
  auto branch=edges;branch.push_back({24,b,node(19,.75)});CHECK_THROWS(FiniteLineReference(definition,branch));
  CHECK_THROWS(FiniteLineReference(definition,edges,1));CHECK(line.ContainsSegment(p(.1),p(.9)));
}
TEST_CASE("Finite rotated planes retain multiple holes concavity and disconnected regions", "[NativeAffineGeometry]") {
  const auto definition=Definitions().at(GENERATE(2u,3u,4u));const AffineChart chart(definition);const auto facets=MultipleFootprints(chart);
  const FinitePlaneReference plane(definition,facets);CHECK(plane.Components()==2);CHECK(plane.ResidentBytes()>sizeof(plane));
  auto p=[&](double x,double y){return chart.Lift({x,y,0});};
  for(auto xy:std::vector<std::array<double,2>>{{.5,.5},{3.5,1.5},{8.5,.5},{1,1},{4,2}})CHECK(plane.Contains(p(xy[0],xy[1])));
  for(auto xy:std::vector<std::array<double,2>>{{1.5,1.5},{4.5,1.5},{2.5,3.5},{7,.5}})CHECK_FALSE(plane.Contains(p(xy[0],xy[1])));
  CHECK(plane.ContainsSegment(p(0,0),p(6,0)));CHECK(plane.ContainsSegment(p(1,1),p(2,1)));
  CHECK(plane.ContainsSegment(p(0,2),p(2,0))); // Tangency at the first body's corner is allowed.
  CHECK_FALSE(plane.ContainsSegment(p(0,1.5),p(6,1.5)));
  CHECK_FALSE(plane.ContainsSegment(p(1,1),p(2,2))); // Endpoints are on the hole, chord is inside it.
  CHECK_FALSE(plane.ContainsSegment(p(1,1.5),p(2,1.5))); // Both endpoints lie strictly inside hole edges.
  CHECK_FALSE(plane.ContainsSegment(p(1.5,1),p(1.5,2)));
  CHECK(plane.ContainsSegment(p(.5,1.5),p(1,1.5)));
  CHECK(plane.ContainsSegment(p(1,1.5),p(.5,1.5)));
  CHECK_FALSE(plane.ContainsSegment(p(1,4),p(4,4)));CHECK_FALSE(plane.ContainsSegment(p(5,.5),p(8.5,.5)));
  CHECK(plane.ContainsFace({p(0,0),p(6,0),p(3,.5)}));
  CHECK_FALSE(plane.ContainsFace({p(1,1),p(2,1),p(2,2)}));
  CHECK_FALSE(plane.ContainsFace({p(0,0),p(6,0),p(3,3)}));
  CHECK_FALSE(plane.ContainsFace({p(5,.5),p(8.5,.5),p(8.5,.75)}));
  CHECK(plane.FeaturesIntersecting(p(.5,1.5),p(5.5,1.5)).size()==4);
  CHECK_THROWS(plane.Project(p(1.5,1.5)));CHECK_THROWS(FinitePlaneReference(definition,facets,1));
  auto reversed=facets;std::reverse(reversed.begin(),reversed.end());
  const FinitePlaneReference reordered(definition,reversed);CHECK(reordered.Components()==2);
  CHECK_FALSE(reordered.ContainsSegment(p(1,1),p(2,2)));CHECK(reordered.ContainsSegment(p(0,2),p(2,0)));
  auto relabeled=facets;for(size_t i=0;i<relabeled.size();++i)relabeled[i].marker=i%5;
  const FinitePlaneReference labels(definition,relabeled);CHECK(labels.Components()==2);
  CHECK(labels.Contains(p(.5,.5)));CHECK_FALSE(labels.Contains(p(1.5,1.5)));
  CHECK(labels.ContainsSegment(p(0,2),p(2,0)));CHECK_FALSE(labels.ContainsFace({p(0,0),p(6,0),p(3,3)}));
  auto inverted=facets;for(auto& facet:inverted)std::swap(facet.v[1],facet.v[2]);
  const FinitePlaneReference opposite(definition,inverted);CHECK(opposite.Components()==2);
  CHECK_FALSE(opposite.Contains(p(1.5,1.5)));CHECK(opposite.ContainsSegment(p(0,2),p(2,0)));
  CHECK(opposite.ContainsSegment(p(0,0),p(6,0)));
  auto bad=facets;bad.push_back(facets.front());CHECK_THROWS(FinitePlaneReference(definition,bad));
  bad=facets;std::swap(bad[0].v[1],bad[0].v[2]);CHECK_THROWS(FinitePlaneReference(definition,bad));
  bad=facets;bad[0].v[0].p.z+=1e-5;CHECK_THROWS(FinitePlaneReference(definition,bad));
  bad=facets;bad[0].v[2].p.z=std::nextafter(bad[0].v[2].p.z,INFINITY);
  CHECK_THROWS(FinitePlaneReference(definition,bad)); // Even within the plane error, a shared ID has one coordinate.
  CHECK(plane.Contains(p(.5,.5)));
}
