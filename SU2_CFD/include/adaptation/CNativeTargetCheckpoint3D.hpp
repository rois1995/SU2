/*!
 * \file CNativeTargetCheckpoint3D.hpp
 * \brief Bounded native BL target/family checkpoint using existing scalar records and reference encoding.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeBLTarget3D.hpp"
#include "CNativeMetricFamily3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed2D.hpp"
#include <filesystem>
#include <fstream>

namespace SU2NativeTargetCheckpoint3D {
using SU2NativeBoundary2D::RecordStream;
using SU2Native3D::ReferenceState;
struct Checkpoint {
  std::shared_ptr<CNativeMetricFamily3D> family;
  double pre_complexity=0,final_complexity=0;bool bracketed=false;
  SU2NativeBLPipeline3D::RampStage stage;
};
namespace detail {
inline uint64_t Checksum(const std::vector<char>& bytes,size_t count) {
  uint64_t h=1469598103934665603ULL;
  for(size_t i=0;i<count;++i)h=(h^static_cast<unsigned char>(bytes[i]))*1099511628211ULL;
  return h;
}
struct Stamp {
  std::array<uint64_t,7> identities{};
  std::array<double,8> values{};
  template<class S> void Fields(S& s){s(identities,values);}
  bool operator==(const Stamp& b)const{return identities==b.identities && values==b.values;}
};
inline Stamp State(const CBoundaryLayerMetric& m) {
  const auto& n=m.GetOuterNormal();const auto& f=m.GetFeatures();
  return {{{m.GetnWall(),m.GetSampleVersion(),m.GetGeometrySampleVersion(),m.GetSupportVersion(),f.identity,n.version,uint64_t(n.frozen)}},
          {{m.GetTangentialScale(),m.GetOuterTransitionCore(),n.protected_distance,n.maximum_growth,n.parameter,f.angle,f.slope,f.maximumRadius}}};
}
struct Header {
  uint64_t magic=0x4e424c5441524731ULL,version=2,reference_bytes=0,nodes=0,owned=0,span=0,layers=0,roles=0;
  std::array<double,8> controls{};
  std::array<double,8> family{}; // surface tolerance, angle, lower, upper, core, tangent scale, anchor, log scale
  uint64_t feature=0,status=0,trials=0,budget_status=0,budget_trials=0,budget_refinements=0,bracketed=0;
  std::array<double,3> estimate{},budget_estimate{};
  double budget_parameter=0,pre=0,final=0;
  std::array<double,5> policy{}; // core, h0, outer parameter, inherited scale, Option-A radius
  Stamp full,base;
  std::array<uint64_t,2> witness_versions{};std::array<double,2> witness_policy{};
  SU2NativeBLPipeline3D::RampStage stage;
  template<class S> void Fields(S& s){s(magic,version,reference_bytes,nodes,owned,span,layers,roles,controls,family,
      feature,status,trials,budget_status,budget_trials,budget_refinements,bracketed,estimate,budget_estimate,
      budget_parameter,pre,final,policy,full,base,witness_versions,witness_policy);
    if(version>=2)s(stage);}
};
inline void String(RecordStream& stream,std::string& value,bool read,size_t maximum) {
  uint64_t n=value.size();stream(n);
  if(n>maximum || n>SIZE_MAX)throw std::length_error("Native target string exceeds admission.");
  if(read)value.resize(size_t(n));
  for(auto& c:value){uint8_t byte=static_cast<unsigned char>(c);stream(byte);if(read)c=char(byte);}
}
inline void Matrix(RecordStream& stream,CBoundaryLayerMetric::Tensor& m,bool read) {
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
    double value=SU2_TYPE::GetValue(m.m[i][j]);stream(value);
    if(!std::isfinite(value))throw std::invalid_argument("Nonfinite native target matrix.");
    if(read)m.m[i][j]=value;
  }
}
inline void History(RecordStream& stream,std::vector<CBoundaryLayerMetric::ProfileChange>& changes,bool read,size_t maximum) {
  uint64_t n=changes.size();stream(n);
  if(n>maximum/24)throw std::length_error("Native target profile history exceeds admission.");
  if(read)changes.resize(size_t(n));
  for(auto& change:changes)stream(change);
}
inline void CheckStage(const Header& h,const CConfig& config,bool exactEvent) {
  if(h.version==1) {
    if(config.GetAdap_BL_H0Ramp())throw std::invalid_argument("Legacy native target checkpoint has no first-height ramp stage.");
  } else {
    const auto current=SU2NativeBLPipeline3D::MakeRampStage(config,h.stage.event);
    if(h.stage.transitions!=current.transitions || h.stage.cycles!=current.cycles ||
       h.stage.signature!=current.signature || h.stage.event>h.stage.cycles ||
       (exactEvent && h.stage.event!=config.GetAdap_BL_RampEvent()))
      throw std::invalid_argument("Native target checkpoint first-height stage/schedule differs from configuration.");
  }
}
inline void CheckHeader(const Header& h,const CConfig& config,size_t bytes,size_t roles) {
  if(h.magic!=0x4e424c5441524731ULL || (h.version!=1 && h.version!=2) || !h.span || h.owned>h.nodes || h.nodes>bytes/184 ||
      h.layers!=config.GetnAdap_BL() || !h.layers || h.roles!=roles || h.reference_bytes>bytes ||
      h.status>3 || h.budget_status>2 || h.trials>UINT_MAX || h.budget_trials>UINT_MAX || h.budget_refinements>UINT_MAX || h.bracketed>1)
    throw std::invalid_argument("Invalid native BL target checkpoint header.");
  for(double x:h.family)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite native target family control.");
  for(double x:h.policy)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite native target policy control.");
  if(!(h.family[4]>0) || !(h.policy[0]>0) || !(h.policy[1]>0) || !(h.final>0) || !std::isfinite(h.final) ||
      !(h.pre>=0) || !std::isfinite(h.pre) || !(h.estimate[0]>0) || !std::isfinite(h.estimate[0]) ||
      !(h.estimate[1]>=0) || !(h.estimate[2]>=0) || !std::isfinite(h.estimate[2]) ||
      !(h.budget_estimate[0]>=0) || !std::isfinite(h.budget_estimate[0]) || !(h.budget_estimate[1]>=0) ||
      !(h.budget_estimate[2]>=0) || !std::isfinite(h.budget_estimate[2]) || !std::isfinite(h.budget_parameter) || h.policy[4]<0)
    throw std::invalid_argument("Invalid native target normalization state.");
  CheckStage(h,config,true);
}
}

inline SU2NativeBLPipeline3D::RampStage InspectStage(const std::vector<char>& bytes,const CConfig& config) {
  using namespace detail;
  if(bytes.size()<24)throw std::invalid_argument("Truncated native target checkpoint.");
  uint64_t checksum=0;std::memcpy(&checksum,bytes.data()+bytes.size()-8,8);
  if(checksum!=Checksum(bytes,bytes.size()-8))throw std::invalid_argument("Damaged native target checkpoint.");
  SU2NativeBoundary2D::RecordStream stream(bytes);Header h;stream(h);
  if(h.magic!=0x4e424c5441524731ULL || (h.version!=1 && h.version!=2))
    throw std::invalid_argument("Invalid native BL target checkpoint header.");
  CheckStage(h,config,false);
  if(h.version==1)return SU2NativeBLPipeline3D::MakeRampStage(config,0);
  return h.stage;
}

inline std::vector<char> Encode(const CNativeMetricFamily3D& f,const ReferenceState& reference,const CConfig& config,
    double pre,double final,bool bracketed,size_t ceiling=SU2Native3D::ReferenceStorageLimit3D,size_t caller_bytes=0) {
  using namespace transfer_memory;using namespace detail;
  if(Add(caller_bytes,f.ResidentBytes(),SU2Native3D::ReferenceStorageBytes(reference),f.nodes.size())>ceiling)
    throw std::length_error("Native target validation exceeds storage admission.");
  f.ValidateCheckpoint(config,reference,f.geometry);
  const auto& p=*f.first_row;
  const auto retained=Add(caller_bytes,f.ResidentBytes());
  const auto authority=SU2Native3D::EncodeReference(reference,ceiling,retained);
  Header h;h.reference_bytes=authority.size();h.nodes=f.nodes.size();h.owned=f.owned;h.span=f.node_span;
  h.layers=f.layer_controls.size();h.roles=p.roles.size();h.controls=f.controls;
  h.family={{f.surface_tolerance,f.feature_angle,f.lower,f.upper,f.core,f.inherited_tangent_scale,f.tangential_anchor,f.normalization.logScale}};
  h.feature=f.feature_identity;h.status=uint64_t(f.normalization.status);h.trials=f.normalization.trials;
  h.budget_status=uint64_t(f.budget_selection.status);h.budget_trials=f.budget_selection.trials;h.budget_refinements=f.budget_selection.refinements;
  h.estimate={{f.normalization.estimate.value,f.normalization.estimate.uncertainty,f.normalization.estimate.minimum}};
  h.budget_estimate={{f.budget_selection.estimate.value,f.budget_selection.estimate.uncertainty,f.budget_selection.estimate.minimum}};
  h.budget_parameter=f.budget_selection.parameter;h.pre=pre;h.final=final;h.bracketed=bracketed;
  const double radius=double(p.A->radius);
  if(static_cast<long double>(radius)!=p.A->radius)throw std::invalid_argument("Native target radius is not a binary64 profile value.");
  h.policy={{p.core,p.h0,p.outer_parameter,p.inherited_tangent_scale,radius}};h.full=State(*p.full);h.base=State(*p.base);
  h.witness_versions=f.witness_versions;h.witness_policy={{f.witness_core,f.witness_radius}};
  h.stage=SU2NativeBLPipeline3D::MakeRampStage(config,config.GetAdap_BL_RampEvent());
  if(Add(retained,SU2Native3D::ReferenceStorageBytes(reference),authority.capacity(),
      Bytes(p.full->GetProfileHistory()),Bytes(p.base->GetProfileHistory()))>ceiling)
    throw std::length_error("Native profile history copying exceeds storage admission.");
  auto full=p.full->GetProfileHistory(),base=p.base->GetProfileHistory();
  auto walk=[&](RecordStream& stream) {
    stream(h);
    for(auto layer:f.layer_controls) {
      String(stream,layer.marker,false,SIZE_MAX);double a=SU2_TYPE::GetValue(layer.firstHeight),b=SU2_TYPE::GetValue(layer.growth),c=SU2_TYPE::GetValue(layer.thickness);stream(a,b,c);
    }
    for(auto role:p.roles){int marker=role.first,kind=role.second;stream(marker,kind);}
    History(stream,full,false,SIZE_MAX);History(stream,base,false,SIZE_MAX);
    for(auto node:f.nodes) {
      stream(node.id,node.local,node.point.x,node.point.y,node.point.z);
      auto selected=f.selected.at(node.local);Matrix(stream,selected,false);
      auto witness=f.composed_witnesses.at(node.local);Matrix(stream,witness,false);
      if(node.local<f.owned) {
        auto frame=f.frames.at(node.local);Matrix(stream,frame,false);
        for(unsigned k=0;k<3;++k){double value=SU2_TYPE::GetValue(f.eigenvalues.at(3*node.local+k));if(!(value>=0)||!std::isfinite(value))throw std::invalid_argument("Invalid original native metric eigenvalue.");stream(value);}
      }
    }
  };
  RecordStream measure;walk(measure);const auto size=Add(measure.Size(),authority.size(),8);
  CheckHeader(h,config,size,reference.marker_names.size());
  if(Add(retained,SU2Native3D::ReferenceStorageBytes(reference),Mul(size,2),authority.capacity())>ceiling)
    throw std::length_error("Native target checkpoint encoding exceeds storage admission.");
  std::vector<char> bytes;bytes.reserve(size);RecordStream stream(bytes);walk(stream);
  bytes.insert(bytes.end(),authority.begin(),authority.end());uint64_t checksum=Checksum(bytes,bytes.size());stream(checksum);
  if(bytes.size()!=size)throw std::logic_error("Native target checkpoint size changed during encoding.");
  return bytes;
}

inline Checkpoint Decode(const std::vector<char>& bytes,const ReferenceState& reference,const CConfig& config,
    size_t ceiling=SU2Native3D::ReferenceStorageLimit3D,size_t caller_bytes=0) {
  using namespace transfer_memory;using namespace detail;
  if(bytes.size()<24 || Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),Mul(bytes.size(),6))>ceiling)
    throw std::length_error("Native target checkpoint decoding exceeds storage admission.");
  uint64_t checksum=0;std::memcpy(&checksum,bytes.data()+bytes.size()-8,8);
  if(checksum!=Checksum(bytes,bytes.size()-8))throw std::invalid_argument("Damaged native target checkpoint.");
  RecordStream stream(bytes);Header h;stream(h);CheckHeader(h,config,bytes.size(),reference.marker_names.size());
  const auto allocation=Add(GrowthBound(h.nodes,sizeof(CNativeMetricFamily3D::Node)+2*sizeof(CBoundaryLayerMetric::Tensor)+1),
      GrowthBound(h.owned,sizeof(CBoundaryLayerMetric::Tensor)+3*sizeof(su2double)),Mul(reference.facets.size(),16384));
  if(Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),Mul(bytes.size(),6),allocation)>ceiling)
    throw std::length_error("Native target reconstruction exceeds storage admission.");
  Checkpoint result;result.stage=h.version>=2?h.stage:SU2NativeBLPipeline3D::MakeRampStage(config,0);
  auto f=std::make_shared<CNativeMetricFamily3D>();
  f->controls=h.controls;f->surface_tolerance=h.family[0];f->feature_angle=h.family[1];f->lower=h.family[2];f->upper=h.family[3];
  f->core=h.family[4];f->inherited_tangent_scale=h.family[5];f->tangential_anchor=h.family[6];f->normalization.logScale=h.family[7];
  f->normalization.status=SU2Complexity::Status(h.status);f->normalization.trials=unsigned(h.trials);
  f->normalization.estimate={h.estimate[0],h.estimate[1],h.estimate[2]};f->budget_selection.status=SU2Complexity::BudgetStatus(h.budget_status);
  f->budget_selection.parameter=h.budget_parameter;f->budget_selection.trials=unsigned(h.budget_trials);f->budget_selection.refinements=unsigned(h.budget_refinements);
  f->budget_selection.estimate={h.budget_estimate[0],h.budget_estimate[1],h.budget_estimate[2]};f->owned=h.owned;f->node_span=h.span;f->boundary_layer=true;f->feature_identity=h.feature;
  f->witness_versions=h.witness_versions;f->witness_core=h.witness_policy[0];f->witness_radius=h.witness_policy[1];
  for(size_t k=0;k<h.layers;++k) {
    auto layer=config.GetAdap_BL(k);std::string name;String(stream,name,true,layer.marker.size());double a=0,b=0,c=0;stream(a,b,c);
    if(name!=layer.marker || a!=layer.firstHeight || b!=layer.growth || c!=layer.thickness)throw std::invalid_argument("Checkpoint wall constraints differ from configuration.");
    f->layer_controls.push_back(layer);
  }
  std::map<int,int> roles;
  for(size_t k=0;k<h.roles;++k){int marker=0,kind=0;stream(marker,kind);if(!roles.emplace(marker,kind).second)throw std::invalid_argument("Duplicate checkpoint marker role.");}
  if(roles!=SU2NativeBLPipeline3D::MarkerRoles(config,reference))throw std::invalid_argument("Checkpoint marker roles differ from configuration.");
  std::vector<CBoundaryLayerMetric::ProfileChange> full,base;History(stream,full,true,bytes.size());History(stream,base,true,bytes.size());
  f->nodes.reserve(h.nodes);f->selected.resize(h.nodes);f->composed_witnesses.resize(h.nodes);f->frames.resize(h.owned);f->eigenvalues.resize(Mul(h.owned,3));std::vector<bool> seen(h.nodes,false);
  for(size_t k=0;k<h.nodes;++k) {
    CNativeMetricFamily3D::Node node;stream(node.id,node.local,node.point.x,node.point.y,node.point.z);
    if(node.local>=h.nodes || seen[node.local] || node.id>=h.span || (k && node.id<=f->nodes.back().id) ||
        !std::isfinite(node.point.x)||!std::isfinite(node.point.y)||!std::isfinite(node.point.z))throw std::invalid_argument("Invalid checkpoint source node.");
    seen[node.local]=true;f->nodes.push_back(node);auto& selected=f->selected[node.local];Matrix(stream,selected,true);
    Matrix(stream,f->composed_witnesses[node.local],true);
    if(node.local<h.owned) {
      auto& frame=f->frames[node.local];Matrix(stream,frame,true);
      for(unsigned j=0;j<3;++j){double value=0;stream(value);if(!(value>=0)||!std::isfinite(value))throw std::invalid_argument("Invalid checkpoint eigenvalue.");f->eigenvalues[3*node.local+j]=value;}
    }
  }
  if(Add(stream.Size(),h.reference_bytes,8)!=bytes.size())throw std::invalid_argument("Native checkpoint length/trailing data mismatch.");
  const auto authority=SU2Native3D::EncodeReference(reference,ceiling,Add(caller_bytes,bytes.size(),allocation));
  if(authority.size()!=h.reference_bytes || !std::equal(authority.begin(),authority.end(),bytes.begin()+stream.Size()))
    throw std::invalid_argument("Checkpoint geometry/association authority differs from source.");
  auto p=std::make_shared<SU2NativeBLPipeline3D::TargetPolicy>();p->authority=reference.original;
  p->full=CBoundaryLayerMetric::FromNativeReference(reference,config);p->base=CBoundaryLayerMetric::FromNativeReference(reference,config,179.999);
  p->full->ReplayProfile(full);p->base->ReplayProfile(base);
  if(!(State(*p->full)==h.full) || !(State(*p->base)==h.base) || p->full->GetFeatures().identity!=h.feature)
    throw std::invalid_argument("Replayed BL target profile differs from checkpoint.");
  p->roles=std::move(roles);p->core=h.policy[0];p->h0=h.policy[1];p->outer_parameter=h.policy[2];p->inherited_tangent_scale=h.policy[3];
  double height=INFINITY;for(const auto& layer:f->layer_controls)height=std::min(height,SU2_TYPE::GetValue(layer.firstHeight));
  if(height!=p->h0 || p->core<p->full->GetOuterTransitionCore() || p->core<p->base->GetOuterTransitionCore() ||
      p->inherited_tangent_scale!=p->full->GetTangentialScale() || p->base->GetFeatures().segments)
    throw std::invalid_argument("Inconsistent checkpoint BL composition controls.");
  // Discover immutable creases first, then admit the existing quadratic strip workspace and spatial index.
  p->A=dc10::BuildOptionA(reference.facets,p->roles,p->full->GetFeatures().angle,0,p->h0);
  const auto strips=GrowthBound(Mul(p->A->creases.size(),p->A->creases.size()),4*sizeof(dc10::StripPair));
  const auto retained=Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),Mul(bytes.size(),6),allocation,p->StorageBytes());
  if(Add(retained,strips)>ceiling)throw std::length_error("Restored target strip preparation exceeds storage admission.");
  p->A->ConfigureRadius(h.policy[4],[&](size_t grid) {
    if(Add(retained,strips,grid)>ceiling)throw std::length_error("Restored target spatial index exceeds storage admission.");
  });
  p->A->distanceLayers=p->base;p->A->core=p->core;f->first_row=p;f->layers=p->full;f->authority=reference.original;
  f->target_extra_bytes=p->StorageBytes()-p->full->StorageBytes();
  f->ValidateCheckpoint(config,reference);
  if(Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),Mul(bytes.size(),6),f->ResidentBytes())>ceiling)
    throw std::length_error("Restored native target exceeds storage admission.");
  result.family=std::move(f);result.pre_complexity=h.pre;result.final_complexity=h.final;result.bracketed=h.bracketed;
  return result;
}

inline Checkpoint DecodeRestoringStage(const std::vector<char>& bytes,const ReferenceState& reference,CConfig& config,
    size_t ceiling=SU2Native3D::ReferenceStorageLimit3D,size_t caller_bytes=0) {
  const auto stage=InspectStage(bytes,config);const auto previous=config.GetAdap_BL_RampEvent();
  config.SetAdap_BL_RampEvent(stage.event);
  try{return Decode(bytes,reference,config,ceiling,caller_bytes);}
  catch(...){config.SetAdap_BL_RampEvent(previous);throw;}
}

inline void WriteInitialContinuation(const std::string& path,const std::string& meshPath,
    const Checkpoint& checkpoint,const ReferenceState& reference,const CGeometry& source,const CConfig& config) {
  if(!checkpoint.family || checkpoint.stage.event!=0 || !checkpoint.stage.Matches(config))
    throw std::invalid_argument("Seed continuation needs a validated event-0 target checkpoint matching this configuration.");
  checkpoint.family->ValidateCheckpoint(config,reference,&source);
  SU2NativeBLPipeline3D::WriteInitialContinuation(path,meshPath,checkpoint.stage,config);
}

inline void Write(const std::string& path,const std::vector<char>& bytes) {
  if(std::filesystem::exists(path))throw std::runtime_error("Refusing to overwrite a native target checkpoint.");
  const auto temporary=path+".tmp";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
  out.write(bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Failed to write native target checkpoint.");
  std::filesystem::rename(temporary,path);
}
inline Checkpoint Read(const std::string& path,const ReferenceState& reference,const CConfig& config,
    size_t ceiling=SU2Native3D::ReferenceStorageLimit3D,size_t caller_bytes=0) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input)throw std::runtime_error("Missing native target checkpoint.");
  const auto length=input.tellg();if(length<0 || uint64_t(length)>ceiling ||
      transfer_memory::Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),transfer_memory::Mul(size_t(length),6))>ceiling)
    throw std::length_error("Native target file exceeds storage admission.");
  std::vector<char> bytes(size_t(length),char(0));input.seekg(0);input.read(bytes.data(),bytes.size());
  if(!input)throw std::runtime_error("Truncated native target checkpoint read.");
  return Decode(bytes,reference,config,ceiling,caller_bytes);
}
inline Checkpoint ReadRestoringStage(const std::string& path,const ReferenceState& reference,CConfig& config,
    size_t ceiling=SU2Native3D::ReferenceStorageLimit3D,size_t caller_bytes=0) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input)throw std::runtime_error("Missing native target checkpoint.");
  const auto length=input.tellg();if(length<0 || uint64_t(length)>ceiling ||
      transfer_memory::Add(caller_bytes,SU2Native3D::ReferenceStorageBytes(reference),transfer_memory::Mul(size_t(length),6))>ceiling)
    throw std::length_error("Native target file exceeds storage admission.");
  std::vector<char> bytes(size_t(length),char(0));input.seekg(0);input.read(bytes.data(),bytes.size());
  if(!input)throw std::runtime_error("Truncated native target checkpoint read.");
  return DecodeRestoringStage(bytes,reference,config,ceiling,caller_bytes);
}
}  // namespace SU2NativeTargetCheckpoint3D
