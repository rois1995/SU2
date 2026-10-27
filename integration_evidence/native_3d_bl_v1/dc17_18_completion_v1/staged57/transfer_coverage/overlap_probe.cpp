#include "SU2_CFD/include/adaptation/ConservativeKernels.hpp"
#include "SU2_CFD/include/adaptation/ConvexClipping.hpp"
#include <array>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
using Tet=std::array<std::array<double,3>,4>;
void accumulate(void* ctx,unsigned short i,double v,const double*,const double*){static_cast<double*>(ctx)[i]+=v;}
int main(){
  std::cout<<std::setprecision(17);unsigned n;std::cin>>n;std::vector<Tet> target(n),donor;
  for(auto& t:target)for(auto& p:t)for(auto& x:p){std::string s;std::cin>>s;x=std::strtod(s.c_str(),nullptr);}
  std::array<int,3> perm{0,1,2};do{Tet t{};for(unsigned i=0;i<3;++i){t[i+1]=t[i];t[i+1][perm[i]]=1;}for(auto& p:t)p[1]+=.37*p[0]+.61*p[2];donor.push_back(t);}while(std::next_permutation(perm.begin(),perm.end()));
  double rawTotal=0,splitTotal=0,targetVolume=0,fill=0,lost=0,worst=0;std::array<double,6> covered{};unsigned worstI=0,worstJ=0;
  for(unsigned i=0;i<n;++i){const double* nodes[4];for(unsigned k=0;k<4;++k)nodes[k]=target[i][k].data();conservative::Frame f;conservative::SetFrame(3,nodes,f);targetVolume+=f.volume;double cov[4]{};
    for(unsigned j=0;j<donor.size();++j){const double* dn[4];double local[4][3];for(unsigned k=0;k<4;++k){dn[k]=donor[j][k].data();for(unsigned d=0;d<3;++d)local[k][d]=dn[k][d]-f.origin[d];}
      const double* lp[4]={local[0],local[1],local[2],local[3]};convex_clip::Polyhedron poly;poly.InitTetrahedron(lp);
      for(unsigned k=0;k<4;++k)poly.Clip(f.G[k][0],f.G[k][1],f.G[k][2],f.a[k]);double raw=0,centroid[3];poly.Moments(raw,centroid);rawTotal+=raw; if(!std::isfinite(raw)){std::cout<<"nonfinite_pair "<<i<<" "<<j<<"\n";for(auto p:target[i])std::cout<<"bad_target "<<p[0]<<" "<<p[1]<<" "<<p[2]<<"\n";for(auto p:donor[j])std::cout<<"bad_donor "<<p[0]<<" "<<p[1]<<" "<<p[2]<<"\n";}
      double pieces[4]{};bool clipped=false;conservative::Overlap(3,f,dn,conservative::SimplexMeasure(3,dn),accumulate,pieces,&clipped);
      double sum=0;for(unsigned k=0;k<4;++k){sum+=pieces[k];cov[k]+=pieces[k];}splitTotal+=sum;covered[j]+=sum;
      if(std::abs(raw-sum)>worst){worst=std::abs(raw-sum);worstI=i;worstJ=j;}
    }for(auto v:cov)fill+=std::max(0.,f.volume/4-v);
  }
  for(auto v:covered)lost+=std::max(0.,1./6-v);
  std::cout<<"target "<<targetVolume<<" raw "<<rawTotal<<" split "<<splitTotal<<" fill "<<fill<<" lost "<<lost<<" worst "<<worst<<" target_id "<<worstI<<" donor_id "<<worstJ<<"\n";
  for(auto v:covered)std::cout<<"donor_coverage "<<v<<"\n";
  for(auto p:target[worstI])std::cout<<"target_point "<<p[0]<<" "<<p[1]<<" "<<p[2]<<"\n";
  for(auto p:donor[worstJ])std::cout<<"donor_point "<<p[0]<<" "<<p[1]<<" "<<p[2]<<"\n";
}
