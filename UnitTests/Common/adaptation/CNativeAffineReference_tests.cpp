/*!
 * \file CNativeAffineReference_tests.cpp
 * \brief Rotated finite affine geometry, disconnected components and body-footprint barriers.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/src/adaptation/CNativePredicates3D_internal.hpp"
#include "../../../Common/include/adaptation/CNativeAffineReference.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
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

TEST_CASE("Declared geometry cannot alias the original physical-source tag", "[NativeAffineGeometry]") {
  SU2Native3D::Facet f{UINT64_MAX,0,{{{0,{1,0,0}},{1,{0,1,0}},{2,{0,0,1}}}}};
  SU2Native3D::AnalyticSurface sphere;
  CHECK_THROWS(SU2Native3D::PlanarReference({f},SIZE_MAX,{{UINT64_MAX,sphere}}));
  const auto definition=Authority(3);const AffineChart chart(definition);
  f.v={{{0,chart.Lift({0,0,0})},{1,chart.Lift({1,0,0})},{2,chart.Lift({0,1,0})}}};
  CHECK_THROWS(SU2Native3D::PlanarReference({f},SIZE_MAX,{},{{UINT64_MAX,definition}}));
}

TEST_CASE("Simultaneous affine adoption preserves every mapping contract and remains idempotent", "[NativeAffineGeometry]") {
  for(unsigned dimension:{2u,3u}) {
    auto a=Authority(dimension),b=a;b.identity++;b.normal={-.61,.37,dimension==3?.23:0};
    const Point original{2.100003,-.700002,dimension==3?.800001:0};
    const auto projected=ProjectIntersection(original,{a,b});
    CHECK(AffineChart(a).ErrorBound(projected)<=a.maximum_error);
    CHECK(AffineChart(b).ErrorBound(projected)<=b.maximum_error);
    const auto again=ProjectIntersection(projected,{a,b});
    CHECK(again.x==projected.x);CHECK(again.y==projected.y);CHECK(again.z==projected.z);
    CHECK(DisplacementBound(projected,projected)==0);
    const long double l1=std::abs((long double)original.x-projected.x)+std::abs((long double)original.y-projected.y)+std::abs((long double)original.z-projected.z);
    CHECK(DisplacementBound(original,projected)>=l1);
    auto conflict=a;conflict.identity+=3;conflict.origin.x+=1;
    CHECK_THROWS(ProjectIntersection(original,{a,conflict}));
    conflict=a;conflict.identity+=3;conflict.normal.y+=1e-15;
    CHECK_THROWS(ProjectIntersection(original,{a,conflict}));
  }
  auto a=Authority(3),b=a,c=a;b.identity++;c.identity+=2;
  b.normal={0,1,0};c.normal={0,0,1};
  const auto projected=ProjectIntersection({3,2,1},{a,b,c});
  for(const auto& g:{a,b,c})CHECK(AffineChart(g).ErrorBound(projected)<=g.maximum_error);
  CHECK_THROWS(DisplacementBound({INFINITY,0,0},{0,0,0}));
  auto zero=Authority(3);zero.maximum_error=0;CHECK_THROWS(ProjectIntersection({0,0,0},{zero}));
}

TEST_CASE("Source columns are bounded proposals and finite-face clearance is conservatively admitted", "[NativeAffineSourceGeometry]") {
  const std::array<Point,3> x{{{0,0,0},{0,0,1},{0,1,0}}},y{{{0,0,0},{1,0,0},{0,0,1}}};
  uint64_t work=0;const auto column=SourceColumn({0,0,0},{x,y},1e-6,work,100);
  CHECK(column.x<0);CHECK(column.y<0);CHECK(column.z==0);CHECK(work>0);CHECK(work<=100);
  auto outer=x;for(auto& p:outer)p.x=-8e-6;
  CHECK(SourceFacesSeparated(x,outer,4e-6));CHECK_FALSE(SourceFacesSeparated(x,outer,1e-5));
  CHECK_FALSE(SourceFacesSeparated(x,x,1e-8));
  auto rotated=x;for(auto& p:rotated){const auto q=p;p={q.x+.37*q.y-.61*q.z,q.y+.23*q.z,q.z};}
  auto shifted=rotated;for(auto& p:shifted)p.x-=1e-3;
  CHECK(SourceFacesSeparated(rotated,shifted,1e-5));
  uint64_t exhausted=0;CHECK_THROWS(SourceColumn({0,0,0},{x,y},1e-6,exhausted,0));
  auto opposite=x;std::swap(opposite[1],opposite[2]);work=0;
  CHECK_THROWS(SourceColumn({0,0,0},{x,opposite},1e-6,work,100));
  CHECK_THROWS(SourceFacesSeparated(x,outer,0));
}

namespace {
std::shared_ptr<const SU2Native3D::PlanarReference> MappedTetrahedron() {
  using namespace SU2Native3D;
  const std::array<Node,4> n{{{1,{1,.37,-8.80388993699896e-19}},{9,{1,1.37,0}},{21,{0,.5,1}},{40,{2,.5,1}}}};
  REQUIRE(Orientation(n[0].p,n[1].p,n[2].p,n[3].p)>0);
  const std::vector<Facet> facets{{100,7,{n[1],n[2],n[3]}},{101,7,{n[0],n[3],n[2]}},
                                  {102,7,{n[0],n[1],n[3]}},{103,7,{n[0],n[2],n[1]}}};
  return std::make_shared<PlanarReference>(facets);
}
}
TEST_CASE("Unrounded mapped parameters preserve exact feature incidences", "[NativeParametricGeometry]") {
  using namespace SU2Native3D;
  const Point b{1,0,0},c{0,1,0};
  for(double t:{.1,.3}) {
    const ParametricPoint p{b,c,b,t,0};
    CHECK(ParametricOrientation({b},{c},p,2)==0);
    CHECK(Orientation(b,c,{1-t,t,0},{0,0,1})!=0);
    const auto bounds=ParametricPointBounds(p);CHECK(bounds[0].x<=1-t);CHECK(bounds[1].x>=1-t);
    CHECK(bounds[0].y==t);CHECK(bounds[1].y==t);
  }
  const ParametricPoint a{{1e16,1e16,1e16},{1e16+8,1e16+16,1e16+24},{},.3,0};
  auto bmodel=a;bmodel.u=std::nextafter(a.u,INFINITY);
  const auto x=ParametricPointBounds(a),y=ParametricPointBounds(bmodel);
  CHECK(x[0].x==y[0].x);CHECK(x[0].y==y[0].y);CHECK(x[0].z==y[0].z);
  CHECK_THROWS(ParametricOrientation(a,a,a,3));
  auto bad=a;bad.u=INFINITY;CHECK_THROWS(ParametricPointBounds(bad));
  const ParametricPoint overflow{{0,0,0},{1e308,0,0},{},1e308,0};CHECK_THROWS(ParametricPointBounds(overflow));
}
TEST_CASE("Mapped finite facets preserve shared parameters complete coverage and error contracts", "[NativeMappedFacetGeometry]") {
  using namespace SU2Native3D;
  const auto original=MappedTetrahedron();std::map<Id,double> errors;for(const auto& f:original->Facets())errors.emplace(f.first,1e-13);
  CHECK_THROWS(MappedFacetReference(original,errors,7,0));
  MappedFacetReference mapped(original,errors,7);
  const auto interior=mapped.FacetPoint(100,.2,.3);CHECK(mapped.ErrorBound(interior,mapped.Realize(interior))<=1e-13);
  std::vector<SurfaceFace> surface;std::map<Id,SurfaceBinding> bindings;std::map<Id,Point> realized;
  for(const auto& row:original->Facets())for(auto n:row.second.v){bindings.emplace(n.id,mapped.Vertex(n.id));realized.emplace(n.id,n.p);}
  const Id inserted=55;bindings.emplace(inserted,mapped.EdgePoint({1,9},.3));realized.emplace(inserted,mapped.Realize(bindings.at(inserted)));
  CHECK(mapped.ErrorBound(bindings.at(inserted),realized.at(inserted))>0);
  CHECK(mapped.ErrorBound(bindings.at(inserted),realized.at(inserted))<=1e-13);
  for(const auto& row:original->Facets()) {
    const auto& f=row.second;bool split=false;
    for(unsigned k=0;k<3;++k)if((f.v[k].id==1&&f.v[(k+1)%3].id==9)||(f.v[k].id==9&&f.v[(k+1)%3].id==1)) {
      surface.push_back({{f.v[k].id,inserted,f.v[(k+2)%3].id},f.id});
      surface.push_back({{inserted,f.v[(k+1)%3].id,f.v[(k+2)%3].id},f.id});split=true;break;
    }
    if(!split)surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
  }
  MappedSurfaceAudit report;std::string reason;
  INFO(reason);REQUIRE(mapped.ValidateSurface(surface,bindings,realized,report,reason,6));
  CHECK(report.nodes==5);CHECK(report.faces==6);CHECK(report.feature_segments==14);
  const auto saved=report;
  CHECK_FALSE(mapped.ValidateSurface(surface,bindings,realized,report,reason,5));CHECK(report.faces==saved.faces);
  CHECK_FALSE(mapped.ValidateSurface(surface,bindings,realized,report,reason,6,0));CHECK(report.faces==saved.faces);
  auto shifted=realized;shifted.at(inserted).x+=1e-6;
  CHECK_FALSE(mapped.ValidateSurface(surface,bindings,shifted,report,reason,6));CHECK(report.faces==saved.faces);
  auto missing=surface;missing.pop_back();CHECK_FALSE(mapped.ValidateSurface(missing,bindings,realized,report,reason,6));
  auto reversed=surface;std::swap(reversed.front().v[0],reversed.front().v[1]);CHECK_FALSE(mapped.ValidateSurface(reversed,bindings,realized,report,reason,6));
  auto bad=bindings;bad.at(inserted).version++;CHECK_FALSE(mapped.ValidateSurface(surface,bad,realized,report,reason,6));
  CHECK_THROWS(mapped.EdgePoint({9,1},.3));CHECK_THROWS(mapped.EdgePoint({1,9},-.1));
  CHECK_THROWS(mapped.FacetPoint(100,1,std::numeric_limits<double>::denorm_min()));
  CHECK_THROWS(mapped.FacetPoint(100,.5,.5));
  auto strict=errors;strict.erase(102);MappedFacetReference mixed(original,strict,7);
  CHECK(mixed.MaximumError(bindings.at(inserted))==0);CHECK_THROWS(mixed.Realize(bindings.at(inserted)));
  std::map<Id,SurfaceBinding> renamed;std::map<Id,Point> points;
  for(const auto& row:bindings){renamed.emplace(row.first+500,row.second);points.emplace(row.first+500,realized.at(row.first));}
  for(auto& f:surface)for(auto& id:f.v)id+=500;
  REQUIRE(mapped.ValidateSurface(surface,renamed,points,report,reason,6));CHECK(report.maximum_mapping_error==saved.maximum_mapping_error);
}

TEST_CASE("Mapped edge edits publish mesh and forward geometry atomically and survive reference persistence", "[NativeMappedPublication3D]") {
  using namespace SU2Native3D;World world;const bool components=GENERATE(false,true);
  const auto original=MappedTetrahedron();std::vector<Facet> facets;
  for(auto row:original->Facets()){row.second.marker=0;facets.push_back(row.second);}
  auto definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;
  for(const auto& f:facets)errors.emplace(f.id,1e-13);
  auto authority=std::make_shared<MappedFacetReference>(definition,errors,7,SIZE_MAX,components);
  Patch old;BoundaryPoints points;std::map<Id,Node> nodes;
  for(const auto& f:facets){old.surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
    for(auto n:f.v){nodes.emplace(n.id,n);points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}}
  old.cells.push_back({0,{nodes.at(1),nodes.at(9),nodes.at(21),nodes.at(40)}});
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,old.surface,11);
  auto reference=PlanarReference::WithParameters(definition,parameters);reference->ExportParameters(old);
  const auto initial=parameters->Snapshot();
  auto restored=BoundaryParameterState::FromPayload(authority,parameters->Payload());
  CHECK(restored->Snapshot()==initial);CHECK(&restored->Geometry()==authority.get());
  auto corrupt=parameters->Payload();corrupt[0]^=1;CHECK_THROWS(BoundaryParameterState::FromPayload(authority,corrupt));
  corrupt=parameters->Payload();corrupt.pop_back();CHECK_THROWS(BoundaryParameterState::FromPayload(authority,corrupt));
  CHECK_THROWS(BoundaryParameterState::FromPayload(authority,parameters->Payload(),1));
  const auto proposed=authority->Realize(authority->EdgePoint({1,9},.3));Patch fresh;std::string reason;
  REQUIRE(SplitEdge(old,{1,9},{55,proposed},100,*reference,fresh,reason));
  REQUIRE(fresh.parameters.size()==5);CHECK(fresh.cells.size()==2);CHECK(fresh.surface.size()==6);
  CHECK(parameters->Snapshot()==initial);
  auto rows=[&](const Patch& patch) {
    std::vector<OwnedCell> result;
    for(const auto& c:patch.cells) {
      OwnedCell row;row.cell=c;
      const std::array<Face,4> face{{{c.v[1].id,c.v[2].id,c.v[3].id},{c.v[0].id,c.v[3].id,c.v[2].id},
          {c.v[0].id,c.v[1].id,c.v[3].id},{c.v[0].id,c.v[2].id,c.v[1].id}}};
      for(unsigned k=0;k<4;++k)for(auto f:patch.surface){auto a=face[k],b=f.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
        if(a==b)row.physical[k]={true,f.facet};}
      result.push_back(row);
    }return result;
  };
  Owned owned;for(auto row:rows(old))if(row.cell.id%world.size==unsigned(world.rank))owned.emplace(row.cell.id,row);
  Directory directory(world,true,parameters.get());REQUIRE(directory.Build(owned,reason));
  const auto directory_before=directory.Snapshot();std::vector<Dependency> removed;std::vector<OwnedCell> added;
  if(!world.rank)removed.push_back({0,0,0});
  for(auto row:rows(fresh))if(row.cell.id%world.size==unsigned(world.rank))added.push_back(row);
  const auto edits=world.rank?std::vector<BoundaryParameterEdit>{}:BoundaryParameterState::Changes(old,fresh,parameters->Revision());
  CHECK_FALSE(directory.Publish(owned,removed,added,world.rank!=0,reason,SIZE_MAX,0,edits));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1,0,edits));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  auto stale=edits;for(auto& e:stale)++e.revision;
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,stale));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  INFO(reason);REQUIRE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,edits));
  CHECK(parameters->Revision()==12);CHECK(parameters->Records().size()==5);CHECK(parameters->Surface().size()==6);
  CHECK(CPassiveComm::Allreduce(uint64_t(owned.size()),CPassiveComm::Op::SUM,world.comm)==2);
  ReferenceState state;state.original=reference;state.facets=facets;state.marker_names={"body"};
  const auto encoded=EncodeReference(state);CHECK(encoded.find(components?"SU2_NATIVE_REFERENCE_3D 6\n":"SU2_NATIVE_REFERENCE_3D 5\n")==0);
  CHECK(EncodeReference(DecodeReference(encoded))==encoded);
  CHECK_THROWS(DecodeReference(encoded+"trailing"));CHECK_THROWS(DecodeReference(encoded,1));
  std::map<Id,Id> numbering;for(auto row:parameters->Records())numbering.emplace(row.first,row.first+1000);
  auto rebound=parameters->Rebind(numbering);CHECK(rebound->Revision()==13);CHECK(&rebound->Geometry()==authority.get());
  const auto& bound=rebound->Records().at(1055).point.binding;const auto& expected=parameters->Records().at(55).point.binding;
  CHECK(bound.feature==expected.feature);CHECK(bound.u==expected.u);CHECK(bound.v==expected.v);CHECK(bound.version==expected.version);
  numbering.erase(numbering.begin());CHECK_THROWS(parameters->Rebind(numbering));
  CHECK(parameters->Revision()==12);
}

TEST_CASE("Mapped realized face orientation is independent of projection axis and scale", "[NativeMappedRealized3D]") {
  using namespace SU2Native3D;
  for(double scale:{1e-300,1.,1e300}) {
    const std::array<Point,3> original{{{0,0,0},{scale,2*scale,-scale},{-scale,scale,3*scale}}};
    KernelStats stats;CHECK(SurfaceOrientationSign(original,original,&stats)>0);
    auto reversed=original;std::swap(reversed[1],reversed[2]);CHECK(SurfaceOrientationSign(original,reversed,&stats)<0);
    auto zero=original;zero[2]=zero[1];CHECK(SurfaceOrientationSign(original,zero,&stats)==0);
    CHECK(stats.exact_orientations>0);
  }
  const std::array<Node,4> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}};
  const std::vector<Facet> facets{{10,0,{nodes[1],nodes[2],nodes[3]}},{11,0,{nodes[0],nodes[3],nodes[2]}},
      {12,0,{nodes[0],nodes[1],nodes[3]}},{13,0,{nodes[0],nodes[2],nodes[1]}}};
  auto original=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;
  for(auto f:facets)errors.emplace(f.id,1e-13);
  auto authority=std::make_shared<MappedFacetReference>(original,errors,7);
  const double delta=std::ldexp(1.,-45);
  const std::array<SurfaceBinding,3> bindings{{authority->FacetPoint(13,.3,.3),authority->FacetPoint(13,.3+delta,.3),authority->FacetPoint(13,.3,.3+delta)}};
  std::array<Point,3> points;for(unsigned i=0;i<3;++i)points[i]=authority->Realize(bindings[i]);
  REQUIRE(authority->ContainsFace(13,bindings,points));
  auto folded=points;folded[1].y=.3-delta;
  CHECK(authority->Admissible(bindings[1],folded[1]));CHECK_FALSE(authority->ContainsFace(13,bindings,folded));
  auto flat=points;flat[1]=flat[0];CHECK(authority->Admissible(bindings[1],flat[1]));CHECK_FALSE(authority->ContainsFace(13,bindings,flat));
  BoundaryPoints records;std::vector<SurfaceFace> skin;
  for(auto f:facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
    for(auto n:f.v)records.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
  auto state=std::make_shared<BoundaryParameterState>(authority,records,skin);
  auto reference=PlanarReference::WithParameters(original,state);const auto saved=state->Snapshot();
  Patch invalid;auto bad=records.begin()->second;bad.node.id=991;bad.node.p.x=std::numeric_limits<double>::quiet_NaN();
  invalid.parameters.emplace(991,bad);CHECK_THROWS(reference->PrivateParameters(invalid));CHECK(state->Snapshot()==saved);
  bad=records.begin()->second;bad.node.id=991;++bad.binding.version;invalid.parameters[991]=bad;
  CHECK_THROWS(reference->PrivateParameters(invalid));CHECK(state->Snapshot()==saved);
  // A child proposal owns a face-interior point absent from accepted metadata.
  Patch child;child.parameters=records;const auto binding=authority->FacetPoint(13,.25,.25);
  const Node inserted{55,authority->Realize(binding)};child.parameters.emplace(55,BoundaryPoint{inserted,binding});
  for(auto f:skin)if(f.facet!=13)child.surface.push_back(f);
  for(auto face:std::array<Face,3>{{{0,2,55},{2,1,55},{1,0,55}}}) {
    child.surface.push_back({face,13});std::array<Node,4> v{nodes[3],Node{},Node{},Node{}};
    for(unsigned i=0;i<3;++i)v[i+1]=face[i]==55?inserted:nodes[face[i]];
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);
    child.cells.push_back({Id(child.cells.size()+100),v});
  }
  const auto moved=ConstrainMovePoint(child,55,{55,{.275,.26,0}},{},*reference);
  CHECK(moved.p.x!=inserted.p.x);CHECK(moved.p.y!=inserted.p.y);CHECK(moved.p.z==0);CHECK(state->Snapshot()==saved);
}

namespace {
std::vector<SU2Native3D::Facet> ComponentCube(bool rotated) {
  using namespace SU2Native3D;std::array<Node,8> nodes;
  for(unsigned i=0;i<8;++i){const double x=i&1,y=(i>>1)&1,z=(i>>2)&1;
    nodes[i]={i,rotated?Point{7.125+x+.25*y+.125*z,-2.25+.375*x+y+.25*z,3.875+.125*x+.5*y+z}:Point{x,y,z}};}
  const std::array<Face,12> faces{{{0,2,3},{0,3,1},{4,5,7},{4,7,6},{0,4,6},{0,6,2},
      {1,3,7},{1,7,5},{0,1,5},{0,5,4},{2,6,7},{2,7,3}}};
  std::vector<Facet> result;for(auto face:faces)result.push_back({Id(result.size()),0,{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});return result;
}
std::shared_ptr<SU2Native3D::BoundaryParameterState> ComponentState(
    const std::shared_ptr<const MappedFacetReference>& geometry,const std::vector<SU2Native3D::Facet>& facets) {
  using namespace SU2Native3D;BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(auto f:facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});}
  return std::make_shared<BoundaryParameterState>(geometry,points,skin);
}
}
TEST_CASE("Mapped components release certified diagonals and persist without changing v5", "[NativeMappedComponent3D]") {
  using namespace SU2Native3D;const bool rotated=GENERATE(false,true);const auto facets=ComponentCube(rotated);
  auto base=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;for(auto f:facets)errors.emplace(f.id,1e-12);
  auto grouped=std::make_shared<MappedFacetReference>(base,errors,7,SIZE_MAX,true);
  auto legacy=std::make_shared<MappedFacetReference>(base,errors,7);
  CHECK(grouped->Canonical(0)==grouped->Canonical(1));CHECK(legacy->Canonical(0)!=legacy->Canonical(1));
  CHECK(grouped->ComponentFacets(0).size()==2);CHECK(grouped->ComponentBorder(0).size()==4);
  CHECK_FALSE(grouped->PhysicalSupport(grouped->EdgePoint({0,3},.5)));CHECK(grouped->PhysicalSupport(grouped->EdgePoint({0,1},.5)));
  CHECK(grouped->PhysicalSupport(grouped->Vertex(0)));
  auto state=ComponentState(grouped,facets);auto reference=PlanarReference::WithParameters(base,state);
  const auto snapshot=state->Snapshot();std::map<Id,Node> nodes;for(auto f:facets)for(auto n:f.v)nodes.emplace(n.id,n);
  const std::array<SurfaceBinding,3> b{grouped->Vertex(0),grouped->Vertex(2),grouped->Vertex(1)};
  const std::array<Point,3> p{nodes.at(0).p,nodes.at(2).p,nodes.at(1).p};
  CHECK(grouped->ContainsFace(0,b,p));CHECK_FALSE(legacy->ContainsFace(0,b,p));
  std::vector<BoundaryFaceChange> changes{{{0,2,3},0,false},{{0,3,1},0,false},{{0,2,1},0,true},{{1,2,3},0,true}};
  std::string reason;const auto publication=state->Prepare({},changes,reason);INFO(reason);CHECK(publication.valid);
  CHECK(state->Snapshot()==snapshot);CHECK_FALSE(state->Prepare({},changes,reason,1).valid);CHECK(state->Snapshot()==snapshot);
  auto damaged=changes;damaged.back().nodes={0,1,3};CHECK_FALSE(state->Prepare({},damaged,reason).valid);CHECK(state->Snapshot()==snapshot);
  ReferenceState retained;retained.original=reference;retained.facets=facets;retained.marker_names={"body"};
  const auto encoded=EncodeReference(retained);CHECK(encoded.find("SU2_NATIVE_REFERENCE_3D 6\n")==0);
  CHECK(EncodeReference(DecodeReference(encoded))==encoded);CHECK_THROWS(DecodeReference(encoded+"trailing"));CHECK_THROWS(DecodeReference(encoded,1));
  auto old=retained;old.original=PlanarReference::WithParameters(base,ComponentState(legacy,facets));
  const auto v5=EncodeReference(old);CHECK(v5.find("SU2_NATIVE_REFERENCE_3D 5\n")==0);CHECK(EncodeReference(DecodeReference(v5))==v5);
  CHECK_THROWS(BoundaryParameterState::FromPayload(legacy,state->Payload()));
  auto roundtrip=BoundaryParameterState::FromPayload(grouped,state->Payload());CHECK(roundtrip->Snapshot()==snapshot);
  std::map<Id,Id> numbering;for(auto row:state->Records())numbering.emplace(row.first,row.first+500);
  auto rebound=state->Rebind(numbering);CHECK(rebound->Geometry().ReferenceComponents());CHECK(rebound->Records().size()==state->Records().size());
}

TEST_CASE("Mapped component movement crosses an original planar seam without moving physical corners", "[NativeMappedComponent3D]") {
  using namespace SU2Native3D;const auto facets=ComponentCube(false);auto base=std::make_shared<PlanarReference>(facets);
  std::map<Id,double> errors;for(auto f:facets)errors.emplace(f.id,1e-13);
  auto g=std::make_shared<MappedFacetReference>(base,errors,7,SIZE_MAX,true);auto state=ComponentState(g,facets);
  std::map<Id,Node> nodes;for(auto f:facets)for(auto n:f.v)nodes.emplace(n.id,n);
  const Node middle{55,g->Realize(g->EdgePoint({0,3},.5))};nodes.emplace(55,middle);
  Patch child;const std::array<std::array<Id,4>,8> cells{{{0,1,55,7},{55,1,3,7},{0,55,2,7},{55,3,2,7},
      {0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}}};
  for(auto ids:cells){Cell c;c.id=child.cells.size();for(unsigned k=0;k<4;++k)c.v[k]=nodes.at(ids[k]);if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[0],c.v[1]);child.cells.push_back(c);}
  for(auto f:facets)if(f.id>1)child.surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},g->Canonical(f.id)});
  for(auto face:std::array<Face,4>{{{0,2,55},{55,2,3},{0,55,1},{55,3,1}}})child.surface.push_back({face,g->Canonical(0)});
  BoundaryPoints points;for(auto row:state->Records())points.emplace(row.first,row.second.point);points.emplace(55,BoundaryPoint{middle,g->EdgePoint({0,3},.5)});
  state=std::make_shared<BoundaryParameterState>(g,points,child.surface);auto reference=PlanarReference::WithParameters(base,state);
  child.cells.resize(4);child.surface.erase(child.surface.begin(),child.surface.end()-4);reference->ExportParameters(child);
  const auto before=state->Snapshot();const auto proposal=ConstrainMovePoint(child,55,{56,{.55,.45,0}},{},*reference);
  CHECK(proposal.p.x!=proposal.p.y);Patch fresh;std::string reason;INFO(reason);
  REQUIRE(MoveVertex(child,55,proposal,100,*reference,fresh,reason));CHECK(fresh.parameters.at(56).binding.dimension==2);
  CHECK(state->Snapshot()==before);CHECK(reference->Parameters()==state.get());
  const auto corner=ConstrainMovePoint(child,0,{57,{.1,.1,.1}},{},*reference);CHECK(corner.p.x==0);CHECK(corner.p.y==0);CHECK(corner.p.z==0);
}

TEST_CASE("Mapped finite components retain two holes a notch and a disconnected body footprint", "[NativeMappedComponent3D]") {
  using namespace SU2Native3D;const auto definition=Definitions().at(GENERATE(2u,3u,4u));const AffineChart chart(definition);auto planar=MultipleFootprints(chart);
  const auto& first=planar.front();const auto a=first.v[0].p,b=first.v[1].p,c=first.v[2].p;
  const Point u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};Point normal{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
  const auto length=std::hypot(normal.x,normal.y,normal.z);normal={normal.x/length,normal.y/length,normal.z/length};
  auto upper=[&](Node n){n.id+=10000;n.p={n.p.x+normal.x,n.p.y+normal.y,n.p.z+normal.z};return n;};
  std::map<Edge,std::vector<Edge>> edges;std::vector<Facet> facets;std::map<Id,AffineAuthority> planes;
  auto top=definition;top.identity+=100;top.origin={top.origin.x+normal.x,top.origin.y+normal.y,top.origin.z+normal.z};
  for(auto f:planar) {
    auto low=f;std::swap(low.v[1],low.v[2]);low.id=facets.size();low.marker=0;planes.emplace(low.id,definition);facets.push_back(low);
    f.id=facets.size();f.marker=1;for(auto& n:f.v)n=upper(n);planes.emplace(f.id,top);facets.push_back(f);
  }
  std::map<Id,Node> nodes;for(auto f:planar)for(auto n:f.v)nodes.emplace(n.id,n);
  for(auto f:planar)for(unsigned k=0;k<3;++k){Edge e{f.v[k].id,f.v[(k+1)%3].id},key=e;std::sort(key.begin(),key.end());edges[key].push_back(e);}
  for(auto row:edges)if(row.second.size()==1){const auto e=row.second.front();const auto p=nodes.at(e[0]),q=nodes.at(e[1]),pt=upper(p),qt=upper(q);
    facets.push_back({Id(facets.size()),2,{p,q,qt}});facets.push_back({Id(facets.size()),2,{p,qt,pt}});}
  auto base=std::make_shared<PlanarReference>(facets,SIZE_MAX,std::map<Id,AnalyticSurface>{},planes);std::map<Id,double> errors;
  for(auto f:facets)errors.emplace(f.id,1e-10);auto g=std::make_shared<MappedFacetReference>(base,errors,7,SIZE_MAX,true);
  auto state=ComponentState(g,facets);const Id root=g->Canonical(0);
  CHECK_FALSE(g->PhysicalSupport(g->Vertex(103)));CHECK(g->PhysicalSupport(g->Vertex(101)));
  CHECK(g->ContainsSegment(root,g->Vertex(200),g->Vertex(2)));CHECK_FALSE(g->ContainsSegment(root,g->Vertex(101),g->Vertex(202)));
  CHECK_FALSE(g->ContainsSegment(root,g->Vertex(100),g->Vertex(106)));CHECK_FALSE(g->ContainsSegment(root,g->Vertex(5),g->Vertex(8)));
  auto face=[&](std::array<Id,3> ids){std::array<SurfaceBinding,3> bind;std::array<Point,3> p;
    for(unsigned k=0;k<3;++k){bind[k]=g->Vertex(ids[k]);p[k]=nodes.at(ids[k]).p;}return g->ContainsFace(root,bind,p);};
  CHECK(face({0,103,6}));CHECK_FALSE(face({101,202,102}));CHECK_FALSE(face({0,303,6}));CHECK_FALSE(face({5,108,8}));
  const auto saved=state->Snapshot();ReferenceState reference;reference.original=PlanarReference::WithParameters(base,state);reference.facets=facets;reference.marker_names={"bottom","top","side"};
  const auto text=EncodeReference(reference);CHECK(EncodeReference(DecodeReference(text))==text);CHECK(state->Snapshot()==saved);
  const auto recovered=DecodeReference(text);CHECK(recovered.original->Mapped());CHECK(recovered.original->Parameters()->Geometry().ReferenceComponents());
}

TEST_CASE("Exact ideal finite-component queries resolve tangencies without rounded intersections", "[NativeMappedComponent3D]") {
  using namespace SU2Native3D;
  for(double scale:{1e-300,1.,1e300}) {
    const std::array<ParametricPoint,3> t{{{{0,0,0}},{{scale,0,0}},{{0,scale,0}}}};
    auto p=[&](double x,double y){return ParametricPoint{{x*scale,y*scale,0}};};
    KernelStats stats;
    CHECK(ParametricSegmentEntersTriangle(p(-1,.25),p(1,.25),t,2,&stats));
    CHECK(ParametricSegmentEntersTriangle(p(.25,.25),p(.25,.25),t,2,&stats));
    CHECK_FALSE(ParametricSegmentEntersTriangle(p(-1,0),p(1,0),t,2,&stats));
    CHECK_FALSE(ParametricSegmentEntersTriangle(p(-1,1),p(1,1),t,2,&stats));
    CHECK_FALSE(ParametricSegmentEntersTriangle(p(1,0),p(0,1),t,2,&stats));
    CHECK_FALSE(ParametricSegmentEntersTriangle(p(-1,2),p(2,2),t,2,&stats));
    CHECK(ParametricOnProjectedSegment(p(0,0),p(1,1),p(.5,.5),2,&stats));
    CHECK_FALSE(ParametricOnProjectedSegment(p(0,0),p(1,1),p(2,2),2,&stats));
    CHECK(ParametricCentroidSide(p(0,0),p(1,0),t,2,&stats)>0);
    const std::array<Point,3> face{{{0,0,0},{scale,0,0},{0,scale,0}}};
    CHECK(SurfaceNormalSign({0,0,scale},face,&stats)>0);
    CHECK(SurfaceNormalSign({0,0,-scale},face,&stats)<0);
    CHECK(SurfaceNormalSign({scale,0,0},face,&stats)==0);
    CHECK(ParametricPlaneSide(p(.25,.25),{0,0,0},{0,0,scale})==0);
    CHECK(ParametricPlaneSide(p(.25,.25),{0,0,scale},{0,0,scale})<0);
    CHECK(stats.exact_orientations>0);
  }
  const ParametricPoint a{{0,0,0}},b{{1,0,0}};
  const std::array<ParametricPoint,3> cancelled{{{{0,1,0}},{{0,-1,0}},{{0,0,0}}}};
  CHECK(ParametricCentroidSide(a,b,cancelled,2)==0);
  auto reversed=cancelled;reversed[2].origin.y=std::numeric_limits<double>::denorm_min();
  CHECK(ParametricCentroidSide(a,b,reversed,2)>0);
  CHECK_THROWS(ParametricCentroidSide(a,b,cancelled,3));
}
