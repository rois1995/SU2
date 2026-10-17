/*!
 * \file CNativeCellSampling3D.hpp
 * \brief Positive cell importance proposal for reassessing an immutable field on a candidate mesh.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeDistributed3D.hpp"
#include <random>
#include <numeric>
#include <cstring>
#include <tuple>
#include <cmath>
#include <stdexcept>

namespace SU2Native3D {
/*! Importance is only a proposal weight, never an interpolated replacement for the original metric. */
struct WeightedCell3D {
  Tetrahedron points{};
  double importance=0;
  Id id=0;
  long double mass=0;
  template<class S>void Fields(S& s){s(points,importance,id);}
};
struct CellSamplingLimit3D : std::runtime_error { using std::runtime_error::runtime_error; };
struct CellSample3D {
  uint64_t row=0;
  Point point;
  double inverse_pdf=0,importance=0,volume=0;
  Id cell=0;
  template<class S>void Fields(S& s){s(row,point,inverse_pdf,importance,volume,cell);}
};
/*! Fixed geometry buckets and canonical coordinates make the stream independent of MPI ownership.
 * Only bounded cell metadata is shuffled; no rank gathers the complete mesh. Every cell has positive
 * probability. Floating probabilities/positions and empirical variances are not proven error bounds. */
class CellSampler3D {
  static constexpr size_t Buckets=256;
  struct Request {
    uint64_t row=0,unit=0;unsigned bucket=0;std::array<uint64_t,3> bary{};
    template<class S>void Fields(S& s){s(row,unit,bucket,bary);}
  };
  World& world;
  size_t ceiling,caller;
  std::vector<WeightedCell3D> cells;
  std::vector<long double> cdf,bucket_cdf;
  std::vector<unsigned> bucket_order;
  std::array<size_t,Buckets+1> offsets{};
  std::array<long double,Buckets> bucket_probability{};
  long double mass=0;
  mutable size_t maximum_bytes=0;
  static bool Less(const Tetrahedron& a,const Tetrahedron& b) {
    for(size_t i=0;i<4;++i) {
      const auto x=std::tie(a[i].x,a[i].y,a[i].z),y=std::tie(b[i].x,b[i].y,b[i].z);
      if(x!=y)return x<y;
    }return false;
  }
  static unsigned Bucket(const Tetrahedron& points) {
    uint64_t hash=1469598103934665603ULL;
    for(auto p:points)for(double x:{p.x,p.y,p.z}) {
      if(x==0)x=0;uint64_t word=0;std::memcpy(&word,&x,sizeof(word));
      word=(word^(word>>30))*0xbf58476d1ce4e5b9ULL;word=(word^(word>>27))*0x94d049bb133111ebULL;
      hash=(hash^(word^(word>>31)))*1099511628211ULL;
    }return unsigned(hash%Buckets);
  }
  static long double Volume(const Tetrahedron& p){return TetrahedronVolume(p);}
  static void Capture(CLocalFailure& failure,const std::exception& e) {
    failure.Set(dynamic_cast<const CellSamplingLimit3D*>(&e) || dynamic_cast<const std::bad_alloc*>(&e)?3:2,0,e.what());
  }
  void Vote(const CLocalFailure& failure) const {
    const auto elected=world.elect(failure);if(elected.any) {if(elected.severity==3)throw CellSamplingLimit3D(elected.message);throw std::runtime_error(elected.message);}
  }
  void Admit(size_t bytes)const {
    CLocalFailure failure;
    maximum_bytes=std::max(maximum_bytes,transfer_memory::Add(caller,bytes));
    if(transfer_memory::Add(caller,bytes)>ceiling)failure.Set(3,0,"Candidate complexity proposal exceeds storage ceiling.");
    Vote(failure);
  }
  static long double Unit(uint64_t x){return std::min(std::ldexp((long double)x,-64),std::nextafter(1.L,0.L));}
 public:
  double preparation_seconds=0,draw_seconds=0;
  size_t ResidentBytes()const {
    return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(cells),transfer_memory::Bytes(cdf),
        transfer_memory::Bytes(bucket_cdf),transfer_memory::Bytes(bucket_order));
  }
  long double CentroidEstimate()const{return mass;}
  size_t MaximumBytes()const{return std::max(maximum_bytes,world.max_exchange_work_bytes);}
  CellSampler3D(World& comm,std::vector<WeightedCell3D> local,size_t maximumBytes,size_t callerBytes=0)
      :world(comm),ceiling(maximumBytes),caller(callerBytes) {
    const auto start=world.seconds();CLocalFailure failure;
    const auto low=CPassiveComm::Allreduce(uint64_t(ceiling),CPassiveComm::Op::MIN,world.comm);
    const auto high=CPassiveComm::Allreduce(uint64_t(ceiling),CPassiveComm::Op::MAX,world.comm);
    if(low!=high || ceiling<4096)failure.Set(1,0,"Inconsistent/invalid candidate complexity storage controls.");Vote(failure);
    Admit(transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(local),
        transfer_memory::GrowthBound(local.size(),sizeof(WeightedCell3D)),
        transfer_memory::Mul(world.size,sizeof(std::vector<WeightedCell3D>))));
    std::vector<std::vector<WeightedCell3D>> outgoing;
    try {outgoing.resize(world.size);for(auto& row:local) {
      for(auto p:row.points)for(double x:{p.x,p.y,p.z})if(!std::isfinite(x))throw std::runtime_error("Nonfinite candidate complexity geometry.");
      if(!(row.importance>0) || !std::isfinite(row.importance))throw std::runtime_error("Nonpositive candidate complexity importance.");
      if(!(Volume(row.points)>0))throw std::runtime_error("Nonpositive candidate complexity cell.");
      for(auto& p:row.points){if(p.x==0)p.x=0;if(p.y==0)p.y=0;if(p.z==0)p.z=0;}
      std::sort(row.points.begin(),row.points.end(),[](Point a,Point b){return std::tie(a.x,a.y,a.z)<std::tie(b.x,b.y,b.z);});
      if(Orientation(row.points[0],row.points[1],row.points[2],row.points[3])<0)std::swap(row.points[0],row.points[1]);
      outgoing[Bucket(row.points)%world.size].push_back(row);
    }}catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    bool admitted=false;
    cells=world.exchange(outgoing,&admitted,ceiling,transfer_memory::Add(caller,sizeof(*this),transfer_memory::Bytes(local)));
    if(!admitted)failure.Set(3,0,"Candidate complexity metadata exchange exceeds storage ceiling.");Vote(failure);
    std::vector<WeightedCell3D>().swap(local);std::vector<std::vector<WeightedCell3D>>().swap(outgoing);
    Admit(transfer_memory::Add(ResidentBytes(),transfer_memory::Mul(cells.size(),sizeof(long double)),
        transfer_memory::Mul(Buckets,4*sizeof(long double)+sizeof(unsigned))));
    std::array<long double,Buckets> local_mass{},global_mass{};
    try {
      for(const auto& row:cells) {
        for(auto p:row.points)for(double x:{p.x,p.y,p.z})if(!std::isfinite(x))throw std::runtime_error("Nonfinite candidate complexity metadata.");
        if(!(row.importance>0) || !std::isfinite(row.importance))throw std::runtime_error("Invalid candidate complexity metadata importance.");
        Volume(row.points);
      }
      std::sort(cells.begin(),cells.end(),[](const auto& a,const auto& b){const auto x=Bucket(a.points),y=Bucket(b.points);return x!=y?x<y:Less(a.points,b.points);});
      for(size_t i=0;i<cells.size();++i) {
        auto& row=cells[i];const auto bucket=Bucket(row.points);
        if(bucket%world.size!=unsigned(world.rank))throw std::runtime_error("Misrouted candidate complexity metadata.");
        if(i && !Less(cells[i-1].points,row.points) && !Less(row.points,cells[i-1].points))throw std::runtime_error("Duplicate candidate complexity simplex.");
        row.mass=Volume(row.points)*row.importance;
        if(!(row.mass>0) || !std::isfinite(row.mass))throw std::runtime_error("Unrepresentable candidate complexity mass.");
        ++offsets[bucket+1];
      }
      for(size_t b=1;b<=Buckets;++b)offsets[b]+=offsets[b-1];
      cdf.resize(cells.size());
      for(unsigned b=0;b<Buckets;++b) {
        std::sort(cells.begin()+offsets[b],cells.begin()+offsets[b+1],[](const auto& a,const auto& c){return a.mass!=c.mass?a.mass<c.mass:Less(a.points,c.points);});
        for(size_t i=offsets[b];i<offsets[b+1];++i)local_mass[b]+=cells[i].mass;
        long double previous=0;
        for(size_t i=offsets[b];i<offsets[b+1];++i) {
          const auto next=i+1==offsets[b+1]?1.L:previous+cells[i].mass/local_mass[b];
          if(!(next>previous) || next-previous<std::ldexp(1.L,-64))throw CellSamplingLimit3D("Candidate cell probability is unresolved by the sampling stream; no cell discarded.");
          cdf[i]=next;previous=next;
        }
      }
    }catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    SU2_MPI::Allreduce(local_mass.data(),global_mass.data(),Buckets,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
    try {
      for(unsigned b=0;b<Buckets;++b)if(global_mass[b]>0){mass+=global_mass[b];bucket_order.push_back(b);}
      if(!(mass>0) || !std::isfinite(mass))throw std::runtime_error("Empty/nonfinite global candidate complexity mass.");
      std::sort(bucket_order.begin(),bucket_order.end(),[&](auto a,auto b){return global_mass[a]!=global_mass[b]?global_mass[a]<global_mass[b]:a<b;});
      long double previous=0;
      for(size_t j=0;j<bucket_order.size();++j) {
        const auto b=bucket_order[j];const auto next=j+1==bucket_order.size()?1.L:previous+global_mass[b]/mass;
        if(!(next>previous) || next-previous<std::ldexp(1.L,-64))throw CellSamplingLimit3D("Candidate bucket probability is unresolved by the sampling stream; no bucket discarded.");
        bucket_probability[b]=next-previous;bucket_cdf.push_back(next);previous=next;
      }
    }catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    preparation_seconds=world.seconds()-start;
  }
  std::vector<CellSample3D> Draw(uint64_t count,uint64_t seed) {
    const auto start=world.seconds();CLocalFailure failure;
    const uint64_t controls[]={count,seed};uint64_t low[2],high[2];
    SU2_MPI::Allreduce(controls,low,2,MPI_UNSIGNED_LONG_LONG,MPI_MIN,world.comm);
    SU2_MPI::Allreduce(controls,high,2,MPI_UNSIGNED_LONG_LONG,MPI_MAX,world.comm);
    if(!std::equal(low,low+2,high) || count<32 || count>1048576)failure.Set(1,0,"Invalid/inconsistent candidate complexity stream.");Vote(failure);
    Admit(transfer_memory::Add(ResidentBytes(),transfer_memory::Mul(count,6*sizeof(Request)+6*sizeof(CellSample3D)),
        transfer_memory::Mul(world.size,2*sizeof(std::vector<Request>)+2*sizeof(std::vector<CellSample3D>))));
    std::vector<std::vector<Request>> outgoing;
    try {outgoing.resize(world.size);if(!world.rank) {
      std::mt19937_64 random(seed);
      for(uint64_t i=0;i<count;++i) {
        const auto at=std::upper_bound(bucket_cdf.begin(),bucket_cdf.end(),Unit(random()))-bucket_cdf.begin();
        Request r;r.row=i;r.bucket=bucket_order.at(at);r.unit=random();for(auto& u:r.bary)u=random();
        outgoing[r.bucket%world.size].push_back(r);
      }
    }}catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    bool admitted=false;const auto requests=world.exchange(outgoing,&admitted,ceiling,transfer_memory::Add(caller,ResidentBytes()));
    if(!admitted)failure.Set(3,0,"Candidate complexity draw exchange exceeds storage ceiling.");Vote(failure);
    std::vector<std::vector<CellSample3D>> reply;
    try {reply.resize(world.size);for(const auto& r:requests) {
      if(r.row>=count || r.bucket>=Buckets || r.bucket%world.size!=unsigned(world.rank))throw std::runtime_error("Invalid candidate complexity draw request.");
      const auto begin=offsets[r.bucket],end=offsets[r.bucket+1];
      const size_t i=std::upper_bound(cdf.begin()+begin,cdf.begin()+end,Unit(r.unit))-cdf.begin();
      if(i>=end)throw std::runtime_error("Missing candidate complexity draw cell.");
      const auto& tet=cells[i].points;std::array<long double,3> u{Unit(r.bary[0]),Unit(r.bary[1]),Unit(r.bary[2])};std::sort(u.begin(),u.end());
      const std::array<long double,4> w{u[0],u[1]-u[0],u[2]-u[1],1-u[2]};Point p;
      for(unsigned j=0;j<3;++j) {
        const auto get=[&](Point v){return j==0?v.x:j==1?v.y:v.z;};long double x=get(tet[0]);
        for(unsigned k=1;k<4;++k)x+=w[k]*((long double)get(tet[k])-get(tet[0]));
        (j==0?p.x:j==1?p.y:p.z)=double(x);
      }
      std::array<long double,4> actual;
      if(!Barycentric(tet,p,actual))throw std::runtime_error("Rounded candidate complexity sample leaves its cell.");
      const auto probability=bucket_probability[r.bucket]*(cdf[i]-(i==begin?0:cdf[i-1]));
      const double inverse=double(Volume(tet)/probability);
      if(!(inverse>0) || !std::isfinite(inverse))throw std::runtime_error("Nonfinite candidate complexity proposal PDF.");
      reply[r.row%world.size].push_back({r.row,p,inverse,cells[i].importance,double(Volume(tet)),cells[i].id});
    }}catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    auto points=world.exchange(reply,&admitted,ceiling,transfer_memory::Add(caller,ResidentBytes(),transfer_memory::Bytes(requests),transfer_memory::Bytes(outgoing)));
    if(!admitted)failure.Set(3,0,"Candidate complexity point exchange exceeds storage ceiling.");Vote(failure);
    try {
      std::sort(points.begin(),points.end(),[](auto a,auto b){return a.row<b.row;});
      const size_t expected=count/size_t(world.size)+(uint64_t(world.rank)<count%uint64_t(world.size));
      if(points.size()!=expected)throw std::runtime_error("Incomplete candidate complexity sample stream.");
      for(size_t i=0;i<points.size();++i)if(points[i].row!=uint64_t(world.rank)+i*world.size)throw std::runtime_error("Duplicate/misrouted candidate complexity sample.");
    }catch(const std::exception& e){Capture(failure,e);}Vote(failure);
    draw_seconds+=world.seconds()-start;return points;
  }
};
} // namespace SU2Native3D
