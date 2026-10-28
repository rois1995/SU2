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
  using namespace SU2Native3D;World world;const unsigned mode=GENERATE(0u,1u,2u);const bool components=mode==1,atlas=mode==2;
  const auto original=MappedTetrahedron();std::vector<Facet> facets;
  for(auto row:original->Facets()){row.second.marker=0;facets.push_back(row.second);}
  auto definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;
  for(const auto& f:facets)errors.emplace(f.id,1e-13);
  auto authority=std::make_shared<MappedFacetReference>(definition,errors,7,SIZE_MAX,components);
  Patch old;BoundaryPoints points;std::map<Id,Node> nodes;
  for(const auto& f:facets){old.surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
    for(auto n:f.v){nodes.emplace(n.id,n);points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}}
  old.cells.push_back({0,{nodes.at(1),nodes.at(9),nodes.at(21),nodes.at(40)}});
  // Each independent API transaction owns its reservation; keep every attempted-work receipt.
  const AtlasBudget transaction_limits{1000000,size_t(1)<<27,4096};AtlasBudget setup_budget=transaction_limits;
  AtlasBudget split_budget=transaction_limits,veto_budget=transaction_limits,storage_budget=transaction_limits;
  AtlasBudget stale_budget=transaction_limits,publication_budget=transaction_limits,bad_trace_budget=transaction_limits;
  BoundaryCoverage descriptors;std::string why;
  if(atlas)for(const auto& face:old.surface) {
    std::array<SurfaceBinding,3> bindings;for(unsigned k=0;k<3;++k)bindings[k]=points.at(face.v[k]).binding;
    FaceCoverage cover;REQUIRE(authority->ResolveSingleChart(face.facet,bindings,cover,setup_budget,why));descriptors.faces.emplace(face.v,cover);
    std::array<EdgeTrace,3> traces;REQUIRE(authority->CoverageTraces(cover,traces,setup_budget,why));
    for(unsigned k=0;k<3;++k)if(face.v[k]<face.v[(k+1)%3])descriptors.traces.emplace(Edge{face.v[k],face.v[(k+1)%3]},traces[k]);
  }
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,old.surface,11,SIZE_MAX,0,descriptors,MobilityPolicy{},atlas?&setup_budget:nullptr);
  auto reference=PlanarReference::WithParameters(definition,parameters);reference->ExportParameters(old,atlas?&setup_budget:nullptr);
  if(atlas)reference=std::shared_ptr<const PlanarReference>(reference->PrivateParameters(old,&split_budget));
  const auto initial=parameters->Snapshot();
  auto restored=BoundaryParameterState::FromPayload(authority,parameters->Payload());
  CHECK(restored->Snapshot()==initial);CHECK(&restored->Geometry()==authority.get());
  auto corrupt=parameters->Payload();corrupt[0]^=1;CHECK_THROWS(BoundaryParameterState::FromPayload(authority,corrupt));
  corrupt=parameters->Payload();corrupt.pop_back();CHECK_THROWS(BoundaryParameterState::FromPayload(authority,corrupt));
  CHECK_THROWS(BoundaryParameterState::FromPayload(authority,parameters->Payload(),1));
  const auto proposed=authority->Realize(authority->EdgePoint({1,9},.3));Patch fresh;std::string reason;
  const bool built=SplitEdge(old,{1,9},{55,proposed},100,*reference,fresh,reason);
  INFO(mode);INFO(reason);REQUIRE(built);
  REQUIRE(fresh.parameters.size()==5);CHECK(fresh.cells.size()==2);CHECK(fresh.surface.size()==6);
  CHECK(parameters->Snapshot()==initial);
  const auto setup_work=setup_budget.work,split_work=split_budget.work;
  if(atlas){CHECK(setup_work>0);CHECK(split_work>0);CHECK(setup_work<=setup_budget.maximum_work);CHECK(split_work<=split_budget.maximum_work);}
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
  CHECK_FALSE(directory.Publish(owned,removed,added,world.rank!=0,reason,SIZE_MAX,0,edits,fresh.coverage,atlas?&veto_budget:nullptr));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1,0,edits,fresh.coverage,atlas?&storage_budget:nullptr));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  auto stale=edits;for(auto& e:stale)++e.revision;
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,stale,fresh.coverage,atlas?&stale_budget:nullptr));
  CHECK(parameters->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  const auto veto_work=veto_budget.work,storage_work=storage_budget.work,stale_work=stale_budget.work;
  const bool published=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,edits,fresh.coverage,atlas?&publication_budget:nullptr);
  INFO("atlas work: setup="<<setup_work<<", split="<<split_work<<", veto="<<veto_work<<", storage="<<storage_work<<", stale="<<stale_work<<", publication="<<publication_budget.work);
  INFO(reason);REQUIRE(published);
  CHECK(setup_budget.work==setup_work);CHECK(split_budget.work==split_work);
  CHECK(veto_budget.work==veto_work);CHECK(storage_budget.work==storage_work);CHECK(stale_budget.work==stale_work);
  CHECK(veto_work<=veto_budget.maximum_work);CHECK(storage_work<=storage_budget.maximum_work);CHECK(stale_work<=stale_budget.maximum_work);
  if(atlas){CHECK(publication_budget.work>0);CHECK(publication_budget.work<=publication_budget.maximum_work);}
  CHECK(parameters->Revision()==12);CHECK(parameters->Records().size()==5);CHECK(parameters->Surface().size()==6);
  CHECK(CPassiveComm::Allreduce(uint64_t(owned.size()),CPassiveComm::Op::SUM,world.comm)==2);
  ReferenceState state;state.original=reference;state.facets=facets;state.marker_names={"body"};
  const auto encoded=EncodeReference(state);CHECK(encoded.find(atlas?"SU2_NATIVE_REFERENCE_3D 8\n":components?"SU2_NATIVE_REFERENCE_3D 6\n":"SU2_NATIVE_REFERENCE_3D 5\n")==0);
  CHECK(EncodeReference(DecodeReference(encoded))==encoded);
  CHECK_THROWS(DecodeReference(encoded+"trailing"));CHECK_THROWS(DecodeReference(encoded,1));
  std::map<Id,Id> numbering;for(auto row:parameters->Records())numbering.emplace(row.first,row.first+1000);
  auto rebound=parameters->Rebind(numbering);CHECK(rebound->Revision()==13);CHECK(&rebound->Geometry()==authority.get());
  const auto& bound=rebound->Records().at(1055).point.binding;const auto& expected=parameters->Records().at(55).point.binding;
  CHECK(bound.feature==expected.feature);CHECK(bound.u==expected.u);CHECK(bound.v==expected.v);CHECK(bound.version==expected.version);
  numbering.erase(numbering.begin());CHECK_THROWS(parameters->Rebind(numbering));
  if(atlas) {
    CHECK_FALSE(parameters->Coverage().faces.empty());CHECK_FALSE(parameters->Coverage().traces.empty());
    std::map<Id,Id> reversed,back;for(const auto& row:parameters->Records()){reversed.emplace(row.first,10000-row.first);back.emplace(10000-row.first,row.first);}
    auto reverse=parameters->Rebind(reversed);auto restored=reverse->Rebind(back);
    CHECK(EncodeBoundaryAtlas(restored->Coverage(),restored->Mobility())==EncodeBoundaryAtlas(parameters->Coverage(),parameters->Mobility()));
    // An existing edge cannot acquire another active-edge parameterization even when its knots remain unchanged.
    auto bad=fresh.coverage;REQUIRE_FALSE(bad.traces.empty());auto& trace=bad.traces.begin()->second;REQUIRE_FALSE(trace.pieces.empty());
    trace.pieces.back().end=ExactRatio{};const auto snapshot=parameters->Snapshot();
    CHECK_FALSE(parameters->Prepare({}, {},why,SIZE_MAX,0,bad,&bad_trace_budget).valid);CHECK(parameters->Snapshot()==snapshot);
    CHECK(bad_trace_budget.work>0);CHECK(bad_trace_budget.work<=bad_trace_budget.maximum_work);
    CHECK(publication_budget.work>0);CHECK(publication_budget.work<=publication_budget.maximum_work);
    CHECK_THROWS_AS(BoundaryParameterState(authority,points,old.surface,11,SIZE_MAX,0,descriptors),std::invalid_argument);
  }
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
    auto movedFace=actual;movedFace[0].x+=2*full;
    CHECK(geometry->SurfaceError(f.id,corners,movedFace)>full);
    auto stale=corners;++stale[0].version;
    CHECK_THROWS(geometry->SurfaceError(f.id,stale,actual));
    auto nonfinite=actual;nonfinite[0].x=NAN;
    CHECK_THROWS(geometry->SurfaceError(f.id,corners,nonfinite));
    CHECK(geometry->SurfaceError(f.id,corners,actual)==full);
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

namespace {
AtlasBudget GeometryAtlasBudget(){return {size_t(1)<<24,size_t(1)<<28,4096};}
ExactRatio GeometryRatio(double x,AtlasBudget& budget) {
  ExactRatio out;std::string reason;REQUIRE(ExactBinary64(x,out,budget,reason));return out;
}
struct CurvedAtlasFixture {
  std::vector<SU2Native3D::Facet> facets;
  std::shared_ptr<MappedFacetReference> geometry;
  explicit CurvedAtlasFixture(double height=0,double bulge=.03) {
    using namespace SU2Native3D;auto cube=ComponentCube(false);const Node middle{8,{.5,.5,height}};
    facets={{0,0,{cube[0].v[0],cube[0].v[1],middle}},
            {1,0,{cube[0].v[1],cube[0].v[2],middle}},
            {2,0,{cube[0].v[2],cube[1].v[2],middle}},
            {3,0,{cube[1].v[2],cube[0].v[0],middle}}};
    for(size_t i=2;i<cube.size();++i){auto f=cube[i];f.id+=2;facets.push_back(f);}
    MappedFacetReference::CurvedNets controls;std::map<Id,double> errors;
    const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:facets) {
      errors.emplace(f.id,1e-12);auto& net=controls[f.id];
      for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;for(unsigned k=0;k<3;++k){x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}net[q]={double(x/3),double(y/3),double(z/3)};}
      if(f.id<4)net[9].z-=bulge;
    }
    geometry=std::make_shared<MappedFacetReference>(std::make_shared<PlanarReference>(facets),errors,29,SIZE_MAX,false,controls);
  }
  FaceCoverage Cover(Id facet,AtlasBudget& budget)const {
    std::array<SurfaceBinding,3> binding;for(unsigned k=0;k<3;++k)binding[k]=geometry->Vertex(facets.at(facet).v[k].id);
    FaceCoverage cover;std::string reason;REQUIRE(geometry->ResolveSingleChart(facet,binding,cover,budget,reason));return cover;
  }
};
}
TEST_CASE("Curved coverage retains exact non-dyadic pieces and complete cubic error", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto budget=GeometryAtlasBudget();std::string reason;
  const auto whole=fixture.Cover(0,budget);FaceCoverage refined;
  ExactRatio third;REQUIRE(ExactDivide(GeometryRatio(1,budget),GeometryRatio(3,budget),third,budget,reason));
  const ExactBary center{third,third};const auto& original=whole.pieces.front();
  for(unsigned k=0;k<3;++k){CoveragePiece p;p.facet=0;p.reference=p.active={original.reference[k],original.reference[(k+1)%3],center};refined.pieces.push_back(p);}
  std::array<SurfaceBinding,3> binding;std::array<Point,3> realized;
  for(unsigned k=0;k<3;++k){const auto n=fixture.facets[0].v[k];binding[k]=g.Vertex(n.id);realized[k]=n.p;}
  std::array<EdgeTrace,3> traces;REQUIRE(g.CoverageTraces(refined,traces,budget,reason));
  INFO(reason);REQUIRE(g.ValidateCoverage(0,binding,realized,refined,traces,budget,reason));
  REQUIRE(g.ValidateCoverageReplacement({whole},{refined},budget,reason));
  const double error=g.SurfaceError(refined,realized,budget);CHECK(error>1e-3);CHECK(error<g.SurfaceError(whole,realized,budget));
  const auto exact_surface=g.Realize(g.FacetPoint(0,1./3,1./3));CHECK(std::abs(exact_surface.z)<=error);
  const auto envelope=g.PrepareSourcePatch(refined,realized,budget);CHECK(envelope.Bounds()[0].z<=exact_surface.z);
  CHECK(envelope.OutwardDisplacement()>=std::abs(exact_surface.z));CHECK(envelope.OutwardDisplacement()<.03);
  CHECK(MappedFacetReference::SourcePreparationBytes(refined)>=envelope.ResidentBytes());
  auto corrupt=refined;std::swap(corrupt.pieces[0].active[0],corrupt.pieces[0].active[1]);
  CHECK_FALSE(g.ValidateCoverage(0,binding,realized,corrupt,traces,budget,reason));
  auto limited=GeometryAtlasBudget();limited.maximum_bytes=1;CHECK_THROWS(g.PrepareSourcePatch(refined,realized,limited));
}
TEST_CASE("Shared curved edge paths compare exact parameterizations across subdivisions", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto budget=GeometryAtlasBudget();std::string reason;
  std::array<EdgeTrace,3> traces;REQUIRE(g.CoverageTraces(fixture.Cover(0,budget),traces,budget,reason));
  const auto& old=traces[0].pieces.front();auto first=old,last=old;ExactRatio du,dv,half=GeometryRatio(.5,budget);
  REQUIRE(ExactAdd(old.first.u,old.last.u,du,budget,reason));REQUIRE(ExactMultiply(du,half,du,budget,reason));
  REQUIRE(ExactAdd(old.first.v,old.last.v,dv,budget,reason));REQUIRE(ExactMultiply(dv,half,dv,budget,reason));
  first.last=last.first={du,dv};first.end=half;EdgeTrace divided;divided.pieces={first,last};
  REQUIRE(g.EqualTraces(traces[0],divided,budget,reason));
  bool equal=false;REQUIRE(g.CompareTraces(traces[0],divided,equal,budget,reason));CHECK(equal);CHECK(reason.empty());
  divided.pieces[0].end=GeometryRatio(.25,budget);equal=true;
  REQUIRE(g.CompareTraces(traces[0],divided,equal,budget,reason));CHECK_FALSE(equal);CHECK(reason.empty());
  CHECK_FALSE(g.EqualTraces(traces[0],divided,budget,reason));CHECK(reason=="Curved edge reference paths or active parameters differ.");
  auto malformed=divided;malformed.pieces.back().end=ExactRatio{};equal=true;
  CHECK_FALSE(g.CompareTraces(traces[0],malformed,equal,budget,reason));CHECK(equal);CHECK_FALSE(reason.empty());
  auto exhausted=GeometryAtlasBudget();exhausted.maximum_work=0;equal=true;
  CHECK_FALSE(g.CompareTraces(traces[0],traces[0],equal,exhausted,reason));CHECK(equal);CHECK_FALSE(reason.empty());
}
TEST_CASE("Coverage replacement cancels interior refinement and retains exact boundary multiplicity", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto construction=GeometryAtlasBudget();std::string reason;
  const auto whole=fixture.Cover(0,construction);auto refined=whole;const auto third=GeometryRatio(3,construction);
  auto center=[&](const std::array<ExactBary,3>& triangle) {
    ExactBary out;ExactRatio sum;
    REQUIRE(ExactAdd(triangle[0].u,triangle[1].u,sum,construction,reason));REQUIRE(ExactAdd(sum,triangle[2].u,sum,construction,reason));
    REQUIRE(ExactDivide(sum,third,out.u,construction,reason));
    REQUIRE(ExactAdd(triangle[0].v,triangle[1].v,sum,construction,reason));REQUIRE(ExactAdd(sum,triangle[2].v,sum,construction,reason));
    REQUIRE(ExactDivide(sum,third,out.v,construction,reason));return out;
  };
  for(unsigned level=0;level<4;++level) {
    FaceCoverage next;
    for(const auto& p:refined.pieces){const auto middle=center(p.reference);for(unsigned k=0;k<3;++k){CoveragePiece child;child.facet=p.facet;child.reference=child.active={p.reference[k],p.reference[(k+1)%3],middle};next.pieces.push_back(std::move(child));}}
    refined=std::move(next);
  }
  auto check=GeometryAtlasBudget();check.maximum_work=size_t(1)<<22;
  INFO(reason);REQUIRE(g.ValidateCoverageReplacement({whole},{refined},check,reason));
  auto inside=[&](const ExactBary& p) {int u,v,sum;ExactRatio total;
    REQUIRE(ExactCompare(p.u,GeometryRatio(0,construction),u,construction,reason));REQUIRE(ExactCompare(p.v,GeometryRatio(0,construction),v,construction,reason));
    REQUIRE(ExactAdd(p.u,p.v,total,construction,reason));REQUIRE(ExactCompare(total,GeometryRatio(1,construction),sum,construction,reason));return u>0&&v>0&&sum<0;};
  const auto interior=std::find_if(refined.pieces.begin(),refined.pieces.end(),[&](const auto& p){return inside(p.reference[0])&&inside(p.reference[1])&&inside(p.reference[2]);});
  REQUIRE(interior!=refined.pieces.end());auto hole=refined;hole.pieces.erase(hole.pieces.begin()+std::distance(refined.pieces.begin(),interior));
  auto hole_budget=GeometryAtlasBudget();CHECK_FALSE(g.ValidateCoverageReplacement({whole},{hole},hole_budget,reason));CHECK(reason.find("changes exact original-chart coverage")!=std::string::npos);
  auto duplicated=refined;duplicated.pieces.insert(duplicated.pieces.end(),refined.pieces.begin(),refined.pieces.end());
  auto duplicate_budget=GeometryAtlasBudget();CHECK_FALSE(g.ValidateCoverageReplacement({whole},{duplicated},duplicate_budget,reason));CHECK(reason.find("changes exact original-chart coverage")!=std::string::npos);
  const auto& r=whole.pieces.front().reference;const ExactBary middle{GeometryRatio(.5,construction),GeometryRatio(0,construction)};
  FaceCoverage boundary_split;for(const auto triangle:{std::array<ExactBary,3>{r[0],middle,r[2]},std::array<ExactBary,3>{middle,r[1],r[2]}}){CoveragePiece p;p.facet=0;p.reference=p.active=triangle;boundary_split.pieces.push_back(std::move(p));}
  auto split_budget=GeometryAtlasBudget();REQUIRE(g.ValidateCoverageReplacement({whole},{boundary_split},split_budget,reason));
  boundary_split.pieces[0].reference[1].v=boundary_split.pieces[1].reference[0].v=GeometryRatio(.125,construction);
  auto mismatch_budget=GeometryAtlasBudget();CHECK_FALSE(g.ValidateCoverageReplacement({whole},{boundary_split},mismatch_budget,reason));CHECK(reason.find("changes exact original-chart coverage")!=std::string::npos);
}
TEST_CASE("Reference region boundaries retain holes components and mandatory trace points", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto budget=GeometryAtlasBudget();std::string reason;
  auto point=[&](double u,double v){return ExactBary{GeometryRatio(u,budget),GeometryRatio(v,budget)};};
  auto piece=[&](const ExactBary& a,const ExactBary& b,const ExactBary& c){CoveragePiece p;p.facet=0;p.reference=p.active={a,b,c};return p;};
  const auto a=point(0,0),b=point(1,0),c=point(0,1),middle=point(.5,0);
  const std::vector<CoveragePiece> divided{piece(a,middle,c),piece(middle,b,c)};
  std::vector<RegionSegment> boundary;REQUIRE(g.RegionBoundary(divided,{},boundary,budget,reason));CHECK(boundary.size()==3);
  std::vector<uint64_t> middle_key;REQUIRE(g.ReferencePointKey(0,middle,middle_key,budget,reason));
  REQUIRE(g.RegionBoundary(divided,{middle_key},boundary,budget,reason));CHECK(boundary.size()==4);
  const auto seam=point(0,.5);std::vector<uint64_t> seam_key;REQUIRE(g.ReferencePointKey(0,seam,seam_key,budget,reason));
  ExactBary other;REQUIRE(g.ReferenceParameters(3,seam_key,other,budget,reason));std::vector<uint64_t> roundtrip;
  REQUIRE(g.ReferencePointKey(3,other,roundtrip,budget,reason));CHECK(roundtrip==seam_key);
  CHECK_FALSE(g.ReferenceParameters(1,middle_key,other,budget,reason));CHECK(reason.empty());
  const std::array<ExactBary,3> outer{a,b,c},inner{point(.125,.125),point(.625,.125),point(.125,.625)};
  std::vector<CoveragePiece> annulus;
  for(unsigned k=0;k<3;++k){const auto next=(k+1)%3;annulus.push_back(piece(outer[k],outer[next],inner[next]));annulus.push_back(piece(outer[k],inner[next],inner[k]));}
  REQUIRE(g.RegionBoundary(annulus,{},boundary,budget,reason));CHECK(boundary.size()==6);
  auto loop_signs=[&](const std::vector<RegionSegment>& segments) {
    std::map<std::vector<uint64_t>,std::pair<std::vector<uint64_t>,const RegionSegment*>> next;
    for(const auto& e:segments){std::vector<uint64_t> first,last;REQUIRE(g.ReferencePointKey(e.facet,e.first,first,budget,reason));REQUIRE(g.ReferencePointKey(e.facet,e.last,last,budget,reason));REQUIRE(next.emplace(first,std::make_pair(last,&e)).second);}
    std::vector<int> signs;std::set<std::vector<uint64_t>> visited;
    for(const auto& row:next)if(!visited.count(row.first)) {
      auto at=row.first;auto area=GeometryRatio(0,budget);
      do {REQUIRE(visited.insert(at).second);const auto& entry=next.at(at);const auto& e=*entry.second;ExactRatio left,right,cross;
        REQUIRE(ExactMultiply(e.first.u,e.last.v,left,budget,reason));REQUIRE(ExactMultiply(e.first.v,e.last.u,right,budget,reason));
        REQUIRE(ExactSubtract(left,right,cross,budget,reason));REQUIRE(ExactAdd(area,cross,area,budget,reason));at=entry.first;
      }while(at!=row.first);
      int sign;REQUIRE(ExactCompare(area,GeometryRatio(0,budget),sign,budget,reason));signs.push_back(sign);
    }
    std::sort(signs.begin(),signs.end());return signs;
  };
  CHECK(loop_signs(boundary)==std::vector<int>{-1,1});
  const std::vector<CoveragePiece> separated{piece(a,point(.25,0),point(0,.25)),piece(b,point(.75,.25),point(.75,0))};
  REQUIRE(g.RegionBoundary(separated,{},boundary,budget,reason));CHECK(loop_signs(boundary)==std::vector<int>{1,1});
  const auto saved=boundary.size();const auto whole=fixture.Cover(0,budget).pieces.front();
  CHECK_FALSE(g.RegionBoundary({whole,whole},{},boundary,budget,reason));CHECK(reason.find("nonunit")!=std::string::npos);CHECK(boundary.size()==saved);
}
TEST_CASE("Cubic disk embedding certifies its real surface and protects C0 topology", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto budget=GeometryAtlasBudget();std::string reason;
  std::vector<CoveragePiece> disk;for(Id f=0;f<4;++f)disk.push_back(fixture.Cover(f,budget).pieces.front());
  INFO(reason);REQUIRE(g.CertifyEmbedding(disk,{.1,.1,-1},budget,reason));
  auto reversed=GeometryAtlasBudget();CHECK_FALSE(g.CertifyEmbedding(disk,{0,0,1},reversed,reason));
  auto notched=disk;notched.erase(notched.begin()+1);auto notch_budget=GeometryAtlasBudget();
  REQUIRE(g.CertifyEmbedding(notched,{0,0,-1},notch_budget,reason));
  const std::vector<CoveragePiece> disconnected{disk[0],disk[2]};auto refusal=GeometryAtlasBudget();
  CHECK_FALSE(g.CertifyEmbedding(disconnected,{0,0,-1},refusal,reason));
  using namespace SU2Native3D;std::array<Node,10> n;
  const std::array<Point,4> ring{{{1,0,0},{0,1,0},{-1,0,0},{0,-1,0}}};
  for(unsigned k=0;k<8;++k)n[k]={k,ring[k%4]};n[8]={8,{0,0,0}};n[9]={9,{0,0,1}};
  std::vector<Facet> faces;std::map<Id,double> errors;
  for(unsigned k=0;k<8;++k){faces.push_back({Id(faces.size()),0,{n[8],n[(k+1)%8],n[k]}});faces.push_back({Id(faces.size()),0,{n[9],n[k],n[(k+1)%8]}});}
  for(const auto& f:faces)errors.emplace(f.id,1e-12);MappedFacetReference branched(std::make_shared<PlanarReference>(faces),errors,31);
  std::vector<CoveragePiece> twice;auto fan_budget=GeometryAtlasBudget();
  for(unsigned k=0;k<8;++k){const auto& f=faces[2*k];FaceCoverage c;const std::array<SurfaceBinding,3> b{{branched.Vertex(f.v[0].id),branched.Vertex(f.v[1].id),branched.Vertex(f.v[2].id)}};
    REQUIRE(branched.ResolveSingleChart(f.id,b,c,fan_budget,reason));twice.push_back(c.pieces.front());}
  CHECK_FALSE(branched.CertifyEmbedding(twice,{0,0,-1},fan_budget,reason));CHECK(reason.find("local degree")!=std::string::npos);
}
TEST_CASE("Geometry mobility releases cubic seams and retains concave features and carriers", "[NativeCurvedAtlas3D]") {
  const CurvedAtlasFixture fixture;auto budget=GeometryAtlasBudget();std::string reason;MobilityPolicy policy;
  REQUIRE(fixture.geometry->BuildMobilityPolicy(60,{{0,2}},policy,budget,reason));CHECK(policy.identity!=0);
  CHECK(fixture.geometry->MobilityDimension(fixture.geometry->Vertex(8),policy)==2);
  CHECK(fixture.geometry->MobilityDimension(fixture.geometry->EdgePoint({0,8},.5),policy)==2);
  CHECK(fixture.geometry->MobilityDimension(fixture.geometry->Vertex(0),policy)==0);
  const CurvedAtlasFixture concave(.9,0);auto concave_budget=GeometryAtlasBudget();MobilityPolicy concave_policy;
  REQUIRE(concave.geometry->BuildMobilityPolicy(60,{{0,2}},concave_policy,concave_budget,reason));
  CHECK(concave_policy.features.count({0,8})==1);CHECK(concave.geometry->MobilityDimension(concave.geometry->Vertex(8),concave_policy)==0);
  const CurvedAtlasFixture flat(0,0);auto facets=flat.facets;facets[0].marker=1;auto errors=flat.geometry->Errors();
  const std::map<Id,AffineAuthority> planes{{0,{77,1,3,{0,0,0},{0,0,-1},1e-12}}};
  MappedFacetReference carrier(std::make_shared<SU2Native3D::PlanarReference>(facets,SIZE_MAX,std::map<Id,SU2Native3D::AnalyticSurface>{},planes),errors,29,SIZE_MAX,false,flat.geometry->Curves());
  auto carrier_budget=GeometryAtlasBudget();MobilityPolicy protected_policy;
  REQUIRE(carrier.BuildMobilityPolicy(60,{{0,2},{1,0}},protected_policy,carrier_budget,reason));
  CHECK(protected_policy.features.count({0,8})==1);CHECK(carrier.MobilityDimension(carrier.EdgePoint({0,8},.5),protected_policy)==1);
  CHECK(std::abs(carrier.Realize(carrier.EdgePoint({0,8},.37)).z)<=1e-12);
  CHECK(protected_policy.identity!=policy.identity);CHECK(protected_policy.roles.at(1)==0);
}

TEST_CASE("Coordinated retained-face covers reparameterize one shared trace atomically", "[NativeMappedPublication3D][NativeAtlasCoordinatedPublication3D]") {
  using namespace SU2Native3D;World world;const auto original=MappedTetrahedron();std::vector<Facet> facets;
  for(auto row:original->Facets()){row.second.marker=0;facets.push_back(row.second);}
  auto definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;
  for(const auto& f:facets)errors.emplace(f.id,1e-13);
  auto geometry=std::make_shared<MappedFacetReference>(definition,errors,7);BoundaryPoints points;std::vector<SurfaceFace> surface;
  std::map<Id,Node> nodes;
  for(const auto& facet:facets){surface.push_back({{facet.v[0].id,facet.v[1].id,facet.v[2].id},facet.id});
    for(auto n:facet.v){nodes.emplace(n.id,n);points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});}}
  auto state=std::make_shared<BoundaryParameterState>(geometry,points,surface);auto construction=GeometryAtlasBudget();std::string reason;
  const Edge shared{1,9};BoundaryCoverage delta,compact;const ExactBary center{GeometryRatio(.25,construction),GeometryRatio(.25,construction)};
  auto mix=[&](const ExactBary& a,const ExactBary& b,double t){ExactBary result;
    const auto left=GeometryRatio(1-t,construction),right=GeometryRatio(t,construction);
    auto coordinate=[&](const ExactRatio& x,const ExactRatio& y){ExactRatio first,last,out;
      REQUIRE(ExactMultiply(x,left,first,construction,reason));REQUIRE(ExactMultiply(y,right,last,construction,reason));
      REQUIRE(ExactAdd(first,last,out,construction,reason));return out;};
    result.u=coordinate(a.u,b.u);result.v=coordinate(a.v,b.v);return result;
  };
  Face changed_face{};FaceCoverage unchanged_cover;
  for(const auto& face:surface)if(std::find(face.v.begin(),face.v.end(),shared[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),shared[1])!=face.v.end()) {
    std::array<SurfaceBinding,3> bindings;for(unsigned k=0;k<3;++k)bindings[k]=points.at(face.v[k]).binding;
    FaceCoverage identity;REQUIRE(geometry->ResolveSingleChart(face.facet,bindings,identity,construction,reason));REQUIRE(identity.pieces.size()==1);compact.faces.emplace(face.v,identity);
    const auto& original_piece=identity.pieces.front();FaceCoverage cover;
    for(unsigned k=0;k<3;++k) {
      const unsigned next=(k+1)%3;Edge edge{face.v[k],face.v[next]},key=edge;std::sort(key.begin(),key.end());
      auto append=[&](ExactBary r0,ExactBary r1,ExactBary a0,ExactBary a1){
        CoveragePiece piece;piece.facet=face.facet;piece.reference={center,r0,r1};piece.active={center,a0,a1};cover.pieces.push_back(std::move(piece));};
      if(key==shared) {
        const auto reference_middle=mix(original_piece.reference[k],original_piece.reference[next],.5);
        const auto active_middle=mix(original_piece.active[k],original_piece.active[next],edge[0]<edge[1]?.25:.75);
        append(original_piece.reference[k],reference_middle,original_piece.active[k],active_middle);
        append(reference_middle,original_piece.reference[next],active_middle,original_piece.active[next]);
      } else append(original_piece.reference[k],original_piece.reference[next],original_piece.active[k],original_piece.active[next]);
    }
    std::array<EdgeTrace,3> traces;REQUIRE(geometry->CoverageTraces(cover,traces,construction,reason));
    for(unsigned k=0;k<3;++k)if(face.v[k]==shared[0]&&face.v[(k+1)%3]==shared[1])delta.traces.emplace(shared,traces[k]);
    auto key=face.v;std::rotate(key.begin(),std::min_element(key.begin(),key.end()),key.end());REQUIRE(key==face.v);
    delta.faces.emplace(key,cover);changed_face=key;unchanged_cover=identity;
  }
  REQUIRE(delta.faces.size()==2);REQUIRE(delta.traces.count(shared));
  auto facade=PlanarReference::WithParameters(definition,state);Patch sparse;sparse.surface=surface;
  CHECK_FALSE(facade->HasExplicitCoverage(sparse));
  auto supplied=sparse;supplied.coverage=delta;CHECK(facade->HasExplicitCoverage(supplied));
  auto overlay_work=GeometryAtlasBudget();auto overlay=facade->PrivateParameters(supplied,&overlay_work);
  CHECK(overlay->HasExplicitCoverage(sparse));CHECK_FALSE(facade->HasExplicitCoverage(sparse));
  EdgeTrace inherited;auto trace_work=GeometryAtlasBudget();REQUIRE(state->ResolveTrace(shared,*state->IncidentFace(shared),inherited,trace_work,reason));
  CHECK_FALSE(geometry->EqualTraces(inherited,delta.traces.at(shared),trace_work,reason));
  const auto trace_snapshot=state->Snapshot();BoundaryCoverage trace_only;trace_only.traces.emplace(shared,inherited);
  auto same_work=GeometryAtlasBudget();const auto same=state->Prepare({}, {},reason,SIZE_MAX,0,trace_only,&same_work);INFO(reason);CHECK(same.valid);
  CHECK(state->Snapshot()==trace_snapshot);trace_only.traces.at(shared)=delta.traces.at(shared);
  auto changed_work=GeometryAtlasBudget();const auto changed=state->Prepare({}, {},reason,SIZE_MAX,0,trace_only,&changed_work);INFO(reason);CHECK_FALSE(changed.valid);
  CHECK(state->Snapshot()==trace_snapshot);
  Owned owned;OwnedCell row;row.cell={0,{nodes.at(1),nodes.at(9),nodes.at(21),nodes.at(40)}};
  for(unsigned k=0;k<4;++k)row.physical[k]={true,100+k};if(!world.rank)owned.emplace(0,row);
  Directory directory(world,true,state.get());REQUIRE(directory.Build(owned,reason));const auto initial=state->Snapshot(),directory_before=directory.Snapshot();
  auto owner_work=GeometryAtlasBudget();
  CHECK_FALSE(directory.Publish(owned,{}, {},true,reason,SIZE_MAX,0,{},delta,&owner_work));
  CHECK(reason.find("every owning volume cell")!=std::string::npos);
  CHECK(state->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  std::vector<Dependency> removed;std::vector<OwnedCell> added;if(!world.rank){removed.push_back({0,0,0});row.version=1;added.push_back(row);}
  auto partial=delta;partial.faces.erase(changed_face);auto partial_work=GeometryAtlasBudget();
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},partial,&partial_work));
  CHECK(reason.find("every accepted incident face")!=std::string::npos);CHECK(state->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  auto inconsistent=delta;inconsistent.faces.at(changed_face)=unchanged_cover;auto inconsistent_work=GeometryAtlasBudget();
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},inconsistent,&inconsistent_work));
  CHECK(state->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  auto memory_work=GeometryAtlasBudget();CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1,0,{},delta,&memory_work));
  CHECK(state->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);
  auto no_work=GeometryAtlasBudget();no_work.maximum_work=0;
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},delta,&no_work));
  CHECK(state->Snapshot()==initial);CHECK(directory.Snapshot()==directory_before);CHECK(no_work.work==0);
  auto publication=GeometryAtlasBudget();const bool accepted=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},delta,&publication);INFO(reason);REQUIRE(accepted);
  CHECK(state->Revision()==2);CHECK(state->Surface().size()==surface.size());CHECK(state->Records().size()==points.size());
  CHECK(facade->HasExplicitCoverage(sparse));
  for(const auto& point:state->Records()){CHECK(point.second.faces==3);CHECK(point.second.point.binding.feature==points.at(point.first).binding.feature);
    CHECK(point.second.point.node.p.x==points.at(point.first).node.p.x);CHECK(point.second.point.node.p.y==points.at(point.first).node.p.y);CHECK(point.second.point.node.p.z==points.at(point.first).node.p.z);}
  CHECK(state->Coverage().faces.size()==2);CHECK(state->Snapshot()!=initial);CHECK(publication.work>0);CHECK(publication.work<=publication.maximum_work);
  if(!world.rank){REQUIRE(owned.size()==1);CHECK(owned.at(0).version==1);}else CHECK(owned.empty());
  auto final_work=GeometryAtlasBudget();EdgeTrace published;REQUIRE(state->ResolveTrace(shared,*state->IncidentFace(shared),published,final_work,reason));
  CHECK(geometry->EqualTraces(published,delta.traces.at(shared),final_work,reason));
  // A later coordinated refresh discards inherited subdivision history without changing CFD identities.
  const auto expanded_bytes=state->PayloadBytes();compact.traces.emplace(shared,inherited);
  if(!world.rank){removed.front().version=1;added.front().version=2;}
  auto refresh_work=GeometryAtlasBudget();const bool refreshed=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},compact,&refresh_work);INFO(reason);REQUIRE(refreshed);
  CHECK(state->Revision()==3);CHECK(state->PayloadBytes()<expanded_bytes);CHECK(state->Coverage().traces.at(shared).pieces.size()==1);
  for(const auto& face:compact.faces){REQUIRE(state->Coverage().faces.count(face.first));CHECK(state->Coverage().faces.at(face.first).pieces.size()==1);}
  for(const auto& point:state->Records())CHECK(point.second.faces==3);
  if(!world.rank)CHECK(owned.at(0).version==2);else CHECK(owned.empty());
}

TEST_CASE("A nonseparating protected crease cannot be buried inside one active face", "[NativeCurvedAtlas3D][NativeAtlasFeatureCoverage3D]") {
  using namespace SU2Native3D;const auto original=MappedTetrahedron();std::vector<Facet> facets;std::map<Id,Node> nodes;
  for(auto row:original->Facets()){row.second.marker=0;facets.push_back(row.second);for(auto n:row.second.v)nodes.emplace(n.id,n);}
  auto definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;for(const auto& f:facets)errors.emplace(f.id,1e-13);
  auto geometry=std::make_shared<MappedFacetReference>(definition,errors,7);BoundaryPoints points;std::vector<SurfaceFace> surface;
  for(const auto& f:facets){surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});}
  auto legacy=std::make_shared<BoundaryParameterState>(geometry,points,surface);auto policy_work=GeometryAtlasBudget();
  auto state=legacy->WithMobilityPolicy(125,{{0,2}},policy_work);const auto& policy=state->Mobility();
  REQUIRE(policy.features.count(Edge{21,40}));REQUIRE(policy.features.size()==1);
  // The crease does not disconnect the global facet graph: the other two tetrahedron faces reconnect both sides.
  REQUIRE(policy.sheets.at(100)==policy.sheets.at(101));
  auto reference=PlanarReference::WithParameters(definition,state);auto construction=GeometryAtlasBudget();std::string reason;
  const auto zero=GeometryRatio(0,construction),one=GeometryRatio(1,construction),half=GeometryRatio(.5,construction);
  const std::map<Id,ExactBary> active{{1,{zero,zero}},{9,{one,zero}},{21,{zero,one}},{40,{half,zero}}};
  FaceCoverage buried;
  for(Id id:{Id(100),Id(101)}) {
    const auto& facet=definition->Get(id);std::array<SurfaceBinding,3> binding;
    for(unsigned k=0;k<3;++k)binding[k]=geometry->Vertex(facet.v[k].id);
    FaceCoverage identity;REQUIRE(geometry->ResolveSingleChart(id,binding,identity,construction,reason));REQUIRE(identity.pieces.size()==1);
    auto piece=identity.pieces.front();for(unsigned k=0;k<3;++k)piece.active[k]=active.at(facet.v[k].id);buried.pieces.push_back(std::move(piece));
  }
  const Face face{1,9,21};std::array<SurfaceBinding,3> binding;std::array<Point,3> realized;
  for(unsigned k=0;k<3;++k){binding[k]=geometry->Vertex(face[k]);realized[k]=nodes.at(face[k]).p;}
  std::array<EdgeTrace,3> oriented;REQUIRE(geometry->CoverageTraces(buried,oriented,construction,reason));
  // Tiling, forward endpoints, shared paths and projected chart positivity alone do not protect a nonseparating crease.
  const bool geometric=geometry->ValidateCoverage(100,binding,realized,buried,oriented,construction,reason);INFO(reason);REQUIRE(geometric);
  Patch candidate;candidate.surface.push_back({face,100});candidate.coverage.faces.emplace(face,buried);
  for(unsigned k=0;k<3;++k){candidate.parameters.emplace(face[k],points.at(face[k]));Edge edge{face[k],face[(k+1)%3]};auto trace=oriented[k];
    if(edge[0]>edge[1]){std::swap(edge[0],edge[1]);REQUIRE(trace.pieces.size()==1);std::swap(trace.pieces.front().first,trace.pieces.front().last);}
    candidate.coverage.traces.emplace(edge,std::move(trace));}
  const auto saved=state->Snapshot();auto query_work=GeometryAtlasBudget();auto query=reference->PrivateParameters(candidate,&query_work);
  CHECK_FALSE(query->ContainsFace(100,realized));
  CHECK_THROWS_WITH(query->SurfaceError(100,realized),"Mapped surface leaves its parameter authority.");
  CHECK(state->Snapshot()==saved);
  auto conforming_work=GeometryAtlasBudget();auto conforming=reference->WithAtlasReservation(conforming_work);const auto& accepted=definition->Get(100);
  const std::array<Point,3> triangle{accepted.v[0].p,accepted.v[1].p,accepted.v[2].p};
  CHECK(conforming->ContainsFace(100,triangle));CHECK(std::isfinite(conforming->SurfaceError(100,triangle)));
  CHECK(state->Snapshot()==saved);
}

TEST_CASE("Single-chart curved certificates agree with a subdivided map and retain refusals", "[NativeCurvedAtlas3D][NativeSingleChartCertificate3D]") {
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto setup=GeometryAtlasBudget();std::string reason;
  const auto one=GeometryRatio(1,setup),zero=GeometryRatio(0,setup);ExactRatio third;
  REQUIRE(ExactDivide(one,GeometryRatio(3,setup),third,setup,reason));
  const auto& facet=fixture.facets[0];
  const std::array<std::array<SurfaceBinding,3>,3> cases{{
    {{g.Vertex(facet.v[0].id),g.Vertex(facet.v[1].id),g.Vertex(facet.v[2].id)}},
    {{g.FacetPoint(0,.125,.125),g.FacetPoint(0,.75,.125),g.FacetPoint(0,.125,.75)}},
    {{g.FacetPoint(0,.1,.25),g.FacetPoint(0,.6,.25),g.FacetPoint(0,.35,.25000001)}}
  }};
  for(size_t sample=0;sample<cases.size();++sample) {
    const auto& binding=cases[sample];std::array<Point,3> actual;
    for(unsigned k=0;k<3;++k)actual[k]=g.Realize(binding[k]);
    FaceCoverage single;REQUIRE(g.ResolveSingleChart(0,binding,single,setup,reason));
    const auto& source=single.pieces.front();ExactBary center{zero,zero};
    for(const auto& p:source.reference){ExactRatio value;
      REQUIRE(ExactAdd(center.u,p.u,value,setup,reason));center.u=std::move(value);
      REQUIRE(ExactAdd(center.v,p.v,value,setup,reason));center.v=std::move(value);}
    REQUIRE(ExactMultiply(center.u,third,center.u,setup,reason));REQUIRE(ExactMultiply(center.v,third,center.v,setup,reason));
    FaceCoverage split;for(unsigned k=0;k<3;++k){CoveragePiece p;p.facet=0;
      p.reference={source.reference[k],source.reference[(k+1)%3],center};
      p.active={source.active[k],source.active[(k+1)%3],ExactBary{third,third}};split.pieces.push_back(std::move(p));}
    std::array<EdgeTrace,3> traces,divided;REQUIRE(g.CoverageTraces(single,traces,setup,reason));REQUIRE(g.CoverageTraces(split,divided,setup,reason));
    for(unsigned k=0;k<3;++k)REQUIRE(g.EqualTraces(traces[k],divided[k],setup,reason));
    auto fast=GeometryAtlasBudget(),generic=GeometryAtlasBudget();
    const bool single_ok=g.ValidateCoverage(0,binding,actual,single,traces,fast,reason);
    if(sample==2)CHECK(reason.find("projected Jacobian")!=std::string::npos);
    const bool split_ok=g.ValidateCoverage(0,binding,actual,split,divided,generic,reason);
    INFO(sample);INFO(reason);CHECK(single_ok==split_ok);CHECK(single_ok==(sample<2));
    if(sample==2)continue;
    auto moved=actual;moved[0].z+=.001;auto denied=GeometryAtlasBudget();
    CHECK_FALSE(g.ValidateCoverage(0,binding,moved,single,traces,denied,reason));
    auto malformed=traces;malformed[0].pieces.front().end=GeometryRatio(.5,setup);auto interval=GeometryAtlasBudget();
    CHECK_FALSE(g.ValidateCoverage(0,binding,actual,single,malformed,interval,reason));
    auto missing=single;missing.pieces.front().active[1].u=GeometryRatio(.5,setup);auto hole=GeometryAtlasBudget();
    CHECK_FALSE(g.ValidateCoverage(0,binding,actual,missing,traces,hole,reason));
    // Equal endpoint keys cannot excuse a bent multi-piece route through the facet interior.
    auto bent=traces;auto first=bent[0].pieces.front(),last=first;
    first.last=last.first=center;first.end=GeometryRatio(.5,setup);bent[0].pieces={first,last};auto path=GeometryAtlasBudget();
    CHECK_FALSE(g.ValidateCoverage(0,binding,actual,single,bent,path,reason));
  }
}

TEST_CASE("Curved geometry error probes use projected associations without publishing mesh IDs", "[NativeCurvedAtlas3D][NativeSingleChartCertificate3D]") {
  using namespace SU2Native3D;
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;std::string reason;auto budget=GeometryAtlasBudget();
  BoundaryPoints points;std::vector<SurfaceFace> skin;std::map<int,int> roles;
  for(const auto& f:fixture.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});roles.emplace(f.marker,2);
    for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,g.Vertex(n.id)});}
  auto accepted=std::make_shared<BoundaryParameterState>(fixture.geometry,points,skin);
  accepted=accepted->WithMobilityPolicy(60,roles,budget);
  const std::shared_ptr<const PlanarReference> definition(fixture.geometry,&g.Reference());
  auto reference=PlanarReference::WithParameters(definition,accepted);
  const auto snapshot=accepted->Snapshot();auto probe=reference->PrivateParameters(Patch{},&budget);REQUIRE(probe);const auto& face=fixture.facets[0];
  const Point flat{.5*face.v[0].p.x+.25*face.v[1].p.x+.25*face.v[2].p.x,
                   .5*face.v[0].p.y+.25*face.v[1].p.y+.25*face.v[2].p.y,
                   .5*face.v[0].p.z+.25*face.v[1].p.z+.25*face.v[2].p.z};
  const auto projected=probe->Project(0,flat);const auto binding=g.FacetPoint(0,.25,.25);
  REQUIRE(g.Admissible(binding,projected));CHECK(projected.z!=flat.z);
  const std::array<SurfaceBinding,3> bindings{{g.Vertex(face.v[0].id),g.Vertex(face.v[1].id),binding}};
  const std::array<Point,3> actual{{face.v[0].p,face.v[1].p,projected}};FaceCoverage cover;
  REQUIRE(g.ResolveSingleChart(0,bindings,cover,budget,reason));const double expected=g.SurfaceError(cover,actual,budget);
  CHECK(probe->SurfaceError(0,actual)==Approx(expected).margin(1e-12));CHECK(std::isfinite(expected));
  CHECK(probe->SurfaceErrorForValidatedFace(0,actual)==probe->SurfaceError(0,actual));
  for(unsigned k=0;k<3;++k) {
    const unsigned j=(k+1)%3,other=(k+2)%3;const auto a=face.v[k],b=face.v[j],c=face.v[other];
    Edge edge{a.id,b.id};std::sort(edge.begin(),edge.end());const auto midpoint=g.EdgePoint(edge,.5);
    const Point input{(a.p.x+b.p.x)/2,(a.p.y+b.p.y)/2,(a.p.z+b.p.z)/2};
    const auto point=probe->Project(0,input);INFO(k);CHECK(g.Admissible(midpoint,point));
    CHECK(g.MobilityDimension(midpoint,accepted->Mobility())==(accepted->Mobility().features.count(edge)?1:2));
    for(unsigned side=0;side<2;++side) {
      const std::array<SurfaceBinding,3> child=side==0?std::array<SurfaceBinding,3>{{g.Vertex(a.id),midpoint,g.Vertex(c.id)}}:
          std::array<SurfaceBinding,3>{{midpoint,g.Vertex(b.id),g.Vertex(c.id)}};
      const std::array<Point,3> triangle=side==0?std::array<Point,3>{{a.p,point,c.p}}:std::array<Point,3>{{point,b.p,c.p}};
      FaceCoverage explicit_cover;const bool resolved=g.ResolveSingleChart(0,child,explicit_cover,budget,reason);INFO(reason);REQUIRE(resolved);
      const auto explicit_error=g.SurfaceError(explicit_cover,triangle,budget);CHECK(probe->SurfaceError(0,triangle)==Approx(explicit_error).margin(1e-12));
      CHECK(probe->SurfaceErrorForValidatedFace(0,triangle)==probe->SurfaceError(0,triangle));
    }
  }
  CHECK(accepted->Snapshot()==snapshot);CHECK(accepted->Records().size()==points.size());
}

TEST_CASE("Accepted face certificates require complete unchanged contracts", "[NativeCurvedAtlas3D][NativeAcceptedFaceCertificate3D]") {
  using namespace SU2Native3D;
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;auto budget=GeometryAtlasBudget();std::string reason;
  BoundaryPoints points;std::vector<SurfaceFace> skin;std::map<int,int> roles;
  for(const auto& f:fixture.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});roles.emplace(f.marker,2);
    for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,g.Vertex(n.id)});}
  auto state=std::make_shared<BoundaryParameterState>(fixture.geometry,points,skin);
  state=state->WithMobilityPolicy(60,roles,budget);
  const std::shared_ptr<const PlanarReference> definition(fixture.geometry,&g.Reference());
  auto reference=PlanarReference::WithParameters(definition,state);const auto saved=state->Snapshot();
  auto face=skin.front();std::array<BoundaryPoint,3> nodes;
  for(unsigned k=0;k<3;++k)nodes[k]=points.at(face.v[k]);BoundaryCoverage empty;
  REQUIRE(reference->HasAcceptedFaceCertificate(face,nodes,empty));
  auto rotated=face;std::rotate(rotated.v.begin(),rotated.v.begin()+1,rotated.v.end());
  auto rotated_nodes=nodes;std::rotate(rotated_nodes.begin(),rotated_nodes.begin()+1,rotated_nodes.end());
  CHECK(reference->HasAcceptedFaceCertificate(rotated,rotated_nodes,empty));
  auto reversed=face;std::swap(reversed.v[1],reversed.v[2]);auto reversed_nodes=nodes;std::swap(reversed_nodes[1],reversed_nodes[2]);
  CHECK_FALSE(reference->HasAcceptedFaceCertificate(reversed,reversed_nodes,empty));
  auto anchor=face;anchor.facet=fixture.facets[1].id;CHECK_FALSE(reference->HasAcceptedFaceCertificate(anchor,nodes,empty));
  auto changed=nodes;changed[0].node.p.x=std::nextafter(changed[0].node.p.x,INFINITY);
  CHECK_FALSE(reference->HasAcceptedFaceCertificate(face,changed,empty));
  changed=nodes;changed[0].binding.version++;CHECK_FALSE(reference->HasAcceptedFaceCertificate(face,changed,empty));
  BoundaryCoverage supplied;supplied.faces.emplace(face.v,fixture.Cover(face.facet,budget));
  CHECK_FALSE(reference->HasAcceptedFaceCertificate(face,nodes,supplied));
  supplied.faces.clear();std::array<EdgeTrace,3> traces;REQUIRE(g.CoverageTraces(fixture.Cover(face.facet,budget),traces,budget,reason));
  Edge edge{face.v[0],face.v[1]};std::sort(edge.begin(),edge.end());supplied.traces.emplace(edge,traces[0]);
  CHECK_FALSE(reference->HasAcceptedFaceCertificate(face,nodes,supplied));
  Patch unrelated;const auto association=g.FacetPoint(fixture.facets.back().id,.2,.2);
  unrelated.parameters.emplace(999,BoundaryPoint{{999,g.Realize(association)},association});
  auto overlay=reference->PrivateParameters(unrelated,&budget);REQUIRE(overlay);
  CHECK(overlay->HasAcceptedFaceCertificate(face,nodes,empty));CHECK(state->Snapshot()==saved);
  auto no_work=GeometryAtlasBudget();no_work.maximum_work=0;auto certified=reference->PrivateParameters(unrelated,&no_work);
  const std::array<Point,3> actual{{nodes[0].node.p,nodes[1].node.p,nodes[2].node.p}};
  CHECK(certified->ContainsFace(face.facet,actual));CHECK(no_work.work==0);
  BoundaryCoverage explicit_data;auto canonical_face=face.v;std::rotate(canonical_face.begin(),std::min_element(canonical_face.begin(),canonical_face.end()),canonical_face.end());
  REQUIRE(canonical_face==face.v);explicit_data.faces.emplace(canonical_face,fixture.Cover(face.facet,budget));
  auto edge_trace=traces[0];if(face.v[0]>face.v[1]){REQUIRE(edge_trace.pieces.size()==1);std::swap(edge_trace.pieces.front().first,edge_trace.pieces.front().last);}
  explicit_data.traces.emplace(edge,edge_trace);
  auto explicit_state=std::make_shared<BoundaryParameterState>(fixture.geometry,points,skin,1,SIZE_MAX,0,explicit_data,state->Mobility(),&budget);
  auto explicit_reference=PlanarReference::WithParameters(definition,explicit_state);Patch copied;copied.coverage=explicit_data;
  auto error_budget=GeometryAtlasBudget();auto error_view=explicit_reference->PrivateParameters(Patch{},&error_budget);
  CHECK(error_view->SurfaceErrorForValidatedFace(face.facet,actual)==error_view->SurfaceError(face.facet,actual));
  auto copy_budget=GeometryAtlasBudget();copy_budget.maximum_work=0;auto copied_view=explicit_reference->PrivateParameters(copied,&copy_budget);
  CHECK(copied_view->HasAcceptedFaceCertificate(face,nodes,empty));CHECK(copied_view->ContainsFace(face.facet,actual));CHECK(copy_budget.work==0);
  copied.coverage.traces.at(edge).pieces.back().end=ExactRatio{};auto changed_budget=GeometryAtlasBudget();
  auto changed_view=explicit_reference->PrivateParameters(copied,&changed_budget);
  CHECK_FALSE(changed_view->HasAcceptedFaceCertificate(face,nodes,empty));CHECK_FALSE(changed_view->ContainsFace(face.facet,actual));
  auto entity_state=state->WithEntityGeometry(1e-3,budget);
  auto entity_reference=reference->WithParameterState(entity_state);
  REQUIRE(entity_state->EntityGeometry());
  CHECK(entity_reference->HasAcceptedFaceCertificate(face,nodes,empty));
  CHECK(entity_reference->ContainsFace(face.facet,actual));
  CHECK(entity_reference->HasAcceptedFaceCertificate(rotated,rotated_nodes,empty));
  CHECK_FALSE(entity_reference->HasAcceptedFaceCertificate(reversed,reversed_nodes,empty));
  CHECK_FALSE(entity_reference->HasAcceptedFaceCertificate(anchor,nodes,empty));
  CHECK_FALSE(entity_reference->HasAcceptedFaceCertificate(face,changed,empty));
  auto changed_point=nodes;changed_point[0].node.p.x=std::nextafter(changed_point[0].node.p.x,INFINITY);
  CHECK_FALSE(entity_reference->HasAcceptedFaceCertificate(face,changed_point,empty));
  BoundaryCoverage override_data;override_data.faces.emplace(face.v,fixture.Cover(face.facet,budget));
  CHECK_FALSE(entity_reference->HasAcceptedFaceCertificate(face,nodes,override_data));
  Patch entity_override;entity_override.coverage=override_data;
  auto entity_view=entity_reference->PrivateParameters(entity_override,&budget);
  CHECK_FALSE(entity_view->HasAcceptedFaceCertificate(face,nodes,empty));
  // A current maximum-valued mesh ID is not an unpublished projected association.
  std::map<Id,Id> numbering;for(const auto& row:state->Records())numbering.emplace(row.first,row.first);
  numbering.at(face.v[0])=UINT64_MAX;auto rebound=state->Rebind(numbering);
  auto renamed=face;renamed.v[0]=UINT64_MAX;const Edge max_edge{std::min(renamed.v[0],renamed.v[1]),std::max(renamed.v[0],renamed.v[1])};
  auto renamed_reference=reference->WithParameterState(rebound);auto max_work=GeometryAtlasBudget();EdgeTrace malformed;
  REQUIRE(rebound->ResolveTrace(max_edge,renamed,malformed,max_work,reason));REQUIRE_FALSE(malformed.pieces.empty());
  malformed.pieces.back().end=ExactRatio{};Patch overridden;overridden.coverage.traces.emplace(max_edge,malformed);
  auto checked=renamed_reference->PrivateParameters(overridden,&max_work);const auto rebound_snapshot=rebound->Snapshot();
  CHECK_FALSE(checked->ContainsFace(face.facet,actual));CHECK_THROWS(checked->SurfaceError(face.facet,actual));
  CHECK(rebound->Snapshot()==rebound_snapshot);
}

TEST_CASE("Curved mobility activation certifies legacy subfaces before certificate reuse", "[NativeCurvedAtlas3D][NativeAcceptedFaceCertificate3D]") {
  using namespace SU2Native3D;
  const CurvedAtlasFixture fixture;const auto& g=*fixture.geometry;const auto& f=fixture.facets[0];
  BoundaryPoints points;std::vector<SurfaceFace> skin;std::map<int,int> roles;
  for(const auto& face:fixture.facets){roles.emplace(face.marker,2);if(face.id!=f.id)skin.push_back({{face.v[0].id,face.v[1].id,face.v[2].id},face.id});
    for(auto n:face.v)points.emplace(n.id,BoundaryPoint{n,g.Vertex(n.id)});}
  const std::array<SurfaceBinding,3> inserted{{g.FacetPoint(f.id,.1,.25),g.FacetPoint(f.id,.6,.25),g.FacetPoint(f.id,.35,.25000001)}};
  for(unsigned k=0;k<3;++k)points.emplace(100+k,BoundaryPoint{{100+k,g.Realize(inserted[k])},inserted[k]});
  const Id a=f.v[0].id,b=f.v[1].id,c=f.v[2].id;
  for(Face face:std::array<Face,7>{{{a,b,100},{c,a,100},{b,c,101},{100,b,101},{c,100,102},{100,101,102},{101,c,102}}})skin.push_back({face,f.id});
  auto legacy=std::make_shared<BoundaryParameterState>(fixture.geometry,points,skin);
  const auto saved=legacy->Snapshot();auto budget=GeometryAtlasBudget();
  CHECK_THROWS_WITH(legacy->WithMobilityPolicy(60,roles,budget),Catch::Matchers::Contains("projected Jacobian"));
  CHECK(legacy->Snapshot()==saved);
}
