/*!
 * \file CNewtonLinearization.hpp
 * \brief Scaled grouped Newton products and native block preconditioning.
 * \author SU2 Contributors
 */
#pragma once

#include "CNewtonEquationSet.hpp"
#include "../../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"
#include "../../../Common/include/linear_algebra/CPreconditioner.hpp"
#include "../../../Common/include/linear_algebra/CSysSolve.hpp"
#include <memory>

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
/*! \brief Frozen native pseudo-time action in scaled group coordinates, with prescribed coordinates eliminated. */
class CNewtonPseudoTime final {
 public:
  using Vector = CSysVector<passivedouble>;

 private:
  const CNewtonEquationSet& equations;
  CGeometry* geometry;
  CConfig* config;
  Vector diagonal;
  su2passivematrix flowMatrix;
  unsigned short flowOffset = 0, flowVariables = 0;
  int invalid = 0;

 public:
  CNewtonPseudoTime(const CNewtonEquationSet&, CGeometry*, CConfig*);
  bool Build(const Vector& scale, const std::vector<unsigned char>& free);
  void Apply(const Vector&, Vector&, bool add = false) const;
  const Vector& GetDiagonal() const { return diagonal; }
};

/*! \brief A = S^-1 J_R S + D in the equation set's fixed coordinates.
 * SetBase refreshes an explicit residual at the captured base, preserving the native implicit matrices.
 * Positive finite scale and the diagonal remain alive and fixed until the next SetBase. Methods are OpenMP collective.
 */
class CNewtonLinearization final : public CMatrixVectorProduct<passivedouble> {
 public:
  using Vector = CSysVector<passivedouble>;

 private:
  CNewtonEquationSet& equations;
  CGeometry* geometry;
  CConfig* config;
  const Vector *scale = nullptr, *diagonal = nullptr, *probeReference = nullptr;
  const CNewtonPseudoTime* nativeMass = nullptr;
  Vector baseResidual, baseState;
  mutable Vector coarseProduct, previousProduct, probeWork;
  std::vector<unsigned char> free;
  double absoluteStep = 0, relativeStep = 0, reconstruction = 1;
  bool physicalStep = false, refineStep = false;
  mutable double relative = 0, stepResolution = 0;

 public:
  CNewtonLinearization(CNewtonEquationSet&, CGeometry*, CConfig*);
  template <class IsFree>
  bool SetBase(const Vector& scaling, const Vector& pseudoTime, double step, double relativeLimit, IsFree isFree,
               const Vector* relativeReference = nullptr, double reconstructionFactor = 1.0,
               bool physicalMetric = false, bool verifyStep = false, const CNewtonPseudoTime* mass = nullptr) {
    assert(scaling.GetNVar() == equations.GetnVar() && pseudoTime.GetNVar() == equations.GetnVar());
    assert(scaling.GetNBlk() == geometry->GetnPoint() && pseudoTime.GetNBlk() == geometry->GetnPoint());
    assert(step > 0 && relativeLimit >= 0);
    assert(reconstructionFactor >= 0 && reconstructionFactor <= 1);
    assert(!relativeReference || relativeReference->GetLocSize() == scaling.GetLocSize());
    // Normalized flame gradients can be sensitive even with first-order reconstruction.
    verifyStep = verifyStep || (config->GetKind_Species_Model() == SPECIES_MODEL::FLAMELET &&
        config->GetFlameletParsedOptions().thickenedflame_correction &&
        std::any_of(equations.GetBlocks().begin(), equations.GetBlocks().end(),
                    [](const CNewtonEquationSet::Block& block) { return block.slot == SPECIES_SOL; }));
    SU2_OMP_SAFE_GLOBAL_ACCESS(scale = &scaling; diagonal = &pseudoTime; probeReference = relativeReference;
                               absoluteStep = step; relativeStep = relativeLimit; reconstruction = reconstructionFactor;
                               physicalStep = physicalMetric; refineStep = verifyStep;
                               nativeMass = mass;
                               if (verifyStep && coarseProduct.GetLocSize() != scaling.GetLocSize()) {
                                 coarseProduct.Initialize(scaling.GetNBlk(), scaling.GetNBlkDomain(), scaling.GetNVar(), 0.0);
                                 previousProduct.Initialize(scaling.GetNBlk(), scaling.GetNBlkDomain(), scaling.GetNVar(), 0.0);
                               })
    // Use the same residual path for the reference and probes, avoiding implicit/explicit roundoff offsets.
    if (!equations.EvaluateResiduals(false, false, reconstruction)) return false;
    equations.Pack(baseState);
    equations.CopyResidual(baseResidual);
    for (const auto& block : equations.GetBlocks()) {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          free[p * equations.GetnVar() + block.offset + v] =
              isFree(block, p, v) && !block.solver->Jacobian.IsPrescribedRow(p, v);
      END_SU2_OMP_FOR
    }
    return true;
  }
  bool SetBase(const Vector& scaling, const Vector& pseudoTime, double step, double relativeLimit) {
    return SetBase(scaling, pseudoTime, step, relativeLimit,
                   [](const CNewtonEquationSet::Block&, unsigned long, unsigned short) { return true; });
  }
  /*! \brief False produces the scaled physical derivative, without the pseudo-time diagonal.
   * Preconditioner coupling can omit step verification; operator and defect products retain it.
   */
  bool Apply(const Vector&, Vector&, bool pseudoTime = true, bool verifyStep = true) const;
  void operator()(const Vector& u, Vector& v) const override { Apply(u, v); }
  const Vector& GetScale() const {
    assert(scale);
    return *scale;
  }
  unsigned short GetnVar() const { return equations.GetnVar(); }
  bool UsesNativeMass() const { return nativeMass != nullptr; }
  bool IsFree(unsigned long p, unsigned short v) const { return free[p * equations.GetnVar() + v]; }
  const std::vector<CNewtonEquationSet::Block>& GetBlocks() const { return equations.GetBlocks(); }
};

/*! \brief Native diagonal solves, optionally subtracting matrix-free lower coupling actions.
 * Each matrix retains its native block size; the full group never enters CSysMatrix scratch buffers.
 */
class CNewtonBlockPreconditioner final : public CPreconditioner<passivedouble> {
  using Mixed = su2mixedfloat;
  using Vector = CNewtonLinearization::Vector;
  struct Block {
    unsigned short offset, variables;
    CSolver* solver;
    std::unique_ptr<CSysSolve<Mixed>> linearSolver;
    std::unique_ptr<CPreconditioner<Mixed>> preconditioner;
    mutable CSysVector<Mixed> input, output;
  };
  const CNewtonLinearization& linearization;
  bool lowerTriangular;
  unsigned innerIterations;
  double innerTolerance;
  CGeometry* geometry;
  CConfig* config;
  mutable unsigned long work = 0;
  std::vector<Block> blocks;
  mutable Vector coupling;

 public:
  CNewtonBlockPreconditioner(const CNewtonLinearization&, CGeometry*, CConfig*, bool lower = false,
                             unsigned innerIterations = 0, double innerTolerance = 0.1);
  void Build() override;
  void operator()(const Vector& u, Vector& v) const override { Solve(u, v, innerIterations, innerTolerance); }
  unsigned long Solve(const Vector&, Vector&, unsigned iterations, double tolerance) const;
  void Multiply(const Vector&, Vector&) const;
  unsigned long GetWork() const { return work; }
};
#endif
