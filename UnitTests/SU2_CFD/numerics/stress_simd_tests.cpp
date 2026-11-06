#include "catch.hpp"
#include "../../../SU2_CFD/include/numerics/flow/flow_diffusion.hpp"
#include "../../../SU2_CFD/include/variables/CNSVariable.hpp"
#include "../../../SU2_CFD/include/numerics_simd/flow/diffusion/viscous_fluxes.hpp"

namespace {
template <size_t Dim>
struct ViscousProbe : CCompressibleViscousFlux<Dim> {
  using Base = CCompressibleViscousFlux<Dim>;
  using Base::Base;
  using Base::viscousTerms;
  void ComputeFlux(Int, const CConfig&, const CGeometry&, const CVariable&, UpdateType, Double,
                   CSysVector<su2double>&, SparseMatrixType&, su2activevector*) const override {}
};

template <size_t Dim>
void CheckFlux(const std::string& mode, double scale) {
  std::stringstream options;
  options << "SOLVER= RANS\nREYNOLDS_NUMBER= 1000\nMACH_NUMBER= 0.1\nKIND_TURB_MODEL= SST\n"
             "SST_OPTIONS= (V2003m)\nMGLEVEL= 0\nSTRESS_DISCREPANCY= " << mode
          << "\nSTRESS_DISCREPANCY_SCALE= " << scale << "\n";
  if (mode == "PRESCRIBED") {
    const std::string source = __FILE__;
    options << "STRESS_DISCREPANCY_FILE= " << source.substr(0, source.find_last_of("/\\"))
            << "/../../fixtures/stress/positive.suf\n";
  }
  if (mode == "EPM_MATCHED") options << "UQ_COMPONENT= 1\nUQ_DELTA_B= 0.7\nUQ_URLX= 0.1\n";
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  config.SetLength_Ref(1);
  config.SetVelocity_Ref(1);
  config.SetDensity_Ref(1);
  config.SetGas_ConstantND(1);
  config.SetModVel_FreeStream(1);
  constexpr size_t N = Double::Size, Vars = Dim + 2, Prim = Dim + 9;
  double velocity[3] = {3, 4, 1};
  CNSVariable state(1.2, velocity, 10, 2 * N, Dim, Vars, &config);
  CVariable turbulence(2 * N, Dim, 2, &config);
  CGeometry geometry;
  geometry.nodes = new CPoint(2 * N, Dim);
  Int left, right, edge(size_t(0));
  CCompressiblePrimitives<Dim, Prim> average;
  CPair<CCompressiblePrimitives<Dim, Prim>> pair;
  VectorDbl<Dim> displacement, normal;
  VectorDbl<Vars> flux;
  MatrixDbl<Vars> ji, jj;
  flux = Double(0.); ji = Double(0.); jj = Double(0.);
  for (size_t d = 0; d < Dim; ++d) {
    displacement(d) = d == 0 ? .5 : 0.;
    normal(d) = d == 0 ? .6 : (d == 1 ? .8 : 0.);
  }
  for (size_t lane = 0; lane < N; ++lane) {
    left[lane] = 2 * lane; right[lane] = 2 * lane + 1;
    for (size_t v = 0; v < Prim; ++v) average.all(v)[lane] = 1.;
    for (size_t d = 0; d < Dim; ++d) average.velocity(d)[lane] = velocity[d] + .2 * lane;
    average.density()[lane] = 1.2 + .1 * lane;
    average.enthalpy()[lane] = 10;
    average.laminarVisc()[lane] = .01;
    average.eddyVisc()[lane] = .06 + .003 * lane;
    average.thermalCond()[lane] = .1;
    for (size_t side = 0; side < 2; ++side) {
      const auto p = 2 * lane + side;
      for (size_t v = 0; v <= Dim; ++v)
        for (size_t d = 0; d < Dim; ++d) state.GetGradient_Primitive()(p, v, d) = 0.;
      turbulence.SetSolution(p, 0, .1 + .01 * lane);
      for (size_t d = 0; d < Dim; ++d) geometry.nodes->SetCoord(p, d, d == 0 ? .25 + .5 * side : .5);
      for (size_t d = 0; d < Dim; ++d)
        for (size_t j = 1; j < Dim; ++j)
          state.GetGradient_Primitive()(p, d + 1, j) = .3 * (d + 1) * (j + 1) + .1 * lane;
    }
  }
  pair.i = average; pair.j = average;
  int mesh = 0;
  const CVariable* turb = &turbulence;
  ViscousProbe<Dim> probe(config, mesh, turb);
  probe.viscousTerms(edge, left, right, average, pair, state, displacement, geometry, config,
                     Double(1), normal, false, flux, ji, jj);
  for (size_t lane = 0; lane < N; ++lane) {
    CAvgGrad_Flow scalar(Dim, Vars, true, &config);
    double primitive[Prim], scalar_normal[Dim];
    for (size_t v = 0; v < Prim; ++v) primitive[v] = average.all(v)[lane];
    for (size_t d = 0; d < Dim; ++d) scalar_normal[d] = normal(d)[lane];
    scalar.SetPrimitive(primitive, primitive);
    scalar.SetPrimVarGradient(state.GetGradient_Primitive(left[lane]), state.GetGradient_Primitive(right[lane]));
    scalar.SetCoord(geometry.nodes->GetCoord(left[lane]), geometry.nodes->GetCoord(right[lane]));
    scalar.SetNormal(scalar_normal);
    scalar.SetTurbKineticEnergy(.1 + .01 * lane, .1 + .01 * lane);
    const auto result = scalar.ComputeResidual(&config);
    for (size_t v = 0; v < Vars; ++v)
      REQUIRE(-flux(v)[lane] == Approx(result[v]).margin(1e-12));
  }
}
}  // namespace

TEST_CASE("Scalar and SIMD viscous stress operators agree", "[stress][simd]") {
  CheckFlux<2>("NONE", 1);
  for (double scale : {0., .4, 1.}) {
    CheckFlux<2>("PRESCRIBED", scale);
    CheckFlux<2>("EPM_MATCHED", scale);
    CheckFlux<3>("EPM_MATCHED", scale);
  }
}
