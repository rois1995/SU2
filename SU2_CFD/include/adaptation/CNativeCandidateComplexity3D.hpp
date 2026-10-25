/*!
 * \file CNativeCandidateComplexity3D.hpp
 * \brief Geometry-only original-donor stencils for bounded normalization on a candidate partition.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeMetricFamily3D.hpp"
#include "CNativeBLTarget3D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"

/*! Preparation verifies original publisher/cell/node bindings. Trials reuse stencils and geometry;
 * no source index, donor tracing, wall search or candidate-node tensor fit occurs during assessment. */
class CNativeCandidateComplexity3D {
  struct Query {
    std::array<size_t,4> nodes;
    std::array<long double,4> weights;
    SU2Native3D::Point point;
    double inverse_pdf=0;
    uint64_t row=0;
    std::vector<CBoundaryLayerMetric::PointSample> geometry,baseGeometry;
    long double weight=0;
  };
  const CNativeMetricFamily3D& family;
  uint64_t count,profileVersion=0;
  size_t storageCeiling=0,callerBytes=0;
  std::vector<Query> queries;
 public:
  uint64_t source_span=0;
  struct Statistics {
    uint64_t assessments=0,density_queries=0,geometry_queries=0;
    double preparation_seconds=0,assessment_seconds=0,reduction_seconds=0;
    double point_SE=0,block_SE=0,numerical_indicator=0;
  } stats;
  size_t ResidentBytes()const {
    size_t bytes=transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(queries));
    for(const auto& q:queries)bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q.geometry),transfer_memory::Bytes(q.baseGeometry));
    return bytes;
  }
  CNativeCandidateComplexity3D(const CNativeMetricFamily3D& original,const SU2Native3D::PreparedComplexity3D& cloud,
      const std::vector<SU2Native3D::DonorCell>& source,size_t ceiling,size_t caller_bytes=0):family(original),count(cloud.count),storageCeiling(ceiling),callerBytes(caller_bytes) {
    using namespace SU2Native3D;World world;CLocalFailure failure;const auto start=world.seconds();
    auto vote=[&](){const auto e=world.elect(failure);if(e.any)throw std::runtime_error(e.message);};
    const auto low=CPassiveComm::Allreduce(count,CPassiveComm::Op::MIN,world.comm),high=CPassiveComm::Allreduce(count,CPassiveComm::Op::MAX,world.comm);
    if(low!=high || count<32 || count>1048576)failure.Set(2,0,"Invalid original candidate integration count.");vote();
    source_span=OriginalSourceNodeSpan(world,source);
    try {
      const auto bytes=transfer_memory::Add(caller_bytes,transfer_memory::GrowthBound(cloud.points.size(),sizeof(Query)+sizeof(std::pair<const Id,const DonorCell*>)+6*sizeof(void*)));
      if(bytes>ceiling)throw std::runtime_error("Original candidate integration preparation exceeds storage ceiling.");
      std::map<Id,const DonorCell*> needed;
      for(const auto& p:cloud.points)needed.emplace(p.cell,nullptr);
      for(const auto& d:source) {
        for(unsigned k=0;k<4;++k) {
          const auto& m=family.selected.at(family.LocalNode(d.cell.v[k].id,source_span));
          if(d.sensor[k].xx!=m.m[0][0] || d.sensor[k].xy!=m.m[0][1] || d.sensor[k].xz!=m.m[0][2] ||
             d.sensor[k].yy!=m.m[1][1] || d.sensor[k].yz!=m.m[1][2] || d.sensor[k].zz!=m.m[2][2])
            throw std::runtime_error("Candidate normalization changed its original selected donor values.");
        }
        const auto it=needed.find(d.cell.id);
        if(it!=needed.end()){if(it->second)throw std::runtime_error("Duplicate original candidate donor identity.");it->second=&d;}
      }
      queries.reserve(cloud.points.size());size_t geometry_bytes=0;
      for(const auto& p:cloud.points) {
        const auto* donor=needed.at(p.cell);
        if(!donor || p.publisher!=world.rank || p.row>=count || !(p.inverse_pdf>0) || !std::isfinite(p.inverse_pdf) ||
           p.extension_component!=donor->extension_component || !std::isfinite(p.point.x) || !std::isfinite(p.point.y) || !std::isfinite(p.point.z))
          throw std::runtime_error("Original candidate integration lost its source binding.");
        Query q;q.weights=p.weights;q.point=p.point;q.inverse_pdf=p.inverse_pdf;q.row=p.row;
        for(unsigned k=0;k<4;++k) {
          if(donor->cell.v[k].id!=p.nodes[k] || !std::isfinite(p.weights[k]) || p.weights[k]<0 || p.weights[k]>1)
            throw std::runtime_error("Original candidate integration changed its source node/weight.");
          q.nodes[k]=family.LocalNode(p.nodes[k],source_span);
        }
        if(family.layers){q.geometry=family.layers->SamplePoints({p.point.x,p.point.y,p.point.z});++stats.geometry_queries;}
        if(family.first_row) {
          q.baseGeometry=family.first_row->base->SamplePoints({p.point.x,p.point.y,p.point.z});
          q.weight=family.first_row->A->Weight(junction_wp0::Of(p.point));++stats.geometry_queries;
        }
        geometry_bytes=transfer_memory::Add(geometry_bytes,transfer_memory::Bytes(q.geometry),transfer_memory::Bytes(q.baseGeometry));
        queries.push_back(std::move(q));
        if(transfer_memory::Add(caller_bytes,sizeof(*this),transfer_memory::Bytes(queries),geometry_bytes)>ceiling)throw std::runtime_error("Original candidate geometry exceeds storage ceiling.");
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}vote();
    const auto total=CPassiveComm::Allreduce(uint64_t(queries.size()),CPassiveComm::Op::SUM,world.comm);
    if(total!=count)failure.Set(2,0,"Incomplete original candidate integration partition.");vote();
    profileVersion=family.layers? family.layers->GetSampleVersion():0;
    stats.preparation_seconds=world.seconds()-start;
  }
  SU2Complexity::Estimate Assess(const std::vector<CBoundaryLayerMetric::Tensor>& current,su2double& core) {
    using namespace SU2Native3D;World world;CLocalFailure failure;const auto start=world.seconds();
    auto vote=[&](){const auto e=world.elect(failure);if(e.any)throw std::runtime_error(e.message);};
    long double n=0,mean=0,m2=0,numerical=0;std::array<long double,8> blocks{};
    try {
      if(current.size()!=family.selected.size() || !(core>0) || !std::isfinite(SU2_TYPE::GetValue(core)))
        throw std::runtime_error("Invalid current original candidate integration field/core.");
      if(family.first_row)family.first_row->Update(SU2_TYPE::GetValue(core),family.controls[3]);
      if(family.layers && profileVersion!=family.layers->GetSampleVersion()) {
        size_t bytes=transfer_memory::Add(callerBytes,sizeof(*this),transfer_memory::Bytes(queries));
        for(auto& q:queries) {
          q.geometry=family.layers->SamplePoints({q.point.x,q.point.y,q.point.z});++stats.geometry_queries;
          if(family.first_row){q.baseGeometry=family.first_row->base->SamplePoints({q.point.x,q.point.y,q.point.z});++stats.geometry_queries;}
          bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q.geometry),transfer_memory::Bytes(q.baseGeometry));
          if(bytes>storageCeiling)throw std::runtime_error("Refreshed original candidate geometry exceeds storage ceiling.");
        }
        profileVersion=family.layers->GetSampleVersion();
      }
      for(const auto& q:queries) {
        DonorCell values;
        for(unsigned k=0;k<4;++k){const auto& m=current.at(q.nodes[k]);values.sensor[k]={
            SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
            SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};}
        const auto sensor=InterpolateSensor(values,q.weights);CBoundaryLayerMetric::Tensor target;
        target.m[0][0]=sensor.xx;target.m[0][1]=target.m[1][0]=sensor.xy;target.m[0][2]=target.m[2][0]=sensor.xz;
        target.m[1][1]=sensor.yy;target.m[1][2]=target.m[2][1]=sensor.yz;target.m[2][2]=sensor.zz;
        if(family.first_row)family.first_row->ApplySamples(q.geometry,q.baseGeometry,q.weight,target);
        else for(const auto& g:q.geometry)family.layers->ApplySample(g.wall,g.sample,target,core,nullptr,false);
        const Tensor packed{SU2_TYPE::GetValue(target.m[0][0]),SU2_TYPE::GetValue(target.m[0][1]),SU2_TYPE::GetValue(target.m[0][2]),
            SU2_TYPE::GetValue(target.m[1][1]),SU2_TYPE::GetValue(target.m[1][2]),SU2_TYPE::GetValue(target.m[2][2])};
        long double condition=0;const auto y=(long double)MetricDensity(packed,nullptr,&condition)*q.inverse_pdf;
        if(!std::isfinite(y))throw std::runtime_error("Nonfinite original candidate complexity contribution.");
        const auto delta=y-mean;++n;mean+=delta/n;m2+=delta*(y-mean);blocks[q.row%8]+=y;
        numerical+=64*std::numeric_limits<double>::epsilon()*condition*std::abs(y);++stats.density_queries;
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}vote();
    const auto reduction=world.seconds();long double local[2]={n,n*mean},global[2]={};
    SU2_MPI::Allreduce(local,global,2,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
    if(uint64_t(global[0])!=count)failure.Set(2,0,"Incomplete current original candidate complexity.");vote();
    const long double average=global[1]/count;
    std::array<long double,10> partial{},total{};partial[0]=m2+n*(mean-average)*(mean-average);partial[1]=numerical;
    for(unsigned b=0;b<8;++b)partial[2+b]=blocks[b];
    SU2_MPI::Allreduce(partial.data(),total.data(),total.size(),MPI_LONG_DOUBLE,MPI_SUM,world.comm);
    stats.point_SE=double(std::sqrt(std::max(0.L,total[0])/(count*(count-1.L))));stats.block_SE=0;
    if(count%8==0){long double spread=0;for(unsigned b=0;b<8;++b)spread+=std::pow(total[2+b]/(count/8)-average,2);stats.block_SE=double(std::sqrt(spread/56));}
    stats.numerical_indicator=double(total[1]/count);
    const auto uncertainty=3*std::max(stats.point_SE,stats.block_SE)+stats.numerical_indicator;
    if(!(double(average)>0) || !std::isfinite(double(average)) || std::isnan(uncertainty))failure.Set(2,0,"Invalid original candidate complexity estimate.");vote();
    ++stats.assessments;stats.reduction_seconds+=world.seconds()-reduction;stats.assessment_seconds+=world.seconds()-start;
    return {double(average),uncertainty}; // Empirical sampling plus first-order conditioning indicators, not a proven bound.
  }
};
