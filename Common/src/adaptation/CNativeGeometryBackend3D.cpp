/*!
 * \file CNativeGeometryBackend3D.cpp
 * \brief Native mapped authority adapter; no alternate geometry or projection fallback.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativeCurvedReference.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace SU2NativeGeometry {
namespace {
void Use(AtlasBudget& budget,size_t scratch=0) {
  if(!budget.ChargeWork())throw std::length_error("Geometry backend work exhausted.");
  const auto bytes=transfer_memory::Add(budget.caller_bytes,scratch);budget.peak_bytes=std::max(budget.peak_bytes,bytes);
  if(bytes==SIZE_MAX||bytes>budget.maximum_bytes)throw std::length_error("Geometry backend storage exhausted.");
}
Id Sheet(const MappedFacetReference& geometry,const MobilityPolicy& policy,Id facet) {
  return policy.identity?policy.sheets.at(facet):geometry.Canonical(facet);
}
}
NativeMappedGeometryBackend3D::NativeMappedGeometryBackend3D(
    std::shared_ptr<const MappedFacetReference> model,const MobilityPolicy& physical_policy)
    :authority(std::move(model)),policy(physical_policy) {
  if(!authority||policy.version!=1)throw std::invalid_argument("Invalid native geometry backend authority/policy.");
}
GeometryBackendIdentity NativeMappedGeometryBackend3D::Identity()const {
  return {GeometryBackendKind::NativeMapped,authority->Version(),1,authority.get()};
}
bool NativeMappedGeometryBackend3D::AssociationValid(const SurfaceBinding& binding,Point point,std::string& reason)const {
  try {
    if(!authority->Admissible(binding,point))throw std::invalid_argument("Point violates its immutable geometry association.");
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::Evaluate(const SurfaceBinding& binding,std::optional<Id> side,
    GeometryEvaluation& result,AtlasBudget& budget,std::string& reason)const {
  try {
    Use(budget,sizeof(GeometryEvaluation));GeometryEvaluation out;out.point=authority->Realize(binding);
    out.realization_error=authority->ErrorBound(binding,out.point);
    if(side) {
      const auto value=authority->EvaluateSurface(*side,binding);out.point=value.point;out.normal=value.normal;
      out.curvature1=value.curvature1;out.curvature2=value.curvature2;out.direction1=value.direction1;out.direction2=value.direction2;out.differential_available=true;
      if(binding.dimension==1) {
        const auto edge=authority->OriginalEdge(binding.feature);const auto& f=authority->Reference().Get(*side);
        const int u=int(f.v[1].id==edge[1])-int(f.v[1].id==edge[0]),v=int(f.v[2].id==edge[1])-int(f.v[2].id==edge[0]);
        Point tangent{u*value.first.x+v*value.second.x,u*value.first.y+v*value.second.y,u*value.first.z+v*value.second.z};
        const double length=std::hypot(tangent.x,tangent.y,tangent.z);
        if(!(length>0&&std::isfinite(length)))throw std::invalid_argument("Unrepresentable native curve tangent.");
        out.tangent={tangent.x/length,tangent.y/length,tangent.z/length};out.tangent_available=true;
      }
    }
    result=out;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::Support(const SurfaceBinding& binding,Id side,GeometrySupport& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    Use(budget,sizeof(GeometrySupport));bool incident=false;authority->ForFacets(binding,[&](Id facet){Use(budget);incident=incident||facet==side;});
    if(!incident)throw std::invalid_argument("Geometry side does not support its association.");
    GeometrySupport out;out.body=authority->Body(side);out.surface=side;out.sheet=Sheet(*authority,policy,side);
    out.marker=authority->Reference().Get(side).marker;out.mobility=authority->MobilityDimension(binding,policy);
    out.feature=out.mobility==2?out.sheet:binding.feature;
    if(out.mobility==1&&policy.identity) {
      if(binding.dimension==1)out.feature=policy.features.at(authority->OriginalEdge(binding.feature));
      else {
        std::optional<Id> chain;
        authority->ForFacets(binding,[&](Id facet){Use(budget);const auto& f=authority->Reference().Get(facet);
          for(unsigned k=0;k<3;++k){SU2Native3D::Edge e{f.v[k].id,f.v[(k+1)%3].id};if(e[0]!=binding.feature&&e[1]!=binding.feature)continue;
            std::sort(e.begin(),e.end());const auto found=policy.features.find(e);if(found==policy.features.end())continue;
            if(chain&&*chain!=found->second)throw std::invalid_argument("Curve association has ambiguous physical feature identity.");chain=found->second;}});
        if(!chain)throw std::invalid_argument("Curve association has no physical feature identity.");out.feature=*chain;
      }
    }
    result=out;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::ContractionAllowed(const SurfaceBinding& removed,const SurfaceBinding& retained,
    bool& allowed,AtlasBudget& budget,std::string& reason)const {
  try {
    Use(budget);Id side=UINT64_MAX;
    authority->ForFacets(removed,[&](Id f){Use(budget);side=std::min(side,f);});
    GeometrySupport from;if(!Support(removed,side,from,budget,reason))return false;
    bool admissible=false;
    authority->ForFacets(retained,[&](Id f){
      Use(budget);GeometrySupport to;if(!Support(retained,f,to,budget,reason))throw std::invalid_argument(reason);
      if(!from.mobility||from.body!=to.body)return;
      if(from.mobility==2){admissible=admissible||from.sheet==to.sheet;return;}
      if(to.mobility==1){admissible=admissible||from.feature==to.feature;return;}
      if(to.mobility!=0)return;
      const auto& facet=authority->Reference().Get(f);
      for(unsigned k=0;k<3;++k){Use(budget);SU2Native3D::Edge edge{facet.v[k].id,facet.v[(k+1)%3].id};
        if(edge[0]!=retained.feature&&edge[1]!=retained.feature)continue;
        std::sort(edge.begin(),edge.end());const auto found=policy.features.find(edge);
        admissible=admissible||(found!=policy.features.end()&&found->second==from.feature);
      }
    });
    allowed=admissible;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::Project(Point query,const GeometryProjectionScope& scope,GeometryProjection& result,
    AtlasBudget& budget,std::string& reason,GeometryProjectionFailure* failure)const {
  if(failure)*failure=GeometryProjectionFailure::Failure;
  try {
    Use(budget,sizeof(GeometryProjection));
    if(!std::isfinite(query.x)||!std::isfinite(query.y)||!std::isfinite(query.z))
      throw std::invalid_argument("Nonfinite geometry projection query.");
    if(scope.body==UINT64_MAX||scope.hint==UINT64_MAX||authority->Body(scope.hint)!=scope.body||
       !(std::isfinite(scope.absolute_error)&&scope.absolute_error>=std::numeric_limits<double>::min()))
      throw std::invalid_argument("Projection needs a declared body, local hint and positive accuracy.");
    GeometrySupport previous;
    if(!Support(scope.previous,scope.hint,previous,budget,reason))return false;
    const Id sheet=scope.sheet.value_or(previous.sheet);
    if(previous.body!=scope.body||previous.sheet!=sheet||(scope.feature&&
       (previous.mobility!=scope.feature->first||previous.feature!=scope.feature->second)))
      throw std::invalid_argument("Projection scope conflicts with its previous geometry association.");
    if(!authority->Curves().count(scope.hint)) {
      if(failure)*failure=GeometryProjectionFailure::Unavailable;
      reason="Native projection is unavailable for this faceted support.";return false;
    }
    using Vector=SU2NativeCurved::Vec<3>;
    const Vector target{query.x,query.y,query.z};
    Use(budget,transfer_memory::Add(transfer_memory::Mul(2,authority->CurvedQueryWorkspaceBytes(0)),sizeof(std::array<Vector,10>)));
    const auto allowed=[&](Id facet){return authority->Body(facet)==scope.body&&Sheet(*authority,policy,facet)==sheet;};
    if(!allowed(scope.hint))throw std::invalid_argument("Projection hint leaves the prescribed sheet.");
    SurfaceBinding binding;long double best=INFINITY;double best_upper=INFINITY;Id first_failed=UINT64_MAX;
    const char* capture=std::getenv("SU2_NATIVE_PROJECTION_CAPTURE");
    struct CandidateRecord {
      Id id,body,sheet;bool allowed=false,converged=false,eligible=false;
      double initial_u=0,initial_v=0,u=0,v=0;SurfaceBinding foot;GeometrySupport support;
    };
    std::vector<CandidateRecord> attempts;

    auto candidate=[&](Id id) {
      Use(budget);const bool permitted=allowed(id);CandidateRecord* record=nullptr;
      if(capture&&*capture){attempts.push_back({id,authority->Body(id),Sheet(*authority,policy,id)});record=&attempts.back();record->allowed=permitted;}
      if(!permitted)return;
      const auto& source=authority->Curves().at(id);std::array<Vector,10> net;
      for(unsigned k=0;k<10;++k)net[k]={source[k].x,source[k].y,source[k].z};
      const auto local_target=SU2NativeCurved::detail::NormalizeTriangleProjection(net,target);
      double u=0,v=0;SU2NativeCurved::detail::ClosestOnTriangle(local_target,net[0],net[1],net[2],u,v);
      if(record){record->initial_u=u;record->initial_v=v;}
      auto evaluate=[&](double a,double b){Use(budget);return SU2NativeCurved::detail::TrianglePatch(net,1-a-b,a,b);};
      auto derivatives=[&](double a,double b,Vector& first,Vector& second){Use(budget);
        SU2NativeCurved::detail::TriangleDerivatives(net,a,b,first,second);};
      const bool converged=SU2NativeCurved::detail::MinimizeTriangle(local_target,u,v,evaluate,derivatives);
      if(record){record->converged=converged;record->u=u;record->v=v;}
      if(!converged){first_failed=std::min(first_failed,id);return;}
      const double tiny=1e-13;
      if(u<tiny)u=0;if(v<tiny)v=0;
      if(1-u-v<tiny){const double sum=u+v;u/=sum;v=1-u;}
      const auto& f=authority->Reference().Get(id);SurfaceBinding foot;
      auto edge=[&](unsigned a,unsigned b,double t){return f.v[a].id<f.v[b].id?
        authority->EdgePoint({f.v[a].id,f.v[b].id},t):authority->EdgePoint({f.v[b].id,f.v[a].id},1-t);};
      if(u==1)foot=authority->Vertex(f.v[1].id);
      else if(v==1)foot=authority->Vertex(f.v[2].id);
      else if(u==0&&v==0)foot=authority->Vertex(f.v[0].id);
      else if(v==0)foot=edge(0,1,u);
      else if(u==0)foot=edge(0,2,v);
      else if(1-u-v==0)foot=edge(1,2,v);
      else foot=authority->FacetPoint(id,u,v);
      GeometrySupport support;
      if(!Support(foot,id,support,budget,reason))throw std::invalid_argument(reason);
      if(record){record->foot=foot;record->support=support;}
      if((previous.mobility<2&&(support.mobility!=previous.mobility||support.feature!=previous.feature))||
          (scope.feature&&(support.mobility!=scope.feature->first||support.feature!=scope.feature->second)))return;
      if(record)record->eligible=true;
      const auto point=authority->Realize(foot);
      const long double distance=std::hypot((long double)query.x-point.x,(long double)query.y-point.y,(long double)query.z-point.z);
      if(!std::isfinite(distance))throw std::invalid_argument("Unrepresentable numerical projection distance.");
      if(distance<best){best=distance;binding=foot;
        best_upper=std::nextafter(double(distance+authority->ErrorBound(foot,point)),INFINITY);
      }
    };
    candidate(scope.hint);
    const auto anchor=authority->Realize(scope.previous);
    const long double anchor_distance=std::hypot((long double)query.x-anchor.x,(long double)query.y-anchor.y,(long double)query.z-anchor.z);
    const double radius=std::nextafter(double(std::min(best,anchor_distance))+scope.absolute_error,INFINITY);
    if(!std::isfinite(radius))throw std::invalid_argument("Unrepresentable numerical projection search radius.");
    std::array<Point,2> box;
    box[0]={std::nextafter(double((long double)query.x-radius),-INFINITY),std::nextafter(double((long double)query.y-radius),-INFINITY),std::nextafter(double((long double)query.z-radius),-INFINITY)};
    box[1]={std::nextafter(double((long double)query.x+radius),INFINITY),std::nextafter(double((long double)query.y+radius),INFINITY),std::nextafter(double((long double)query.z+radius),INFINITY)};
    if(best_upper>scope.absolute_error)for(Id id:authority->NearbySurfaceFacets(scope.hint,box)) {
      if(id!=scope.hint)candidate(id);
      if(best_upper<=scope.absolute_error)break;
    }
    if(!std::isfinite(best)) {
      std::string replay;
      if(capture&&*capture) {
        static thread_local size_t sequence=0;const char* rank=std::getenv("OMPI_COMM_WORLD_RANK");
        replay=std::string(capture)+"."+(rank?rank:"serial")+"."+std::to_string(sequence++)+".txt";
        std::ofstream out(replay);if(!out)throw std::runtime_error("Cannot save requested projection replay: "+replay);
        out<<std::setprecision(17)<<"SU2_PROJECTION_REPLAY 1\nquery "<<query.x<<' '<<query.y<<' '<<query.z
           <<"\nscope "<<scope.body<<' '<<scope.hint<<' '<<sheet<<' '<<(scope.feature?int(scope.feature->first):-1)
           <<' '<<(scope.feature?scope.feature->second:UINT64_MAX)<<' '<<scope.absolute_error
           <<"\nprevious "<<scope.previous.version<<' '<<scope.previous.dimension<<' '<<scope.previous.feature<<' '<<scope.previous.u<<' '<<scope.previous.v
           <<"\nprevious_support "<<previous.body<<' '<<previous.sheet<<' '<<previous.mobility<<' '<<previous.feature
           <<"\nwork "<<budget.work<<"\ncandidates "<<attempts.size()<<'\n';
        for(const auto& a:attempts) {
          out<<"candidate "<<a.id<<' '<<a.body<<' '<<a.sheet<<' '<<a.allowed<<' '<<a.converged<<' '<<a.eligible
             <<' '<<a.initial_u<<' '<<a.initial_v<<' '<<a.u<<' '<<a.v<<'\n';
          if(!a.allowed)continue;
          out<<"support "<<a.support.body<<' '<<a.support.sheet<<' '<<a.support.mobility<<' '<<a.support.feature
             <<"\nfoot "<<a.foot.version<<' '<<a.foot.dimension<<' '<<a.foot.feature<<' '<<a.foot.u<<' '<<a.foot.v<<'\n';
          const auto& facet=authority->Reference().Get(a.id);out<<"vertices";for(auto n:facet.v)out<<' '<<n.id;out<<"\nnet";
          for(auto p:authority->Curves().at(a.id))out<<' '<<p.x<<' '<<p.y<<' '<<p.z;out<<'\n';
        }
        out.close();if(!out)throw std::runtime_error("Cannot finish requested projection replay: "+replay);
      }
      if(failure)*failure=GeometryProjectionFailure::Unavailable;
      reason="No converged numerical projection on the prescribed body/sheet/feature; first failed facet "+std::to_string(first_failed)+"."+(replay.empty()?"":" Replay: "+replay);return false;
    }
    Id side=UINT64_MAX;
    authority->ForFacets(binding,[&](Id facet){Use(budget);
      if(authority->Body(facet)==scope.body&&Sheet(*authority,policy,facet)==sheet)side=std::min(side,facet);});
    if(side==UINT64_MAX)throw std::invalid_argument("Closest native candidate leaves the prescribed sheet.");
    GeometrySupport support;if(!Support(binding,side,support,budget,reason))return false;
    if((previous.mobility<2&&(support.mobility!=previous.mobility||support.feature!=previous.feature))||
       (scope.feature&&(support.mobility!=scope.feature->first||support.feature!=scope.feature->second)))
      throw std::invalid_argument("Closest native candidate leaves the prescribed physical feature.");
    GeometryProjection out;out.binding=binding;out.surface=side;out.distance_lower=0;
    if(!Evaluate(out.binding,side,out.evaluation,budget,reason))return false;
    // A realized model foot bounds closest distance from above; no global minimum is certified here.
    out.distance_upper=std::nextafter(double(best+out.evaluation.realization_error),INFINITY);
    if(!std::isfinite(out.distance_upper))throw std::invalid_argument("Unrepresentable numerical projection bound.");
    if(failure)*failure=GeometryProjectionFailure::None;
    result=out;reason.clear();return true;
  }catch(const std::exception& e){if(failure)*failure=GeometryProjectionFailure::Failure;reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::RegionErrorBounds(const FaceCoverage& coverage,const std::array<Point,3>& flat,
    GeometryRegionBounds& result,AtlasBudget& budget,std::string& reason)const {
  try {
    Use(budget,MappedFacetReference::SourcePreparationBytes(coverage));GeometryRegionBounds out;
    out.maximum_error=authority->SurfaceError(coverage,flat,budget);
    out.bounds=authority->PrepareSourcePatch(coverage,flat,budget).Bounds();result=out;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::PatchErrorBounds(const GeometrySupport& scope,
    const std::vector<std::array<Point,3>>& physical,double tolerance,
    MappedFacetReference::SurfaceUnionBounds& result,AtlasBudget& budget,std::string& reason,GeometryErrorDirection direction)const {
  try {
    if(direction!=GeometryErrorDirection::Both&&direction!=GeometryErrorDirection::ModelToMesh)throw std::invalid_argument("Invalid geometry error direction.");
    if(scope.body==UINT64_MAX)throw std::invalid_argument("Surface-union query lacks a physical body.");
    const auto count=authority->Reference().Facets().size();Use(budget,transfer_memory::Mul(count,sizeof(Id)));
    std::vector<Id> facets;facets.reserve(count);
    for(const auto& row:authority->Reference().Facets()) {
      Use(budget);if(authority->Body(row.first)==scope.body && row.second.marker==scope.marker &&
          (scope.sheet==UINT64_MAX || Sheet(*authority,policy,row.first)==scope.sheet))facets.push_back(row.first);
    }
    struct Restore {AtlasBudget& b;size_t caller;~Restore(){b.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
    budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,transfer_memory::Mul(facets.capacity(),sizeof(Id)));
    const auto bounds=direction==GeometryErrorDirection::Both?authority->BoundSurfaceUnion(facets,physical,tolerance,budget):
        authority->BoundModelSurface(facets,physical,tolerance,budget);
    result=bounds;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
} // namespace SU2NativeGeometry
