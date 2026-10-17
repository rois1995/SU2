/*!
 * \file CNativeNormalizationStage3D.hpp
 * \brief One bounded original-family correction between private native remeshing stages.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeCandidateComplexity3D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../solvers/CSolver.hpp"

inline SU2Native3D::NormalizationStage3D NativeNormalizationStage3D(CSolver& solver,CGeometry& geometry,
    const CConfig& config,const SU2Native3D::ReferenceState& reference,const su2activematrix& selected) {
  using namespace SU2Native3D;NormalizationStage3D job;const auto family=solver.GetNativeMetricFamily3D();
  bool eligible=family && family->geometry==&geometry && family->Compatible(config) &&
      (!config.GetnAdap_BL() || (family->boundary_layer && family->authority==reference.original)) &&
      (config.GetAdap_BL_Budget() || family->normalization.status!=SU2Complexity::Status::EstimatedTolerance) && selected.rows()>=family->owned && selected.cols()>=6;
  // Prediction may change only some ranks: eligibility belongs to the complete original field.
  if(eligible)for(size_t i=0;i<family->owned;++i)for(unsigned a=0,k=0;a<3;++a)for(unsigned b=a;b<3;++b,++k)
    eligible=eligible && selected(i,k)==family->selected[i].m[a][b];
  if(!CPassiveComm::Allreduce(int(eligible),CPassiveComm::Op::MIN))return job;
  job.retained_bytes=family->ResidentBytes()-(family->layers?family->layers->StorageBytes():0);
  job.propose=[family,&solver,&geometry,&config,&reference](const ComplexityReassessment3D& current,
      const PreparedComplexity3D& cloud,const std::vector<DonorCell>& original,double currentCore,
      const std::function<bool()>& within,size_t ceiling,size_t caller_bytes) {
    World world;NormalizationProposal3D result;result.log_scale=family->normalization.logScale;
    result.estimate={current.value,current.uncertainty};const double target=family->controls[4];
    const double residual=current.value-target,allowed=std::max(target*1e-6,current.uncertainty);
    if(!current.estimated || !std::isfinite(current.uncertainty) || std::abs(residual)<=allowed) {
      result.reason="within_reassessed_uncertainty";return result;
    }
    if((residual>0 && result.log_scale<=family->lower) || (residual<0 && result.log_scale>=family->upper)) {
      result.reason="outside_reassessed_scale_bounds";result.status=SU2Complexity::Status::OutsideBracket;
      if(config.GetAdap_BL_Budget() && current.value-current.uncertainty>target) {
        result.valid=false;result.reason="estimated_budget_incompatible_at_original_family_lower_bound";
      }
      return result;
    }
    size_t adjacency=0;for(size_t i=0;i<family->owned;++i)adjacency=transfer_memory::Add(adjacency,geometry.nodes->GetnPoint(i));
    const auto trial_bytes=transfer_memory::Add(
        transfer_memory::Mul(family->selected.size(),5*sizeof(CBoundaryLayerMetric::Tensor)),
        transfer_memory::Mul(family->owned,sizeof(CBoundaryLayerMetric::Tensor)+3*sizeof(su2double)+sizeof(std::vector<unsigned long>)+16),
        transfer_memory::GrowthBound(adjacency,sizeof(unsigned long)));
    result.maximum_bytes=transfer_memory::Add(caller_bytes,trial_bytes);
    if(world.maximum(result.maximum_bytes>ceiling)) {result.reason="normalization_storage_limited";return result;}
    CLocalFailure failure;
    auto vote=[&](){const auto e=world.elect(failure);if(e.any)throw std::runtime_error(e.message);};
    auto time=[&](){if(world.maximum(within && !within()))throw std::runtime_error("Native normalization stage time budget exhausted.");};
    time();
    const auto begin=world.seconds();
    CNativeCandidateComplexity3D assess(*family,cloud,original,ceiling,
        transfer_memory::Add(caller_bytes,trial_bytes));
    su2double core=currentCore>0?currentCore:family->core;
    const auto before=assess.Assess(family->selected,core);
    result.estimate=before;result.maximum_bytes=transfer_memory::Add(result.maximum_bytes,assess.ResidentBytes());
    if(std::abs(before.value-target)<=std::max(target*1e-6,before.uncertainty)) {
      result.reason="within_reassessed_sampling_and_numerical_uncertainty";
      result.seconds=world.seconds()-begin;result.geometry_seconds=assess.stats.preparation_seconds;
      result.assessment_seconds=assess.stats.assessment_seconds;result.density_queries=assess.stats.density_queries;
      result.geometry_queries=assess.stats.geometry_queries;return result;
    }
    std::shared_ptr<CNativeMetricEvaluation3D> evaluation;
    try {
      evaluation=std::make_shared<CNativeMetricEvaluation3D>();evaluation->family=family;
      evaluation->assess=[&assess,time](const std::vector<CBoundaryLayerMetric::Tensor>& sensor,su2double& trialCore){time();return assess.Assess(sensor,trialCore);};
    }catch(const std::exception& e){failure.Set(3,0,e.what());}vote();
    std::string reason;
    result.valid=solver.RecomputeMetricFamily3D(&geometry,&config,*evaluation,&reference,reason);
    result.reason=reason;result.log_scale=evaluation->normalization.logScale;result.core=evaluation->core;
    result.estimate=evaluation->normalization.estimate;result.status=evaluation->normalization.status;
    result.trials=evaluation->normalization.trials;result.seconds=world.seconds()-begin;
    result.geometry_seconds=assess.stats.preparation_seconds;result.assessment_seconds=assess.stats.assessment_seconds;
    result.density_queries=assess.stats.density_queries;result.geometry_queries=assess.stats.geometry_queries;
    evaluation->assess={};
    if(!result.valid)return result;
    if(config.GetAdap_BL_Budget() && result.status==SU2Complexity::Status::OutsideBracket &&
        result.estimate.value-result.estimate.uncertainty>target) {
      result.valid=false;result.reason="estimated_budget_incompatible_on_reassessed_original_family";return result;
    }
    int changed=0;
    for(size_t i=0;i<family->owned;++i)for(unsigned a=0;a<3;++a)for(unsigned b=a;b<3;++b)
      changed|=evaluation->normalized.at(i).m[a][b]!=family->selected[i].m[a][b];
    result.changed=world.maximum(changed);
    if(!result.changed){result.reason="integration_limited_unchanged_source";return result;}
    result.reason="bounded_original_family_correction";
    result.retained_bytes=transfer_memory::Add(sizeof(*evaluation),transfer_memory::Bytes(evaluation->normalized));
    result.source_value=[evaluation,family,span=assess.source_span](Id node) {
      const auto& m=evaluation->normalized.at(family->LocalNode(node,span));
      return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
          SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
    };
    return result;
  };
  return job;
}
