// Probe the frozen-target derivative versus the refreshed shared-flux response; no CFD run.
#include "../../SU2_CFD/include/solvers/MixingMeanFlux.hpp"
#include <cassert>
#include <iostream>
using namespace MixingMeanFlux;
using State = std::array<Flux,4>;

int main() {
  const double x[4]={.4,0,.6,0},y[4]={0,.4,0,.6};
  const double weights[4]={.3,.7,.4,.6};
  const std::vector<double> radii={.4,.6};
  State u{},d{};
  for(size_t i=0;i<4;++i)for(size_t j=0;j<5;++j){u[i][j]=1+.2*i+.1*j;d[i][j]=.3+.03*i-.02*j;}
  auto raw=[](const State& state) {
    State result{};
    for(size_t i=0;i<4;++i)for(size_t j=0;j<5;++j)
      for(size_t k=0;k<5;++k)result[i][j]+=(j==k?1+.1*i+.2*j:.01*(j+1)*(k+1))*state[i][k];
    return result;
  };
  auto sum=[&](const State& values) {
    std::vector<Flux> result(2);
    for(size_t i=0;i<4;++i){const auto flux=Cylindrical(values[i],x[i],y[i]);for(size_t j=0;j<5;++j)result[i/2][j]+=flux[j];}
    return result;
  };
  const std::vector<Flux> donor(2);
  const auto fixed=Common(radii,sum(raw(u)),41,radii,donor,41);
  auto residual=[&](double step,bool refresh) {
    State state=u;for(size_t i=0;i<4;++i)for(size_t j=0;j<5;++j)state[i][j]+=step*d[i][j];
    State result=raw(state);const auto total=sum(result);
    const auto target=refresh?Common(radii,total,41,radii,donor,41):fixed;
    for(size_t i=0;i<4;++i){Flux diff{};for(size_t j=0;j<5;++j)diff[j]=target[i/2][j]-total[i/2][j];
      const auto c=CartesianCorrection(diff,weights[i],x[i],y[i]);for(size_t j=0;j<5;++j)result[i][j]+=c[j];}
    return result;
  };
  const auto derivative=raw(d);const auto mean=sum(derivative);
  const auto fp=residual(1e-5,false),fm=residual(-1e-5,false);
  const auto rp=residual(1e-5,true),rm=residual(-1e-5,true);
  double frozen_error=0,refreshed_error=0,missing_response=0;
  State frozen_product{};
  for(size_t i=0;i<4;++i){
    Flux negative{},half{};for(size_t j=0;j<5;++j){negative[j]=-mean[i/2][j];half[j]=-.5*mean[i/2][j];}
    const auto current=CartesianCorrection(negative,weights[i],x[i],y[i]);
    const auto consistent=CartesianCorrection(half,weights[i],x[i],y[i]);
    for(size_t j=0;j<5;++j){
      frozen_product[i][j]=derivative[i][j]+current[j];
      const double fd_frozen=(fp[i][j]-fm[i][j])/2e-5,fd_refresh=(rp[i][j]-rm[i][j])/2e-5;
      frozen_error=std::max(frozen_error,std::abs(frozen_product[i][j]-fd_frozen));
      refreshed_error=std::max(refreshed_error,std::abs(derivative[i][j]+consistent[j]-fd_refresh));
      missing_response=std::max(missing_response,std::abs(frozen_product[i][j]-fd_refresh));
    }
  }
  double cancelled_mean=0;
  for(const auto& row:sum(frozen_product))for(double value:row)cancelled_mean=std::max(cancelled_mean,std::abs(value));
  assert(frozen_error<1e-8 && refreshed_error<1e-8 && cancelled_mean<1e-12 && missing_response>.01);
  std::cout<<"{\"frozen_target_fd_error\":"<<frozen_error<<",\"refreshed_target_fd_error_with_self_response\":"<<refreshed_error
           <<",\"current_product_error_when_target_refreshed\":"<<missing_response<<",\"current_boundary_mean_derivative\":"<<cancelled_mean<<"}\n";
  // Scalar model: F(u)=0.5*a*u, target refreshed between solves. theta=dt*a, NOT the case CFL.
  for(double theta:{1.,2.,4.,8.,20.}){
    const double off=1-.5*theta/(1+theta),frozen=1-.5*theta,self=1/(1+.5*theta);
    assert(std::abs(off)<1 && std::abs(self)<1);
    if(theta>4)assert(std::abs(frozen)>1);
    std::cout<<"{\"theta\":"<<theta<<",\"amplification_no_extra_jv\":"<<off<<",\"amplification_frozen_target_jv\":"<<frozen
             <<",\"amplification_with_local_target_response\":"<<self<<"}\n";
  }
}
