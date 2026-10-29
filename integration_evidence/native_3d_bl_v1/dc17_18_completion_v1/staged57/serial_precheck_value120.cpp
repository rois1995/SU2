#include "../../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
using namespace SU2Native3D;using namespace SU2NativeGeometry;
AtlasBudget RemappingBudget(){return {size_t(1)<<29,size_t(1)<<28,4096};}
struct RemappingFixture {
  std::vector<Facet> facets;
  std::shared_ptr<PlanarReference> definition;
  std::shared_ptr<MappedFacetReference> geometry;
  std::shared_ptr<BoundaryParameterState> state;
  std::shared_ptr<const PlanarReference> reference;
  BoundaryPoints points;
  std::vector<SurfaceFace> surface;
  Patch patch;
  MobilityPolicy policy;
  explicit RemappingFixture(bool crease) {
    if(crease){std::array<Node,8> c;for(unsigned i=0;i<8;++i)c[i]={10+10*i,{double(i&1),double((i>>1)&1),double((i>>2)&1)}};
      const Node p{0,{.5,0,0}};
      facets={{0,0,{c[0],c[2],p}},{1,0,{p,c[2],c[3]}},{2,0,{p,c[3],c[1]}},
              {3,0,{c[0],p,c[4]}},{4,0,{p,c[1],c[5]}},{5,0,{p,c[5],c[4]}}};
      const unsigned q[4][4]={{4,5,7,6},{2,6,7,3},{0,4,6,2},{1,3,7,5}};
      for(const auto& f:q){facets.push_back({Id(facets.size()),0,{c[f[0]],c[f[1]],c[f[2]]}});facets.push_back({Id(facets.size()),0,{c[f[0]],c[f[2]],c[f[3]]}});}
    }else{const Node a{10,{0,0,0}},b{20,{1,0,0}},c{30,{1,1,0}},d{40,{0,1,0}},p{0,{.5,.5,0}},top{50,{.5,.5,1}};
      facets={{0,0,{a,p,b}},{1,0,{b,p,c}},{2,0,{c,p,d}},{3,0,{d,p,a}},
              {4,0,{a,b,top}},{5,0,{b,c,top}},{6,0,{c,d,top}},{7,0,{d,a,top}}};}
    definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;MappedFacetReference::CurvedNets nets;
    const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:facets){errors.emplace(f.id,1e-12);
      if(!crease){auto& net=nets[f.id];for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;
          for(unsigned k=0;k<3;++k){x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}
          net[q]={double(x/3),double(y/3),double(z/3)};}if(f.id<4)net[9].z=-.02;}}
    geometry=std::make_shared<MappedFacetReference>(definition,errors,71,SIZE_MAX,false,nets);
    auto budget=RemappingBudget();std::string reason;assert(geometry->BuildMobilityPolicy(45,{{0,2}},policy,budget,reason));
    for(const auto& f:facets){surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
      for(const auto& n:f.v)points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});
      if(std::find(surface.back().v.begin(),surface.back().v.end(),Id(0))!=surface.back().v.end())patch.surface.push_back(surface.back());}
    state=std::make_shared<BoundaryParameterState>(geometry,points,surface,1,SIZE_MAX,0,BoundaryCoverage{},policy,&budget);
    reference=PlanarReference::WithParameters(definition,state);
    // Intentionally leave patch.parameters empty: the accepted reference owns the original bindings.
  }
  std::vector<SurfaceFace> Moved(Id old,Id fresh,const std::vector<SurfaceFace>& faces)const {
    auto result=faces;for(auto& face:result)for(auto& id:face.v)if(id==old)id=fresh;return result;
  }
};
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);struct Finalize {~Finalize(){SU2_MPI::Finalize();}} finalize;std::cout<<std::setprecision(17);RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;
auto state=fixture.state->WithEntityGeometry(.01,budget);auto reference=PlanarReference::WithParameters(fixture.definition,state);auto scoped=reference->WithAtlasReservation(budget);SetShapeAdmission(ShapeAdmission::ValidityOnly);Patch old;
for(const auto& f:fixture.facets)if(f.id<4){std::array<Node,4> n{{f.v[0],f.v[1],f.v[2],fixture.points.at(50).node}};if(Orientation(n[0].p,n[1].p,n[2].p,n[3].p)<0)std::swap(n[0],n[1]);old.cells.push_back({Id(old.cells.size()),n});}
old.surface=fixture.surface;scoped->ExportParameters(old);Patch fresh;bool ok=CollapseVertex(old,0,10,100,*scoped,fresh,reason);std::cout<<"collapse="<<ok<<" reason="<<reason<<"\n";if(!ok)return 1;
for(const auto& c:old.cells)if(std::none_of(c.v.begin(),c.v.end(),[](Node n){return n.id==10;})){OwnedCell row;row.cell=c;const auto& v=c.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};for(unsigned k=0;k<4;++k)for(const auto& physical:fixture.surface){auto a=faces[k],b=physical.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)row.physical[k]={true,physical.facet};}
std::cout<<"cell="<<c.id<<" veto="<<CollapseConstraintViolation(row,0,fixture.points.at(10).node,*scoped,.01,{})<<"\n";
for(unsigned k=0;k<4;++k)if(row.physical[k].present){std::array<Point,3> p;for(unsigned i=0;i<3;++i){Id id=faces[k][i];if(id==0)id=10;p[i]=fixture.points.at(id).node.p;}const auto f=row.physical[k].facet;try{std::cout<<"facet="<<f<<" canonical="<<scoped->Canonical(f)<<" contains="<<scoped->ContainsFace(f,p)<<" canonical_contains="<<scoped->ContainsFace(scoped->Canonical(f),p)<<" deviation="<<scoped->SurfaceDeviation(f,p)<<" validated_error="<<scoped->SurfaceErrorForValidatedFace(f,p)<<"\n";}catch(const std::exception& e){std::cout<<"error="<<e.what()<<"\n";}}}
auto ctx=scoped->PrivateParameters(fresh);for(const auto& f:fresh.surface){std::array<Point,3> p;for(unsigned i=0;i<3;++i)p[i]=fixture.points.at(f.v[i]).node.p;try{std::cout<<"freshfacet="<<f.facet<<" deviation="<<ctx->SurfaceDeviation(f.facet,p)<<" validated_error="<<ctx->SurfaceErrorForValidatedFace(f.facet,p)<<"\n";}catch(const std::exception& e){std::cout<<"fresherror="<<e.what()<<"\n";}}}
