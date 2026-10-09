/* Local wall-model numerical checks; LGPL-2.1-or-later, SU2 Contributors. */
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "../../SU2_CFD/include/solvers/WallFunctionModel.hpp"

using WallFunctionModel::Input;
using WallFunctionModel::State;

void Require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

double Oracle(const Input& p, double u) {
  auto v = [](const su2double& x) { return SU2_TYPE::GetValue(x); };
  const double tw = p.isothermal ? v(p.wall_temperature) :
      (v(p.normal_temperature) + v(p.recovery)*v(p.velocity)*v(p.velocity)/(2*v(p.cp))) /
      (1+v(p.heat_flux)*v(p.wall_viscosity)*v(p.gas_constant)*v(p.velocity)/(v(p.pressure)*v(p.conductivity)*u*u));
  const double rho = v(p.pressure)/(v(p.gas_constant)*tw);
  const double up = v(p.velocity)/u;
  const double gamma = v(p.recovery)*u*u/(2*v(p.cp)*tw);
  const double beta = v(p.heat_flux)*v(p.wall_viscosity)/(rho*tw*v(p.conductivity)*u);
  const double q = std::sqrt(beta*beta+4*gamma);
  const double white = std::exp(v(p.kappa)/std::sqrt(gamma)*
      (std::asin((2*gamma*up-beta)/q)-std::asin(-beta/q))-v(p.kappa)*v(p.B));
  const double ku = v(p.kappa)*up;
  const double y = up+white-std::exp(-v(p.kappa)*v(p.B))*(1+ku+ku*ku/2+ku*ku*ku/6);
  return rho*u*v(p.distance)/v(p.wall_viscosity)-y;
}

double Reference(const Input& p, double around) {
  double low = around*.9, high = around*1.1;
  Require(std::isfinite(Oracle(p,low)) && Oracle(p,low)<0, "reference lower bracket");
  Require(std::isfinite(Oracle(p,high)) && Oracle(p,high)>0, "reference upper bracket");
  for (int i=0;i<80;++i) {
    const double middle = (low+high)/2;
    if (Oracle(p,middle)<0) low=middle;
    else high=middle;
  }
  return (low+high)/2;
}

int main() {
  try {
    int checks=0;
    const double accuracy = std::max(1e-8, 128.0*std::numeric_limits<passivedouble>::epsilon());
    for (bool isothermal : {false,true}) for (double heat : {-1000.,0.,1000.})
      for (double velocity : {10.,150.,500.}) for (double seed : {1e-12,1.,1000.}) {
        Input p{150000,300,320,1.8e-5,1.9e-5,.026,.001,velocity,heat,287.058,1004.703,.89,.41,5.5,isothermal};
        State result;
        if (!WallFunctionModel::Solve(p,seed,200,.5,result)) {
          std::cerr << "isothermal=" << isothermal << " heat=" << heat << " velocity=" << velocity << " seed=" << seed << '\n';
          throw std::runtime_error("root must converge");
        }
        const double root=SU2_TYPE::GetValue(result.u_tau);
        const double reference=Reference(p,root);
        Require(std::abs(root/reference-1)<accuracy,"root agrees with independent bisection");
        Require(SU2_TYPE::GetValue(result.temperature)>0 && SU2_TYPE::GetValue(result.eddy_viscosity)>0,"positive state");
        const double step=root*1e-5;
        const double fd=(Oracle(p,root+step)-Oracle(p,root-step))/(2*step);
        const double derivative=SU2_TYPE::GetValue(WallFunctionModel::Derivative(p,result));
        Require(std::abs(derivative/fd-1)<std::max(1e-5,accuracy*8),"consistent root derivative");
        if (!isothermal && heat==0) {
          Require(std::abs(SU2_TYPE::GetValue(result.temperature)-(300+.89*velocity*velocity/(2*1004.703)))<std::max(1e-10,accuracy*SU2_TYPE::GetValue(result.temperature)),"adiabatic thermal identity");
        }
#if defined(CODI_FORWARD_TYPE) || defined(CODI_REVERSE_TYPE)
#ifdef CODI_REVERSE_TYPE
        auto& tape = su2double::getTape();
        tape.reset();
        tape.setActive();
        tape.registerInput(p.velocity);
#else
        p.velocity.setGradient(1);
#endif
        State differentiated;
        Require(WallFunctionModel::Solve(p,seed,200,.5,differentiated),"AD root converges");
#ifdef CODI_REVERSE_TYPE
        tape.registerOutput(differentiated.u_tau);
        tape.setPassive();
        differentiated.u_tau.setGradient(1);
        tape.evaluate();
        const double ad_root = p.velocity.getGradient();
#else
        const double ad_root = differentiated.u_tau.getGradient();
#endif
        Input plus=p,minus=p;
        plus.velocity=velocity+velocity*1e-5;
        minus.velocity=velocity-velocity*1e-5;
        State right,left;
        Require(WallFunctionModel::Solve(plus,seed,200,.5,right) && WallFunctionModel::Solve(minus,seed,200,.5,left),"FD roots converge");
        const double fd_root=(SU2_TYPE::GetValue(right.u_tau)-SU2_TYPE::GetValue(left.u_tau))/(2*velocity*1e-5);
        Require(std::abs(ad_root/fd_root-1)<1e-4,"root AD sensitivity agrees with finite difference");
#endif
        ++checks;
      }
    Input p{150000,300,300,1.8e-5,1.8e-5,.026,.001,150,0,287.058,1004.703,.89,.41,5.5,false};
    State result;
    for (double x : {-1e-4, -1e-8, 0., 1e-8, 1e-4})
      Require(std::abs(SU2_TYPE::GetValue(WallFunctionModel::ExpMinusOne(x))-std::expm1(x))<std::max(1e-15,accuracy*std::abs(std::expm1(x))),
              "small exponential difference agrees with standard reference");
    State invalid;
    Require(!WallFunctionModel::Evaluate(p,1e-12,invalid),"overflow candidate rejected");
    p.heat_flux=-1e6;
    Require(!WallFunctionModel::Evaluate(p,1,invalid),"negative thermal denominator rejected");
    p.heat_flux=0;
    Require(!WallFunctionModel::Solve(p,1,0,.5,result),"iteration limit fails explicitly");
    Require(!WallFunctionModel::Solve(p,1,1,.5,result),"insufficient iterations fail explicitly");
    Require(!WallFunctionModel::Solve(p,1,200,0,result),"invalid relaxation rejected");
    p.distance=0;
    Require(!WallFunctionModel::Solve(p,1,200,.5,result),"zero distance rejected");
    p.distance=.001; p.velocity=0;
    Require(!WallFunctionModel::Solve(p,1,200,.5,result),"zero shear disables model");
    p.velocity=150; p.pressure=std::numeric_limits<double>::quiet_NaN();
    Require(!WallFunctionModel::Solve(p,1,200,.5,result),"non-finite thermodynamics rejected");
    std::cout<<checks<<" root/derivative cases and failure checks passed\n";
  } catch (const std::exception& e) {
    std::cerr<<e.what()<<'\n';
    return 1;
  }
}
