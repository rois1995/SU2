/*!
 * \file newton_equations.cpp
 * \brief Native solver state mapping, grouped residuals and cross derivatives.
 * \author SU2 Contributors
 */
#include "catch.hpp"
#include "../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../SU2_CFD/include/integration/CNewtonEquationSet.hpp"
#include "../../SU2_CFD/include/integration/CNewtonLinearization.hpp"
#include "../../Common/include/linear_algebra/CSysSolve.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
using Vector = CSysVector<passivedouble>;

class NewtonDriver : public CSinglezoneDriver {
 public:
  explicit NewtonDriver(char* path) : CSinglezoneDriver(path, 1, SU2_MPI::GetComm()) {}
  CConfig* Config() { return config_container[0]; }
  CGeometry* Geometry() { return geometry_container[0][0][0]; }
  CSolver** Solvers() { return solver_container[0][0][0]; }
  CNumerics*** Numerics() { return numerics_container[0][0][0]; }
};

class Silence {
  std::streambuf* old = std::cout.rdbuf(nullptr);

 public:
  ~Silence() { std::cout.rdbuf(old); }
};

struct Problem {
  std::string path;
  std::unique_ptr<NewtonDriver> driver;
  explicit Problem(const std::string& physics) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    path = (std::filesystem::temp_directory_path() /
            ("su2_newton_" + std::to_string(stamp) + "_" + std::to_string(SU2_MPI::GetRank()) + ".cfg"))
               .string();
    {
      std::ofstream config(path);
      config << "MESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= 5,5,5\nMESH_BOX_LENGTH= 1,1,1\n"
                "MARKER_HEATFLUX= (y_minus, 0)\nMARKER_FAR= (x_minus,x_plus,y_plus)\n"
                "MACH_NUMBER= 0.2\nREYNOLDS_NUMBER= 1e6\nMUSCL_FLOW= NO\nMUSCL_TURB= NO\n"
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
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      if (!geometry->nodes->GetSolidBoundary(p))
        for (auto j = 0u; j < equations->GetnVar(); ++j)
          known(p, j) = 0.01 * (1.0 + j + geometry->nodes->GetCoord(p, 0));
    auto isFree = [&](const CNewtonEquationSet::Block&, unsigned long p, unsigned short) {
      return !geometry->nodes->GetSolidBoundary(p);
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
          positive(p, j) = geometry->nodes->GetSolidBoundary(p)
                               ? 101.0 * u(p, j)
                               : (positive(p, j) - negative(p, j)) / (2e-5 * scale(p, j)) + 100.0 * u(p, j);
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
      if (geometry->nodes->GetSolidBoundary(p))
        for (auto j = 0u; j < equations->GetnVar(); ++j) zero(p, j) = 0.01;
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
