// Frozen-field integration experiment; geometry searches reuse the production bounded donor service.
#pragma once
#include "../../../Common/include/adaptation/CNativeDonor3D.hpp"
#include <map>
namespace SU2NativeBLReplay {
using namespace SU2Native3D;
using Metric=CBoundaryLayerMetric::Tensor;
struct RawCell { Cell cell;std::array<Metric,4> sensor; };
struct Query {uint64_t row=0;Point point;double inversePDF=0;};
struct Request {
  uint64_t row=0,cell=0;int caller=0;Point point;
  template<class S>void Fields(S& s){s(row,cell,caller,point);}
};
struct Response {
  uint64_t row=0,cell=0;double sensor=0,composed=0,correction=0;
  template<class S>void Fields(S& s){s(row,cell,sensor,composed,correction);}
};
inline void Correction(CBoundaryLayerMetric& layers,const char* domainPath,const char* queryPath) {
  World world;constexpr size_t ceiling=size_t(2)<<30;const auto start=SU2_MPI::Wtime();
  auto vote=[&](CLocalFailure& failure){const auto selected=world.elect(failure);if(selected.any)throw std::runtime_error(selected.message);};
  CLocalFailure failure;std::map<Id,RawCell> originals;std::vector<DonorCell> geometry;double core=0;uint64_t cellCount=0;
  auto read=[&](std::ifstream& in,auto& value){in.read(reinterpret_cast<char*>(&value),sizeof(value));if(!in)throw std::runtime_error("Truncated frozen correction input.");};
  try {
    std::ifstream domain(domainPath,std::ios::binary);uint64_t magic=0;read(domain,magic);read(domain,cellCount);read(domain,core);
    if(magic!=0x4e41545352433344ULL || !cellCount || cellCount>1000000 || !(core>0) || !std::isfinite(core))
      throw std::runtime_error("Invalid frozen correction domain header.");
    for(uint64_t i=0;i<cellCount;++i) {
      uint64_t id=0,nodes[4];double x[12],m[36];read(domain,id);read(domain,nodes);read(domain,x);read(domain,m);
      if(id%world.size!=unsigned(world.rank))continue;
      RawCell raw;raw.cell.id=id;
      for(double v:x)if(!std::isfinite(v))throw std::runtime_error("Nonfinite original donor coordinate.");
      for(double v:m)if(!std::isfinite(v))throw std::runtime_error("Nonfinite original raw sensor coefficient.");
      for(unsigned k=0;k<4;++k) {
        raw.cell.v[k]={nodes[k],{x[3*k],x[3*k+1],x[3*k+2]}};
        for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)raw.sensor[k].m[a][b]=m[9*k+3*a+b];
        ValidateTensor({m[9*k],m[9*k+1],m[9*k+2],m[9*k+4],m[9*k+5],m[9*k+8]});
      }
      auto& v=raw.cell.v;const auto orientation=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
      if(!orientation)throw std::runtime_error("Degenerate original correction donor.");
      if(orientation<0){std::swap(v[0],v[1]);std::swap(raw.sensor[0],raw.sensor[1]);}
      DonorCell locator;locator.cell=raw.cell;locator.sensor.fill({1,0,0,1,0,1});
      // Identity is ONLY locator metadata: original RAW9 density is evaluated by this record's owner below.
      geometry.push_back(locator);
      if(!originals.emplace(id,std::move(raw)).second)throw std::runtime_error("Duplicate original correction donor ID.");
    }
    if(domain.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing frozen correction domain data.");
  }catch(const std::exception& error){failure.Set(2,0,error.what());}vote(failure);
  const auto rawBytes=transfer_memory::Mul(originals.size(),sizeof(RawCell)+sizeof(Id)+6*sizeof(void*));
  DonorImport source(world,true);std::string reason;
  if(!source.Build(geometry,reason,ceiling,rawBytes))throw std::runtime_error(reason);
  std::vector<Query> queries;uint64_t count=0;
  try {
    std::ifstream input(queryPath,std::ios::binary);uint64_t magic=0;read(input,magic);read(input,count);
    if(magic!=0x4e41545155455259ULL || count<2 || count>1048576)throw std::runtime_error("Invalid correction query header.");
    queries.reserve(count);
    for(uint64_t i=0;i<count;++i) {
      double values[4];read(input,values);for(double x:values)if(!std::isfinite(x))throw std::runtime_error("Nonfinite correction query.");
      if(!(values[3]>0))throw std::runtime_error("Invalid correction proposal density.");
      queries.push_back({i,{values[0],values[1],values[2]},values[3]});
    }
    if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing correction query data.");
  }catch(const std::exception& error){failure.Set(2,0,error.what());}vote(failure);
  const size_t heldBytes=transfer_memory::Add(rawBytes,transfer_memory::Bytes(geometry),transfer_memory::Bytes(queries));
  const size_t callerBytes=transfer_memory::Add(heldBytes,source.ResidentBytes());
  std::ofstream out("correction_rank"+std::to_string(world.rank)+".csv");
  out<<std::setprecision(17)<<"row,donor,sensor,composed,correction,weighted_correction\n";
  long double local[6]={},global[6]={};double ownerSeconds=0;
  for(uint64_t first=0;first<count;first+=8*world.size) {
    std::vector<Point> points;std::vector<Query> batch;
    for(unsigned j=0;j<8;++j){const auto i=first+j*world.size+world.rank;if(i<count){batch.push_back(queries[i]);points.push_back(queries[i].point);}}
    const auto footprint=source.ProbePoints(points,ceiling,heldBytes);
    if(!footprint.valid)throw std::runtime_error(footprint.reason);
    std::vector<std::vector<Request>> requests(world.size);
    std::map<uint64_t,Response> values;
    try {
      for(const auto& query:batch) {
        uint64_t selected=UINT64_MAX;
        for(const auto& donor:footprint.cells) {
          const auto& v=donor.cell.v;std::array<long double,4> weights;
          if(Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},query.point,weights))selected=std::min(selected,donor.cell.id);
        }
        if(selected==UINT64_MAX)values.emplace(query.row,Response{query.row,UINT64_MAX,0,0,0});
        else requests[selected%world.size].push_back({query.row,selected,world.rank,query.point});
      }
    }catch(const std::exception& error){failure.Set(2,first,error.what());}vote(failure);
    bool admitted=false;const auto received=world.exchange(requests,&admitted,ceiling,callerBytes);
    if(!admitted)failure.Set(3,first,"Correction query exchange exceeds storage budget.");vote(failure);
    std::vector<std::vector<Response>> replies(world.size);const auto ownerStart=SU2_MPI::Wtime();
    try {
      for(const auto& request:received) {
        const auto found=originals.find(request.cell);
        if(found==originals.end() || request.caller<0 || request.caller>=world.size)throw std::runtime_error("Invalid original correction owner.");
        const auto& raw=found->second;const auto& v=raw.cell.v;std::array<long double,4> weights;
        if(!Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},request.point,weights))throw std::runtime_error("Correction query leaves its original donor.");
        Metric sensor;
        for(unsigned k=0;k<4;++k)for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)sensor.m[a][b]+=double(weights[k])*raw.sensor[k].m[a][b];
        auto logarithm=[](Metric metric) {
          su2double vectors[3][3],val[3],work[3];CBlasStructure::EigenDecomposition(metric.m,vectors,val,3,work);
          double result=0;for(auto x:val){if(!(x>0))throw std::runtime_error("Nonpositive correction sensor eigenvalue.");result+=std::log(SU2_TYPE::GetValue(x));}return .5*result;
        };
        const double ls=logarithm(sensor);Metric composed=sensor;const su2double x[3]={request.point.x,request.point.y,request.point.z};
        layers.ApplyPoint(x,composed,core,nullptr,false);const double lt=logarithm(composed);
        const double background=std::exp(ls),target=std::exp(lt),delta=background*std::expm1(lt-ls);
        if(!std::isfinite(background) || !std::isfinite(target) || !std::isfinite(delta))throw std::runtime_error("Nonfinite composed correction density.");
        if(lt-ls < -4096*std::numeric_limits<double>::epsilon()*std::max({1.,std::abs(ls),std::abs(lt)}))
          throw std::runtime_error("Composed correction materially weakens the original sensor.");
        replies[request.caller].push_back({request.row,request.cell,background,target,delta});
      }
    }catch(const std::exception& error){failure.Set(2,first,error.what());}ownerSeconds+=SU2_MPI::Wtime()-ownerStart;vote(failure);
    const auto returned=world.exchange(replies,&admitted,ceiling,callerBytes);
    if(!admitted)failure.Set(3,first,"Correction value exchange exceeds storage budget.");
    try {
      for(const auto& row:returned)if(!values.emplace(row.row,row).second)throw std::runtime_error("Duplicate correction response.");
      if(values.size()!=batch.size())throw std::runtime_error("Incomplete original-domain correction response.");
      for(const auto& query:batch) {
        const auto& value=values.at(query.row);const long double y=(long double)value.correction*query.inversePDF;
        if(!std::isfinite(y))throw std::runtime_error("Nonfinite correction weight.");
        local[0]+=y;local[1]+=y*y;++local[2];local[3]+=value.cell!=UINT64_MAX;
        local[4]+=y<0?std::abs(y):0;local[5]+=y!=0;
        out<<query.row<<','<<value.cell<<','<<value.sensor<<','<<value.composed<<','<<value.correction<<','<<y<<'\n';
      }
      if(!out)throw std::runtime_error("Cannot save correction rows.");
    }catch(const std::exception& error){failure.Set(2,first,error.what());}vote(failure);
  }
  out.close();if(!out)failure.Set(2,0,"Cannot close correction output.");vote(failure);
  SU2_MPI::Allreduce(local,global,6,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
  if(uint64_t(global[2])!=count)throw std::runtime_error("Incomplete global correction count.");
  const long double mean=global[0]/count,variance=std::max(0.L,(global[1]-global[0]*mean)/(count*(count-1.L)));
  double localTimes[3]={SU2_MPI::Wtime()-start,source.Statistics().build_seconds,ownerSeconds},times[3];
  SU2_MPI::Allreduce(localTimes,times,3,MPI_DOUBLE,MPI_MAX,world.comm);
  if(!world.rank) {
    std::ofstream result("correction_global.json");result<<std::setprecision(21)<<"{\"samples\":"<<count<<",\"inside\":"<<global[3]
      <<",\"nonzero\":"<<global[5]<<",\"correction\":"<<mean<<",\"empirical_standard_error\":"<<std::sqrt(variance)
      <<",\"mean_absolute_negative_roundoff\":"<<global[4]/count<<",\"ranks\":"<<world.size
      <<",\"seconds_max\":"<<times[0]<<",\"index_build_seconds_max\":"<<times[1]<<",\"owner_density_seconds_max\":"<<times[2]
      <<",\"standard_error_is_proven_bound\":false,\"original_RAW9_preserved\":true,\"complete_original_domain_search\":true,\"production_normalization_changed\":false}\n";
    if(!result)throw std::runtime_error("Cannot save global correction result.");
  }
}
}  // namespace SU2NativeBLReplay
