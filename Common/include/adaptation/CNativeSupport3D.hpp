/*!
 * \file CNativeSupport3D.hpp
 * \brief Bounded faceted-wall support sampling; proposal geometry never substitutes for the target metric.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeMesh3D.hpp"
#include "CTransferMemory.hpp"
#include "../adt/CADTElemClass.hpp"
#include "../option_structure.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>

namespace SU2Native3D {
struct SupportWall3D {
  std::vector<Point> vertices;
  std::vector<std::array<size_t,3>> triangles;
  double first_height=0,growth=1,thickness=0;
};
struct SupportLimit3D : std::runtime_error { using std::runtime_error::runtime_error; };
struct SupportPoint3D {Point point;double inverse_pdf=0;unsigned kind=0;};
struct SupportStats3D {
  std::array<size_t,3> components{};
  uint64_t preparation_work=0,pdf_candidates=0;
  size_t retained_bytes=0;
  double preparation_seconds=0,pdf_seconds=0;
};
/*! A face-prism/edge-sector/vertex-lune mixture covering the complete finite-triangle tube.
 * PDF sums every covering component, including different walls. It is an importance proposal,
 * not the physical wall distance/density. Floating PDFs and pseudorandom samples provide no proven bound.
 * Geometry survives scale trials; new samples can extend the same deterministic global stream. */
class SupportSampler3D {
  using V=std::array<long double,3>;
  struct Component {
    unsigned kind=0;
    V origin{},edge0{},edge1{},normal{},plane0{},plane1{},inward0{},inward1{},center{},side{};
    long double h0=0,b=0,radius=0,width=0,length=0,measure=0,coefficient=0,weight=0;
  };
  std::vector<Component> components;
  std::vector<size_t> order;
  std::vector<long double> cdf;
  std::unique_ptr<CADTElemClass> index;
  size_t ceiling;
  uint64_t maximum_work;
  SupportStats3D stats;
  static V add(V a,V b){for(unsigned j=0;j<3;++j)a[j]+=b[j];return a;}
  static V sub(V a,V b){for(unsigned j=0;j<3;++j)a[j]-=b[j];return a;}
  static V mul(V a,long double s){for(auto& x:a)x*=s;return a;}
  static long double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
  static V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
  static V unit(V a){const auto n=std::sqrt(dot(a,a));if(!(n>0))throw std::invalid_argument("Degenerate support direction.");return mul(a,1/n);}
  static V vector(Point p){return {p.x,p.y,p.z};}
  static V perpendicular(V axis){V basis{};const auto j=std::min_element(axis.begin(),axis.end(),[](auto a,auto b){return std::abs(a)<std::abs(b);})-axis.begin();basis[j]=1;return unit(cross(axis,basis));}
  void work(uint64_t n=1) {
    if(n>maximum_work-stats.preparation_work-stats.pdf_candidates)throw SupportLimit3D("Geometric support work budget exhausted; PDF not truncated.");
    stats.preparation_work+=n;
  }
  void admit(size_t bytes)const{if(bytes==SIZE_MAX || bytes>ceiling)throw SupportLimit3D("Geometric support storage budget exhausted.");}
  static long double moment(const Component& c) {
    const auto r=std::min(c.radius,c.b>0?c.h0/c.b:c.radius);const auto k=c.kind;
    long double value=std::pow(r,k+1)/(c.h0*(k+1));
    if(c.radius>r) {
      if(k==0)value+=std::log(c.radius/r)/c.b;
      else value+=(std::pow(c.radius,k)-std::pow(r,k))/(k*c.b);
    }
    return value;
  }
  static long double radial(const Component& c,long double u) {
    const auto split=std::min(c.radius,c.b>0?c.h0/c.b:c.radius);const auto k=c.kind;
    const auto mass=u*moment(c),low=std::pow(split,k+1)/(c.h0*(k+1));
    long double r=std::pow(mass*c.h0*(k+1),1.L/(k+1));
    if(mass>low)r=k?std::pow(std::pow(split,k)+k*c.b*(mass-low),1.L/k):split*std::exp(c.b*(mass-low));
    return std::clamp(r,0.L,c.radius);
  }
  void append(Component c,long double jacobian,long double importance) {
    c.measure=jacobian*moment(c);c.weight=c.measure*importance;
    if(!(c.measure>=0) || !std::isfinite(c.weight) || c.weight<0)throw std::invalid_argument("Invalid support component mass.");
    if(c.weight>0){++stats.components[c.kind];components.push_back(std::move(c));}
  }
  static long double componentDensity(const Component& c,V p) {
    const auto w=sub(p,c.origin);long double r=0;
    if(c.kind==0) {
      const auto n=cross(c.edge0,c.edge1);const auto square=dot(n,n);
      const auto a=dot(cross(w,c.edge1),n)/square,b=dot(cross(c.edge0,w),n)/square;
      if(a<0 || b<0 || a+b>1)return 0;r=std::abs(dot(w,c.normal));
    } else if(c.kind==1) {
      const auto z=dot(w,c.normal);const V q{dot(w,c.plane0),dot(w,c.plane1),0};
      if(z<0 || z>c.length || dot(q,c.inward0)>0 || dot(q,c.inward1)>0)return 0;
      r=std::sqrt(dot(q,q));
    } else {
      if(dot(w,c.inward0)>0 || dot(w,c.inward1)>0)return 0;r=std::sqrt(dot(w,w));
    }
    return r<=c.radius?c.coefficient/std::max(c.h0,c.b*r):0;
  }
 public:
  SupportSampler3D(const std::vector<SupportWall3D>& walls,size_t maximum_bytes,uint64_t work_limit)
      :ceiling(maximum_bytes),maximum_work(work_limit) {
    const auto start=std::chrono::steady_clock::now();if(walls.empty() || !work_limit)throw std::invalid_argument("Empty support or work control.");
    size_t vertices=0,faces=0;
    for(const auto& wall:walls){vertices=transfer_memory::Add(vertices,wall.vertices.size());faces=transfer_memory::Add(faces,wall.triangles.size());}
    // Includes component growth, edge maps, adjacency and ADT construction temporaries; checked before allocation.
    admit(transfer_memory::Add(sizeof(*this),transfer_memory::Mul(vertices,4096),transfer_memory::Mul(faces,8192)));
    for(const auto& wall:walls) {
      if(wall.vertices.empty() || wall.triangles.empty() || !(wall.first_height>0) || !(wall.growth>=1) ||
         !(wall.thickness>=wall.first_height) || !std::isfinite(wall.first_height+wall.growth+wall.thickness))
        throw std::invalid_argument("Invalid faceted support wall/profile.");
      Component base;base.h0=wall.first_height;base.b=2.L*(wall.growth-1)/(wall.growth+1);base.radius=wall.thickness;
      const auto count=wall.vertices.size();std::vector<V> x; x.reserve(count);
      for(auto p:wall.vertices){for(double a:{p.x,p.y,p.z})if(!std::isfinite(a))throw std::invalid_argument("Nonfinite support coordinate.");x.push_back(vector(p));}
      std::vector<long double> trace(count),angle(count),importance(count);
      std::map<std::array<size_t,2>,std::vector<size_t>> opposite;
      std::vector<std::vector<size_t>> neighbors(count);
      for(const auto& triangle:wall.triangles) {
        work();for(auto id:triangle)if(id>=count)throw std::invalid_argument("Invalid support triangle vertex.");
        const auto a=x[triangle[0]],u=sub(x[triangle[1]],a),v=sub(x[triangle[2]],a),normal=cross(u,v);const auto square=dot(normal,normal);
        if(!(square>0))throw std::invalid_argument("Degenerate support triangle.");
        const auto logarithm=std::log(4.L/3*square);
        for(unsigned k=0;k<3;++k) {
          const auto i=triangle[k],j=triangle[(k+1)%3],l=triangle[(k+2)%3];const auto p=unit(sub(x[j],x[i])),q=unit(sub(x[l],x[i]));
          const auto w=std::acos(std::clamp(dot(p,q),-1.L,1.L));trace[i]+=w*logarithm;angle[i]+=w;
          std::array<size_t,2> edge{i,j};std::sort(edge.begin(),edge.end());auto& incident=opposite[edge];incident.push_back(l);
          if(incident.size()>2)throw std::invalid_argument("Nonmanifold support edge.");
        }
      }
      for(size_t i=0;i<count;++i){if(!(angle[i]>0))throw std::invalid_argument("Unused support vertex.");importance[i]=std::exp(-.5L*trace[i]/angle[i]);}
      for(const auto& triangle:wall.triangles) {
        auto c=base;c.origin=x[triangle[0]];c.edge0=sub(x[triangle[1]],c.origin);c.edge1=sub(x[triangle[2]],c.origin);
        const auto n=cross(c.edge0,c.edge1);const auto length=std::sqrt(dot(n,n));c.normal=mul(n,1/length);
        append(c,length,std::max({importance[triangle[0]],importance[triangle[1]],importance[triangle[2]]}));
      }
      for(const auto& entry:opposite) {
        work();const auto i=entry.first[0],j=entry.first[1];neighbors[i].push_back(j);neighbors[j].push_back(i);
        auto c=base;c.kind=1;c.origin=x[i];c.edge0=sub(x[j],x[i]);c.length=std::sqrt(dot(c.edge0,c.edge0));c.normal=mul(c.edge0,1/c.length);
        c.plane0=perpendicular(c.normal);c.plane1=unit(cross(c.normal,c.plane0));
        auto inward=[&](size_t node){const auto w=sub(x[node],c.origin);return V{dot(w,c.plane0),dot(w,c.plane1),0};};
        c.inward0=inward(entry.second[0]);c.inward1=inward(entry.second.back());
        const auto determinant=c.inward0[0]*c.inward1[1]-c.inward0[1]*c.inward1[0];
        c.width=std::atan2(std::abs(determinant),-dot(c.inward0,c.inward1));
        if(c.width>0) {
          const auto direction=unit(c.inward0);V side{-direction[1],direction[0],0};side=mul(side,determinant<0?-1:1);
          c.center=mul(add(mul(direction,std::sin(c.width/2)),mul(side,std::cos(c.width/2))),-1);c.side={-c.center[1],c.center[0],0};
          append(c,c.length*c.width,std::max(importance[i],importance[j]));
        }
      }
      for(size_t i=0;i<count;++i) {
        auto c=base;c.kind=2;c.origin=x[i];long double minimum=2;
        std::vector<V> directions;for(auto j:neighbors[i])directions.push_back(unit(sub(x[j],x[i])));
        for(auto a:directions)for(auto b:directions){work();const auto product=dot(a,b);if(product<minimum){minimum=product;c.inward0=a;c.inward1=b;}}
        auto axis=cross(c.inward0,c.inward1);const auto sine=std::sqrt(dot(axis,axis));
        c.width=std::atan2(sine,-dot(c.inward0,c.inward1));
        if(c.width>0) {
          c.normal=sine>0?mul(axis,1/sine):perpendicular(c.inward0);const auto plane=unit(cross(c.normal,c.inward0));
          c.center=add(mul(c.inward0,std::sin(c.width/2)),mul(plane,std::cos(c.width/2)));c.side=unit(cross(c.normal,c.center));
          append(c,2*c.width,importance[i]);
        }
      }
    }
    if(components.empty())throw std::invalid_argument("No positive support component.");
    order.resize(components.size());std::iota(order.begin(),order.end(),size_t(0));
    std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){return components[a].weight<components[b].weight;});
    long double total=0;for(auto i:order)total+=components[i].weight;
    if(!(total>0) || !std::isfinite(total))throw std::runtime_error("Nonfinite support mixture mass.");
    long double previous=0;cdf.reserve(order.size());
    for(size_t k=0;k<order.size();++k) {
      auto& c=components[order[k]];const auto next=k+1==order.size()?1.L:previous+c.weight/total;
      if(!(next>previous))throw std::runtime_error("Positive support probability lost to rounding.");
      c.coefficient=(next-previous)/c.measure;cdf.push_back(next);previous=next;
    }
    const auto n=components.size();size_t retained=0,peak=0;CADTElemClass::PredictBytes(3,2*n,n,2*n,&retained,&peak);
    admit(transfer_memory::Add(transfer_memory::Bytes(components),transfer_memory::Bytes(order),transfer_memory::Bytes(cdf),
        transfer_memory::Mul(n,2*3*sizeof(su2double)+3*sizeof(unsigned long)+2*sizeof(unsigned short)),peak));
    std::vector<su2double> coordinates;coordinates.reserve(6*n);std::vector<unsigned long> connectivity(2*n),ids(n);
    for(size_t i=0;i<n;++i) {
      const auto& c=components[i];V low=c.origin,high=c.origin;
      if(c.kind<2)for(auto p:{add(c.origin,c.edge0),c.kind==0?add(c.origin,c.edge1):c.origin})for(unsigned j=0;j<3;++j){low[j]=std::min(low[j],p[j]);high[j]=std::max(high[j],p[j]);}
      auto bound=[&](long double value,double direction) {
        const double rounded=std::nextafter(double(value),direction);
        if(!std::isfinite(rounded))throw std::invalid_argument("Support box exceeds finite coordinate range.");
        coordinates.push_back(rounded);
      };
      for(unsigned j=0;j<3;++j)bound(low[j]-c.radius,-INFINITY);
      for(unsigned j=0;j<3;++j)bound(high[j]+c.radius,INFINITY);
      connectivity[2*i]=2*i;connectivity[2*i+1]=2*i+1;ids[i]=i;
    }
    std::vector<unsigned short> types(n,LINE),markers(n,0);
    index=std::make_unique<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
    stats.retained_bytes=transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(components),transfer_memory::Bytes(order),transfer_memory::Bytes(cdf),index->GetAllocatedBytes(),index->RetainedWorkspaceBound(1));admit(stats.retained_bytes);
    stats.preparation_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  }
  const SupportStats3D& Statistics()const{return stats;}
  double Density(Point point) {
    const auto start=std::chrono::steady_clock::now();const su2double p[3]={point.x,point.y,point.z};
    for(double x:{point.x,point.y,point.z})if(!std::isfinite(x))throw std::invalid_argument("Nonfinite support query.");
    std::vector<unsigned long> candidates;index->DetermineIntersectingElements(p,p,candidates);
    if(candidates.size()>32768 || candidates.size()>maximum_work-stats.preparation_work-stats.pdf_candidates)
      throw SupportLimit3D("Support PDF candidate/work ceiling; no contribution discarded.");
    stats.pdf_candidates+=candidates.size();admit(transfer_memory::Add(stats.retained_bytes,transfer_memory::Bytes(candidates)));
    std::sort(candidates.begin(),candidates.end());long double result=0;
    for(auto i:candidates)result+=componentDensity(components.at(i),vector(point));
    stats.pdf_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    if(result<0 || !std::isfinite(result) || result>std::numeric_limits<double>::max())throw std::runtime_error("Invalid complete support PDF.");
    return double(result);
  }
  std::vector<SupportPoint3D> Draw(size_t count,uint64_t seed,uint64_t first=0) {
    if(count>1048576 || first>1048576 || count>1048576-first)throw std::invalid_argument("Invalid support stream range.");
    admit(transfer_memory::Add(stats.retained_bytes,transfer_memory::Mul(count,sizeof(SupportPoint3D)),transfer_memory::GrowthBound(components.size(),sizeof(unsigned long))));
    std::mt19937_64 random(seed);random.discard(5*first);std::vector<SupportPoint3D> result;result.reserve(count);
    for(size_t i=0;i<count;++i) {
      std::array<long double,5> u;for(auto& x:u)x=std::min(std::ldexp(static_cast<long double>(random()),-64),std::nextafter(1.L,0.L));
      const auto k=std::upper_bound(cdf.begin(),cdf.end(),u[0])-cdf.begin();const auto& c=components.at(order.at(k));const auto r=radial(c,u[3]);V p;
      if(c.kind==0) {const auto a=std::sqrt(u[1]);p=add(c.origin,add(mul(c.edge0,a*(1-u[2])),mul(c.edge1,a*u[2])));p=add(p,mul(c.normal,u[4]<.5L?-r:r));}
      else if(c.kind==1) {
        const auto phi=(u[2]-.5L)*c.width;const auto d=add(mul(c.center,std::cos(phi)),mul(c.side,std::sin(phi)));
        p=add(add(c.origin,mul(c.edge0,u[1])),mul(add(mul(c.plane0,d[0]),mul(c.plane1,d[1])),r));
      } else {
        const auto z=2*u[1]-1,phi=(u[2]-.5L)*c.width;const auto d=add(mul(c.center,-std::cos(phi)),mul(c.side,std::sin(phi)));
        p=add(c.origin,mul(add(mul(d,std::sqrt(std::max(0.L,1-z*z))),mul(c.normal,z)),r));
      }
      const Point actual{double(p[0]),double(p[1]),double(p[2])};const double pdf=Density(actual);const auto selected=c.coefficient/std::max(c.h0,c.b*r);
      if(!(pdf>0) || selected>(1+1e-8L)*pdf || !std::isfinite(1/pdf))throw std::runtime_error("Sample has no complete finite mixture PDF.");
      result.push_back({actual,1/pdf,c.kind});
    }
    return result;
  }
};
}  // namespace SU2Native3D
