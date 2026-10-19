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
/*!
 * \class CNewtonPseudoTime
 * \brief Frozen native pseudo-time action with prescribed coordinates eliminated.
 * \details Apply S^-1*D*S, or S^-1*P*D*P*S for periodic groups. D contains native local partial-cell
 * masses, including dense flow preconditioning; P averages periodic copies with momentum rotations.
 * Build and Apply are collective over the current OpenMP team. The scale must outlive the built action.
 */
class CNewtonPseudoTime final {
 public:
  using Vector = CSysVector<passivedouble>;

 private:
  const CNewtonEquationSet& equations;
  CGeometry* geometry;
  CConfig* config;
  Vector diagonal;
  const Vector* scaling = nullptr;
  mutable Vector periodicInput, periodicProduct;
  su2passivematrix flowMatrix;
  unsigned short flowOffset = 0, flowVariables = 0;
  int invalid = 0;

 public:
  CNewtonPseudoTime(const CNewtonEquationSet&, CGeometry*, CConfig*);
  /*! \brief Freeze native masses at the base time steps; return false for invalid entries. */
  bool Build(const Vector& scale, const std::vector<unsigned char>& free);
  /*! \brief Assign the mass action to the output, or accumulate it when add is true. */
  void Apply(const Vector&, Vector&, bool add = false) const;
  const Vector& GetDiagonal() const { return diagonal; }
};

/*!
 * \class CNewtonLinearization
 * \brief Matrix-free A = S^-1*J_F*S + frozen pseudo-time action in fixed group coordinates q=M_base*w.
 * \details J_F differentiates the full refreshed residual, including cross-equation and periodic consistency
 * terms. SetBase refreshes an explicit residual while preserving native implicit matrices. Positive finite
 * scales and pseudo-time data must remain alive and fixed until the next SetBase. Methods are OpenMP collective.
 */
class CNewtonLinearization final : public CMatrixVectorProduct<passivedouble> {
 public:
  using Vector = CSysVector<passivedouble>;
  /*! \brief Distinguish invalid directions, infeasible probes and unresolved derivatives. */
  enum class Failure { None, Direction, Admissibility, Verification };

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
  mutable double relative = 0, stepResolution = 0, probeFactor = 1;
  mutable bool centralFirst = false; /*!< \brief This base already needed central differences. */

 public:
  CNewtonLinearization(CNewtonEquationSet&, CGeometry*, CConfig*);
  /*!
   * \brief Bind base scales, probe controls and free coordinates before applying the operator.
   * \details relativeReference controls probe size independently of the state scaling. physicalMetric retains
   * legacy flow probe sizing; verifyStep requests refinement. mass supplies the native dense/projected action.
   * The optional relaxed reconstruction belongs only to a direction model; acceptance uses the physical operator.
   * \return False when the explicit base residual cannot be evaluated.
   */
  template <class IsFree>
  bool SetBase(const Vector& scaling, const Vector& pseudoTime, double step, double relativeLimit, IsFree isFree,
               const Vector* relativeReference = nullptr, double reconstructionFactor = 1.0,
               bool physicalMetric = false, bool verifyStep = false, const CNewtonPseudoTime* mass = nullptr) {
    assert(scaling.GetNVar() == equations.GetnVar() && pseudoTime.GetNVar() == equations.GetnVar());
    assert(scaling.GetNBlk() == geometry->GetnPoint() && pseudoTime.GetNBlk() == geometry->GetnPoint());
    assert(step > 0 && relativeLimit >= 0);
    assert(reconstructionFactor >= 0 && reconstructionFactor <= 1);
    assert(!relativeReference || relativeReference->GetLocSize() == scaling.GetLocSize());
    /*--- Normalized flame gradients can be sensitive even with first-order reconstruction. ---*/
    verifyStep = verifyStep || (config->GetKind_Species_Model() == SPECIES_MODEL::FLAMELET &&
        config->GetFlameletParsedOptions().thickenedflame_correction &&
        std::any_of(equations.GetBlocks().begin(), equations.GetBlocks().end(),
                    [](const CNewtonEquationSet::Block& block) { return block.slot == SPECIES_SOL; }));
    SU2_OMP_SAFE_GLOBAL_ACCESS(scale = &scaling; diagonal = &pseudoTime; probeReference = relativeReference;
                               absoluteStep = step; relativeStep = relativeLimit; reconstruction = reconstructionFactor;
                               physicalStep = physicalMetric; refineStep = verifyStep; probeFactor = 1;
                               centralFirst = false;
                               nativeMass = mass;
                               if (verifyStep && coarseProduct.GetLocSize() != scaling.GetLocSize()) {
                                 coarseProduct.Initialize(scaling.GetNBlk(), scaling.GetNBlkDomain(), scaling.GetNVar(), 0.0);
                                 previousProduct.Initialize(scaling.GetNBlk(), scaling.GetNBlkDomain(), scaling.GetNVar(), 0.0);
                               })
    /*--- Use the same residual path for the reference and probes, avoiding implicit/explicit roundoff offsets. ---*/
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
  /*!
   * \brief Differentiate the full physical residual along the supplied scaled direction.
   * \details Refinement measures relative error against the requested operator, J_F or J_F plus frozen mass.
   * \param[in] pseudoTime - Include the frozen pseudo-time action when true.
   * \param[in] verifyStep - Permit the configured probe refinement; only approximate preconditioner coupling omits it.
   * \param[in] residualRuntime - Optional physical row for unverified preconditioner coupling; other rows return zero.
   * \param[out] failure - Optional thread-local diagnostic; probe and verification controls are unchanged.
   * \return False, with a nonfinite output, if no feasible, resolved derivative can be obtained.
   */
  bool Apply(const Vector&, Vector&, bool pseudoTime = true, bool verifyStep = true,
             unsigned short residualRuntime = 0, Failure* failure = nullptr) const;
  void operator()(const Vector& u, Vector& v) const override { Apply(u, v); }
  const Vector& GetScale() const {
    assert(scale);
    return *scale;
  }
  /*! \brief Apply S^-1*P*S to a preconditioner vector; full derivative directions remain unprojected. */
  void ProjectPeriodic(Vector& vector) const {
    assert(scale);
    equations.ProjectPeriodic(vector, scale);
  }
  unsigned short GetnVar() const { return equations.GetnVar(); }
  bool UsesNativeMass() const { return nativeMass != nullptr; }
  bool IsFree(unsigned long p, unsigned short v) const { return free[p * equations.GetnVar() + v]; }
  const std::vector<CNewtonEquationSet::Block>& GetBlocks() const { return equations.GetBlocks(); }
};

/*!
 * \class CNewtonBlockPreconditioner
 * \brief Native block solves, optionally subtracting matrix-free lower coupling actions.
 * \details Each matrix retains its native block size; the full group never enters CSysMatrix scratch buffers.
 * Native matrices approximate the full operator. In particular, periodic P*A*P+(I-P) is a preconditioner,
 * while arbitrary trial directions remain present in the residual derivative used for acceptance.
 * Its approximate inverse preserves this structure as P*M^-1*P+(I-P), with scaling undone around P.
 */
class CNewtonBlockPreconditioner final : public CPreconditioner<passivedouble> {
  using Mixed = su2mixedfloat;
  using Vector = CNewtonLinearization::Vector;
  struct Block {
    unsigned short runtime, offset, variables;
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
  mutable unsigned failedProducts = 0;
  mutable CNewtonLinearization::Failure failure = CNewtonLinearization::Failure::None;
  mutable double rhsMagnitude = 0;
  std::vector<Block> blocks;
  mutable Vector coupling, periodicRHS;

 public:
  CNewtonBlockPreconditioner(const CNewtonLinearization&, CGeometry*, CConfig*, bool lower = false,
                             unsigned innerIterations = 0, double innerTolerance = 0.1);
  void Build() override;
  void operator()(const Vector& u, Vector& v) const override { Solve(u, v, innerIterations, innerTolerance); }
  /*! \brief Apply block solves; native block-diagonal startup disables cross-equation products. */
  unsigned long Solve(const Vector&, Vector&, unsigned iterations, double tolerance, bool coupling = true) const;
  /*! \brief Apply the native block matrices for a legacy startup direction model. */
  void Multiply(const Vector&, Vector&) const;
  unsigned long GetWork() const { return work; }
  unsigned GetFailedProducts() const { return failedProducts; }
  CNewtonLinearization::Failure GetFailure() const { return failure; }
};
#endif
