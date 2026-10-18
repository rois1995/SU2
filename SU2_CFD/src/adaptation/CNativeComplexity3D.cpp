/*!
 * \file CNativeComplexity3D.cpp
 * \brief Retained geometry, fresh trial tensors, globally assessed bounded complexity work.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeComplexity3D.hpp"
#include "../../../Common/include/adaptation/CNativeDonor3D.hpp"
#include "../../../Common/include/adaptation/CNativeSupport3D.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include <map>

namespace {
using namespace SU2Native3D;
using Metric=CBoundaryLayerMetric::Tensor;
void Vote(World& world,const CLocalFailure& failure) {
  const auto elected=world.elect(failure);
  if(elected.any)throw std::runtime_error(elected.message);
}
struct Request {
  Id cell=0;Point point;double inversePDF=0;
  template<class S>void Fields(S& s){s(cell,point,inversePDF);}
};
double LogDensity(const Metric& metric) {
  su2double vectors[3][3],values[3],work[3];
  CBlasStructure::EigenDecomposition(metric.m,vectors,values,3,work);
  double logarithm=0;
  for(auto x:values) {
    const auto value=SU2_TYPE::GetValue(x);
    if(!(value>0) || !std::isfinite(value))throw std::runtime_error("Non-SPD complexity trial tensor.");
    logarithm+=std::log(value);
  }
  return .5*logarithm;
}
Metric Interpolate(const std::array<unsigned long,4>& nodes,const std::array<double,4>& weights,
                   const std::vector<Metric>& sensor) {
  Metric metric;
  for(unsigned k=0;k<4;++k)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
    metric.m[i][j]+=weights[k]*sensor.at(nodes[k]).m[i][j];
  return metric;
}
// Lower-triangle convention matches EigenDecomposition. Scaling/long double protect background determinants.
long double Determinant(const Metric& metric,long double* condition=nullptr) {
  long double scale=0;
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<=i;++j)scale=std::max(scale,std::abs((long double)SU2_TYPE::GetValue(metric.m[i][j])));
  if(!(scale>0) || !std::isfinite(scale))throw std::runtime_error("Invalid background sensor scale.");
  const long double a=SU2_TYPE::GetValue(metric.m[0][0])/scale,b=SU2_TYPE::GetValue(metric.m[1][0])/scale,
      c=SU2_TYPE::GetValue(metric.m[2][0])/scale,d=SU2_TYPE::GetValue(metric.m[1][1])/scale,
      e=SU2_TYPE::GetValue(metric.m[2][1])/scale,f=SU2_TYPE::GetValue(metric.m[2][2])/scale;
  if(!(a>0))throw std::runtime_error("Non-SPD original background sensor.");
  const auto pivot=d-b*b/a,off=e-b*c/a,last=f-c*c/a;
  if(!(pivot>0))throw std::runtime_error("Non-SPD original background sensor.");
  const auto final=last-off*off/pivot;
  if(!(final>0) || !std::isfinite(final))throw std::runtime_error("Non-SPD original background sensor.");
  if(condition) {
    const auto l10=b/a,l20=c/a,l21=off/pivot;
    const auto norm=std::max({std::abs(a)+std::abs(b)+std::abs(c),std::abs(b)+std::abs(d)+std::abs(e),
                              std::abs(c)+std::abs(e)+std::abs(f)});
    const auto traceInverse=1/a+(1+l10*l10)/pivot+(1+l21*l21+std::pow(l20-l21*l10,2))/final;
    *condition=std::max(*condition,norm*traceInverse);
  }
  const auto result=a*pivot*final*scale*scale*scale;
  if(!(result>0) || !std::isfinite(result))throw std::runtime_error("Nonfinite background determinant.");
  return result;
}
struct Rule {std::array<double,4> weights;double volume;};
const std::vector<Rule>& BackgroundRule() {
  static const auto rules=[] {
    std::vector<Rule> result;
    const double x3[]={.5*(1-std::sqrt(.6)),.5,.5*(1+std::sqrt(.6))},w3[]={5./18,4./9,5./18};
    const double x2[]={.5*(1-1/std::sqrt(3.)),.5*(1+1/std::sqrt(3.))};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(auto w:x2) {
      const auto u=x3[i],v=x3[j];result.push_back({{1-u,u*(1-v),u*v*(1-w),u*v*w},3*u*u*v*w3[i]*w3[j]});
    }
    return result;
  }();return rules;
}
} // namespace

struct CNativeComplexity3D::Preparation {
  World world;
  DonorImport donor{world};
  std::vector<DonorCell> geometry;
  std::map<Id,size_t> original;
  std::shared_ptr<SupportSampler3D> sampler;
  bool retained=false;
  size_t Bytes()const {
    return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(geometry),
        transfer_memory::Mul(original.size(),sizeof(Id)+sizeof(size_t)+6*sizeof(void*)),donor.ResidentBytes(),
        sampler?sampler->Statistics().retained_bytes:0);
  }
};
CNativeComplexity3D::~CNativeComplexity3D()=default;
size_t CNativeComplexity3D::ResidentBytes()const {
  auto bytes=transfer_memory::Add(transfer_memory::Bytes(source),transfer_memory::Bytes(volumes),
      transfer_memory::Bytes(queries),transfer_memory::Bytes(backgroundSensor),preparation?preparation->Bytes():0);
  for(const auto& q:queries)bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q.geometry));
  return bytes;
}
void CNativeComplexity3D::PreparationCounts() {
  if(!preparation)return;
  statistics.preparationBytes=preparation->Bytes();
  const auto& d=preparation->donor.Statistics();statistics.donorBuilds=d.builds;statistics.donorBatches=d.imports;
  if(preparation->sampler) {
    const auto& s=preparation->sampler->Statistics();statistics.supportWork=s.preparation_work;statistics.supportCandidates=s.pdf_candidates;
  }
  statistics.retainedBytes=ResidentBytes();
}
CNativeComplexity3D::CNativeComplexity3D(CBoundaryLayerMetric& model,std::vector<SourceCell> cells,
    size_t nNodes,size_t maximumBytes,size_t count,uint64_t streamSeed,bool retain)
    :initialSamples(count),seed(streamSeed),layers(model),source(std::move(cells)),sourceNodes(nNodes),ceiling(maximumBytes) {
  World world;CLocalFailure failure;const auto start=world.seconds();
  const uint64_t controls[]={maximumBytes,count,seed,uint64_t(retain)};uint64_t low[4],high[4];
  SU2_MPI::Allreduce(controls,low,4,MPI_UNSIGNED_LONG_LONG,MPI_MIN,world.comm);
  SU2_MPI::Allreduce(controls,high,4,MPI_UNSIGNED_LONG_LONG,MPI_MAX,world.comm);
  if(!std::equal(low,low+4,high) || count<32 || count>1048576 || ceiling<4096)
    failure.Set(2,0,"Invalid/inconsistent bounded-complexity controls.");
  Vote(world,failure);
  try {
    preparation=std::make_unique<Preparation>();preparation->retained=retain;
    if(transfer_memory::Add(transfer_memory::Bytes(source),transfer_memory::Mul(source.size(),sizeof(DonorCell)+sizeof(Id)+6*sizeof(void*)+sizeof(long double)))>ceiling)
      throw std::runtime_error("Complexity source exceeds storage ceiling.");
    preparation->geometry.reserve(source.size());volumes.reserve(source.size());
    for(size_t i=0;i<source.size();++i) {
      auto& row=source[i];auto& v=row.cell.v;
      for(auto node:row.nodes)if(node>=sourceNodes)throw std::runtime_error("Invalid original complexity node.");
      const auto sign=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
      if(!sign)throw std::runtime_error("Degenerate original complexity cell.");
      if(sign<0){std::swap(v[0],v[1]);std::swap(row.nodes[0],row.nodes[1]);}
      volumes.push_back(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)/6);
      if(!preparation->original.emplace(row.cell.id,i).second)throw std::runtime_error("Duplicate original complexity cell.");
      DonorCell locator;locator.cell=row.cell;locator.sensor.fill({1,0,0,1,0,1});preparation->geometry.push_back(locator);
    }
  }catch(const std::exception& error){failure.Set(2,0,error.what());}Vote(world,failure);
  std::string reason;
  if(!preparation->donor.Build(preparation->geometry,reason,ceiling,ResidentBytes()))throw std::runtime_error(reason);
  if(!world.rank)try {
    const auto held=ResidentBytes();if(held>=ceiling)throw SupportLimit3D("Complexity support exceeds storage ceiling.");
    preparation->sampler=layers.PrepareCorrectionSupport3D(std::min(ceiling/2,ceiling-held),80000000);
  }catch(const std::exception& error){failure.Set(2,0,error.what());}Vote(world,failure);
  Extend(count,reason,true);
  PreparationCounts();
  if(!retain){preparation.reset();statistics.preparationBytes=0;statistics.retainedBytes=ResidentBytes();}
  profileVersion=layers.GetOuterNormal().version;
  statistics.preparationSeconds=world.seconds()-start;
}
bool CNativeComplexity3D::Extend(size_t count,std::string& reason,bool initial) {
  auto& p=*preparation;auto& world=p.world;CLocalFailure failure;
  auto vote=[&]() {
    const auto e=world.elect(failure);if(!e.any)return true;
    if(!initial && e.severity==3){reason=e.message;return false;}
    throw std::runtime_error(e.message);
  };
  auto resource=[](const std::string& message){return message.find("budget")!=std::string::npos || message.find("ceiling")!=std::string::npos;};
  const auto predicted=transfer_memory::Add(ResidentBytes(),transfer_memory::GrowthBound(count,sizeof(Query)),
      transfer_memory::GrowthBound(count,sizeof(Request)),transfer_memory::Mul(count,sizeof(SupportPoint3D)));
  if(predicted>ceiling)failure.Set(3,0,"Complexity extension exceeds preparation storage ceiling.");
  if(!vote())return false;
  std::vector<std::vector<Request>> distribution;
  try {
    distribution.resize(world.size);
    if(!world.rank) {
      const auto draws=p.sampler->Draw(count,seed,statistics.samples);
      for(size_t i=0;i<draws.size();++i)distribution[(statistics.samples+i)%world.size].push_back({i,draws[i].point,draws[i].inverse_pdf});
    }
  }catch(const SupportLimit3D& error){failure.Set(3,0,error.what());}
   catch(const std::bad_alloc& error){failure.Set(3,0,error.what());}
   catch(const std::exception& error){failure.Set(4,0,error.what());}
  if(!vote())return false;
  PreparationCounts();
  if(initial && !p.retained && !world.rank)p.sampler.reset();
  size_t held=transfer_memory::Add(ResidentBytes(),transfer_memory::GrowthBound(count,sizeof(Query)));
  bool admitted=false;const auto draws=world.exchange(distribution,&admitted,ceiling,held);
  if(!admitted)failure.Set(3,0,"Complexity proposal distribution exceeds storage ceiling.");if(!vote())return false;
  std::vector<std::vector<Request>>().swap(distribution);held=transfer_memory::Add(held,transfer_memory::Bytes(draws));
  const auto rounds=(count+8*world.size-1)/(8*world.size);uint64_t localInside=0;std::vector<Query> extra;size_t extraGeometry=0;
  for(size_t round=0;round<rounds;++round) {
    std::vector<Point> points;
    try {
      points.reserve(8);
      for(size_t j=8*round;j<std::min(draws.size(),8*(round+1));++j)points.push_back(draws[j].point);
    }catch(const std::bad_alloc& error){failure.Set(3,round,error.what());}
     catch(const std::exception& error){failure.Set(4,round,error.what());}
    if(!vote())return false;
    const auto footprint=p.donor.ProbePoints(points,ceiling,transfer_memory::Add(held,transfer_memory::Bytes(extra),extraGeometry));
    if(!footprint.valid) {
      if(!initial && resource(footprint.reason)){reason=footprint.reason;return false;}
      throw std::runtime_error(footprint.reason);
    }
    std::vector<std::vector<Request>> requests;
    try {
      requests.resize(world.size);
      if(footprint.cells.size()!=footprint.source_rank.size())throw std::runtime_error("Missing source owner for complexity query.");
      for(size_t j=0;j<points.size();++j) {
        size_t best=SIZE_MAX;std::array<Id,4> key;key.fill(UINT64_MAX);
        for(size_t i=0;i<footprint.cells.size();++i) {
          const auto& candidate=footprint.cells[i];const auto& v=candidate.cell.v;std::array<long double,4> w;
          if(candidate.Key()<key && Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},points[j],w)){best=i;key=candidate.Key();}
        }
        if(best==SIZE_MAX)continue;
        ++localInside;const auto& draw=draws[8*round+j];
        requests.at(footprint.source_rank[best]).push_back({footprint.cells[best].cell.id,draw.point,draw.inversePDF});
      }
    }catch(const std::bad_alloc& error){failure.Set(3,round,error.what());}
     catch(const std::exception& error){failure.Set(4,round,error.what());}if(!vote())return false;
    const auto incoming=world.exchange(requests,&admitted,ceiling,transfer_memory::Add(held,transfer_memory::Bytes(extra),extraGeometry));
    if(!admitted)failure.Set(3,round,"Complexity stencil routing exceeds storage ceiling.");if(!vote())return false;
    try {
      for(const auto& request:incoming) {
        const auto& row=source.at(p.original.at(request.cell));const auto& v=row.cell.v;std::array<long double,4> weights;
        if(!Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},request.point,weights))throw std::runtime_error("Complexity query leaves original source.");
        Query q;q.nodes=row.nodes;q.point=request.point;q.inversePDF=request.inversePDF;q.batch=batches;
        for(unsigned k=0;k<4;++k)q.weights[k]=double(weights[k]);
        ++statistics.preparationGeometryQueries;q.geometry=layers.SamplePoints({request.point.x,request.point.y,request.point.z});
        const size_t bytes=transfer_memory::Add(held,transfer_memory::Bytes(extra),extraGeometry,transfer_memory::Bytes(q.geometry));
        if(bytes>ceiling)throw SupportLimit3D("Complexity appended stencils exceed storage ceiling.");
        extraGeometry=transfer_memory::Add(extraGeometry,transfer_memory::Bytes(q.geometry));
        extra.push_back(std::move(q));
      }
    }catch(const SupportLimit3D& error){failure.Set(3,round,error.what());}
     catch(const std::bad_alloc& error){failure.Set(3,round,error.what());}
     catch(const std::exception& error){failure.Set(4,round,error.what());}
    if(!vote())return false;
  }
  uint64_t counts[]={localInside,extra.size()},totals[2];
  SU2_MPI::Allreduce(counts,totals,2,MPI_UNSIGNED_LONG_LONG,MPI_SUM,world.comm);
  if(totals[0]!=totals[1])failure.Set(4,0,"Incomplete original complexity stencil routing.");if(!vote())return false;
  try {
    const auto needed=transfer_memory::Add(ResidentBytes(),transfer_memory::GrowthBound(queries.size()+extra.size(),sizeof(Query)),transfer_memory::Bytes(extra));
    size_t bytes=needed;for(const auto& q:extra)bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q.geometry));
    if(bytes>ceiling)throw SupportLimit3D("Complexity append commit exceeds storage ceiling.");
    queries.reserve(queries.size()+extra.size());
  }catch(const SupportLimit3D& error){failure.Set(3,0,error.what());}
   catch(const std::bad_alloc& error){failure.Set(3,0,error.what());}
   catch(const std::exception& error){failure.Set(4,0,error.what());}
  if(!vote())return false;
  static_assert(std::is_nothrow_move_constructible<Query>::value,"Complexity publication must not throw after collective admission.");
  queries.insert(queries.end(),std::make_move_iterator(extra.begin()),std::make_move_iterator(extra.end()));
  batchSizes[batches++]=count;statistics.samples+=count;statistics.inside+=totals[0];PreparationCounts();return true;
}
bool CNativeComplexity3D::Refine(std::string& reason) {
  reason.clear();World world;CLocalFailure failure;
  const uint64_t controls[]={statistics.samples,batches,statistics.refinementAttempts,profileVersion};uint64_t low[4],high[4];
  SU2_MPI::Allreduce(controls,low,4,MPI_UNSIGNED_LONG_LONG,MPI_MIN,world.comm);
  SU2_MPI::Allreduce(controls,high,4,MPI_UNSIGNED_LONG_LONG,MPI_MAX,world.comm);
  if(!std::equal(low,low+4,high) || profileVersion!=layers.GetOuterNormal().version)
    failure.Set(4,0,"Inconsistent/stale complexity refinement stage.");Vote(world,failure);
  if(!preparation || !preparation->retained){reason="sampling_preparation_not_retained";return false;}
  if(statistics.refinementAttempts==2 || statistics.samples>=std::min<size_t>(1048576,4*initialSamples) || statistics.samples>1048576-statistics.samples){reason="sampling_stage_budget";return false;}
  ++statistics.refinementAttempts;const auto start=world.seconds();
  bool valid=false;
  try {valid=Extend(statistics.samples,reason,false);}
  catch(...){PreparationCounts();statistics.preparationSeconds+=world.seconds()-start;throw;}
  PreparationCounts();statistics.preparationSeconds+=world.seconds()-start;
  statistics.refinements+=valid;return valid;
}

CNativeComplexity3D::Assessment CNativeComplexity3D::Assess(const std::vector<CBoundaryLayerMetric::Tensor>& sensor,su2double core,bool audit) {
  World world;CLocalFailure failure;const auto start=world.seconds();long double local[10]={},global[10]={},batchLocal[6]={},batchGlobal[6]={};
  uint64_t counts[2]={},totals[2];
  try {
    if(sensor.size()!=sourceNodes || !(SU2_TYPE::GetValue(core)>0) || !std::isfinite(SU2_TYPE::GetValue(core)))
      throw std::runtime_error("Invalid current complexity trial field/core.");
    const auto version=layers.GetOuterNormal().version;
    if(version!=profileVersion) {
      const auto refreshStart=world.seconds();size_t bytes=transfer_memory::Add(transfer_memory::Bytes(source),
          transfer_memory::Bytes(queries),transfer_memory::Bytes(volumes),transfer_memory::Bytes(backgroundSensor),preparation?preparation->Bytes():0);
      for(auto& q:queries) {
        q.geometry=layers.SamplePoints({q.point.x,q.point.y,q.point.z});++statistics.profileGeometryQueries;
        bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q.geometry));
        if(bytes>ceiling)throw std::runtime_error("Refreshed BL profile samples exceed storage ceiling.");
      }
      statistics.retainedBytes=bytes;profileVersion=version;++statistics.profileRefreshes;
      statistics.preparationSeconds+=world.seconds()-refreshStart;
    }
    const auto backgroundStart=world.seconds();
    bool same=backgroundValid && backgroundSensor.size()==sensor.size();
    for(size_t node=0;same && node<sensor.size();++node)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
      same=same && backgroundSensor[node].m[i][j]==sensor[node].m[i][j];
    if(same) {
      local[0]=background[0];local[1]=background[1];local[2]=background[2];local[6]=background[3];
      ++statistics.backgroundReuses;
    }else {
      const auto previousBytes=transfer_memory::Bytes(backgroundSensor);
      const auto needed=transfer_memory::Mul(sensor.size(),sizeof(CBoundaryLayerMetric::Tensor));
      if(needed>previousBytes && transfer_memory::Add(statistics.retainedBytes,needed-previousBytes)>ceiling)
        throw std::runtime_error("Original sensor background cache exceeds storage ceiling.");
      for(size_t cell=0;cell<source.size();++cell) {
        const auto& row=source[cell];const auto volume=volumes[cell];
        long double rootMean=0,meanDet=0,meanDensity=0,condition=0;
        for(auto node:row.nodes)rootMean+=std::cbrt(Determinant(sensor[node],&condition))/4;
        for(const auto& q:BackgroundRule()) {
          const auto det=Determinant(Interpolate(row.nodes,q.weights,sensor),&condition);
          meanDet+=q.volume*det;meanDensity+=q.volume*std::sqrt(det);
        }
        // Minkowski+Jensen lower and cubic-determinant Jensen upper in exact arithmetic; no outward rounding proof.
        const auto lower=volume*std::pow(rootMean,1.5L),upper=volume*std::sqrt(meanDet);
        // First-order roundoff model for binary64 interpolation and determinant conditioning; NOT a proven bound.
        const auto numerical=64*std::numeric_limits<double>::epsilon()*condition*std::max(lower,upper);
        if(!(lower>0 && upper>0) || lower>upper+numerical)throw std::runtime_error("Invalid background complexity interval beyond numerical indicator.");
        local[0]+=std::min(lower,upper);local[1]+=std::max(lower,upper);local[2]+=volume*meanDensity;local[6]+=numerical;
        statistics.backgroundEvaluations+=BackgroundRule().size()+4;
      }
      backgroundSensor=sensor;
      statistics.retainedBytes=transfer_memory::Add(statistics.retainedBytes-previousBytes,transfer_memory::Bytes(backgroundSensor));
      background={local[0],local[1],local[2],local[6]};backgroundValid=true;++statistics.backgroundBuilds;
    }
    statistics.backgroundSeconds+=world.seconds()-backgroundStart;const auto correctionStart=world.seconds();
    for(const auto& q:queries) {
      const auto sensorMetric=Interpolate(q.nodes,q.weights,sensor);auto composed=sensorMetric;
      for(const auto& sample:q.geometry)layers.ApplySample(sample.wall,sample.sample,composed,core,nullptr,false);
      if(audit) {
        auto direct=sensorMetric;const su2double p[]={q.point.x,q.point.y,q.point.z};layers.ApplyPoint(p,direct,core,nullptr,false);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(direct.m[i][j]!=composed.m[i][j])
          throw std::runtime_error("Retained geometry changed current direct composed tensor.");
        ++counts[1];
      }
      const auto a=LogDensity(sensorMetric),b=LogDensity(composed),difference=b-a;
      if(difference < -4096*std::numeric_limits<double>::epsilon()*std::max({1.,std::abs(a),std::abs(b)}))
        throw std::runtime_error("Complexity composition materially weakens current sensor.");
      const long double y=(long double)std::exp(a)*std::expm1(difference)*q.inversePDF;
      if(!std::isfinite(y))throw std::runtime_error("Nonfinite current correction contribution.");
      batchLocal[q.batch]+=y;local[3]+=y;local[4]+=y*y;local[5]+=y!=0;++counts[0];++statistics.correctionEvaluations;
      if(layers.GetOuterNormal().version) {
        // Density lower contribution from sensor and each individually mandatory wall, not independent BL costs.
        auto mandatory=a;
        for(const auto& sample:q.geometry)if(sample.sample.distance<=layers.GetOuterNormal().protected_distance) {
          CBoundaryLayerMetric::Tensor wall;for(unsigned i=0;i<3;++i)wall.m[i][i]=core;
          layers.ApplySample(sample.wall,sample.sample,wall,core,nullptr,false);
          mandatory=std::max(mandatory,LogDensity(wall));
        }
        const long double z=(long double)std::exp(a)*std::expm1(mandatory-a)*q.inversePDF;
        if(!std::isfinite(z))throw std::runtime_error("Nonfinite mandatory BL contribution.");
        batchLocal[3+q.batch]+=z;local[7]+=z;local[8]+=z*z;local[9]+=z!=0;
      }
    }
    statistics.correctionSeconds+=world.seconds()-correctionStart;
  }catch(const std::exception& error){failure.Set(2,statistics.assessments,error.what());}Vote(world,failure);
  const auto reductionStart=world.seconds();
  SU2_MPI::Allreduce(local,global,10,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
  SU2_MPI::Allreduce(counts,totals,2,MPI_UNSIGNED_LONG_LONG,MPI_SUM,world.comm);
  if(totals[0]!=statistics.inside)failure.Set(2,0,"Incomplete complexity trial.");Vote(world,failure);
  SU2_MPI::Allreduce(batchLocal,batchGlobal,6,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
  statistics.reductionSeconds+=world.seconds()-reductionStart;
  const long double count=statistics.samples,mean=global[3]/count;
  const auto sem=std::sqrt(std::max(0.L,(global[4]-global[3]*mean)/(count*(count-1))));
  auto between=[&](unsigned offset,long double mean) {
    if(batches<2)return 0.L;
    long double scatter=0;for(unsigned k=0;k<batches;++k)scatter+=batchSizes[k]*std::pow(batchGlobal[offset+k]/batchSizes[k]-mean,2);
    return std::sqrt(scatter/((batches-1)*count));
  };
  Assessment result;result.batches=batches;result.betweenBatchError=double(3*between(0,mean));result.sensorLower=double(global[0]);result.sensorUpper=double(global[1]);result.sensorQuadrature=double(global[2]);
  result.correction=double(mean);result.standardError=double(sem);result.numericalIndicator=double(global[6]);result.samples=statistics.samples;result.inside=totals[0];
  result.nonzero=uint64_t(global[5]);result.directChecks=totals[1];
  result.global={double(mean+(global[0]+global[1])/2),result.nonzero<32?INFINITY:double(std::max(3*sem,(long double)result.betweenBatchError)+(global[1]-global[0])/2+global[6])};
  if(layers.GetOuterNormal().version) {
    const auto m=global[7]/count;
    const auto error=std::sqrt(std::max(0.L,(global[8]-global[7]*m)/(count*(count-1))));
    result.mandatory={double(m+(global[0]+global[1])/2),global[9]<32?INFINITY:double(3*std::max(error,between(3,m))+(global[1]-global[0])/2+global[6])};
  }
  if(!(result.global.value>0) || !std::isfinite(result.global.value) || std::isnan(result.global.uncertainty))
    throw std::runtime_error("Invalid global bounded complexity estimate.");
  ++statistics.assessments;statistics.assessmentSeconds+=world.seconds()-start;return result;
}
