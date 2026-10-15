/*!
 * \file newton_equations.cpp
 * \brief Native solver state mapping, grouped residuals and cross derivatives.
 * \author SU2 Contributors
 */
#include "catch.hpp"
#include "../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../SU2_CFD/include/integration/CNewtonEquationSet.hpp"
#include "../../SU2_CFD/include/integration/CNewtonLinearization.hpp"
#include "../../SU2_CFD/include/integration/CNewtonIntegration.hpp"
#include "../../SU2_CFD/include/integration/CNewtonLegacyIntegration.hpp"
#include "../../SU2_CFD/include/variables/CFlowVariable.hpp"
#include "../../Common/include/linear_algebra/CSysSolve.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace {
using Vector = CSysVector<passivedouble>;

class NewtonDriver : public CSinglezoneDriver {
 public:
  explicit NewtonDriver(char* path) : CSinglezoneDriver(path, 1, SU2_MPI::GetComm()) {}
  CConfig* Config() { return config_container[0]; }
  CGeometry* Geometry() { return geometry_container[0][0][0]; }
  CSolver** Solvers() { return solver_container[0][0][0]; }
  CNumerics*** Numerics() { return numerics_container[0][0][0]; }
  void FlowIteration(CIntegration& integration) {
    Config()->SetGlobalParam(Config()->GetKind_Solver(), RUNTIME_FLOW_SYS);
    integration.MultiGrid_Iteration(geometry_container, solver_container, numerics_container, config_container,
                                    RUNTIME_FLOW_SYS, 0, 0);
  }
};

class Silence {
  std::streambuf* old = std::cout.rdbuf(nullptr);

 public:
  ~Silence() { std::cout.rdbuf(old); }
};

struct Problem {
  std::string path;
  std::unique_ptr<NewtonDriver> driver;
  explicit Problem(const std::string& physics, unsigned cells = 5, bool muscl = false) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    path = (std::filesystem::temp_directory_path() /
            ("su2_newton_" + std::to_string(stamp) + "_" + std::to_string(SU2_MPI::GetRank()) + ".cfg"))
               .string();
    {
      std::ofstream config(path);
      config << "MESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= " << cells << "," << cells
             << ",5\nMESH_BOX_LENGTH= 1,1,1\n"
                "MARKER_HEATFLUX= (y_minus, 0)\nMARKER_FAR= (x_minus,x_plus,y_plus)\n"
                "MACH_NUMBER= 0.2\nREYNOLDS_NUMBER= 1e6\nMUSCL_FLOW= "
             << (muscl ? "YES" : "NO")
             << "\nMUSCL_TURB= NO\n"
                "MUSCL_SPECIES= NO\nMGLEVEL= 0\nOUTPUT_FILES= NONE\n"
                "SCREEN_OUTPUT= (INNER_ITER)\nHISTORY_OUTPUT= (ITER,RMS_RES)\n"
             << "CONV_NUM_METHOD_FLOW= " << (physics.find("SOLVER= INC_") == std::string::npos ? "ROE" : "FDS") << "\n"
             << physics;
    }
    Silence silence;
    driver = std::make_unique<NewtonDriver>(&path[0]);
  }
  ~Problem() {
    Silence silence;
    driver.reset();
    std::filesystem::remove(path);
  }
  std::unique_ptr<CNewtonEquationSet> Equations(const std::vector<unsigned short>& selected) {
    return std::make_unique<CNewtonEquationSet>(driver->Geometry(), driver->Solvers(), driver->Numerics(),
                                                driver->Config(), selected);
  }
};

const std::string sst = "SOLVER= RANS\nKIND_TURB_MODEL= SST\n";
const std::string sa = "SOLVER= RANS\nKIND_TURB_MODEL= SA\n";
const std::string species =
    "SOLVER= INC_NAVIER_STOKES\nINC_DENSITY_MODEL= VARIABLE\nINC_ENERGY_EQUATION= YES\n"
    "FLUID_MODEL= FLUID_MIXTURE\nKIND_SCALAR_MODEL= SPECIES_TRANSPORT\nSPECIES_INIT= 0.4\n"
    "MOLECULAR_WEIGHT= 28.96,16.043\nSPECIFIC_HEAT_CP= 1009.39,2225\n"
    "VISCOSITY_MODEL= CONSTANT_VISCOSITY\nMU_CONSTANT= 1.8551e-5,1.1e-5\n"
    "PRANDTL_LAM= 0.72,0.72\nPRANDTL_TURB= 0.9,0.9\nDIFFUSIVITY_CONSTANT= 1e-5\n";

bool Residual(CNewtonEquationSet& equations, bool implicit = false) {
  bool valid = false;
  SU2_OMP_PARALLEL {
    const bool result = equations.EvaluateResiduals(implicit);
    SU2_OMP_MASTER
    valid = result;
    END_SU2_OMP_MASTER
  }
  END_SU2_OMP_PARALLEL
  return valid;
}

Vector MakeVector(NewtonDriver& driver, unsigned short variables, double value = 0) {
  auto* geometry = driver.Geometry();
  return Vector(geometry->GetnPoint(), geometry->GetnPointDomain(), variables, value);
}

double MaxDifference(const Vector& a, const Vector& b) {
  double difference = 0;
  for (auto i = 0ul; i < a.GetNElmDomain(); ++i) difference = std::max(difference, std::abs(a[i] - b[i]));
  double global = 0;
  SU2_MPI::Allreduce(&difference, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  return global;
}
}  // namespace

TEST_CASE("Newton equation groups map native and conservative scalar increments", "[NewtonEquations]") {
  for (const auto& physics :
       {sa, sa + "SA_OPTIONS= NEGATIVE, WITHFT2\n", sst, sst + "KIND_TRANS_MODEL= LM\n", species}) {
    CAPTURE(physics);
    Problem problem(physics);
    auto& driver = *problem.driver;
    std::vector<unsigned short> selected = {RUNTIME_FLOW_SYS};
    for (auto runtime : {RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS})
      if (driver.Solvers()[driver.Config()->GetContainerPosition(runtime)]) selected.push_back(runtime);
    auto equations = problem.Equations(selected);
    REQUIRE(Residual(*equations));
    auto base = MakeVector(driver, equations->GetnVar());
    auto direction = MakeVector(driver, equations->GetnVar(), 0.125);
    auto scale = MakeVector(driver, equations->GetnVar(), 1.0);
    auto actual = MakeVector(driver, equations->GetnVar());
    for (const auto& block : equations->GetBlocks()) {
      auto* nodes = block.solver->GetNodes();
      // Physical-time arrays must survive trial installation, even when present in a steady test.
      nodes->GetSolution_time_n().resize(driver.Geometry()->GetnPoint(), block.variables) = 7.0;
      nodes->GetSolution_time_n1().resize(driver.Geometry()->GetnPoint(), block.variables) = 9.0;
      for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) nodes->SetSolution_Old(p, v, 2.5 + v);
    }
    SU2_OMP_PARALLEL { equations->Capture(); }
    END_SU2_OMP_PARALLEL
    equations->Pack(base);
    equations->SetTrial(direction, 0.02, scale);
    equations->Pack(actual);
    for (auto i = 0ul; i < base.GetNElmDomain(); ++i) CHECK(actual[i] == Approx(base[i] + 0.0025));
    equations->SetTrial(direction, -0.03, scale);
    equations->SetTrial(direction, 0.02, scale);
    auto repeated = MakeVector(driver, equations->GetnVar());
    equations->Pack(repeated);
    CHECK(MaxDifference(actual, repeated) == 0.0);
    equations->SetTrial(direction, 0.0, scale);
    equations->Pack(actual);
    CHECK(MaxDifference(base, actual) == 0.0);
    for (const auto& block : equations->GetBlocks()) {
      auto* nodes = block.solver->GetNodes();
      for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) {
          CHECK(nodes->GetSolution_Old(p, v) == 2.5 + v);
          CHECK(nodes->GetSolution_time_n(p, v) == 7.0);
          CHECK(nodes->GetSolution_time_n1(p, v) == 9.0);
        }
    }
    if (physics == sa + "SA_OPTIONS= NEGATIVE, WITHFT2\n")
      CHECK(driver.Solvers()[TURB_SOL]->GetSolutionBounds(0).first < 0);
    if (physics == sa) CHECK(driver.Solvers()[TURB_SOL]->GetSolutionBounds(0).first >= 0);
    // A conventional outer update can replace history after the preceding Newton capture.
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) block.solver->GetNodes()->SetSolution_Old(p, v, 4.5 + v);
    REQUIRE(Residual(*equations));
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          CHECK(block.solver->GetNodes()->GetSolution_Old(p, v) == 4.5 + v);
    CHECK_THROWS_AS(problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_FLOW_SYS}), std::invalid_argument);
    CHECK_THROWS_AS(problem.Equations({}), std::invalid_argument);
    if (!driver.Solvers()[SPECIES_SOL])
      CHECK_THROWS_AS(problem.Equations({RUNTIME_SPECIES_SYS}), std::invalid_argument);
  }
}

TEST_CASE("Grouped residuals refresh cross derivatives independently of trial order", "[NewtonEquations]") {
  for (const auto& physics : {sa, sst, sst + "KIND_TRANS_MODEL= LM\n", species}) {
    CAPTURE(physics);
    Problem problem(physics);
    auto& driver = *problem.driver;
    auto* geometry = driver.Geometry();
    std::vector<unsigned short> selected = {RUNTIME_FLOW_SYS};
    for (auto runtime : {RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS})
      if (driver.Solvers()[driver.Config()->GetContainerPosition(runtime)]) selected.push_back(runtime);
    auto equations = problem.Equations(selected);
    const auto& blocks = equations->GetBlocks();
    auto direction = MakeVector(driver, equations->GetnVar());
    auto scale = MakeVector(driver, equations->GetnVar(), 1.0);
    auto base = MakeVector(driver, equations->GetnVar());
    auto positive = MakeVector(driver, equations->GetnVar());
    auto negative = MakeVector(driver, equations->GetnVar());
    auto repeated = MakeVector(driver, equations->GetnVar());
    // Impose a smooth nonuniform field so cross responses are observable.
    for (const auto& block : blocks) {
      auto* nodes = block.solver->GetNodes();
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
        const auto x = geometry->nodes->GetCoord(p, 0);
        const auto y = geometry->nodes->GetCoord(p, 1);
        if (block.slot == FLOW_SOL)
          nodes->SetSolution(p, 1, nodes->GetSolution(p, 1) * (1.0 + 0.1 * x));
        else
          nodes->SetSolution(p, 0, nodes->GetSolution(p, 0) * (1.0 + 0.1 * x));
        if (!geometry->nodes->GetSolidBoundary(p)) {
          const auto mass =
              block.solver->GetConservative() ? driver.Solvers()[FLOW_SOL]->GetNodes()->GetDensity(p) : 1.0;
          direction(p, block.offset) = 0.001 * mass * std::abs(nodes->GetSolution(p, 0)) * (1.0 + x + 0.3 * y);
        }
      }
      block.solver->Set_OldSolution();
    }
    REQUIRE(Residual(*equations, true));
    std::vector<double> diagonal;
    for (const auto& block : blocks)
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
        const auto* values = block.solver->Jacobian.GetBlock(p, p);
        for (auto i = 0u; i < block.variables * block.variables; ++i) diagonal.push_back(values[i]);
      }
    REQUIRE(Residual(*equations));
    auto entry = 0ul;
    for (const auto& block : blocks)
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
        const auto* values = block.solver->Jacobian.GetBlock(p, p);
        for (auto i = 0u; i < block.variables * block.variables; ++i) CHECK(values[i] == diagonal[entry++]);
      }
    for (const auto& block : blocks) block.solver->Set_OldSolution();
    SU2_OMP_PARALLEL { equations->Capture(); }
    END_SU2_OMP_PARALLEL
    equations->CopyResidual(base);
    auto evaluate = [&](double alpha, Vector& residual) {
      const auto settings = driver.Config()->GetEquationSettings();
      equations->SetTrial(direction, alpha, scale);
      REQUIRE(Residual(*equations));
      const auto restored = driver.Config()->GetEquationSettings();
      CHECK(restored.convection == settings.convection);
      CHECK(restored.time == settings.time);
      CHECK(restored.fem == settings.fem);
      CHECK(restored.centered == settings.centered);
      CHECK(restored.upwind == settings.upwind);
      CHECK(restored.limiter == settings.limiter);
      CHECK(restored.muscl == settings.muscl);
      CHECK(restored.residualOnly == settings.residualOnly);
      CHECK(restored.kappa == settings.kappa);
      CHECK(restored.physicalTime == settings.physicalTime);
      CHECK(restored.physicalTimeND == settings.physicalTimeND);
      equations->CopyResidual(residual);
      for (auto i = 0ul; i < residual.GetNElmDomain(); ++i) REQUIRE(std::isfinite(residual[i]));
    };
    evaluate(0.01, positive);
    evaluate(-0.02, negative);
    evaluate(0.01, repeated);
    CHECK(MaxDifference(positive, repeated) == 0.0);
    evaluate(0.0, repeated);
    CHECK(MaxDifference(base, repeated) == 0.0);
    // Compare independent centered stencils at two step sizes for the whole grouped residual.
    evaluate(0.005, positive);
    evaluate(-0.005, negative);
    Vector derivative(positive);
    derivative -= negative;
    derivative /= 0.01;
    evaluate(0.0025, positive);
    evaluate(-0.0025, negative);
    Vector refined(positive);
    refined -= negative;
    refined /= 0.005;
    CHECK(MaxDifference(derivative, refined) < 1e-4 * std::max(1.0, SU2_TYPE::GetValue(refined.norm())));
    // Change only the auxiliary unknown, then only flow, to detect the off-diagonal blocks.
    for (auto i = 0ul; i < direction.GetLocSize(); ++i) direction[i] = 0.0;
    const auto& auxiliary = blocks[1];
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      if (!geometry->nodes->GetSolidBoundary(p)) direction(p, auxiliary.offset) = 0.001;
    evaluate(0.01, positive);
    double flowResponse = 0;
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < blocks[0].variables; ++v) flowResponse += std::abs(positive(p, v) - base(p, v));
    double globalResponse = 0;
    SU2_MPI::Allreduce(&flowResponse, &globalResponse, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    CHECK(globalResponse > 0.0);
    // With fixed flow conservative energy, pressure responds to k by -(gamma-1)*rho.
    if (physics == sst) {
      evaluate(0.0, negative);
      std::vector<double> pressure(geometry->GetnPoint());
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        pressure[p] = driver.Solvers()[FLOW_SOL]->GetNodes()->GetPressure(p);
      evaluate(1.0, positive);
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
        if (!geometry->nodes->GetSolidBoundary(p))
          CHECK((driver.Solvers()[FLOW_SOL]->GetNodes()->GetPressure(p) - pressure[p]) / 0.001 ==
                Approx(-0.4).margin(1e-7));
    }
    direction = passivedouble(0);
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      if (!geometry->nodes->GetSolidBoundary(p)) direction(p, 1) = 0.001;
    evaluate(0.01, positive);
    double auxiliaryResponse = 0;
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = auxiliary.offset; v < equations->GetnVar(); ++v)
        auxiliaryResponse += std::abs(positive(p, v) - base(p, v));
    SU2_MPI::Allreduce(&auxiliaryResponse, &globalResponse, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    CHECK(globalResponse > 0.0);
    REQUIRE(equations->Restore());
    equations->CopyResidual(repeated);
    CHECK(MaxDifference(base, repeated) == 0.0);
  }
}

TEST_CASE("Invalid Newton trials are rejected without silently projecting a derivative", "[NewtonEquations]") {
  Problem problem(sst);
  auto& driver = *problem.driver;
  auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS});
  REQUIRE(Residual(*equations));
  equations->Capture();
  auto base = MakeVector(driver, equations->GetnVar());
  auto actual = MakeVector(driver, equations->GetnVar());
  auto direction = MakeVector(driver, equations->GetnVar());
  auto scale = MakeVector(driver, equations->GetnVar(), 1.0);
  equations->Pack(base);
  for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p) direction(p, 0) = -1e6;
  equations->SetTrial(direction, 1.0, scale);
  CHECK_FALSE(Residual(*equations));
  REQUIRE(equations->Restore());
  equations->Pack(actual);
  CHECK(MaxDifference(base, actual) == 0.0);
  CNewtonLinearization linearization(*equations, driver.Geometry(), driver.Config());
  auto diagonal = MakeVector(driver, equations->GetnVar());
  REQUIRE(linearization.SetBase(scale, diagonal, 1e20, 0));
  CHECK_FALSE(linearization.Apply(direction, actual));
  for (auto i = 0ul; i < actual.GetLocSize(); ++i) CHECK(std::isnan(actual[i]));
  REQUIRE(equations->Restore());
}

TEST_CASE("Grouped Newton products and block preconditioners solve a coupled linearization", "[NewtonLinearization]") {
  auto allEquations = species;
  allEquations.replace(allEquations.find("INC_NAVIER_STOKES"), std::string("INC_NAVIER_STOKES").size(), "INC_RANS");
  allEquations += "KIND_TURB_MODEL= SST\nKIND_TRANS_MODEL= LM\n";
  // Scalar edge fluxes currently support eight transported species; exercise all four equation blocks together.
  for (const auto& item : std::vector<std::pair<std::string, double>>{
           {"SPECIES_INIT", 0.02}, {"MOLECULAR_WEIGHT", 20.0}, {"SPECIFIC_HEAT_CP", 1000.0},
           {"MU_CONSTANT", 1e-5}, {"PRANDTL_LAM", 0.72}, {"PRANDTL_TURB", 0.9}}) {
    const auto count = item.first == "SPECIES_INIT" ? 8 : 9;
    std::ostringstream values;
    for (int j = 0; j < count; ++j) values << (j ? "," : "") << item.second;
    const auto begin = allEquations.find(item.first + "= ");
    REQUIRE(begin != std::string::npos);
    allEquations.replace(begin, allEquations.find('\n', begin) - begin, item.first + "= " + values.str());
  }
  unsigned family = 0;
  for (const auto& physics : {sa, sa + "SA_OPTIONS= NEGATIVE, WITHFT2\n", sst,
                             sst + "KIND_TRANS_MODEL= LM\n", species, allEquations}) {
    ++family;
    CAPTURE(physics);
    Problem problem(physics + "LINEAR_SOLVER_PREC= ILU\n");
    auto& driver = *problem.driver;
    auto* geometry = driver.Geometry();
    std::vector<unsigned short> selected = {RUNTIME_FLOW_SYS};
    for (auto runtime : {RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS})
      if (driver.Solvers()[driver.Config()->GetContainerPosition(runtime)]) selected.push_back(runtime);
    auto equations = problem.Equations(selected);
    if (physics == allEquations) {
      CHECK(equations->GetnVar() == 16);
      CHECK(equations->GetBlocks().size() == 4);
    }
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto j = 0u; j < block.variables; ++j) {
          auto* nodes = block.solver->GetNodes();
          nodes->SetSolution(p, j, nodes->GetSolution(p, j) * (1.0 + 0.05 * geometry->nodes->GetCoord(p, 0)));
          // Strain magnitude is not differentiable at zero; use a nonuniform velocity for smooth references.
          if (block.slot == FLOW_SOL && j == 1)
            nodes->SetSolution(p, j, nodes->GetSolution(p, j) *
                (1.0 + 0.07 * geometry->nodes->GetCoord(p, 0) + 0.03 * geometry->nodes->GetCoord(p, 1)));
          if (block.slot == FLOW_SOL && j == 2) nodes->SetSolution(p, j, 0.02 * nodes->GetSolution(p, 1));
          if (block.slot == TRANS_SOL && j == 0)
            nodes->SetSolution(p, j, 0.5 * (1.0 + 0.05 * geometry->nodes->GetCoord(p, 0)));
          if (block.slot == TURB_SOL && physics == sa + "SA_OPTIONS= NEGATIVE, WITHFT2\n")
            nodes->SetSolution(p, j, -0.5 * std::abs(nodes->GetSolution(p, j)));
        }
    for (const auto& block : equations->GetBlocks()) block.solver->Jacobian.TrackPrescribedRows();
    REQUIRE(Residual(*equations, true));
    REQUIRE(Residual(*equations, true));
    REQUIRE(Residual(*equations, true));
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      if (!geometry->nodes->GetSolidBoundary(p)) CHECK(driver.Solvers()[FLOW_SOL]->GetNodes()->GetStrainMag(p) > 0);
    equations->Capture();
    auto scale = MakeVector(driver, equations->GetnVar());
    equations->Pack(scale);
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto j = 0u; j < block.variables; ++j) {
          const auto index = block.offset + j;
          double ref = 0;
          if (block.slot == FLOW_SOL && j >= 1 && j <= geometry->GetnDim())
            ref = std::abs(scale(p, block.offset + 1));
          else if (block.solver->GetLimiterReference())
            ref = SU2_TYPE::GetValue(equations->MassFactor(block, p) * block.solver->GetLimiterReference()[j]);
          scale(p, index) = std::max(std::abs(scale(p, index)), std::abs(ref));
          if (scale(p, index) == 0) scale(p, index) = 1.0;
        }
    auto diagonal = MakeVector(driver, equations->GetnVar(), 100.0);
    auto known = MakeVector(driver, equations->GetnVar());
    auto freeDofs = MakeVector(driver, equations->GetnVar());
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) {
          const auto j = block.offset + v;
          freeDofs(p, j) = !block.solver->Jacobian.IsPrescribedRow(p, v);
          if (freeDofs(p, j)) known(p, j) = 0.01 * (1.0 + j + geometry->nodes->GetCoord(p, 0));
        }
    auto isFree = [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
      return freeDofs(p, b.offset + v) != 0;
    };
    CNewtonLinearization linearization(*equations, geometry, driver.Config());
    bool baseValid = false;
    SU2_OMP_PARALLEL {
      const bool valid = linearization.SetBase(scale, diagonal, 1e-6, 1e-4, isFree);
      SU2_OMP_MASTER
      baseValid = valid;
      END_SU2_OMP_MASTER
    }
    END_SU2_OMP_PARALLEL
    REQUIRE(baseValid);
    auto product = MakeVector(driver, equations->GetnVar());
    auto apply = [&](const Vector& u, Vector& v) {
      bool valid = false;
      SU2_OMP_PARALLEL {
        const bool result = linearization.Apply(u, v);
        SU2_OMP_MASTER
        valid = result;
        END_SU2_OMP_MASTER
      }
      END_SU2_OMP_PARALLEL
      REQUIRE(valid);
    };
    // Independent centered residual stencil, including scaling and the chosen pseudo-time mass.
    auto reference = [&](const Vector& u) {
      auto positive = MakeVector(driver, equations->GetnVar());
      auto negative = MakeVector(driver, equations->GetnVar());
      equations->SetTrial(u, 1e-5, scale, isFree);
      REQUIRE(Residual(*equations));
      equations->CopyResidual(positive);
      equations->SetTrial(u, -1e-5, scale, isFree);
      REQUIRE(Residual(*equations));
      equations->CopyResidual(negative);
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto j = 0u; j < equations->GetnVar(); ++j)
          positive(p, j) = freeDofs(p, j)
                               ? (positive(p, j) - negative(p, j)) / (2e-5 * scale(p, j)) + 100.0 * u(p, j)
                               : 101.0 * u(p, j);
      return positive;
    };
    auto rhs = reference(known);
    apply(known, product);
    CHECK(MaxDifference(rhs, product) < 1e-4 * std::max(1.0, SU2_TYPE::GetValue(rhs.norm())));
    auto zero = MakeVector(driver, equations->GetnVar());
    const auto counts = equations->GetResidualEvaluations();
    apply(zero, product);
    CHECK(product.norm() == 0.0);
    CHECK(equations->GetResidualEvaluations() == counts);
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto j = 0u; j < equations->GetnVar(); ++j)
        if (!freeDofs(p, j)) zero(p, j) = 0.01;
    apply(zero, product);
    for (auto i = 0ul; i < product.GetNElmDomain(); ++i) CHECK(product[i] == Approx(101.0 * zero[i]).margin(1e-12));
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) block.solver->Jacobian.AddVal2Diag(p, 100.0);
    for (bool triangular : {false, true}) {
      CAPTURE(triangular);
      CNewtonBlockPreconditioner preconditioner(linearization, geometry, driver.Config(), triangular);
      auto solution = MakeVector(driver, equations->GetnVar());
      CSysSolve<passivedouble> krylov;
      krylov.SetxIsZero(true);
      unsigned iterations = 0;
      double reported = 0;
      SU2_OMP_PARALLEL {
        preconditioner.Build();
        double residual = 1e-6;
        const auto used = krylov.FGMRES_LinSolver(rhs, solution, linearization, preconditioner, 1e-6, 80,
                                                 residual, false, driver.Config());
        SU2_OMP_MASTER {
          iterations = used;
          reported = residual;
        }
        END_SU2_OMP_MASTER
      }
      END_SU2_OMP_PARALLEL
      CHECK(iterations > 0);
      CHECK(iterations <= 80);
      CHECK(reported < 1e-5);
      auto actual = reference(solution);
      actual -= rhs;
      const double defect = SU2_TYPE::GetValue(actual.norm() / rhs.norm());
      CHECK(defect < 1e-4);
      if (SU2_MPI::GetRank() == MASTER_NODE)
        std::cout << "Newton linearization: family=" << family << ", variables=" << equations->GetnVar()
                  << ", lower=" << triangular << ", iterations=" << iterations << ", reported=" << reported
                  << ", centered_defect=" << defect << '\n';
    }
    REQUIRE(equations->Restore());
  }
}

TEST_CASE("Native strong rows are tracked independently of the preconditioner values", "[NewtonController]") {
  Problem problem(sst);
  auto& driver = *problem.driver;
  auto* geometry = driver.Geometry();
  auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS});
  for (const auto& block : equations->GetBlocks()) block.solver->Jacobian.TrackPrescribedRows();
  REQUIRE(Residual(*equations, true));
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    const bool wall = geometry->nodes->GetSolidBoundary(p);
    auto& flow = driver.Solvers()[FLOW_SOL]->Jacobian;
    auto& turb = driver.Solvers()[TURB_SOL]->Jacobian;
    CHECK_FALSE(flow.IsPrescribedRow(p, 0));
    CHECK_FALSE(flow.IsPrescribedRow(p, 3));
    CHECK(flow.IsPrescribedRow(p, 1) == wall);
    CHECK(flow.IsPrescribedRow(p, 2) == wall);
    CHECK(turb.IsPrescribedRow(p, 0) == wall);
    CHECK(turb.IsPrescribedRow(p, 1) == wall);
  }
  REQUIRE(Residual(*equations));
  auto& matrix = driver.Solvers()[TURB_SOL]->Jacobian;
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
    CHECK(matrix.IsPrescribedRow(p, 0) == geometry->nodes->GetSolidBoundary(p));
  SU2_OMP_PARALLEL { matrix.SetValDiagonalZero(); }
  END_SU2_OMP_PARALLEL
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) CHECK_FALSE(matrix.IsPrescribedRow(p, 0));
  matrix.DeleteValsRowi(0, 0);
  CHECK(matrix.IsPrescribedRow(0, 0));
  SU2_OMP_PARALLEL { matrix.SetValZero(); }
  END_SU2_OMP_PARALLEL
  CHECK_FALSE(matrix.IsPrescribedRow(0, 0));
  const su2double prescribed[2] = {0.0, 0.0};
  matrix.EnforceSolutionAtNode(0, prescribed, driver.Solvers()[TURB_SOL]->LinSysRes);
  CHECK(matrix.IsPrescribedRow(0, 0));
  CHECK(matrix.IsPrescribedRow(0, 1));
}

TEST_CASE("One Newton controller advances native individual equations and coupled groups", "[NewtonController]") {
  auto allPhysics = species;
  allPhysics.replace(allPhysics.find("INC_NAVIER_STOKES"), std::string("INC_NAVIER_STOKES").size(), "INC_RANS");
  allPhysics += "KIND_TURB_MODEL= SST\nKIND_TRANS_MODEL= LM\n";
  for (const auto& physics : {std::string("SOLVER= NAVIER_STOKES\n"), sa, sa + "SA_OPTIONS= NEGATIVE, WITHFT2\n", sst,
                              sst + "KIND_TRANS_MODEL= LM\n", species, allPhysics}) {
    CAPTURE(physics);
    for (bool coupled : {false, true}) {
      CAPTURE(coupled);
      Problem problem(physics + "CFL_NUMBER= 0.05\nLINEAR_SOLVER_PREC= ILU\n");
      auto& driver = *problem.driver;
      auto* geometry = driver.Geometry();
      std::vector<unsigned short> selected = {RUNTIME_FLOW_SYS};
      for (auto runtime : {RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS})
        if (driver.Solvers()[driver.Config()->GetContainerPosition(runtime)]) selected.push_back(runtime);
      auto equations = problem.Equations(selected);
      if (physics.find("SA_OPTIONS= NEGATIVE") != std::string::npos)
        for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
          driver.Solvers()[TURB_SOL]->GetNodes()->SetSolution(
              p, 0, -0.5 * std::abs(driver.Solvers()[TURB_SOL]->GetNodes()->GetSolution(p, 0)));
      REQUIRE(Residual(*equations));
      driver.Config()->SetGlobalParam(driver.Config()->GetKind_Solver(), RUNTIME_FLOW_SYS);
      SU2_OMP_PARALLEL {
        driver.Solvers()[FLOW_SOL]->SetTime_Step(geometry, driver.Solvers(), driver.Config(), MESH_0, 0);
      }
      END_SU2_OMP_PARALLEL
      NonlinearGlobalization::Options<> options;
      options.diagnostics = false;
      options.adaptCFL = false;
      options.adaptiveForcing = false;
      std::vector<std::unique_ptr<CNewtonIntegration>> controllers;
      if (coupled)
        controllers.emplace_back(std::make_unique<CNewtonIntegration>(selected, options, true));
      else
        for (auto runtime : selected)
          controllers.emplace_back(std::make_unique<CNewtonIntegration>(std::vector<unsigned short>{runtime}, options));
      for (unsigned iteration = 0; iteration < 3; ++iteration) {
        driver.Config()->SetInnerIter(iteration);
        for (auto& controller : controllers) {
          controller->Iterate(geometry, driver.Solvers(), driver.Numerics(), driver.Config());
          INFO(controller->GetNumericalFailureMessage());
          INFO("base=" << controller->GetLastAttempt().base << ", slope=" << controller->GetLastAttempt().slope
                       << ", defect=" << controller->GetLastAttempt().linearDefect
                       << ", evaluations=" << controller->GetLastAttempt().search.evaluations);
          REQUIRE_FALSE(controller->GetNumericalFailure());
          const auto& attempt = controller->GetLastAttempt();
          REQUIRE(attempt.search.status == Armijo::Status::Accepted);
          CHECK(attempt.linearDefect <= options.etaMax);
          if (attempt.base > 0)
            CHECK(attempt.search.value <= attempt.base + options.armijo * attempt.search.step * attempt.slope);
        }
      }
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
        if (geometry->nodes->GetSolidBoundary(p)) {
          CHECK(driver.Solvers()[FLOW_SOL]->GetNodes()->GetSolution(p, 1) == 0.0);
          CHECK(driver.Solvers()[FLOW_SOL]->GetNodes()->GetSolution(p, 2) == 0.0);
        }
      // The reported residual belongs to the final accepted state, independently recomputed here.
      REQUIRE(Residual(*equations));
      for (const auto& block : equations->GetBlocks())
        for (auto v = 0u; v < block.variables; ++v) {
          double local = 0, global = 0;
          for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
            local += pow(SU2_TYPE::GetValue(block.solver->LinSysRes(p, v)), 2);
          SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
          const auto expected = std::sqrt(global / geometry->GetGlobal_nPointDomain());
          if (coupled || block.runtime == selected.back())
            CHECK(SU2_TYPE::GetValue(block.solver->GetRes_RMS(v)) == Approx(expected).epsilon(1e-10).margin(1e-12));
        }
    }
  }
}

TEST_CASE("A failed coupled Newton update restores every selected equation", "[NewtonController]") {
  for (bool linearFailure : {true, false}) {
    CAPTURE(linearFailure);
    Problem problem(sst + std::string("CFL_NUMBER= ") + (linearFailure ? "100" : "0.05") +
                    "\nLINEAR_SOLVER_PREC= ILU\n");
    auto& driver = *problem.driver;
    auto* geometry = driver.Geometry();
    auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS});
    for (const auto& block : equations->GetBlocks()) {
      block.solver->Jacobian.TrackPrescribedRows();
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p) block.solver->GetNodes()->SetUnderRelaxation(p, 0.37);
      block.solver->GetNodes()->GetSolution_time_n().resize(geometry->GetnPoint(), block.variables) = 7.0;
      block.solver->GetNodes()->GetSolution_time_n1().resize(geometry->GetnPoint(), block.variables) = 9.0;
    }
    REQUIRE(Residual(*equations, true));
    REQUIRE(Residual(*equations, true));
    equations->Capture();
    auto base = MakeVector(driver, equations->GetnVar());
    auto actual = MakeVector(driver, equations->GetnVar());
    equations->Pack(base);
    NonlinearGlobalization::Options<> options;
    options.diagnostics = false;
    if (linearFailure) {
      options.linearIterations = 1;
      options.etaMin = 1e-8;
      options.linearTolerance = options.etaMax = 1e-6;
    } else {
      options.armijo = 0.99;
      options.maxBacktracks = 1;
    }
    options.maxRetries = 0;
    CNewtonIntegration controller({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS}, options, true);
    controller.Iterate(geometry, driver.Solvers(), driver.Numerics(), driver.Config());
    REQUIRE(controller.GetNumericalFailure());
    CHECK(controller.GetLastAttempt().linearFailure == linearFailure);
    if (!linearFailure) CHECK(controller.GetLastAttempt().search.status == Armijo::Status::EvaluationLimit);
    CHECK(controller.GetLastAttempt().rollbackStateError == 0.0);
    CHECK(controller.GetLastAttempt().rollbackMeritError == 0.0);
    equations->Pack(actual);
    CHECK(MaxDifference(base, actual) == 0.0);
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) {
          CHECK(block.solver->GetNodes()->GetUnderRelaxation(p) == 0.37);
          CHECK(block.solver->GetNodes()->GetSolution_time_n(p, v) == 7.0);
          CHECK(block.solver->GetNodes()->GetSolution_time_n1(p, v) == 9.0);
        }
  }
}

TEST_CASE("Grouped physical checks respect enthalpy and composition domains", "[NewtonController]") {
  auto mixture = species;
  for (const auto& item : std::vector<std::pair<std::string, std::string>>{{"SPECIES_INIT", "0.4"},
                                                                           {"MOLECULAR_WEIGHT", "24"},
                                                                           {"SPECIFIC_HEAT_CP", "1200"},
                                                                           {"MU_CONSTANT", "1.2e-5"},
                                                                           {"PRANDTL_LAM", "0.72"},
                                                                           {"PRANDTL_TURB", "0.9"}}) {
    const auto start = mixture.find(item.first + "= ");
    REQUIRE(start != std::string::npos);
    mixture.insert(mixture.find('\n', start), "," + item.second);
  }
  Problem problem(mixture);
  auto& driver = *problem.driver;
  auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_SPECIES_SYS});
  REQUIRE(Residual(*equations));
  auto* nodes = driver.Solvers()[FLOW_SOL]->GetNodes();
  for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
    nodes->SetSolution(p, 3, -0.05 * nodes->GetSpecificHeatCp(p) * nodes->GetTemperature(p));
  REQUIRE(Residual(*equations));
  equations->Capture();
  auto base = MakeVector(driver, equations->GetnVar());
  auto actual = MakeVector(driver, equations->GetnVar());
  auto scale = MakeVector(driver, equations->GetnVar(), 1.0);
  auto direction = MakeVector(driver, equations->GetnVar());
  equations->Pack(base);
  for (auto p = 0ul; p < driver.Geometry()->GetnPointDomain(); ++p) {
    CHECK(nodes->GetSolution(p, 3) < 0);
    CHECK(nodes->GetTemperature(p) > 0);
  }
  for (double change : {-1.0, 1.0}) {
    for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p) direction(p, 4) = change;
    equations->SetTrial(direction, 1.0, scale);
    CHECK_FALSE(Residual(*equations));
    REQUIRE(equations->Restore());
    equations->Pack(actual);
    CHECK(MaxDifference(base, actual) == 0.0);
  }
  direction = passivedouble(0);
  for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p) direction(p, 4) = direction(p, 5) = 0.2;
  equations->SetTrial(direction, 1.0, scale);
  CHECK_FALSE(Residual(*equations));
  REQUIRE(equations->Restore());
  equations->Pack(actual);
  CHECK(MaxDifference(base, actual) == 0.0);
}


TEST_CASE("Large native Newton groups repeat states and decisions", "[.][NewtonRepeatability]") {
  Problem problem(sa + "CFL_NUMBER= 0.05\nLINEAR_SOLVER_PREC= ILU\n"
                       "LINEAR_SOLVER_PREC_THREADS= 1\nLINEAR_SOLVER_ILU_LEVEL_SCHEDULING= YES\n", 193);
  auto& driver = *problem.driver;
  auto* geometry = driver.Geometry();
  auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS});
  const auto variables = equations->GetnVar();
  int threads = 0;
  std::vector<unsigned long> counts(omp_get_max_threads(), 0);
  SU2_OMP_PARALLEL {
    SU2_OMP_MASTER
    threads = omp_get_num_threads();
    END_SU2_OMP_MASTER
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) ++counts[omp_get_thread_num()];
    END_SU2_OMP_FOR
  }
  END_SU2_OMP_PARALLEL
  REQUIRE(geometry->GetnPointDomain() >= 4096ul * threads);
  for (int thread = 0; thread < threads; ++thread) REQUIRE(counts[thread] >= 3072);
  std::ofstream metadata("newton_repeatability_" + std::to_string(SU2_MPI::GetRank()) + ".csv");
  metadata.precision(17);
  metadata << "rank,threads,owned_points,global_points,preconditioner_parts,ilu_levels\n"
           << SU2_MPI::GetRank() << ',' << threads << ',' << geometry->GetnPointDomain() << ','
           << geometry->GetGlobal_nPointDomain() << ",1,1\n";
  metadata << "thread,owned_points\n";
  for (int thread = 0; thread < threads; ++thread) metadata << thread << ',' << counts[thread] << '\n';
  const auto artifactPrefix = "newton_repeatability_" + std::to_string(SU2_MPI::GetRank());
  std::filesystem::copy_file(problem.path, artifactPrefix + ".cfg");
  std::ofstream partition(artifactPrefix + "_partition.bin", std::ios::binary);
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    const auto index = geometry->nodes->GetGlobalIndex(p);
    partition.write(reinterpret_cast<const char*>(&index), sizeof(index));
    for (auto d = 0u; d < geometry->GetnDim(); ++d) {
      const double coordinate = geometry->nodes->GetCoord(p, d);
      partition.write(reinterpret_cast<const char*>(&coordinate), sizeof(coordinate));
    }
  }
  REQUIRE(partition.good());
  for (const auto& block : equations->GetBlocks()) block.solver->Jacobian.TrackPrescribedRows();
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
    const double x = geometry->nodes->GetCoord(p, 0), y = geometry->nodes->GetCoord(p, 1);
    const double wave = std::sin(6.283185307179586 * x) * std::sin(3.141592653589793 * y);
    auto* flow = driver.Solvers()[FLOW_SOL]->GetNodes();
    flow->SetSolution(p, 1, flow->GetSolution(p, 1) + 0.001 * flow->GetSolution(p, 0) * wave);
    auto* turb = driver.Solvers()[TURB_SOL]->GetNodes();
    turb->SetSolution(p, 0, turb->GetSolution(p, 0) * (1 + 0.2 * wave));
  }
  REQUIRE(Residual(*equations, true));
  REQUIRE(Residual(*equations, true));
  REQUIRE(Residual(*equations));
  auto base = MakeVector(driver, variables), residual = base, reference = base;
  auto scale = base, probe = base, input = base, product = base, productReference = base;
  auto diagonal = MakeVector(driver, variables, 100.0);
  auto save = [&](const std::string& name, const Vector& vector) {
    std::ofstream out("newton_repeatability_" + std::to_string(SU2_MPI::GetRank()) + "_" + name + ".bin",
                      std::ios::binary);
    for (auto i = 0ul; i < vector.GetNElmDomain(); ++i) {
      const double value = vector[i];
      out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    REQUIRE(out.good());
  };
  SU2_OMP_PARALLEL {
    equations->Capture();
    equations->BuildScales(scale, probe);
  }
  END_SU2_OMP_PARALLEL
  equations->Pack(base);
  equations->CopyResidual(reference);
  save("initial", base);
  save("residual", reference);
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
    for (auto v = 0u; v < variables; ++v)
      input(p, v) = 0.1 * std::sin(geometry->nodes->GetCoord(p, 0) + 2 * geometry->nodes->GetCoord(p, 1) + v);
  CNewtonLinearization linearization(*equations, geometry, driver.Config());
  bool valid = false;
  SU2_OMP_PARALLEL {
    const bool result = linearization.SetBase(
        scale, diagonal, 1e-8 * std::sqrt(variables * geometry->GetGlobal_nPointDomain()), 1e-4,
        [](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
          return !b.solver->Jacobian.IsPrescribedRow(p, v);
        },
        &probe);
    SU2_OMP_MASTER
    valid = result;
    END_SU2_OMP_MASTER
  }
  END_SU2_OMP_PARALLEL
  REQUIRE(valid);
  for (unsigned repeat = 0; repeat < 3; ++repeat) {
    REQUIRE(equations->Restore());
    REQUIRE(Residual(*equations));
    equations->CopyResidual(residual);
    INFO("physical residual difference = " << std::scientific << MaxDifference(reference, residual));
    CHECK(MaxDifference(reference, residual) == 0.0);
    SU2_OMP_PARALLEL {
      const bool result = linearization.Apply(input, product);
      SU2_OMP_MASTER
      valid = result;
      END_SU2_OMP_MASTER
    }
    END_SU2_OMP_PARALLEL
    REQUIRE(valid);
    if (repeat == 0) productReference = product;
    CHECK(MaxDifference(productReference, product) == 0.0);
  }
  save("product", productReference);
  REQUIRE(equations->Restore());

  NonlinearGlobalization::Options<> options;
  options.adaptCFL = options.adaptiveForcing = false;
  CNewtonIntegration accepted({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS}, options, false, "repeat_accepted");
  accepted.Iterate(geometry, driver.Solvers(), driver.Numerics(), driver.Config());
  INFO(accepted.GetNumericalFailureMessage());
  REQUIRE_FALSE(accepted.GetNumericalFailure());
  REQUIRE(accepted.GetLastAttempt().search.status == Armijo::Status::Accepted);
  equations->Pack(base);
  save("accepted", base);
  for (const auto& block : equations->GetBlocks()) {
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) block.solver->GetNodes()->SetUnderRelaxation(p, 0.37);
    block.solver->GetNodes()->GetSolution_time_n().resize(geometry->GetnPoint(), block.variables) = 7.0;
    block.solver->GetNodes()->GetSolution_time_n1().resize(geometry->GetnPoint(), block.variables) = 9.0;
  }
  for (bool linearFailure : {true, false}) {
    auto settings = options;
    settings.maxRetries = 0;
    if (linearFailure) {
      settings.linearIterations = 1;
      settings.etaMin = 1e-16;
      settings.linearTolerance = settings.etaMax = 1e-14;
    } else {
      settings.armijo = 0.99;
      settings.maxBacktracks = 1;
    }
    CNewtonIntegration failed({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS}, settings, false,
                              linearFailure ? "repeat_linear_failure" : "repeat_search_failure");
    failed.Iterate(geometry, driver.Solvers(), driver.Numerics(), driver.Config());
    REQUIRE(failed.GetNumericalFailure());
    CHECK(failed.GetLastAttempt().linearFailure == linearFailure);
    CHECK(failed.GetLastAttempt().rollbackStateError == 0.0);
    CHECK(failed.GetLastAttempt().rollbackMeritError == 0.0);
    equations->Pack(residual);
    CHECK(MaxDifference(base, residual) == 0.0);
    double historyError = 0;
    for (const auto& block : equations->GetBlocks())
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
        historyError = std::max(historyError,
                                std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetUnderRelaxation(p)) - 0.37));
        for (auto v = 0u; v < block.variables; ++v) {
          historyError = std::max(
              historyError, std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetSolution_time_n(p, v)) - 7.0));
          historyError = std::max(
              historyError, std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetSolution_time_n1(p, v)) - 9.0));
        }
      }
    CHECK(historyError == 0.0);
  }
  REQUIRE(equations->Restore());
  REQUIRE(Residual(*equations, true));
  for (const auto& block : equations->GetBlocks()) {
    driver.Config()->SetGlobalParam(driver.Config()->GetKind_Solver(), block.runtime);
    SU2_OMP_PARALLEL {
      block.solver->SetTime_Step(geometry, driver.Solvers(), driver.Config(), MESH_0, 0);
      block.solver->PrepareImplicitIteration(geometry, driver.Solvers(), driver.Config());
    }
    END_SU2_OMP_PARALLEL
  }
  auto timeStep = MakeVector(driver, variables);
  std::ofstream matrix(artifactPrefix + "_matrix.bin", std::ios::binary);
  for (const auto& block : equations->GetBlocks())
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      for (auto v = 0u; v < block.variables; ++v)
        timeStep(p, block.offset + v) = SU2_TYPE::GetValue(block.solver->GetNodes()->GetDelta_Time(p));
      for (auto neighbor = 0u; neighbor <= geometry->nodes->GetnPoint(p); ++neighbor) {
        const auto q = neighbor == 0 ? p : geometry->nodes->GetPoint(p, neighbor - 1);
        const auto view = block.solver->Jacobian.GetBlockView(p, q);
        for (auto v = 0u; v < block.variables; ++v)
          for (auto w = 0u; w < block.variables; ++w) {
            const double value = SU2_TYPE::GetValue(view(v, w));
            matrix.write(reinterpret_cast<const char*>(&value), sizeof(value));
          }
      }
    }
  REQUIRE(matrix.good());
  save("time_step", timeStep);
  CNewtonBlockPreconditioner native(linearization, geometry, driver.Config());
  SU2_OMP_PARALLEL {
    native.Build();
    native(input, product);
  }
  END_SU2_OMP_PARALLEL
  save("preconditioned", product);

  auto* flow = driver.Solvers()[FLOW_SOL];
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    flow->LinSysRes(p, 0) = p == 0 ? 1e8 : (p % 1024 == 0 && p < 1024ul * threads ? 1.0 : 0.0);
    for (auto v = 1u; v < flow->GetnVar(); ++v) flow->LinSysRes(p, v) = 1;
  }
  double local = 1e16, global = 0;
  // Three small partials added before 1e16 give a different answer than this specified thread order.
  for (int thread = 1; thread < threads; ++thread) local += 1;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  const double expected = std::sqrt(global / geometry->GetGlobal_nPointDomain());
  unsigned long localPoint = geometry->nodes->GetGlobalIndex(0), selectedPoint = 0;
  SU2_MPI::Allreduce(&localPoint, &selectedPoint, 1, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
  double localCoord[2], expectedCoord[2];
  for (auto d = 0u; d < 2; ++d)
    localCoord[d] = localPoint == selectedPoint ? SU2_TYPE::GetValue(geometry->nodes->GetCoord(0, d))
                                                : std::numeric_limits<double>::lowest();
  SU2_MPI::Allreduce(localCoord, expectedCoord, 2, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  for (unsigned repeat = 0; repeat < 32; ++repeat) {
    SU2_OMP_PARALLEL { flow->RefreshResidualStatistics(geometry, driver.Config()); }
    END_SU2_OMP_PARALLEL
    CHECK(SU2_TYPE::GetValue(flow->GetRes_RMS(0)) == expected);
    CHECK(flow->GetPoint_Max(1) == selectedPoint);
    for (auto d = 0u; d < 2; ++d) CHECK(SU2_TYPE::GetValue(flow->GetPoint_Max_Coord(1)[d]) == expectedCoord[d]);
  }
}

TEST_CASE("Native Newton block preconditioning waits for every input producer", "[NewtonLinearization]") {
  Problem problem(sa + "LINEAR_SOLVER_PREC= ILU\n");
  auto& driver = *problem.driver;
  auto* geometry = driver.Geometry();
  auto equations = problem.Equations({RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS});
  for (const auto& block : equations->GetBlocks()) block.solver->Jacobian.TrackPrescribedRows();
  REQUIRE(Residual(*equations, true));
  REQUIRE(Residual(*equations, true));
  equations->Capture();
  const auto variables = equations->GetnVar();
  auto scale = MakeVector(driver, variables), probe = scale;
  equations->BuildScales(scale, probe);
  auto diagonal = MakeVector(driver, variables, 100.0);
  for (const auto& block : equations->GetBlocks())
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) block.solver->Jacobian.AddVal2Diag(p, 100.0);
  CNewtonLinearization linearization(*equations, geometry, driver.Config());
  REQUIRE(linearization.SetBase(
      scale, diagonal, 1e-8 * std::sqrt(variables * geometry->GetGlobal_nPointDomain()), 1e-4,
      [](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
        return !b.solver->Jacobian.IsPrescribedRow(p, v);
      },
      &probe));
  const int threads = std::min(4, omp_get_max_threads());
  auto known = MakeVector(driver, variables, 0.125);
  auto delayed = MakeVector(driver, variables), actual = delayed, expected = delayed;
  for (bool lower : {false, true}) {
    CAPTURE(lower, threads);
    CNewtonBlockPreconditioner preconditioner(linearization, geometry, driver.Config(), lower);
    SU2_OMP_PARALLEL_ON(threads) {
      preconditioner.Build();
      preconditioner(known, expected);
    }
    END_SU2_OMP_PARALLEL
    delayed = passivedouble(0);
    SU2_OMP_PARALLEL_ON(threads) {
      if (threads > 1 && omp_get_thread_num() == threads - 1)
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
      // CSysVector producers use nowait; the consumer changes the work partition to native point blocks.
      delayed = known;
      preconditioner(delayed, actual);
    }
    END_SU2_OMP_PARALLEL
    CHECK(MaxDifference(expected, actual) == 0.0);
  }
}

TEST_CASE("Legacy Newton preconditioning waits for every input producer", "[NewtonLinearization]") {
  for (const auto& policy : {"TIME_DISCRE_FLOW= EULER_EXPLICIT\nNEWTON_KRYLOV_IPARAM= 0,0,1,0\n",
                             "TIME_DISCRE_FLOW= EULER_IMPLICIT\nNEWTON_KRYLOV_IPARAM= 0,0,1,0\n",
                             "TIME_DISCRE_FLOW= EULER_IMPLICIT\nNEWTON_KRYLOV_IPARAM= 0,2,1,0\n"}) {
    CAPTURE(policy);
    Problem problem(std::string("SOLVER= NAVIER_STOKES\nNEWTON_KRYLOV= YES\nCFL_NUMBER= 0.05\n") +
                    "LINEAR_SOLVER_PREC= ILU\nNEWTON_KRYLOV_DPARAM= 0,0.1,0,1e-6,1\n" + policy);
    auto& driver = *problem.driver;
    CNewtonLegacyIntegration integration;
    {
      Silence silence;
      driver.FlowIteration(integration);
    }
    const int threads = std::min(4, omp_get_max_threads());
    auto known = MakeVector(driver, driver.Solvers()[FLOW_SOL]->GetnVar(), 0.125);
    auto delayed = MakeVector(driver, known.GetNVar()), actual = delayed, expected = delayed;
    SU2_OMP_PARALLEL_ON(threads) { integration.Preconditioner(known, expected); }
    END_SU2_OMP_PARALLEL
    SU2_OMP_PARALLEL_ON(threads) {
      if (threads > 1 && omp_get_thread_num() == threads - 1)
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
      delayed = known;
      integration.Preconditioner(delayed, actual);
    }
    END_SU2_OMP_PARALLEL
    CHECK(MaxDifference(expected, actual) == 0.0);
  }
}

TEST_CASE("Grouped physical residuals isolate legacy Jacobian relaxation", "[NewtonEquations]") {
  Problem problem("SOLVER= NAVIER_STOKES\n", 5, true);
  auto& driver = *problem.driver;
  auto* config = driver.Config();
  auto* geometry = driver.Geometry();
  auto* nodes = driver.Solvers()[FLOW_SOL]->GetNodes();
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
    for (auto v = 0u; v < driver.Solvers()[FLOW_SOL]->GetnVar(); ++v)
      nodes->SetSolution(p, v,
                         nodes->GetSolution(p, v) *
                             (1 + 0.02 * std::sin(geometry->nodes->GetCoord(p, 0) + geometry->nodes->GetCoord(p, 1))));
  auto equations = problem.Equations({RUNTIME_FLOW_SYS});
  auto reference = MakeVector(driver, equations->GetnVar()), actual = reference;
  for (bool implicit : {false, true}) {
    CAPTURE(implicit);
    config->SetNewtonKrylovRelaxation(1.0);
    REQUIRE(Residual(*equations, implicit));
    equations->CopyResidual(reference);
    for (double relaxation : {0.25, 0.0, 1.0}) {
      CAPTURE(relaxation);
      config->SetNewtonKrylovRelaxation(relaxation);
      REQUIRE(Residual(*equations, implicit));
      CHECK(SU2_TYPE::GetValue(config->GetNewtonKrylovRelaxation()) == relaxation);
      equations->CopyResidual(actual);
      CHECK(MaxDifference(reference, actual) == 0.0);
    }
  }
  config->SetNewtonKrylovRelaxation(0.25);
  equations->Capture();
  auto scale = MakeVector(driver, equations->GetnVar(), 1.0), direction = MakeVector(driver, equations->GetnVar());
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) direction(p, 0) = -2 * nodes->GetSolution(p, 0);
  equations->SetTrial(direction, 1.0, scale);
  CHECK_FALSE(Residual(*equations));
  CHECK(SU2_TYPE::GetValue(config->GetNewtonKrylovRelaxation()) == 0.25);
  REQUIRE(equations->Restore());
  CHECK(SU2_TYPE::GetValue(config->GetNewtonKrylovRelaxation()) == 0.25);
}


TEST_CASE("The common controller applies legacy flow direction policies", "[NewtonFlowPolicy]") {
  struct Profile {
    const char* integers;
    const char* doubles;
    bool startup, deflation, automatic;
    const char* inner = "NONE";
  };
  for (const auto& profile : std::vector<Profile>{{"2,1,2,0", "1,0.1,-2,1e-6,1", true, false, false},
                                                  {"0,2,1,0", "0,0.1,0,1e-6,0.5", false, false, false},
                                                  {"0,2,1,1", "0,0.1,0,1e-6,1", false, true, false},
                                                  {"0,0,1,0", "0,0.1,0,1e-6,-1", false, false, true},
                                                  {"2,1,2,0", "1,0.1,-2,1e-6,1", true, false, false, "BCGSTAB"},
                                                  {"2,2,2,0", "1,0.1,-2,1e-6,1", true, false, false, "SMOOTHER"},
                                                  {"0,4,1,0", "0,0.1,0,1e-6,1", false, false, false, "BCGSTAB"},
                                                  {"0,4,1,0", "0,0.1,0,1e-6,1", false, false, false, "SMOOTHER"}}) {
    for (bool coupled : {false, true}) {
      CAPTURE(profile.integers, profile.doubles, profile.inner, coupled);
      const std::string physics = coupled ? sa : "SOLVER= NAVIER_STOKES\n";
      Problem problem(
          physics +
              "NEWTON_KRYLOV= YES\nCFL_NUMBER= 0.05\nLINEAR_SOLVER_PREC= ILU\n"
              "LINEAR_SOLVER_RESTART_FREQUENCY= 20\nLINEAR_SOLVER_RESTART_DEFLATION= 5\n"
              "BODY_FORCE= YES\nBODY_FORCE_VECTOR= 0.1,0,0\n"
              "LINEAR_SOLVER_ITER= " +
              (profile.automatic ? std::string("1") : std::string("40")) +
              "\nLINEAR_SOLVER_ERROR= " + (profile.automatic ? std::string("1e-12") : std::string("0.001")) +
              "\nLINEAR_SOLVER_INNER= " + profile.inner + "\nNEWTON_KRYLOV_IPARAM= " + profile.integers + "\nNEWTON_KRYLOV_DPARAM= " + profile.doubles + "\n",
          5, true);
      auto& driver = *problem.driver;
      NonlinearGlobalization::Options<> options;
      options.adaptiveForcing = false;
      options.adaptCFL = false;
      options.diagnostics = false;
      std::vector<unsigned short> systems{RUNTIME_FLOW_SYS};
      if (coupled) systems.push_back(RUNTIME_TURB_SYS);
      CNewtonIntegration integration(systems, options, coupled, "flow_policy", true);
      double priorReconstruction = 1;
      for (unsigned iteration = 0; iteration < 4; ++iteration) {
        driver.Config()->SetInnerIter(iteration);
        driver.Config()->SetNewtonKrylovRelaxation(0.73);
        integration.Iterate(driver.Geometry(), driver.Solvers(), driver.Numerics(), driver.Config());
        INFO(integration.GetNumericalFailureMessage());
        const auto& attempt = integration.GetLastAttempt();
        INFO("defect=" << attempt.linearDefect << ", base=" << attempt.base << ", target=" << attempt.linearTarget);
        REQUIRE_FALSE(integration.GetNumericalFailure());
        CHECK(attempt.search.status == Armijo::Status::Accepted);
        CHECK(attempt.linearDefect <= options.etaMax);
        CHECK(attempt.startup == (profile.startup && iteration < 2));
        CHECK(attempt.deflated == (profile.deflation && !attempt.startup));
        if (!attempt.startup && !profile.automatic)
          CHECK(attempt.reconstruction == (std::string(profile.doubles).find("0.5") != std::string::npos ? 0.5 : 1.0));
        if (profile.startup && !attempt.startup) {
          CHECK(attempt.toleranceFactor > 1);
          CHECK(attempt.toleranceFactor <= 2);
        }
        if (profile.automatic && iteration > 0) CHECK(attempt.reconstruction < priorReconstruction);
        priorReconstruction = attempt.reconstruction;
        CHECK(SU2_TYPE::GetValue(driver.Config()->GetNewtonKrylovRelaxation()) == 0.73);
        if (attempt.startup)
          CHECK(attempt.linearIterations > 0);
        else if (profile.startup || profile.deflation || std::string(profile.integers).find("0,0") != 0)
          CHECK(attempt.preconditionerIterations > 0);
      }
    }
  }
}

TEST_CASE("Newton probes preserve reconstruction iteration history", "[NewtonEquations]") {
  Problem problem("SOLVER= NAVIER_STOKES\n", 5, true);
  auto& driver = *problem.driver;
  auto* flow = dynamic_cast<CFlowVariable*>(driver.Solvers()[FLOW_SOL]->GetNodes());
  REQUIRE(flow);
  auto& counters = flow->NonPhysicalEdgeCounter;
  REQUIRE(counters.size() > 0);
  for (auto e = 0ul; e < counters.size(); ++e) counters[e] = 1 + e % 3;
  const auto saved = counters;
  auto equations = problem.Equations({RUNTIME_FLOW_SYS});
  REQUIRE(Residual(*equations));
  equations->Capture();
  auto reference = MakeVector(driver, equations->GetnVar()), actual = reference;
  equations->CopyResidual(reference);
  for (unsigned repeat = 0; repeat < 3; ++repeat) {
    REQUIRE(Residual(*equations));
    equations->CopyResidual(actual);
    CHECK(MaxDifference(reference, actual) == 0.0);
    for (auto e = 0ul; e < counters.size(); ++e) CHECK(counters[e] == saved[e]);
  }
  auto direction = MakeVector(driver, equations->GetnVar()), scale = MakeVector(driver, equations->GetnVar(), 1);
  for (auto p = 0ul; p < driver.Geometry()->GetnPoint(); ++p)
    direction(p, 0) = -2 * flow->GetSolution(p, 0);
  equations->SetTrial(direction, 1, scale);
  CHECK_FALSE(Residual(*equations));
  REQUIRE(equations->Restore());
  CHECK(equations->GetRestorationError() == 0.0);
  equations->CopyResidual(actual);
  CHECK(MaxDifference(reference, actual) == 0.0);
  equations->CommitHistory();
  for (auto e = 0ul; e < counters.size(); ++e) CHECK(counters[e] == saved[e] - 1);
}

TEST_CASE("Reconstructed Newton products converge independently of the requested probe size", "[NewtonLinearization]") {
  Problem problem("SOLVER= NAVIER_STOKES\nNEWTON_KRYLOV= YES\nNEWTON_KRYLOV_IPARAM= 0,0,1,0\n", 5, true);
  auto& driver = *problem.driver;
  auto* config = driver.Config();
  auto* geometry = driver.Geometry();
  auto* solver = driver.Solvers()[FLOW_SOL];
  auto* nodes = solver->GetNodes();
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
    const double x = geometry->nodes->GetCoord(p, 0), y = geometry->nodes->GetCoord(p, 1);
    nodes->SetSolution(p, 0, nodes->GetSolution(p, 0) * (1 + 0.02 * std::sin(0.7 + 1.3 * x + 3.9 * y)));
    nodes->SetSolution(p, 1, nodes->GetSolution(p, 1) * (1 + 0.015 * std::cos(0.2 + 2.1 * x + 0.9 * y)));
    nodes->SetSolution(p, 2, 0.03 * nodes->GetSolution(p, 1) * (0.8 + 0.2 * std::sin(0.6 + x + 2.3 * y)));
    nodes->SetSolution(p, 3, nodes->GetSolution(p, 3) * (1 + 0.01 * std::cos(0.1 + 1.7 * x + 4.1 * y)));
  }
  solver->Jacobian.TrackPrescribedRows();
  auto equations = problem.Equations({RUNTIME_FLOW_SYS});
  REQUIRE(Residual(*equations, true));
  REQUIRE(Residual(*equations, true));
  equations->Capture();
  auto scale = MakeVector(driver, equations->GetnVar()), probe = scale;
  equations->BuildScales(scale, probe);
  auto diagonal = MakeVector(driver, equations->GetnVar());
  auto direction = diagonal, actual = diagonal, physical = diagonal;
  auto isFree = [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
    return !b.solver->Jacobian.IsPrescribedRow(p, v);
  };
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
    for (auto j = 0u; j < solver->GetnVar(); ++j)
      if (!solver->Jacobian.IsPrescribedRow(p, j))
        direction(p, j) = 0.01 * (1 + j) * (1 + geometry->nodes->GetCoord(p, 0) + 0.3 * geometry->nodes->GetCoord(p, 1));
  const double stateRMS = scale.norm() / std::sqrt(geometry->GetGlobal_nPointDomain());
  config->SetNewtonKrylovRelaxation(0.73);
  for (double reconstruction : {1.0, 0.25}) {
    CAPTURE(reconstruction);
    auto evaluate = [&](double h) {
      auto residual = MakeVector(driver, equations->GetnVar());
      bool valid = false;
      SU2_OMP_PARALLEL {
        equations->SetTrial(direction, h, scale, isFree);
        const bool result = equations->EvaluateResiduals(false, false, reconstruction);
        SU2_OMP_MASTER
        valid = result;
        END_SU2_OMP_MASTER
        equations->CopyResidual(residual);
      }
      END_SU2_OMP_PARALLEL
      REQUIRE(valid);
      return residual;
    };
    auto reference = [&](double h) {
      auto positive = evaluate(h), negative = evaluate(-h);
      for (auto i = 0ul; i < positive.GetLocSize(); ++i) positive[i] = (positive[i] - negative[i]) / (2 * h * scale[i]);
      return positive;
    };
    auto centered = reference(1e-5), refined = reference(5e-6);
    centered -= refined;
    CHECK(centered.norm() < 1e-4 * refined.norm());
    for (double requested : {1e-4, 1e-6}) {
      CAPTURE(requested);
      evaluate(0);
      CNewtonLinearization linearization(*equations, geometry, config);
      bool validBase = false, validProduct = false;
      SU2_OMP_PARALLEL {
        const bool base = linearization.SetBase(scale, diagonal, requested * stateRMS, 1e-4, isFree, &probe,
                                                reconstruction, true, true);
        const bool product = base && linearization.Apply(direction, actual, false);
        SU2_OMP_MASTER {
          validBase = base;
          validProduct = product;
        }
        END_SU2_OMP_MASTER
      }
      END_SU2_OMP_PARALLEL
      REQUIRE(validBase);
      REQUIRE(validProduct);
      auto error = actual;
      error -= refined;
      INFO("relative centered error=" << error.norm() / refined.norm());
      CHECK(error.norm() < 2e-4 * refined.norm());
      CHECK(SU2_TYPE::GetValue(config->GetNewtonKrylovRelaxation()) == 0.73);
    }
    if (reconstruction == 1)
      physical = actual;
    else {
      actual -= physical;
      CHECK(actual.norm() > 1e-3 * physical.norm());
    }
  }
  equations->SetTrial(direction, 0, scale, isFree);
  CNewtonLinearization unresolved(*equations, geometry, config);
  REQUIRE(unresolved.SetBase(scale, diagonal, 1e-20, 0, isFree));
  CHECK_FALSE(unresolved.Apply(direction, actual));
  REQUIRE(equations->Restore());
  CHECK(equations->GetRestorationError() == 0.0);
}
