#include "../../SU2_CFD/include/solvers/MixingMeanFlux.hpp"
#include <cassert>
#include <numeric>
using namespace MixingMeanFlux;
int main() {
  const std::vector<double> ra={0.4,0.43,0.5}, rb={0.39,0.45,0.49,0.51};
  const auto ea=Edges(ra),eb=Edges(rb);
  std::vector<Flux> a(ra.size()),b(rb.size());
  for (size_t i=0;i<a.size();++i) for(size_t v=0;v<5;++v) a[i][v]=(i+1)*(v+1)*(ea[i+1]-ea[i])/41;
  for (size_t i=0;i<b.size();++i) for(size_t v=0;v<5;++v) b[i][v]=-double(i+2)*(v+1)*(eb[i+1]-eb[i])/73;
  const auto ca=Common(ra,a,41,rb,b,73),cb=Common(rb,b,73,ra,a,41);
  for(size_t v=0;v<5;++v) {
    double sum=0; for(const auto& f:ca)sum+=41*f[v]; for(const auto& f:cb)sum+=73*f[v];
    assert(std::abs(sum)<1e-13);
  }
  // Constant flux density is reproduced on nonmatching radial meshes and unequal pitches.
  for(size_t i=0;i<a.size();++i) for(size_t v=0;v<5;++v)a[i][v]=(v+1)*(ea[i+1]-ea[i])/41;
  for(size_t i=0;i<b.size();++i) for(size_t v=0;v<5;++v)b[i][v]=-double(v+1)*(eb[i+1]-eb[i])/73;
  const auto constant=Common(ra,a,41,rb,b,73);
  for(size_t i=0;i<a.size();++i)for(size_t v=0;v<5;++v)assert(std::abs(constant[i][v]-a[i][v])<1e-14);
  const Flux delta={{2,3,4,5,6}}; Flux reconstructed={};
  const double x[]={0.4,0.3,-0.2},y[]={0.0,0.4,0.3},w[]={0.2,0.3,0.5};
  for(size_t i=0;i<3;++i) {
    const auto c=CartesianCorrection(delta,w[i],x[i],y[i]); const auto r=std::hypot(x[i],y[i]);
    reconstructed[0]+=c[0]; reconstructed[1]+=c[3]; reconstructed[2]+=x[i]*c[2]-y[i]*c[1];
    reconstructed[3]+=(x[i]*c[1]+y[i]*c[2])/r; reconstructed[4]+=c[4];
  }
  for(size_t v=0;v<5;++v)assert(std::abs(reconstructed[v]-delta[v])<1e-13);
  bool failed=false;try {Common(ra,a,0,rb,b,73);}catch(const std::runtime_error&){failed=true;}assert(failed);
  failed=false;try {Common({0.5,0.3},std::vector<Flux>(2),41,rb,b,73);}catch(const std::runtime_error&){failed=true;}assert(failed);
}
