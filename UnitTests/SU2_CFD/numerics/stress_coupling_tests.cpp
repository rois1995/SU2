#include "catch.hpp"
#include "../../../SU2_CFD/include/numerics/flow/flow_diffusion.hpp"
#include "../../../SU2_CFD/include/numerics/turbulent/turb_sources.hpp"
#include "../../../SU2_CFD/include/variables/CEulerVariable.hpp"

namespace {
std::string Fixture(const std::string& name) {
  const std::string source = __FILE__;
  return source.substr(0, source.find_last_of("/\\")) + "/../../fixtures/stress/" + name + ".suf";
}
std::unique_ptr<CConfig> Configuration(const std::string& model, const std::string& mode,
                                       const std::string& file = "positive", double scale = 1, bool inc = false,
                                       const std::string& extra = "") {
  std::stringstream text;
  text << "SOLVER= " << (inc ? "INC_RANS" : "RANS")
       << "\nREYNOLDS_NUMBER= 1000\nMACH_NUMBER= 0.1\nKIND_TURB_MODEL= " << (model == "SA_NEG" ? "SA" : model)
       << "\nSST_OPTIONS= (V2003m)\nMGLEVEL= 0\nSTRESS_DISCREPANCY= " << mode << "\nSTRESS_DISCREPANCY_SCALE= " << scale
       << "\n";
  if (model == "SA_NEG") text << "SA_OPTIONS= (NEGATIVE, WITHFT2)\n";
  text << extra << "\n";
  if (inc) text << "INC_DENSITY_MODEL= CONSTANT\nVISCOSITY_MODEL= CONSTANT_VISCOSITY\nMU_CONSTANT= 0.001\n";
  if (mode == "PRESCRIBED") text << "STRESS_DISCREPANCY_FILE= " << Fixture(file) << "\n";
  if (mode == "EPM_MATCHED") text << "UQ_COMPONENT= 3\nUQ_DELTA_B= 1\nUQ_URLX= 1\n";
  auto result = std::unique_ptr<CConfig>(new CConfig(text, SU2_COMPONENT::SU2_CFD, false));
  result->SetLength_Ref(1);
  result->SetVelocity_Ref(1);
  result->SetDensity_Ref(1);
  return result;
}
struct State {
  double v[12] = {1, 3, 4, 1, 1.2, 10, 1, 0.01, 0.06, 0.1, 1, 1};
  double scalar[2] = {0.1, 1}, vort[3] = {0, 0, -2}, left[2] = {0.25, 0.5}, right[2] = {0.75, 0.5},
         normal[2] = {0.6, 0.8};
  su2activematrix gradient{5, 2}, scalar_gradient{2, 2};
  State() {
    gradient = 0.;
    scalar_gradient = 0.;
  }
  void Set(CNumerics& numerics) {
    numerics.SetPrimitive(v, v);
    numerics.SetScalarVar(scalar, scalar);
    numerics.SetPrimVarGradient(CMatrixView<const double>(gradient), CMatrixView<const double>(gradient));
    numerics.SetScalarVarGradient(CMatrixView<const double>(scalar_gradient),
                                  CMatrixView<const double>(scalar_gradient));
    numerics.SetVorticity(vort, vort);
    numerics.SetStrainMag(2, 2);
    numerics.SetDistance(1, 1);
    numerics.SetVolume(3);
    numerics.SetF1blending(1, 1);
    numerics.SetF2blending(.31);
    numerics.SetCrossDiff(0);
    numerics.SetCoord(left, right);
    numerics.SetNormal(normal);
    numerics.SetTurbKineticEnergy(.1, .1);
  }
};
using Indices = CEulerVariable::CIndices<unsigned short>;
const double constants[] = {.85, 1, .5, .856, .075, .0828, .09, .31, .5, .5, 10};
}  // namespace

TEST_CASE("Prescribed stress has conservative momentum and compressible work flux", "[stress][flux]") {
  for (bool inc : {false, true}) {
    auto base = Configuration("SA", "NONE", "positive", 1, inc);
    auto active = Configuration("SA", "PRESCRIBED", "positive", 1, inc);
    std::unique_ptr<CNumerics> a(inc ? static_cast<CNumerics*>(new CAvgGradInc_Flow(2, 4, true, base.get()))
                                     : static_cast<CNumerics*>(new CAvgGrad_Flow(2, 4, true, base.get())));
    std::unique_ptr<CNumerics> b(inc ? static_cast<CNumerics*>(new CAvgGradInc_Flow(2, 4, true, active.get()))
                                     : static_cast<CNumerics*>(new CAvgGrad_Flow(2, 4, true, active.get())));
    State state;
    auto delta = [&]() {
      state.Set(*a);
      state.Set(*b);
      const auto fa = a->ComputeResidual(base.get());
      const auto fb = b->ComputeResidual(active.get());
      std::array<double, 4> d{};
      for (int i = 0; i < 4; ++i) d[i] = fb[i] - fa[i];
      return d;
    };
    const auto forward = delta();
    REQUIRE(forward[0] == 0);
    REQUIRE(forward[1] == Approx(-1.2 * .16 * .8));
    REQUIRE(forward[2] == Approx(-1.2 * .16 * .6));
    REQUIRE(forward[3] == Approx(inc ? 0. : 3 * forward[1] + 4 * forward[2]).margin(1e-14));
    for (auto& x : state.normal) x = -x;
    const auto reverse = delta();
    for (int i = 0; i < 4; ++i) REQUIRE(reverse[i] == Approx(-forward[i]).margin(1e-14));
    for (auto& x : state.normal) x = -x;
    const double h = 1e-6;
    state.v[4] = 1.2 + h;
    const auto plus = delta();
    state.v[4] = 1.2 - h;
    const auto minus = delta();
    for (int i = 1; i < 4; ++i) REQUIRE((plus[i] - minus[i]) / (2 * h) == Approx(forward[i] / 1.2).margin(1e-9));
    state.v[4] = 1.2;
    state.v[1] += h;
    const auto up = delta();
    state.v[1] -= 2 * h;
    const auto down = delta();
    REQUIRE((up[3] - down[3]) / (2 * h) == Approx(inc ? 0. : forward[1]).margin(1e-9));
  }
}

TEST_CASE("Stress-coupled SST adds signed work before both production limits", "[stress][SST]") {
  for (const std::string file : {"positive", "negative", "zero"})
    for (double scale : {0., .5, 1.}) {
      auto config = Configuration("SST", "PRESCRIBED", file, scale);
      CSourcePieceWise_TurbSST<Indices> source(2, 2, constants, 0, 0, config.get());
      State state;
      state.gradient(1, 1) = 2;
      state.Set(source);
      const double e = (file == "zero" ? 0 : (file == "positive" ? .16 : -.16)) * scale;
      const double raw = .24 - 1.2 * 2 * e, pk = std::max(0., std::min(raw, .108));
      const auto result = source.ComputeResidual(config.get());
      REQUIRE(result[0] == Approx(3 * (pk - .0108)).margin(1e-13));
      REQUIRE(result[1] == Approx(3 * (10 * pk - .09)).margin(1e-13));
      source.SetDistance(0, 0);
      state.scalar[0] = 0;
      state.v[8] = 0;
      const auto wall = source.ComputeResidual(config.get());
      REQUIRE(wall[0] == 0);
      REQUIRE(wall[1] == 0);
    }
}

TEST_CASE("Zero stress recovers source residuals exactly and SA has no direct source change", "[stress][zero]") {
  for (const std::string model : {"SA", "SA_NEG", "SST"})
    for (const std::string file : {"positive", "zero"}) {
      auto base = Configuration(model, "NONE"),
           active = Configuration(model, "PRESCRIBED", file, model == "SST" && file != "zero" ? 0 : 1);
      std::unique_ptr<CNumerics> a, b;
      if (model == "SST") {
        a.reset(new CSourcePieceWise_TurbSST<Indices>(2, 2, constants, 0, 0, base.get()));
        b.reset(new CSourcePieceWise_TurbSST<Indices>(2, 2, constants, 0, 0, active.get()));
      } else {
        a.reset(SAFactory<Indices>(2, base.get()));
        b.reset(SAFactory<Indices>(2, active.get()));
      }
      State state;
      state.gradient(1, 1) = 2;
      state.Set(*a);
      state.Set(*b);
      const auto original = a->ComputeResidual(base.get()), changed = b->ComputeResidual(active.get());
      for (int j = 0; j < (model == "SST" ? 2 : 1); ++j) REQUIRE(original[j] == changed[j]);
    }
}

TEST_CASE("Matched EPM is a deviatoric difference with one moderation", "[stress][EPM]") {
  auto config = Configuration("SST", "EPM_MATCHED");
  double grad[2][2] = {{0, 2}, {0, 0}}, point[2] = {.5, .5}, difference[3][3];
  CNumerics::ComputeStressDiscrepancy(2, config.get(), point, grad, 1.2, .06, .1, difference);
  REQUIRE(difference[0][1] == Approx(.1));
  REQUIRE(difference[1][0] == Approx(.1));
  REQUIRE(difference[0][0] + difference[1][1] + difference[2][2] == Approx(0).margin(1e-14));
  REQUIRE(CNumerics::ComputeReynoldsStressProduction(2, difference, grad, 1.2) == Approx(-.24));
  CSourcePieceWise_TurbSST<Indices> source(2, 2, constants, 0, 0, config.get());
  State state;
  state.gradient(1, 1) = 2;
  state.Set(source);
  const auto result = source.ComputeResidual(config.get());
  REQUIRE(result[0] == Approx(-3 * .0108).margin(1e-13));
  REQUIRE(result[1] == Approx(-3 * .09).margin(1e-13));
}

TEST_CASE("Physical stress converts once to internal velocity units", "[stress][units]") {
  auto config = Configuration("SA", "PRESCRIBED");
  double point[2] = {.25, .25}, tensor[3][3];
  config->SetLength_Ref(2);
  config->SetVelocity_Ref(4);
  config->EvaluateStressDiscrepancy(point, 2, tensor);
  REQUIRE(tensor[0][1] == Approx(.16 / 16));
}

TEST_CASE("Boundary numerics preserve the parent for validated zero prescribed stress", "[stress][boundary]") {
  auto base = Configuration("SA", "NONE"), active = Configuration("SA", "PRESCRIBED", "zero");
  CAvgGrad_Flow a(2, 4, false, base.get()), b(2, 4, false, active.get());
  State state;
  state.Set(a);
  state.Set(b);
  const auto expected = a.ComputeResidual(base.get()), actual = b.ComputeResidual(active.get());
  for (int i = 0; i < 4; ++i) REQUIRE(actual[i] == expected[i]);
}

TEST_CASE("QCR donor tensor is the kinematic difference from the linear parent", "[stress][QCR]") {
  double grad[2][2] = {{0, 2}, {0, 0}}, e[3][3];
  CNumerics::ComputeQCRStressDiscrepancy(2, grad, 1.2, .06, e);
  REQUIRE(e[0][0] == Approx(.06));
  REQUIRE(e[1][1] == Approx(-.06));
  REQUIRE(e[2][2] == 0);
  REQUIRE(e[0][1] == 0);
  REQUIRE(e[1][0] == 0);
}

TEST_CASE("Tiny positive SST kinetic energy avoids dividing a clipped production by tiny viscosity", "[stress][SST]") {
  auto config = Configuration("SST", "PRESCRIBED", "negative");
  CSourcePieceWise_TurbSST<Indices> source(2, 2, constants, 0, 0, config.get());
  State state;
  state.gradient(1, 1) = 2;
  state.scalar[0] = 1e-310;
  state.v[8] = .6e-310;
  state.Set(source);
  const auto result = source.ComputeResidual(config.get());
  REQUIRE(std::isfinite(result[0]));
  REQUIRE(std::isfinite(result[1]));
  REQUIRE(result[1] == Approx(3 * (1.08 - .09)).margin(1e-12));
}

TEST_CASE("Matched EPM controls preserve the same parent and one effective blend", "[stress][EPM]") {
  const double grad[3][3] = {{-.2, .1, .03}, {-.04, -.1, .02}, {.05, .07, .3}}, point[3] = {0, 0, 0};
  for (int component : {1, 2, 3})
    for (bool permute : {false, true})
      for (double blend : {0., .1, 1.}) {
        std::stringstream input;
        input << "SOLVER=RANS\nREYNOLDS_NUMBER=1000\nMACH_NUMBER=0.1\nKIND_TURB_MODEL=SST\n"
              << "SST_OPTIONS=V2003m\nMGLEVEL=0\nSTRESS_DISCREPANCY=EPM_MATCHED\nUQ_COMPONENT=" << component
              << "\nUQ_PERMUTE=" << (permute ? "YES" : "NO") << "\nUQ_DELTA_B=1\nUQ_URLX=" << blend;
        CConfig config(input, SU2_COMPONENT::SU2_CFD, false);
        double e[3][3], r[3][3], parent[3][3];
        CNumerics::ComputeStressDiscrepancy(3, &config, point, grad, 1.2, .06, .1, e);
        CNumerics::ComputePerturbedRSM(3, component, permute, 1., 1., grad, 1.2, .06, .1, r);
        CNumerics::ComputeStressTensor(3, parent, grad, .06, 0., 0., true);
        const double trace = (r[0][0] + r[1][1] + r[2][2]) / 3;
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j)
            REQUIRE(e[i][j] == Approx(blend * (r[i][j] - (i == j ? trace : 0) + parent[i][j] / 1.2)).margin(1e-13));
      }
}
