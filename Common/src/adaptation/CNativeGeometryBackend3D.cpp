/*!
 * \file CNativeGeometryBackend3D.cpp
 * \brief Native mapped authority adapter; no alternate geometry or projection fallback.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

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
    AtlasBudget& budget,std::string& reason)const {
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
    if(!authority->Curves().count(scope.hint))throw std::invalid_argument("Certified native projection is unavailable for this faceted support.");
    // Bound the existing query's reservation by both remaining work and admitted storage.
    size_t low=0,high=budget.maximum_work-budget.work;const auto remaining_bytes=budget.maximum_bytes-budget.caller_bytes;
    while(low<high){Use(budget);const auto middle=low+(high-low)/2+(high-low)%2;
      if(authority->CurvedQueryWorkspaceBytes(middle)<=remaining_bytes)low=middle;else high=middle-1;}
    const auto reserved=std::min(low,budget.maximum_work-budget.work);
    if(!reserved){if(budget.work==budget.maximum_work)budget.ChargeWork();throw std::length_error("Projection query has no admitted work/storage reservation.");}
    const auto scratch=authority->CurvedQueryWorkspaceBytes(reserved);Use(budget,scratch);
    const auto before=budget.work;const auto work=std::min(reserved,budget.maximum_work-before);
    if(!work){budget.ChargeWork();throw std::length_error("Projection query work exhausted before search.");}
    if(!budget.ChargeWork(work))throw std::length_error("Projection query work exhausted before search."); // An exceptional query retains its conservative reservation as attempted work.
    // Delegated numerical/storage failures do not themselves establish a caller work denial.
    const auto candidate=authority->ClosestCurved(scope.hint,query,scope.absolute_error,work,remaining_bytes);
    budget.work=before+candidate.work;
    Id side=UINT64_MAX;
    authority->ForFacets(candidate.binding,[&](Id facet){Use(budget);
      if(authority->Body(facet)==scope.body&&Sheet(*authority,policy,facet)==sheet)side=std::min(side,facet);});
    if(side==UINT64_MAX)throw std::invalid_argument("Closest native candidate leaves the prescribed sheet.");
    GeometrySupport support;if(!Support(candidate.binding,side,support,budget,reason))return false;
    if((previous.mobility<2&&(support.mobility!=previous.mobility||support.feature!=previous.feature))||
       (scope.feature&&(support.mobility!=scope.feature->first||support.feature!=scope.feature->second)))
      throw std::invalid_argument("Closest native candidate leaves the prescribed physical feature.");
    GeometryProjection out;out.binding=candidate.binding;out.surface=side;out.distance_lower=candidate.lower;out.distance_upper=candidate.upper;
    if(!Evaluate(out.binding,side,out.evaluation,budget,reason))return false;
    result=out;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool NativeMappedGeometryBackend3D::RegionErrorBounds(const FaceCoverage& coverage,const std::array<Point,3>& flat,
    GeometryRegionBounds& result,AtlasBudget& budget,std::string& reason)const {
  try {
    Use(budget,MappedFacetReference::SourcePreparationBytes(coverage));GeometryRegionBounds out;
    out.maximum_error=authority->SurfaceError(coverage,flat,budget);
    out.bounds=authority->PrepareSourcePatch(coverage,flat,budget).Bounds();result=out;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
} // namespace SU2NativeGeometry
