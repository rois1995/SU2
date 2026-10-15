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
    unsigned short variables;
    std::vector<su2double> solution, oldSolution, evaluationHistory, relaxation;
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

  void RestoreSolutions();
  void SaveHistory();
  void RestoreHistory();
  void Communicate();
  void Refresh();
  bool CheckState(bool compareFlow);

 public:
  CNewtonEquationSet(CGeometry*, CSolver**, CNumerics***, CConfig*, const std::vector<unsigned short>& runtimes);
  const std::vector<Block>& GetBlocks() const { return blocks; }
  unsigned short GetnVar() const { return variables; }
  const std::array<unsigned long, 4>& GetResidualEvaluations() const { return residualEvaluations; }
  /*! \brief Capture an accepted base after its residual/property refresh, fixing the scalar density factors. */
  void Capture();

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
  bool EvaluateResiduals(bool implicit = false);
  /*! \brief Rebuild coefficients at the captured state; retain physical-time history and saved relaxation. */
  bool Restore();
};
