// Geometry-only proposal on a final private capture, not a replay of an earlier transaction.
#include "../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
using namespace SU2Native3D;
using namespace SU2NativeGeometry;
static std::vector<char> Read(const char* path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open input");
  const auto size=in.tellg();if(size<0 || size>std::streamoff(size_t(128)<<20))throw std::length_error("Input exceeds capture ceiling");
  in.seekg(0);std::vector<char> out(size_t(size),0);if(!in.read(out.data(),size))throw std::runtime_error("Cannot read input");return out;
}
int main(int argc,char** argv) try {
  if(argc!=5)throw std::invalid_argument("usage: replay authority payload owned vertex");
  const Id vertex=std::stoull(argv[4]);const auto refbytes=Read(argv[1]);
  auto archive=DecodeReference(std::string(refbytes.begin(),refbytes.end()),ReferenceStorageLimit3D);
  if(!archive.original||!archive.original->Parameters())throw std::runtime_error("Authority lacks parameters");
  const auto payload=Read(argv[2]);auto state=BoundaryParameterState::FromPayload(archive.original->Parameters()->Authority(),payload,ReferenceStorageLimit3D,0);
  auto reference=PlanarReference::WithParameters(archive.original,state);const auto bytes=Read(argv[3]);RecordStream input(bytes);uint64_t count=0;input(count);
  if(count>4096)throw std::length_error("Capture exceeds bound");Patch old;std::map<Id,Point> points;Id maxcell=0,maxnode=0;
  for(uint64_t i=0;i<count;++i){OwnedCell row;input(row);maxcell=std::max(maxcell,row.cell.id);bool contains=false;
    for(const auto& n:row.cell.v){maxnode=std::max(maxnode,n.id);points[n.id]=n.p;contains|=n.id==vertex;}if(!contains)continue;
    old.cells.push_back(row.cell);const auto& v=row.cell.v;
    const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned k=0;k<4;++k)if(row.physical[k].present)old.surface.push_back({faces[k],row.physical[k].facet});}
  if(!input.End()||old.cells.empty()||!points.count(vertex)||maxnode==UINT64_MAX||maxcell>UINT64_MAX-4096)throw std::runtime_error("Invalid captured star");
  const size_t scratch=size_t(16)<<20;size_t low=0,high=scratch*8;
  while(low<high){const size_t bits=low+(high-low+1)/2;if(transfer_memory::Mul(2,CurvedAtlasWorkspaceBytes(bits))<=scratch/2)low=bits;else high=bits-1;}
  const size_t caller=transfer_memory::Add(transfer_memory::Bytes(refbytes),transfer_memory::Bytes(payload),transfer_memory::Bytes(bytes),state->ResidentBytes(),state->Geometry().ResidentBytes(),transfer_memory::Bytes(old.cells),transfer_memory::Bytes(old.surface));
  const Tensor tensor{4,0,0,4,0,4};const auto origin=points.at(vertex);std::set<Id> neighbors;
  for(const auto& cell:old.cells)for(const auto& n:cell.v)if(n.id!=vertex)neighbors.insert(n.id);
  std::vector<std::pair<Point,Tensor>> samples;double radius=INFINITY;
  for(Id n:neighbors){const Point q=points.at(n);samples.emplace_back(q,tensor);radius=std::min(radius,std::hypot(q.x-origin.x,q.y-origin.y,q.z-origin.z));}
  std::vector<Point> targets{MetricMeanPoint(origin,samples)};
  for(double step:{.25,.125,.0625})for(unsigned axis=0;axis<3;++axis)for(double sign:{-1.,1.}){
    Point q=origin;if(axis==0)q.x+=sign*step*radius;else if(axis==1)q.y+=sign*step*radius;else q.z+=sign*step*radius;targets.push_back(q);}
  auto metric=[](const Patch& patch){double q=INFINITY,j=INFINITY,score=INFINITY,maxedge=0;bool positive=true;
    for(const auto& cell:patch.cells){const auto& v=cell.v;positive&=Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0;
      const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},Tensor{4,0,0,4,0,4});q=std::min(q,m.mean_ratio);j=std::min(j,m.minimum_scaled_jacobian);
      score=std::min(score,std::min(m.mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian));maxedge=std::max(maxedge,m.maximum_edge);}
    return std::make_tuple(positive,q,j,score,maxedge);};
  auto edges=[](const Patch& patch){std::map<Edge,long double> out;
    for(const auto& c:patch.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){auto a=c.v[i],b=c.v[j];if(a.id>b.id)std::swap(a,b);
      const long double dx=(long double)a.p.x-b.p.x,dy=(long double)a.p.y-b.p.y,dz=(long double)a.p.z-b.p.z;out[{a.id,b.id}]=2*std::sqrt(dx*dx+dy*dy+dz*dz);}return out;};
  const auto before=metric(old);const auto oldedges=edges(old);std::cout<<std::setprecision(17)<<"scope=final_private_state_proposal vertex="<<vertex<<" star="<<old.cells.size()<<" faces="<<old.surface.size()<<" targets="<<targets.size()<<" entity="<<state->EntityGeometry()<<" geometry_version="<<state->Geometry().Version()<<" old_positive="<<std::get<0>(before)<<" old_q="<<std::get<1>(before)<<" old_J="<<std::get<2>(before)<<" old_score="<<std::get<3>(before)<<" old_maxedge="<<std::get<4>(before)<<'\n';
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  // Captured proposal only: replace the sliver's working boundary diagonal over its complete vertex star.
  if(vertex==134792)try {
    AtlasBudget budget{67207168,size_t(128)<<20,low,0,caller,0};auto scoped=reference->WithAtlasReservation(budget);
    Patch source=old;scoped->ExportParameters(source);Patch proposal=source;
    auto key=[](Face f){std::sort(f.begin(),f.end());return f;};
    std::set<Face> retired;Id facet=UINT64_MAX;
    for(const auto& f:source.surface)if(std::all_of(f.v.begin(),f.v.end(),[](Id n){return n==1||n==5||n==134792||n==114056;})) {
      retired.insert(key(f.v));if(facet!=UINT64_MAX&&facet!=f.facet)throw std::runtime_error("Mixed retired facets");facet=f.facet;
    }
    if(retired.size()!=2)throw std::runtime_error("Expected two retired physical faces");
    proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[](const Cell& c){return c.id==122900;}),proposal.cells.end());
    if(proposal.cells.size()+1!=source.cells.size())throw std::runtime_error("Captured sliver missing");
    std::map<Face,SurfaceFace> physical;for(const auto& f:source.surface)if(!retired.count(key(f.v)))physical.emplace(key(f.v),f);
    std::set<Face> prior;for(auto f:Incidence(source.cells).Boundary())prior.insert(key(f));
    const auto skin=Incidence(proposal.cells).Boundary();size_t added=0;
    for(auto f:skin)if(!prior.count(key(f))){physical.emplace(key(f),SurfaceFace{f,facet});++added;}
    if(added!=2)throw std::runtime_error("Expected two promoted physical faces");
    proposal.surface.clear();for(const auto& f:physical)proposal.surface.push_back(f.second);
    std::string reason;if(!ValidatePrescribedInterface(source.cells,proposal.cells,skin,reason))throw std::runtime_error(reason);
    scoped->ExportPointParameters(proposal);std::map<Edge,EdgeTrace> features;
    if(!RebuildFeatureTraces(source,proposal.surface,proposal.parameters,*scoped,features,budget,reason))throw std::runtime_error(reason);
    proposal.coverage.faces.clear();proposal.coverage.traces=std::move(features);
    auto context=scoped->PrivateParameters(proposal,&budget);const auto& geometry=context?*context:*scoped;double error=0;
    for(const auto& f:proposal.surface){std::array<Point,3> xyz{points.at(f.v[0]),points.at(f.v[1]),points.at(f.v[2])};
      if(!geometry.ContainsFace(f.facet,xyz))throw std::runtime_error("Proposed physical face outside geometry");
      error=std::max(error,geometry.SurfaceErrorForValidatedFace(f.facet,xyz));}
    const auto m=metric(proposal);
    std::cout<<"shelling_native_checks=pass cells="<<proposal.cells.size()<<" positive="<<std::get<0>(m)<<" score="<<std::get<3>(m)<<" surface_error="<<error<<" tolerance=.001 geometry_pass="<<(error<=.001)<<" feature_traces="<<proposal.coverage.traces.size()<<" work="<<budget.work<<" exhausted="<<budget.work_exhausted<<std::endl;
  }catch(const std::exception& e){std::cout<<"shelling_native_checks=fail reason="<<e.what()<<std::endl;}
  for(size_t i=0;i<targets.size();++i){AtlasBudget budget{67207168,size_t(128)<<20,low,0,caller,0};auto scoped=reference->WithAtlasReservation(budget);Patch source=old;scoped->ExportParameters(source);
    Patch fresh;std::string reason;const bool ok=MoveVertex(source,vertex,{maxnode+1,targets[i]},maxcell+1,*scoped,fresh,reason);
    const auto opwork=budget.work;std::cout<<"target="<<i<<" xyz="<<targets[i].x<<','<<targets[i].y<<','<<targets[i].z<<" valid="<<ok<<" reason="<<reason<<" op_work="<<opwork<<" exhausted="<<budget.work_exhausted<<" peak="<<budget.peak_bytes;
    if(ok){const auto m=metric(fresh);const auto after=edges(fresh);long double removed=0,added=0;
      for(const auto& e:oldedges)if(!after.count(e.first)){const auto x=std::max(0.L,e.second-MaximumMetricEdge);removed+=x*x;}
      for(const auto& e:after)if(!oldedges.count(e.first)){const auto x=std::max(0.L,e.second-MaximumMetricEdge);added+=x*x;}
      std::cout<<" positive="<<std::get<0>(m)<<" q="<<std::get<1>(m)<<" J="<<std::get<2>(m)<<" score="<<std::get<3>(m)<<" maxedge="<<std::get<4>(m)<<" removed_excess="<<removed<<" added_excess="<<added;
      const auto b=fresh.parameters.find(maxnode+1);if(b!=fresh.parameters.end())std::cout<<" projected="<<b->second.node.p.x<<','<<b->second.node.p.y<<','<<b->second.node.p.z<<" binding="<<b->second.binding.dimension<<','<<b->second.binding.feature<<','<<b->second.binding.u<<','<<b->second.binding.v;
      try{auto v=scoped->PrivateParameters(fresh,&budget);const auto& g=v?*v:*scoped;double error=0;
        for(const auto& f:fresh.surface){std::array<Point,3> xyz{};for(unsigned k=0;k<3;++k){bool found=false;for(const auto& c:fresh.cells)for(const auto& n:c.v)if(n.id==f.v[k]){xyz[k]=n.p;found=true;}if(!found)throw std::runtime_error("Surface node missing");}error=std::max(error,g.SurfaceErrorForValidatedFace(f.facet,xyz));}
        std::cout<<" post_surface_error="<<error;
      }catch(const std::exception& e){std::cout<<" post_surface_error_failure="<<e.what();}}
    std::cout<<" final_work="<<budget.work<<" final_peak="<<budget.peak_bytes<<std::endl;
  }
  return 0;
} catch(const std::exception& e){std::cerr<<"replay_error="<<e.what()<<'\n';return 1;}
