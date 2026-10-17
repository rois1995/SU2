/*!
 * \file CNewtonEquationSet.hpp
 * \brief State and residual interface for selected Newton equation systems.
 * \author SU2 Contributors
 */
#pragma once

#include "CIntegration.hpp"
#include <memory>
#include <stdexcept>

/*!
 * \class CNewtonEquationSet
 * \brief Install trial states and assemble the physical residual of an ordered equation group.
 * \details A group uses fixed coordinates q = M_base*w, where w is each solver's stored state.
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

  /*! \brief Scratch used only during a residual evaluation; sequential groups may share it. */
  struct ResidualWorkspace {
    struct Buffers {
      std::vector<su2double> evaluationHistory, boundarySolution, boundaryInput;
    };
    std::array<Buffers, MAX_SOLS> systems;
    std::vector<su2double> trialFlow;
    std::vector<int8_t> savedReconstruction;
  };

 private:
  struct Snapshot {
    CSolver* solver;
    unsigned short slot, variables;
    std::vector<su2double> solution, oldSolution, relaxation;
    std::vector<unsigned char> prescribed;
    ResidualWorkspace::Buffers* work;
  };
  CGeometry* geometry;
  CSolver** solvers;
  CNumerics*** numerics;
  CConfig* config;
  std::vector<Block> blocks;
  std::shared_ptr<ResidualWorkspace> workspace;
  std::vector<Snapshot> snapshots;
  std::vector<su2double> density;
  unsigned short variables = 0;
  std::array<unsigned long, 4> residualEvaluations{};
  unsigned long flowBoundaryEvaluations = 0;
  bool captured = false, valid = true;
  bool reuseBoundaryGradients = false; /*!< \brief Collective eligibility from this evaluation's boundary closure. */
  double invalidCount = 0;
  su2vector<int8_t>* reconstructionHistory = nullptr;
  std::vector<int8_t> acceptedReconstruction, nextReconstruction;
  mutable su2activematrix periodicWork;

  /*! \brief Restore all dependent primary values, iteration history, row masks and relaxation. */
  void RestoreSolutions();
  /*! \brief Save numerical iteration history before a residual evaluation. */
  void SaveHistory();
  /*! \brief Undo evaluation side effects without touching physical-time solution arrays. */
  void RestoreHistory();
  /*! \brief Exchange every dependent solver state before refreshing derived quantities. */
  void Communicate();
  /*! \brief Refresh live flow, turbulence, transition and species coefficients in dependency order. */
  void Refresh();
  /*! \brief Remove the physical residual on coordinates prescribed by native boundary conditions. */
  void InstallStrongValues(CSolver*) const;
  /*! \brief Apply native strong boundary data and record the corresponding prescribed rows. */
  void StrongState(unsigned short runtime);
  /*! \brief Add B(w)(I-P)w, retaining arbitrary periodic trial directions in the residual. */
  void AddPeriodicConstraints(CSolver*);
  /*! \brief Reject invalid states/properties and unintended native clipping of free flow values. */
  bool CheckState(bool compareFlow, bool modelBounds = false);

 public:
  /*! \brief Bind native solvers in the requested packing order and allocate dependent-state snapshots. */
  CNewtonEquationSet(CGeometry*, CSolver**, CNumerics***, CConfig*, const std::vector<unsigned short>& runtimes,
                     std::shared_ptr<ResidualWorkspace> work = {});
  const std::vector<Block>& GetBlocks() const { return blocks; }
  unsigned short GetnVar() const { return variables; }
  const std::array<unsigned long, 4>& GetResidualEvaluations() const { return residualEvaluations; }
  /*! \brief Count boundary-only flow evaluations separately from complete residual assemblies. */
  unsigned long GetFlowBoundaryEvaluations() const { return flowBoundaryEvaluations; }
  /*!
   * \brief Apply the periodic average P, rotating flow momentum into each local frame.
   * \param[in,out] vector - Packed correction vector.
   * \param[in] scale - Optional positive diagonal S; apply S^-1 P S when supplied.
   */
  void ProjectPeriodic(CSysVector<passivedouble>&, const CSysVector<passivedouble>* scale = nullptr) const;
  /*! \brief Synchronize native base time steps without changing the physical right hand side. */
  void SynchronizeTimeSteps();
  /*! \brief Capture an accepted base after its residual/property refresh, fixing the scalar density factors. */
  void Capture();

  /*!
   * \brief Construct feasible perturbations a,b with a-b=direction on free coordinates.
   * \details The two states are base+h*S*a and base+h*S*b in fixed group coordinates.
   * Moving both probes inward preserves the requested derivative at a bound without clipping its direction.
   * \param[out] shifted - Whether the second probe differs from the captured base.
   * \return False when neither the requested pair nor an inward shift is feasible at this step size.
   */
  bool FeasibleProbes(const CSysVector<passivedouble>& direction, const CSysVector<passivedouble>& scale,
                      double step, const std::vector<unsigned char>& free, CSysVector<passivedouble>& a,
                      CSysVector<passivedouble>& b, bool& shifted);

  /*! \brief Build positive state scales and separate references for relative finite-difference probes. */
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
          /*--- Flow and transported flamelet enthalpy can cross zero at finite temperature. ---*/
          const bool enthalpy = (block.slot == FLOW_SOL && v == block.variables - 1) ||
              (block.slot == SPECIES_SOL && config->GetKind_Species_Model() == SPECIES_MODEL::FLAMELET && v == 1);
          if (enthalpy && !compressible)
            ref = mass * std::abs(SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetNodes()->GetSpecificHeatCp(p) *
                                                    solvers[FLOW_SOL]->GetNodes()->GetTemperature(p)));
          scale(p, j) = std::max(std::abs(SU2_TYPE::GetValue(scale(p, j))), ref);
          if (scale(p, j) == 0) scale(p, j) = 1;
          probe(p, j) = scale(p, j);
          /*--- Resolve momentum probes in a moving internal flow even when its freestream is nearly at rest.
           *     This reference only sizes differences; the residual and state scales above remain unchanged. ---*/
          if (block.slot == FLOW_SOL && compressible && v >= 1 && v <= geometry->GetnDim()) {
            const auto* flow = solvers[FLOW_SOL]->GetNodes();
            const double speed = std::max(std::sqrt(SU2_TYPE::GetValue(flow->GetVelocity2(p))),
                                          SU2_TYPE::GetValue(flow->GetSoundSpeed(p)));
            probe(p, j) = std::max(SU2_TYPE::GetValue(probe(p, j)), SU2_TYPE::GetValue(density[p]) * speed);
          }
          if (block.slot == TURB_SOL && config->GetKind_Turb_Model() == TURB_MODEL::SST)
            probe(p, j) = std::max(mass * std::abs(SU2_TYPE::GetValue(block.solver->GetNodes()->GetSolution_Old(p, v))),
                                    ref * 1e-12);
        }
      END_SU2_OMP_FOR
    }
  }

  /*!
   * \brief Restore the captured base, then install q_trial=q_base+step*S*direction on free coordinates.
   * \details Unselected dependent states and Solution_Old are restored as well; physical-time history is untouched.
   */
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

  /*! \brief Fixed coordinate factor M_base; never substitute the density of a trial state here. */
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
  /*! \brief Pack the current physical residual in the requested group order, without state scaling. */
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

  /*!
   * \brief Rebuild live coefficients and physical residuals, with no pseudo-time term.
   * \details Periodic groups use F(w)=P*R(w)+B(w)*(I-P)*w, where B is live density for conservative
   * scalars and identity otherwise. Trial unknowns are not averaged. Numerical iteration history is frozen.
   * \param[in] implicit - Also assemble native matrices for the block preconditioner.
   * \param[in] allEquations - Refresh residuals outside the selected group for outer-driver statistics.
   * \param[in] reconstruction - Reconstruction factor for an explicitly requested derivative model.
   * \param[in] residualRuntime - Optional single residual row for explicit preconditioner products; zero selects all.
   * \return False if primary values, refreshed properties, strong boundary data or residuals are invalid.
   */
  bool EvaluateResiduals(bool implicit = false, bool allEquations = false, double reconstruction = 1.0,
                         unsigned short residualRuntime = 0);
  /*! \brief Rebuild coefficients at the captured state; retain physical-time history and saved relaxation. */
  bool Restore();
  /*! \brief Advance reconstruction iteration history once, after accepting a flow update. */
  void CommitHistory();
  /*! \brief Collective maximum restoration error over dependent state, row metadata, history and relaxation. */
  double GetRestorationError();
};
