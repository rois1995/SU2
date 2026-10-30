/* Bounded diagnostic census of unique construction edges; never an acceptance gate. */
#pragma once
#include "CNativeDistributed3D.hpp"
#include "CNativeMetric3D.hpp"
#include "CTransferMemory.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace SU2Native3D {
struct EdgeProgressSample {
  Edge edge{}; double length=0; bool surface=false; uint64_t age=0; unsigned disposition=0; uint64_t disposition_round=0;
  template<class S> void Fields(S& s){s(edge,length,surface,age,disposition,disposition_round);}
};
struct GenerationProgress3D {
  uint64_t unique_edges=0,long_surface=0,long_interior=0;
  double hinge_squared=0,max_length=0;
  std::array<uint64_t,3> histogram{};
  std::vector<EdgeProgressSample> top;
};
namespace generation_progress_detail {
struct Summary {
  uint64_t unique_edges=0,long_surface=0,long_interior=0;
  double hinge_squared=0,max_length=0;
  std::array<uint64_t,3> histogram{};
  unsigned count=0;std::array<EdgeProgressSample,8> top{};
  template<class S> void Fields(S& s){s(unique_edges,long_surface,long_interior,hinge_squared,max_length,histogram,count,top);}
};
inline bool Earlier(const EdgeProgressSample& a,const EdgeProgressSample& b) {
  return a.length!=b.length ? a.length>b.length : a.edge<b.edge;
}
inline size_t Owner(Edge edge,int ranks) {
  uint64_t h=1469598103934665603ULL;
  for(auto id:edge){h^=id;h*=1099511628211ULL;h^=id>>32;h*=1099511628211ULL;}
  return size_t(h%uint64_t(ranks));
}
}
inline bool MeasureGenerationProgress(World& world,const std::vector<EdgeProgressSample>& samples,
    GenerationProgress3D& output,std::string& reason,size_t ceiling=SIZE_MAX,size_t caller_bytes=0) {
  using namespace generation_progress_detail;
  CLocalFailure failure;
  auto vote=[&]() {const auto elected=world.elect(failure);if(elected.any)reason=elected.message;return !elected.any;};
  auto admit=[&](size_t n) {if(n==SIZE_MAX || n>ceiling)failure.Set(3,0,"Generation progress exceeds storage admission.");return vote();};
  const auto base=transfer_memory::Add(caller_bytes,transfer_memory::Bytes(samples));
  const auto packing=transfer_memory::Add(base,transfer_memory::Mul(world.size,sizeof(std::vector<EdgeProgressSample>)),
      transfer_memory::GrowthBound(samples.size(),sizeof(EdgeProgressSample)));
  if(!admit(packing))return false;
  std::vector<std::vector<EdgeProgressSample>> send;
  try {
    send.resize(world.size);
    for(auto sample:samples) {
      std::sort(sample.edge.begin(),sample.edge.end());
      if(sample.edge[0]==sample.edge[1] || !std::isfinite(sample.length) || sample.length<0)
        throw std::invalid_argument("Invalid generation progress edge sample.");
      send[Owner(sample.edge,world.size)].push_back(sample);
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!vote())return false;
  bool accepted=false;auto received=world.exchange(send,&accepted,ceiling,base);
  if(!accepted){reason="Generation progress exchange exceeds storage admission.";return false;}
  const auto live=transfer_memory::Add(base,transfer_memory::Bytes(send),transfer_memory::Bytes(received),
      transfer_memory::Mul(received.size(),sizeof(std::pair<const Edge,EdgeProgressSample>)+4*sizeof(void*)),
      transfer_memory::Mul(9,sizeof(EdgeProgressSample)),2*sizeof(Summary));
  if(!admit(live))return false;
  Summary local;
  try {
    std::map<Edge,EdgeProgressSample> edges;
    for(const auto& sample:received) {
      auto inserted=edges.emplace(sample.edge,sample);
      if(!inserted.second){auto& a=inserted.first->second;a.length=std::max(a.length,sample.length);
        a.surface=a.surface||sample.surface;a.age=std::max(a.age,sample.age);
        if(sample.disposition_round>a.disposition_round){a.disposition=sample.disposition;a.disposition_round=sample.disposition_round;}
        else if(sample.disposition_round==a.disposition_round)a.disposition=std::max(a.disposition,sample.disposition);}
    }
    std::vector<EdgeProgressSample> top;top.reserve(std::min<size_t>(9,edges.size()));
    for(const auto& row:edges) {
      const auto& a=row.second;++local.unique_edges;local.max_length=std::max(local.max_length,a.length);
      const unsigned bucket=a.length<=MaximumMetricEdge?0:a.length<=2*MaximumMetricEdge?1:2;++local.histogram[bucket];
      if(bucket){if(a.surface)++local.long_surface;else ++local.long_interior;
        const double h=a.length/MaximumMetricEdge-1;local.hinge_squared+=h*h;}
      top.push_back(a);std::sort(top.begin(),top.end(),Earlier);if(top.size()>8)top.pop_back();
    }
    if(!std::isfinite(local.hinge_squared))throw std::overflow_error("Generation progress hinge overflow.");
    local.count=unsigned(top.size());std::copy(top.begin(),top.end(),local.top.begin());
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!vote())return false;
  const auto summary_live=transfer_memory::Add(live,transfer_memory::Mul(world.size,sizeof(std::vector<Summary>)+3*sizeof(Summary)),
      transfer_memory::Mul(8*size_t(world.size),sizeof(EdgeProgressSample)));
  if(!admit(summary_live))return false;
  std::vector<std::vector<Summary>> summaries;
  try{summaries.assign(world.size,{local});}catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!vote())return false;
  auto all=world.exchange(summaries,&accepted,ceiling,live);
  if(!accepted){reason="Generation progress summary exchange exceeds storage admission.";return false;}
  GenerationProgress3D result;
  try {
    result.top.reserve(8*size_t(world.size));
    for(const auto& s:all){result.unique_edges+=s.unique_edges;result.long_surface+=s.long_surface;result.long_interior+=s.long_interior;
      result.hinge_squared+=s.hinge_squared;result.max_length=std::max(result.max_length,s.max_length);
      for(unsigned i=0;i<3;++i)result.histogram[i]+=s.histogram[i];
      for(unsigned i=0;i<s.count;++i)result.top.push_back(s.top[i]);}
    if(!std::isfinite(result.hinge_squared))throw std::overflow_error("Generation progress total hinge overflow.");
    std::sort(result.top.begin(),result.top.end(),Earlier);if(result.top.size()>8)result.top.resize(8);
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!vote())return false;
  output=std::move(result);reason.clear();return true;
}
}
