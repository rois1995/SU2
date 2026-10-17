/*!
 * \file CNativeChordEnvelope.hpp
 * \brief Bounded nearest-feature distance partition along a faceted-wall chord.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace SU2Native3D {

/*! Geometry-only cuts, NOT a replacement for the authoritative pointwise metric or exact donor predicates.
 * Each finite face/edge/vertex contributes a restricted quadratic distance. Their lower envelope includes
 * global feature competition, so band roots do not require nearest-wall searches for every candidate root.
 * Long-double construction is numerical, not an outward-rounded distance certificate. */
class ChordEnvelope {
 public:
  struct WorkLimit : std::runtime_error {using std::runtime_error::runtime_error;};
 private:
  using V=std::array<long double,3>;
  struct Feature {V r,v;long double lo=0,hi=1;};
  struct Piece {long double lo,hi;size_t feature;};
  static constexpr size_t Missing=std::numeric_limits<size_t>::max(),MaximumPieces=65536;
  V origin,velocity;
  std::vector<Feature> features;
  std::vector<Piece> pieces{{0,1,Missing}};
  size_t work=0,workLimit;
  static long double Dot(const V& a,const V& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
  static V Cross(const V& a,const V& b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
  static V Delta(const std::array<double,3>& a,const std::array<double,3>& b){
    for(unsigned j=0;j<3;++j)if(!std::isfinite(a[j]) || !std::isfinite(b[j]))throw std::invalid_argument("Nonfinite native chord feature.");
    return {(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};
  }
  static V At(const Feature& f,long double t){return {std::fma(t,f.v[0],f.r[0]),std::fma(t,f.v[1],f.r[1]),std::fma(t,f.v[2],f.r[2])};}
  static bool Clip(Feature& f,long double value,long double slope) {
    if(slope>0)f.lo=std::max(f.lo,-value/slope);
    else if(slope<0)f.hi=std::min(f.hi,-value/slope);
    else if(value<0)return false;
    return f.lo<f.hi;
  }
  void Add(Feature candidate) {
    if(!(candidate.lo<candidate.hi))return;
    if(features.size()==MaximumPieces)throw WorkLimit("Native chord distance feature budget exhausted.");
    const size_t index=features.size();features.push_back(candidate);
    std::vector<Piece> replacement;
    auto emit=[&](Piece p) {
      if(!(p.lo<p.hi))return;
      if(!replacement.empty() && replacement.back().hi==p.lo && replacement.back().feature==p.feature)
        replacement.back().hi=p.hi;
      else {if(replacement.size()==MaximumPieces)throw WorkLimit("Native chord distance partition budget exhausted.");replacement.push_back(p);}
    };
    for(const auto& p:pieces) {
      const auto left=std::max(p.lo,candidate.lo),right=std::min(p.hi,candidate.hi);
      if(!(left<right)){emit(p);continue;}
      emit({p.lo,left,p.feature});
      if(p.feature==Missing)emit({left,right,index});
      else {
        if(++work>workLimit)throw WorkLimit("Native chord distance comparison budget exhausted.");
        const auto& old=features[p.feature];const auto middle=left+(right-left)/2,half=(right-left)/2;
        const auto a=At(candidate,middle),b=At(old,middle);
        const auto aa=half*half*(Dot(candidate.v,candidate.v)-Dot(old.v,old.v));
        const auto bb=2*half*(Dot(a,candidate.v)-Dot(b,old.v));
        const auto cc=Dot(a,a)-Dot(b,b);
        std::vector<long double> cuts{left,right};
        auto root=[&](long double u){if(u>-1 && u<1){const auto t=std::fma(half,u,middle);if(t>left && t<right)cuts.push_back(t);}};
        if(aa==0){if(bb!=0)root(-cc/bb);}
        else {
          const auto disc=std::fma(bb,bb,-4*aa*cc);
          if(disc>=0){const auto q=-.5L*(bb+std::copysign(std::sqrt(disc),bb));
            if(q==0)root(-bb/(2*aa));else{root(q/aa);root(cc/q);}}
        }
        std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
        for(size_t k=1;k<cuts.size();++k) {
          const auto t=cuts[k-1]+(cuts[k]-cuts[k-1])/2;
          const auto c=At(candidate,t),o=At(old,t);
          emit({cuts[k-1],cuts[k],Dot(c,c)<Dot(o,o)?index:p.feature});
        }
      }
      emit({right,p.hi,p.feature});
    }
    pieces.swap(replacement);
  }
 public:
  explicit ChordEnvelope(const std::array<double,3>& first,const std::array<double,3>& last,size_t maximumWork=1u<<20)
      :velocity(Delta(last,first)),workLimit(maximumWork) {
    for(unsigned j=0;j<3;++j){if(!std::isfinite(first[j]) || !std::isfinite(last[j]))throw std::invalid_argument("Nonfinite native chord.");origin[j]=first[j];}
    if(!(Dot(velocity,velocity)>0) || !workLimit)throw std::invalid_argument("Invalid native chord distance control.");
  }
  void Vertex(const std::array<double,3>& p){
    for(auto x:p)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite native chord vertex.");
    Add({{origin[0]-p[0],origin[1]-p[1],origin[2]-p[2]},velocity});
  }
  void Segment(const std::array<double,3>& a,const std::array<double,3>& b) {
    auto edge=Delta(b,a);const auto length=std::sqrt(Dot(edge,edge));
    if(!(length>0))throw std::invalid_argument("Degenerate native chord support edge.");
    for(auto& x:edge)x/=length;
    V r{origin[0]-a[0],origin[1]-a[1],origin[2]-a[2]};
    const auto projection=Dot(r,edge),slope=Dot(velocity,edge);Feature f{r,velocity};
    if(!Clip(f,projection,slope) || !Clip(f,length-projection,-slope))return;
    for(unsigned j=0;j<3;++j){f.r[j]=std::fma(-projection,edge[j],r[j]);f.v[j]=std::fma(-slope,edge[j],velocity[j]);}
    Add(f);
  }
  void Face(const std::array<std::array<double,3>,3>& p) {
    auto normal=Cross(Delta(p[1],p[0]),Delta(p[2],p[0]));const auto norm=std::sqrt(Dot(normal,normal));
    if(!(norm>0))throw std::invalid_argument("Degenerate native chord support face.");
    for(auto& x:normal)x/=norm;
    Feature f{{},velocity};
    for(unsigned k=0;k<3;++k) {
      const auto inward=Cross(normal,Delta(p[(k+1)%3],p[k]));
      const V r{origin[0]-p[k][0],origin[1]-p[k][1],origin[2]-p[k][2]};
      if(!Clip(f,Dot(r,inward),Dot(velocity,inward)))return;
    }
    const V r{origin[0]-p[0][0],origin[1]-p[0][1],origin[2]-p[0][2]};
    const auto offset=Dot(r,normal),slope=Dot(velocity,normal);
    for(unsigned j=0;j<3;++j){f.r[j]=offset*normal[j];f.v[j]=slope*normal[j];}
    Add(f);
  }
  long double Distance(long double t) const {
    if(!(t>=0 && t<=1) || features.empty())throw std::invalid_argument("Invalid native chord distance query.");
    for(const auto& p:pieces)if(t>=p.lo && t<=p.hi && p.feature!=Missing){const auto r=At(features[p.feature],t);return std::sqrt(Dot(r,r));}
    throw std::runtime_error("Incomplete native chord feature covering.");
  }
  std::vector<double> Cuts(const std::vector<double>& levels,bool featureTransitions=true) const {
    for(auto x:levels)if(!std::isfinite(x) || x<0)throw std::invalid_argument("Invalid native distance level.");
    std::vector<double> cuts{0,1};
    auto keep=[&](long double t){if(t>0 && t<1){if(cuts.size()==MaximumPieces)throw WorkLimit("Native chord distance cuts exceed budget.");cuts.push_back(double(t));}};
    const Feature* previous=nullptr;
    for(const auto& p:pieces)if(p.feature!=Missing) {
      const auto& f=features[p.feature];
      // Coplanar tiles with identical displacement functions do not add artificial distance breaks.
      if(featureTransitions && (!previous || f.r!=previous->r || f.v!=previous->v))keep(p.lo);
      previous=&f;
      const auto speed=Dot(f.v,f.v);if(!(speed>0))continue;
      const auto center=-Dot(f.r,f.v)/speed;
      const auto r=At(f,center);const auto minimum=std::sqrt(Dot(r,r));
      for(double level:levels) {
        if(!std::isfinite(level) || level<0)throw std::invalid_argument("Invalid native distance level.");
        const long double square=((long double)level-minimum)*((long double)level+minimum);
        if(square<0)continue;
        const auto width=std::sqrt(square/speed);
        for(auto t:{center-width,center+width})if(t>=p.lo && t<=p.hi)keep(t);
      }
    }
    std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());return cuts;
  }
  size_t Comparisons() const{return work;}
  size_t Features() const{return features.size();}
  size_t Pieces() const{return pieces.size();}
};
}  // namespace SU2Native3D
