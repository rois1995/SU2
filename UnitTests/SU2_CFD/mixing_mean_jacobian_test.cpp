#include "../../SU2_CFD/include/solvers/MixingMeanFlux.hpp"
#include <cassert>
#include <iostream>
using namespace MixingMeanFlux;

int main() {
  const double x[3]={0.4,-0.3,0.2}, y[3]={0.1,0.2,-0.4}, weights[3]={0.2,0.3,0.5};
  const double u[3][5]={{1,2,3,4,5},{2,3,4,5,6},{3,4,5,6,7}};
  const double d[3][5]={{.2,-.3,.4,-.1,.7},{-.5,.2,-.4,.9,.1},{.8,-.4,.2,-.3,.6}};
  VertexJacobian jac[3];
  for (size_t i=0;i<3;++i) {
    jac[i].point=i; jac[i].x=x[i];jac[i].y=y[i];jac[i].weight=weights[i];
    for(size_t col=0;col<5;++col) {
      Flux column{};
      for(size_t row=0;row<5;++row)column[row]=(row==col ? 2.0 : .1*(row+1)*(col+1))*(i+1);
      const auto cyl=Cylindrical(column,x[i],y[i]);
      for(size_t row=0;row<5;++row)jac[i].value[5*row+col]=cyl[row];
    }
  }
  auto residual=[&](double step) {
    std::array<Flux,3> raw{},result{};
    Flux total{},fixed={{2,3,4,5,6}};
    for(size_t i=0;i<3;++i) {
      double state[5];for(size_t j=0;j<5;++j)state[j]=u[i][j]+step*d[i][j];
      raw[i]=jac[i].Apply(state);
      for(size_t j=0;j<5;++j)total[j]+=raw[i][j];
    }
    for(size_t i=0;i<3;++i) {
      Flux diff;for(size_t j=0;j<5;++j)diff[j]=fixed[j]-total[j];
      const auto cart=CartesianCorrection(raw[i],1,x[i],y[i]);
      const auto correction=CartesianCorrection(diff,weights[i],x[i],y[i]);
      for(size_t j=0;j<5;++j)result[i][j]=cart[j]+correction[j];
    }
    return result;
  };
  Flux sum{};
  for(size_t i=0;i<3;++i) {const auto value=jac[i].Apply(d[i]);for(size_t j=0;j<5;++j)sum[j]-=value[j];}
  const auto plus=residual(1e-5),minus=residual(-1e-5);
  Flux corrected_sum{};
  for(size_t i=0;i<3;++i) {
    const auto raw=CartesianCorrection(jac[i].Apply(d[i]),1,x[i],y[i]);
    const auto correction=CartesianCorrection(sum,weights[i],x[i],y[i]);
    Flux applied{};
    for(size_t j=0;j<5;++j) {
      applied[j]=raw[j]+correction[j];
      assert(std::abs(applied[j]-(plus[i][j]-minus[i][j])/2e-5)<2e-8);
    }
    const auto cylindrical=Cylindrical(applied,x[i],y[i]);
    for(size_t j=0;j<5;++j)corrected_sum[j]+=cylindrical[j];
  }
  for(auto value:corrected_sum)assert(std::abs(value)<1e-12);
  // Shared-flux mapping is linear, including unequal pitch and nonmatching radial bins.
  const std::vector<double> a={.3,.4,.5},b={.29,.35,.43,.52};
  std::vector<Flux> fa(3),fb(4),da(3),db(4);
  for(size_t i=0;i<3;++i)for(size_t j=0;j<5;++j){fa[i][j]=(i+1)*(j+1);da[i][j]=.2*(i+2)*(j+1);}
  for(size_t i=0;i<4;++i)for(size_t j=0;j<5;++j){fb[i][j]=-.3*(i+1)*(j+1);db[i][j]=-.1*(i+2)*(j+1);}
  const auto tangent=Common(a,da,41,b,db,73),base=Common(a,fa,41,b,fb,73);
  for(size_t i=0;i<3;++i)for(size_t j=0;j<5;++j)fa[i][j]+=1e-5*da[i][j];
  for(size_t i=0;i<4;++i)for(size_t j=0;j<5;++j)fb[i][j]+=1e-5*db[i][j];
  const auto shifted=Common(a,fa,41,b,fb,73);
  for(size_t i=0;i<3;++i)for(size_t j=0;j<5;++j)assert(std::abs((shifted[i][j]-base[i][j])/1e-5-tangent[i][j])<1e-8);
  std::cout<<"Frozen-mean nonlocal Jacobian and nonmatching unequal-pitch transfer tangents passed\n";
}
