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
  // The corner query needs the complete ball of node 0: every incident physical face constrains it.
  auto ball=child;reference->ExportParameters(ball);
  child.cells.resize(4);child.surface.erase(child.surface.begin(),child.surface.end()-4);reference->ExportParameters(child);
  const auto before=state->Snapshot();const auto proposal=ConstrainMovePoint(child,55,{56,{.55,.45,0}},{},*reference);
  CHECK(proposal.p.x!=proposal.p.y);Patch fresh;std::string reason;INFO(reason);
  REQUIRE(MoveVertex(child,55,proposal,100,*reference,fresh,reason));CHECK(fresh.parameters.at(56).binding.dimension==2);
  CHECK(state->Snapshot()==before);CHECK(reference->Parameters()==state.get());
  const auto corner=ConstrainMovePoint(ball,0,{57,{.1,.1,.1}},{},*reference);CHECK(corner.p.x==0);CHECK(corner.p.y==0);CHECK(corner.p.z==0);
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
  CHECK_THROWS_AS(MappedFacetReference(base,errors,7,4096*facets.size(),true),std::length_error);
  CHECK_FALSE(g->PhysicalSupport(g->Vertex(103)));CHECK(g->PhysicalSupport(g->Vertex(101)));
  CHECK(g->ContainsSegment(root,g->Vertex(200),g->Vertex(2)));CHECK_FALSE(g->ContainsSegment(root,g->Vertex(101),g->Vertex(202)));
  CHECK(g->ContainsSegment(root,g->Vertex(100),g->Vertex(106)));
  CHECK_FALSE(g->ContainsSegment(root,g->Vertex(100),g->Vertex(206)));CHECK_FALSE(g->ContainsSegment(root,g->Vertex(5),g->Vertex(8)));
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

TEST_CASE("DC-17 fitted symmetry plane lets original vertices slide on a tilted noisy plane", "[NativeMappedComponent3D]") {
  // A tilted plane (any orientation) whose imported vertices deviate by ~1e-15 from exact coplanarity: the exact
  // coplanar merge keeps every facet its own root, so the promoted reference pins the original interior vertex; the
  // fitted affine plane authority (FitPlanarMarkers) gives it one root and it slides in the plane. Pyramid volume:
  // 3x3 plane grid (ids 0..8, centre 4, marker 0 = symmetry) coned to an apex (id 9, side marker 1).
  using namespace SU2Native3D;
  const double rotation=GENERATE(0.,.7,2.1);
  const Point raw{.37+rotation,1.,-.61+.5*rotation};const double nl=std::hypot(raw.x,raw.y,raw.z);
  const Point n{raw.x/nl,raw.y/nl,raw.z/nl};
  auto cross=[](Point a,Point b){return Point{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
  auto unit=[](Point a){const double l=std::hypot(a.x,a.y,a.z);return Point{a.x/l,a.y/l,a.z/l};};
  const Point t1=unit(cross(n,std::abs(n.x)<.9?Point{1,0,0}:Point{0,1,0})),t2=cross(n,t1);
  const Point o{1.25,-.5,2.};
  auto at=[&](double u,double v,double w){return Point{o.x+u*t1.x+v*t2.x+w*n.x,o.y+u*t1.y+v*t2.y+w*n.y,o.z+u*t1.z+v*t2.z+w*n.z};};
  std::array<Node,10> nodes;
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){const Id id=3*i+j;nodes[id]={id,at(double(i)-1,double(j)-1,1e-15*(double((5*id)%3)-1))};}
  nodes[9]={9,at(0,0,1)};
  const Point inside=at(0,0,.3);
  auto outward=[&](Facet f){const Point a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
    const Point m=cross(Point{b.x-a.x,b.y-a.y,b.z-a.z},Point{c.x-a.x,c.y-a.y,c.z-a.z});
    const Point g{(a.x+b.x+c.x)/3-inside.x,(a.y+b.y+c.y)/3-inside.y,(a.z+b.z+c.z)/3-inside.z};
    if(m.x*g.x+m.y*g.y+m.z*g.z<0)std::swap(f.v[1],f.v[2]);return f;};
  std::vector<Facet> facets;std::vector<std::array<Id,3>> plane;
  for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j){const Id a=3*i+j,b=3*(i+1)+j,c=3*(i+1)+j+1,d=3*i+j+1;
    for(auto t:{std::array<Id,3>{a,b,c},std::array<Id,3>{a,c,d}}){plane.push_back(t);facets.push_back(outward({Id(facets.size()),0,{nodes[t[0]],nodes[t[1]],nodes[t[2]]}}));}}
  const std::array<Id,8> ring{0,1,2,5,8,7,6,3};
  for(unsigned k=0;k<8;++k)facets.push_back(outward({Id(facets.size()),1,{nodes[ring[k]],nodes[ring[(k+1)%8]],nodes[9]}}));
  auto base=std::make_shared<PlanarReference>(facets);
  CHECK(base->Canonical(0)!=base->Canonical(1));  // the noisy plane is not merged exactly
  size_t fitted=0;const auto planes=FitPlanarMarkers(base,facets,{0},1e-13,fitted);
  REQUIRE(fitted==1);CHECK(planes->Affine().size()==8);CHECK(planes->Canonical(0)==planes->Canonical(7));
  size_t none=0;CHECK(FitPlanarMarkers(base,facets,{1},1e-13,none)==base);CHECK(none==0);  // the folded sides are not planar
  {// code review 1: a residual above the contract keeps the exact faceted authority (no fit, no throw); the
   // production contract equals the promotion's declared error (1e-13 x scale)
   auto rough=facets;for(auto& f:rough)for(auto& v:f.v)if(v.id==4){v.p.x+=3e-12*n.x;v.p.y+=3e-12*n.y;v.p.z+=3e-12*n.z;}
   auto roughBase=std::make_shared<PlanarReference>(rough);size_t k=0;CHECK(FitPlanarMarkers(roughBase,rough,{0},1e-13,k)==roughBase);CHECK(k==0);
   size_t k2=0;const auto fit13=FitPlanarMarkers(base,facets,{0},1e-13,k2);CHECK(k2==1);double d=0;CHECK_NOTHROW(PromoteFacetedReference(fit13,facets,1e-13,d));}
  auto star=[&](const std::shared_ptr<const PlanarReference>& reference){Patch p;
    for(size_t f=0;f<plane.size();++f){const auto& t=plane[f];if(t[0]!=4&&t[1]!=4&&t[2]!=4)continue;
      Cell c{Id(p.cells.size()),{nodes[9],nodes[t[0]],nodes[t[1]],nodes[t[2]]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);p.cells.push_back(c);
      p.surface.push_back({{facets[f].v[0].id,facets[f].v[1].id,facets[f].v[2].id},Id(f)});}
    reference->ExportParameters(p);return p;};
  const Point target=at(.12,-.07,0);
  double declared=0;
  {const auto pinned=PromoteFacetedReference(base,facets,1e-13,declared);auto child=star(pinned);Patch fresh;std::string reason;
   const auto proposal=ConstrainMovePoint(child,4,{100,target},{},*pinned);
   CHECK_FALSE(MoveVertex(child,4,proposal,1000,*pinned,fresh,reason));}
  const auto reference=PromoteFacetedReference(planes,facets,1e-13,declared);auto child=star(reference);
  const auto proposal=ConstrainMovePoint(child,4,{100,target},{},*reference);
  Patch fresh;std::string reason;
  REQUIRE(MoveVertex(child,4,proposal,1000,*reference,fresh,reason));INFO(reason);
  Point moved{};for(const auto& c:fresh.cells)for(const auto& v:c.v)if(v.id==100)moved=v.p;
  CHECK(std::hypot(moved.x-nodes[4].p.x,moved.y-nodes[4].p.y,moved.z-nodes[4].p.z)>.05);
  CHECK(std::abs((moved.x-o.x)*n.x+(moved.y-o.y)*n.y+(moved.z-o.z)*n.z)<=1e-12);
  CHECK(fresh.parameters.at(100).binding.dimension==2);
}

TEST_CASE("Curved mapped controls persist physical geometry separately from exact parameter topology", "[NativeCurvedMapped3D]") {
  using namespace SU2Native3D;
  const auto tetrahedron=MappedTetrahedron();
  std::vector<Facet> facets;for(auto row:tetrahedron->Facets()){row.second.marker=0;facets.push_back(row.second);}
  const auto original=std::make_shared<PlanarReference>(facets);
  MappedFacetReference::CurvedNets controls;std::map<Id,double> errors;
  const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for(const auto& f:facets) {
    auto& net=controls[f.id];errors[f.id]=1e-12;
    for(unsigned k=0;k<3;++k)net[k]=f.v[k].p;
    for(unsigned q=3;q<10;++q) {
      long double sum[3]={};
      for(unsigned k=0;k<3;++k){sum[0]+=powers[q][k]*(long double)f.v[k].p.x;sum[1]+=powers[q][k]*(long double)f.v[k].p.y;sum[2]+=powers[q][k]*(long double)f.v[k].p.z;}
      net[q]={double(sum[0]/3),double(sum[1]/3),double(sum[2]/3)};
    }
    const auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
    net[9].x+=.02*((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y));
    net[9].y+=.02*((b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z));
    net[9].z+=.02*((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));
  }
  CHECK_THROWS(MappedFacetReference(original,errors,9,1,false,controls));
  CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,true,controls));
  auto geometry=std::make_shared<MappedFacetReference>(original,errors,9,SIZE_MAX,false,controls);
  auto points=ComponentState(geometry,facets);
  ReferenceState state{PlanarReference::WithParameters(original,points),facets,{"wall"},.1};
  const auto encoded=EncodeReference(state);REQUIRE(encoded.find("SU2_NATIVE_REFERENCE_3D 7\n")==0);
  const auto restored=DecodeReference(encoded);CHECK(EncodeReference(restored)==encoded);
  CHECK_THROWS(DecodeReference(encoded,1024));
  const auto& recovered=restored.original->Parameters()->Geometry();
  REQUIRE(recovered.Curves().size()==controls.size());
  CHECK_THROWS(geometry->ClosestCurved(facets[0].id,{0,0,0},0,4096));
  CHECK_THROWS(geometry->ClosestCurved(facets[0].id,{0,0,0},1e-6,0));
  CHECK_THROWS(geometry->ClosestCurved(facets[0].id,{0,0,0},1e-6,4096,1));
  CHECK_THROWS(geometry->ClosestCurved(facets[0].id,{NAN,0,0},1e-6,4096));
  CHECK_THROWS(geometry->ClosestCurved(facets[0].id,{0,0,0},1e-6,1));
  for(const auto& f:facets) {
    const auto known=geometry->Realize(geometry->FacetPoint(f.id,.25,.25));
    const auto on=geometry->ClosestCurved(f.id,known,1e-8,4096);
    CHECK(on.upper<=1e-8);CHECK(on.lower<=geometry->MaximumError(on.binding));
    CHECK(geometry->Admissible(on.binding,on.point));
    const Point query{known.x+.01*on.normal.x,known.y+.01*on.normal.y,known.z+.01*on.normal.z};
    const auto near=geometry->ClosestCurved(f.id,query,1e-6,4096);
    INFO("curved work "<<near.work);CHECK(near.upper-near.lower<=1e-6);
    CHECK(near.lower<=.01+1e-12);CHECK(near.upper>=.01-1e-12);
    const auto replay=recovered.ClosestCurved(f.id,query,1e-6,4096);
    CHECK(replay.lower==near.lower);CHECK(replay.upper==near.upper);CHECK(replay.facet==near.facet);
    std::array<SurfaceBinding,3> corners;std::array<Point,3> actual;
    for(unsigned k=0;k<3;++k){corners[k]=geometry->Vertex(f.v[k].id);actual[k]=f.v[k].p;}
    REQUIRE(geometry->ContainsFace(f.id,corners,actual));
    const auto full=geometry->SurfaceError(f.id,corners,actual);
    REQUIRE(full>1e-3);CHECK(geometry->EnvelopeError(f.id)>=full);
    CHECK(state.original->SurfaceError(f.id,actual)==Approx(full));
    const auto& net=controls.at(f.id);
    for(unsigned i=1;i<8;++i)for(unsigned j=1;i+j<9;++j) {
      const double u=i/10.,v=j/10.,w=1-u-v;const auto binding=geometry->FacetPoint(f.id,u,v);
      const auto p=geometry->Realize(binding),q=recovered.Realize(binding);
      CHECK(p.x==q.x);CHECK(p.y==q.y);CHECK(p.z==q.z);CHECK(geometry->Admissible(binding,p));
      const long double c[10]={std::pow((long double)w,3),std::pow((long double)u,3),std::pow((long double)v,3),
          3*w*w*(long double)u,3*w*u*(long double)u,3*u*u*(long double)v,3*u*v*(long double)v,
          3*w*v*(long double)v,3*w*w*(long double)v,6*w*u*(long double)v};
      long double x=0,y=0,z=0;for(unsigned k=0;k<10;++k){x+=c[k]*net[k].x;y+=c[k]*net[k].y;z+=c[k]*net[k].z;}
      CHECK(std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z))<=geometry->ErrorBound(binding,p)+2e-15);
    }
    const std::array<SurfaceBinding,3> child{{geometry->FacetPoint(f.id,.2,.2),geometry->FacetPoint(f.id,.3,.2),geometry->FacetPoint(f.id,.2,.3)}};
    std::array<Point,3> childPoints;for(unsigned k=0;k<3;++k)childPoints[k]=geometry->Realize(child[k]);
    CHECK(geometry->ContainsFace(f.id,child,childPoints));
    CHECK(geometry->SurfaceError(f.id,child,childPoints)<full/8);
    childPoints[0].x+=.001;CHECK_FALSE(geometry->ContainsFace(f.id,child,childPoints));
  }
  auto disconnected=facets;
  for(auto f:facets){f.id+=1000;for(auto& n:f.v){n.id+=1000;n.p.x+=10;}disconnected.push_back(f);}
  MappedFacetReference mixed(std::make_shared<PlanarReference>(disconnected),errors,9,SIZE_MAX,false,controls);
  const auto strictPoint=disconnected.back().v[0];const auto strictBinding=mixed.Vertex(strictPoint.id);
  CHECK(mixed.ErrorBound(strictBinding,strictPoint.p)==0);CHECK(mixed.Admissible(strictBinding,strictPoint.p));
  CHECK(mixed.Realize(strictBinding).x==strictPoint.p.x);
  CHECK_THROWS(mixed.ClosestCurved(disconnected.back().id,strictPoint.p,1e-6,4096));
  auto twoControls=controls;auto twoErrors=errors;
  for(const auto& row:controls){auto net=row.second;for(auto& p:net)p.x+=10;twoControls[row.first+1000]=net;twoErrors[row.first+1000]=1e-12;}
  MappedFacetReference two(std::make_shared<PlanarReference>(disconnected),twoErrors,9,SIZE_MAX,false,twoControls);
  const auto first=two.Realize(two.FacetPoint(facets[0].id,.25,.25));
  const Point other{first.x+10,first.y,first.z};
  const auto restricted=two.ClosestCurved(facets[0].id,other,1e-5,8192);
  CHECK(restricted.facet<1000);CHECK(restricted.lower>8);
  const auto second=two.ClosestCurved(facets[0].id+1000,other,1e-8,4096);
  CHECK(second.facet>=1000);CHECK(second.upper<1e-8);
  auto separated=facets;for(auto& f:separated)f.marker=int(f.id);
  MappedFacetReference byMarker(std::make_shared<PlanarReference>(separated),errors,9,SIZE_MAX,false,controls);
  const auto markerPoint=byMarker.Realize(byMarker.FacetPoint(separated[1].id,.25,.25));
  const auto sameMarker=byMarker.ClosestCurved(separated[0].id,markerPoint,1e-6,4096);
  CHECK(sameMarker.facet==separated[0].id);CHECK(sameMarker.lower>0);
  Patch patch;Cell cell;std::map<Id,Node> vertices;
  for(const auto& f:facets){for(auto n:f.v)vertices.emplace(n.id,n);patch.surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});}
  size_t next=0;for(auto v:vertices)cell.v[next++]=v.second;
  if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
  patch.cells.push_back(cell);state.original->ExportParameters(patch);FirstHeightReport report;std::string reason;
  CHECK_FALSE(CheckFirstHeight(patch,{{0,.1}},*state.original,report,reason));
  CHECK(report.reference_clearance_failures>0); // Each opposite tetrahedron corner lies on another wall face.
  auto changedControls=controls;changedControls.begin()->second[9].x+=.001;
  auto changedGeometry=std::make_shared<MappedFacetReference>(original,errors,9,SIZE_MAX,false,changedControls);
  auto changedPoints=ComponentState(changedGeometry,facets);
  CHECK(points->SnapshotBytes()==changedPoints->SnapshotBytes());
  CHECK(points->Snapshot()!=changedPoints->Snapshot());
  World world;Owned owned;
  if(!world.rank) {
    OwnedCell row;row.cell=cell;
    const std::array<Face,4> cellFaces{{{cell.v[1].id,cell.v[2].id,cell.v[3].id},{cell.v[0].id,cell.v[3].id,cell.v[2].id},
        {cell.v[0].id,cell.v[1].id,cell.v[3].id},{cell.v[0].id,cell.v[2].id,cell.v[1].id}}};
    for(unsigned k=0;k<4;++k)for(const auto& f:facets) {
      auto a=cellFaces[k];Face b{f.v[0].id,f.v[1].id,f.v[2].id};std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
      if(a==b)row.physical[k]={true,f.id};
    }
    owned.emplace(cell.id,row);
  }
  Directory agreed(world,true,points.get());REQUIRE(agreed.Build(owned,reason));
  auto rankPoints=world.rank?changedPoints:points;const auto saved=rankPoints->Snapshot();
  Directory inconsistent(world,true,rankPoints.get());
  CHECK(inconsistent.Build(owned,reason)==(world.size==1));
  if(world.size>1)CHECK(reason.find("authority/records differ")!=std::string::npos);
  CHECK(rankPoints->Snapshot()==saved);
  auto bad=controls;bad.begin()->second[0].x+=.001;CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,false,bad));
  bad=controls;bad.begin()->second[3].x+=.001;CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,false,bad));
  bad=controls;bad.begin()->second[9].x=INFINITY;CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,false,bad));
  bad=controls;bad.begin()->second[9].x+=100;CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,false,bad));
  bad=controls;bad.erase(bad.begin());CHECK_THROWS(MappedFacetReference(original,errors,9,SIZE_MAX,false,bad));
}

TEST_CASE("Curved first-height clearance and principal curvature use the persisted polynomial", "[NativeCurvedMapped3D]") {
  using namespace SU2Native3D;
  const std::array<Node,4> n{{{0,{0,0,0}},{1,{3,0,0}},{2,{0,3,0}},{3,{0,0,-3}}}};
  const std::vector<Facet> facets{{0,0,{n[2],n[1],n[3]}},{1,0,{n[0],n[3],n[1]}},
      {2,0,{n[0],n[2],n[3]}},{3,0,{n[0],n[1],n[2]}}};
  const auto base=std::make_shared<PlanarReference>(facets);
  MappedFacetReference::CurvedNets nets;std::map<Id,double> errors;
  const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for(const auto& f:facets) {
    errors[f.id]=1e-12;
    for(unsigned q=0;q<10;++q) {
      long double p[3]={};for(unsigned k=0;k<3;++k){p[0]+=powers[q][k]*(long double)f.v[k].p.x;
        p[1]+=powers[q][k]*(long double)f.v[k].p.y;p[2]+=powers[q][k]*(long double)f.v[k].p.z;}
      nets[f.id][q]={double(p[0]/3),double(p[1]/3),double(p[2]/3)};
    }
  }
  {
    auto tilted=facets;auto tiltedNets=nets;
    auto rotate=[](Point p){return Point{.6*p.x+.8*p.z,p.y,-.8*p.x+.6*p.z};};
    for(auto& f:tilted)for(auto& node:f.v)node.p=rotate(node.p);
    for(auto& row:tiltedNets)for(auto& point:row.second)point=rotate(point);
    const auto tiltedBase=std::make_shared<PlanarReference>(tilted);
    const MappedFacetReference plane(tiltedBase,errors,1,SIZE_MAX,false,tiltedNets);
    auto query=plane.Realize(plane.FacetPoint(3,.25,.25));query.x-=.8e-6;query.z-=.6e-6;
    const auto clearance=plane.ClosestCurved(3,query,1e-10,16384);
    CHECK(clearance.clearance_lower>0);CHECK(clearance.clearance_lower<=clearance.upper);
    CHECK(clearance.upper==Approx(1e-6).margin(1e-12));
    const std::array<SurfaceBinding,3> binding{{plane.Vertex(0),plane.Vertex(1),plane.Vertex(2)}};
    std::array<Point,3> source;for(unsigned k=0;k<3;++k)source[k]=tilted[3].v[k].p;
    const auto patch=plane.PrepareSourcePatch(3,binding,source);
    CHECK(patch.OutwardDisplacement()<1e-10);
    for(auto& p:source){p.x+=.8e-3;p.z+=.6e-3;}uint64_t work=0;
    CHECK(patch.Separated(source,work,1024));
  }
  nets[3][9].z=.1;
  auto geometry=std::make_shared<MappedFacetReference>(base,errors,1,SIZE_MAX,false,nets);
  const std::array<SurfaceBinding,3> sourceBinding{{geometry->Vertex(0),geometry->Vertex(1),geometry->Vertex(2)}};
  const std::array<Point,3> sourceFace{{n[0].p,n[1].p,n[2].p}};
  const auto sourcePatch=geometry->PrepareSourcePatch(3,sourceBinding,sourceFace);
  CHECK(sourcePatch.OutwardDisplacement()>=.1);CHECK(sourcePatch.OutwardDisplacement()<.100000001);
  const auto bounds=sourcePatch.Bounds();CHECK(bounds[0].z<0);CHECK(bounds[1].z>.1);
  uint64_t sourceWork=0;
  // The exact graph peaks at .6/27, but coverage must certify the complete child-chord hull.
  CHECK_FALSE(sourcePatch.Separated({{{0,0,.03},{3,0,.03},{0,3,.03}}},sourceWork,1024));
  CHECK(sourcePatch.Separated({{{0,0,.2},{3,0,.2},{0,3,.2}}},sourceWork,1024));
  // Both triangles' edges are horizontal; their axes miss this curved-hull side separation.
  CHECK(sourcePatch.Separated({{{2,2,.02},{3,1.5,.02},{1.5,3,.02}}},sourceWork,1024));
  sourceWork=0;
  CHECK_THROWS_AS(sourcePatch.Separated({{{0,0,.01},{3,0,.01},{0,3,.01}}},sourceWork,0),std::length_error);
  CHECK_THROWS(geometry->PrepareSourcePatch(3,sourceBinding,{{{0,0,.01},n[1].p,n[2].p}}));
  const std::array<SurfaceBinding,3> childBinding{{geometry->Vertex(0),geometry->EdgePoint({0,1},.5),geometry->FacetPoint(3,.25,.25)}};
  std::array<Point,3> childFace;for(unsigned k=0;k<3;++k)childFace[k]=geometry->Realize(childBinding[k]);
  const auto childPatch=geometry->PrepareSourcePatch(3,childBinding,childFace);sourceWork=0;
  CHECK(childPatch.Separated({{{0,0,.2},{3,0,.2},{0,3,.2}}},sourceWork,1024));
  const auto point=geometry->Realize(geometry->FacetPoint(3,1./3,1./3));
  const auto differential=geometry->ClosestCurved(3,point,1e-10,16384);
  CHECK(differential.upper<1e-10);CHECK(differential.normal.z==Approx(1).margin(1e-10));
  // z=.6*u*v*(1-u-v), x=3*u, y=3*v has Hessian eigenvalues -1/45 and -1/15 at its center.
  CHECK(differential.curvature1==Approx(-1./45).margin(1e-8));
  CHECK(differential.curvature2==Approx(-1./15).margin(1e-8));
  auto dot=[](Point a,Point b){return a.x*b.x+a.y*b.y+a.z*b.z;};
  CHECK(dot(differential.direction1,differential.direction1)==Approx(1).margin(1e-12));
  CHECK(dot(differential.direction2,differential.direction2)==Approx(1).margin(1e-12));
  CHECK(dot(differential.direction1,differential.direction2)==Approx(0).margin(1e-12));
  const auto fluid=geometry->ClosestCurved(3,{point.x,point.y,point.z-.01},1e-8,16384);
  CHECK(fluid.clearance_lower>0);CHECK(fluid.clearance_lower<=.01+1e-12);
  const auto outside=geometry->ClosestCurved(3,{point.x,point.y,point.z+.01},1e-8,16384);
  CHECK(outside.clearance_lower<0);
  nets[3][9].z=-.1;geometry=std::make_shared<MappedFacetReference>(base,errors,2,SIZE_MAX,false,nets);
  const auto fluidward=geometry->PrepareSourcePatch(3,{{geometry->Vertex(0),geometry->Vertex(1),geometry->Vertex(2)}},sourceFace);
  CHECK(fluidward.OutwardDisplacement()<1e-10);sourceWork=0;
  CHECK(fluidward.Separated({{{0,0,.001},{3,0,.001},{0,3,.001}}},sourceWork,1024));
  auto parameters=ComponentState(geometry,facets);const auto reference=PlanarReference::WithParameters(base,parameters);
  CHECK(reference->SearchWorkspaceBytes()>=geometry->CurvedQueryWorkspaceBytes(MappedFacetReference::ClearanceWork));
  for(double h:{.001,.1}) {
    Patch patch;patch.cells.push_back({0,{n[0],n[2],n[1],{100,{1,1,-h}}}});
    patch.surface.push_back({{0,1,2},3});reference->ExportParameters(patch);
    FirstHeightReport report;std::string reason;
    const bool accepted=CheckFirstHeight(patch,{{0,h}},*reference,report,reason);INFO(reason);
    CHECK(accepted==(h==.1));CHECK(report.height_failures==0);CHECK(report.footprint_failures==0);
    CHECK(report.reference_clearance_failures==(h==.1?0:1));
    CHECK((report.minimum_reference_clearance_ratio>0)==(h==.1));
  }
}
