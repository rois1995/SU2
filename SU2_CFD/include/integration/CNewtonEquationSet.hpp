/*!
 * \file CNewtonEquationSet.hpp
 * \brief State and residual interface for selected Newton equation systems.
 * \author SU2 Contributors
 */
#pragma once

#include "CIntegration.hpp"
#include <stdexcept>

/*!
 * \brief A group uses fixed coordinates q = M_base*w, where w is each solver's stored state.
 * For conservative scalars M_base = rho_base; otherwise it is the identity. Density in the
 * physical residual remains live, including its cross derivatives. Physical-time history is never used as scratch.
 * Methods with point loops are collective over an existing OpenMP team, or may be called serially.
 */
class CNewtonEquationSet : private CIntegration {
 public:
  struct Block {
    unsigned short runtime, slot, offset, variables;
    CSolver* solver;
  };

 private:
  struct Snapshot {
    CSolver* solver;
    unsigned short slot, variables;
    std::vector<su2double> solution, oldSolution, evaluationHistory, relaxation, boundarySolution, boundaryInput;
    std::vector<unsigned char> prescribed;
  };
  CGeometry* geometry;
  CSolver** solvers;
  CNumerics*** numerics;
  CConfig* config;
  std::vector<Block> blocks;
  std::vector<Snapshot> snapshots;
  std::vector<su2double> density;
  unsigned short variables = 0;
  std::array<unsigned long, 4> residualEvaluations{};
  bool captured = false, valid = true;
  double invalidCount = 0;
  std::vector<su2double> trialFlow;
  su2vector<int8_t>* reconstructionHistory = nullptr;
  std::vector<int8_t> acceptedReconstruction, savedReconstruction, nextReconstruction;

  void RestoreSolutions();
  void SaveHistory();
  void RestoreHistory();
  void Communicate();
  void Refresh();
  void InstallStrongValues(CSolver*) const;
  void StrongState(unsigned short runtime);
  bool CheckState(bool compareFlow, bool modelBounds = false);

 public:
  CNewtonEquationSet(CGeometry*, CSolver**, CNumerics***, CConfig*, const std::vector<unsigned short>& runtimes);
  const std::vector<Block>& GetBlocks() const { return blocks; }
  unsigned short GetnVar() const { return variables; }
  const std::array<unsigned long, 4>& GetResidualEvaluations() const { return residualEvaluations; }
  /*! \brief Capture an accepted base after its residual/property refresh, fixing the scalar density factors. */
  void Capture();

  /*! \brief Construct feasible perturbations a,b with a-b=direction on free coordinates. */
  bool FeasibleProbes(const CSysVector<passivedouble>& direction, const CSysVector<passivedouble>& scale,
                      double step, const std::vector<unsigned char>& free, CSysVector<passivedouble>& a,
                      CSysVector<passivedouble>& b, bool& shifted);

  template <class Scalar>
  void BuildScales(CSysVector<Scalar>& scale, CSysVector<Scalar>& probe) const {
    Pack(scale);
    double velocity = 0;
    for (auto d = 0u; d < geometry->GetnDim(); ++d)
      velocity += pow(SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetVelocity_Inf(d)), 2);
    velocity = std::sqrt(velocity);
    const bool compressible = config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE;
    const auto rhoInf = SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetDensity_Inf());
    for (const auto& block : blocks) {
      const auto* reference = block.solver->GetLimiterReference();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v) {
          const auto j = block.offset + v;
          const auto mass = SU2_TYPE::GetValue(MassFactor(block, p));
          double ref = reference ? mass * std::abs(SU2_TYPE::GetValue(reference[v])) : 0;
          if (block.slot == FLOW_SOL && v >= 1 && v <= geometry->GetnDim())
            ref = velocity * (compressible ? SU2_TYPE::GetValue(density[p]) : 1.0);
          if (block.slot == FLOW_SOL && v == 0 && !compressible) ref = rhoInf * velocity * velocity;
          // Incompressible flow stores reference-relative enthalpy, which can be zero or negative at positive temperature.
          if (block.slot == FLOW_SOL && v == block.variables - 1 && !compressible)
            ref = std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetSpecificHeatCp(p) *
                                             block.solver->GetNodes()->GetTemperature(p)));
          scale(p, j) = std::max(std::abs(SU2_TYPE::GetValue(scale(p, j))), ref);
          if (scale(p, j) == 0) scale(p, j) = 1;
          probe(p, j) = scale(p, j);
          if (block.slot == TURB_SOL && config->GetKind_Turb_Model() == TURB_MODEL::SST)
            probe(p, j) = std::max(mass * std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetSolution_Old(p, v))),
                                    ref * 1e-12);
        }
      END_SU2_OMP_FOR
    }
  }

  /*! \brief Restore every dependent solution and Solution_Old before installing all selected trial values. */
  template <class Scalar, class IsFree>
  void SetTrial(const CSysVector<Scalar>& direction, double step, const CSysVector<Scalar>& scale, IsFree isFree) {
    assert(captured && direction.GetNVar() == variables && scale.GetNVar() == variables);
    RestoreSolutions();
    for (const auto& block : blocks) {
      auto* nodes = block.solver->GetNodes();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
        for (auto v = 0u; v < block.variables; ++v) {
          if (step == 0 || !isFree(block, p, v)) continue;
          const auto j = block.offset + v;
          const su2double change = step * direction(p, j) * scale(p, j) / MassFactor(block, p);
          nodes->SetSolution(p, v, nodes->GetSolution(p, v) + change);
        }
      }
      END_SU2_OMP_FOR
    }
  }
  template <class Scalar>
  void SetTrial(const CSysVector<Scalar>& direction, double step, const CSysVector<Scalar>& scale) {
    SetTrial(direction, step, scale, [](const Block&, unsigned long, unsigned short) { return true; });
  }

  su2double MassFactor(const Block& block, unsigned long p) const {
    return block.solver->GetConservative() ? density[p] : su2double(1.0);
  }

  /*! \brief Read the current state in the same fixed coordinates as SetTrial and the native linear corrections. */
  template <class Scalar>
  void Pack(CSysVector<Scalar>& state) const {
    assert(captured && state.GetNVar() == variables);
    for (const auto& block : blocks) {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          state(p, block.offset + v) =
              SU2_TYPE::GetValue(MassFactor(block, p) * block.solver->GetNodes()->GetSolution(p, v));
      END_SU2_OMP_FOR
    }
  }
  template <class Scalar>
  void CopyResidual(CSysVector<Scalar>& residual) const {
    assert(residual.GetNVar() == variables);
    for (const auto& block : blocks) {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          residual(p, block.offset + v) = SU2_TYPE::GetValue(block.solver->LinSysRes(p, v));
      END_SU2_OMP_FOR
    }
  }

  /*! \brief Rebuild dependent coefficients and all selected physical residuals, with no pseudo-time term. */
  bool EvaluateResiduals(bool implicit = false, bool allEquations = false, double reconstruction = 1.0);
  /*! \brief Rebuild coefficients at the captured state; retain physical-time history and saved relaxation. */
  bool Restore();
  /*! \brief Advance reconstruction iteration history once, after accepting a flow update. */
  void CommitHistory();
  /*! \brief Collective maximum restoration error over dependent state, row metadata, history and relaxation. */
  double GetRestorationError();
};
