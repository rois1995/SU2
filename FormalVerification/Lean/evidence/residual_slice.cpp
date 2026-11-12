
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
using su2double = double;
struct Block {
  std::array<double,5> v{};
  double& operator()(unsigned, unsigned i) { return v.at(i); }
};
void legacy(unsigned nDim, const double* UnitNormal, double gridNormalVel, Block& LinSysRes) {
  const unsigned iPoint = 0, iVel = 1;
  const bool compressibleProjection = true;

    su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }


}
void repaired(unsigned nDim, const double* UnitNormal, double gridNormalVel, Block& LinSysRes) {
  const unsigned iPoint = 0, iVel = 1;
  const bool compressibleProjection = true;

    su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    if (compressibleProjection) {
      /*--- Project in the wall frame: normal momentum follows density, with consistent pressure work. ---*/
      normalRes -= gridNormalVel * LinSysRes(iPoint, 0);
      LinSysRes(iPoint, iVel + nDim) -= gridNormalVel * normalRes;
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }


}
void refactored(unsigned nDim, const double* UnitNormal, double gridNormalVel, Block& LinSysRes) {
  const unsigned iPoint = 0, iVel = 1;
  const bool compressibleProjection = true;

    su2double wallResidual = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      wallResidual += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    if (compressibleProjection) {
      /*--- Project in the wall frame: normal momentum follows density, with consistent pressure work. ---*/
      wallResidual -= gridNormalVel * LinSysRes(iPoint, 0);
      LinSysRes(iPoint, iVel + nDim) -= gridNormalVel * wallResidual;
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      LinSysRes(iPoint, iVel + iDim) -= wallResidual * UnitNormal[iDim];
    }


}
double normalMomentum(unsigned nDim, const double* UnitNormal, Block& LinSysRes) {
  const unsigned iPoint = 0, iVel = 1;

    su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
  return normalRes;
}
void helperFactored(unsigned nDim, const double* UnitNormal, double gridNormalVel, Block& LinSysRes) {
  const unsigned iPoint = 0, iVel = 1;
  const bool compressibleProjection = true;
    su2double normalRes = normalMomentum(nDim, UnitNormal, LinSysRes);
    if (compressibleProjection) {
      /*--- Project in the wall frame: normal momentum follows density, with consistent pressure work. ---*/
      normalRes -= gridNormalVel * LinSysRes(iPoint, 0);
      LinSysRes(iPoint, iVel + nDim) -= gridNormalVel * normalRes;
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }


}

int main() {
  const double n[3] = {1,0,0};
  Block bad, good;
  bad.v = {0.5,0,0,0,0}; good = bad;
  legacy(2,n,2,bad); repaired(2,n,2,good);
  const double oldDefect = (2+bad.v[1]) - 2*(1+bad.v[0]);
  const double newDefect = (2+good.v[1]) - 2*(1+good.v[0]);
  assert(oldDefect == -1 && newDefect == 0);
  std::mt19937 gen(1729);
  std::uniform_real_distribution<double> sample(-10,10);
  unsigned cases = 0;
  for (unsigned dim : {2u,3u}) {
    for (unsigned trial=0; trial<500; ++trial) {
      double normal[3]{}, wall[3]{}, len2=0, g=0;
      for (unsigned i=0;i<dim;++i) { normal[i]=sample(gen); wall[i]=sample(gen); len2+=normal[i]*normal[i]; }
      for (unsigned i=0;i<dim;++i) { normal[i]/=std::sqrt(len2); g+=wall[i]*normal[i]; }
      Block input;
      for (unsigned i=0;i<dim+2;++i) input.v[i]=sample(gen);
      Block actual=input, renamedActual=input, factoredActual=input;
      repaired(dim,normal,g,actual);
      refactored(dim,normal,g,renamedActual);
      helperFactored(dim,normal,g,factoredActual);
      assert(std::memcmp(actual.v.data(),renamedActual.v.data(),sizeof(actual.v))==0);
      assert(std::memcmp(actual.v.data(),factoredActual.v.data(),sizeof(actual.v))==0);
      double a=0, defect=-actual.v[0]*g;
      for (unsigned i=0;i<dim;++i) a+=(input.v[i+1]-input.v[0]*wall[i])*normal[i];
      assert(actual.v[0]==input.v[0]);
      for (unsigned i=0;i<dim;++i) {
        const double expected=input.v[i+1]-a*normal[i];
        assert(std::abs(actual.v[i+1]-expected)<1e-11);
        defect+=actual.v[i+1]*normal[i];
      }
      assert(std::abs(actual.v[dim+1]-(input.v[dim+1]-g*a))<1e-10);
      assert(std::abs(defect)<1e-11);
      ++cases;
    }
  }
  std::cout << "source residual slice: old defect=-1, repaired defect=0; "
            << cases << " finite 2D/3D cases; renamed and helper-factored slices bitwise equal\n";
}
