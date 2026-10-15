/*!
 * \file CNewtonLinearization.hpp
 * \brief Scaled grouped Newton products and native block preconditioning.
 * \author SU2 Contributors
 */
#pragma once

#include "CNewtonEquationSet.hpp"
#include "../../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"
#include "../../../Common/include/linear_algebra/CPreconditioner.hpp"
#include <memory>

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
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
  Vector baseResidual, baseState;
  std::vector<unsigned char> free;
  double absoluteStep = 0, relativeStep = 0;
  mutable double relative = 0;

 public:
  CNewtonLinearization(CNewtonEquationSet&, CGeometry*, CConfig*);
  template <class IsFree>
  bool SetBase(const Vector& scaling, const Vector& pseudoTime, double step, double relativeLimit, IsFree isFree,
               const Vector* relativeReference = nullptr) {
    assert(scaling.GetNVar() == equations.GetnVar() && pseudoTime.GetNVar() == equations.GetnVar());
    assert(scaling.GetNBlk() == geometry->GetnPoint() && pseudoTime.GetNBlk() == geometry->GetnPoint());
    assert(step > 0 && relativeLimit >= 0);
    assert(!relativeReference || relativeReference->GetLocSize() == scaling.GetLocSize());
    SU2_OMP_SAFE_GLOBAL_ACCESS(scale = &scaling; diagonal = &pseudoTime;
                               probeReference = relativeReference; absoluteStep = step; relativeStep = relativeLimit;)
    // Use the same residual path for the reference and probes, avoiding implicit/explicit roundoff offsets.
    if (!equations.EvaluateResiduals()) return false;
    equations.Pack(baseState);
    equations.CopyResidual(baseResidual);
    for (const auto& block : equations.GetBlocks()) {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          free[p * equations.GetnVar() + block.offset + v] = isFree(block, p, v);
      END_SU2_OMP_FOR
    }
    return true;
  }
  bool SetBase(const Vector& scaling, const Vector& pseudoTime, double step, double relativeLimit) {
    return SetBase(scaling, pseudoTime, step, relativeLimit,
            [](const CNewtonEquationSet::Block&, unsigned long, unsigned short) { return true; });
  }
  /*! \brief False produces the scaled physical derivative, without the pseudo-time diagonal. */
  bool Apply(const Vector&, Vector&, bool pseudoTime = true) const;
  void operator()(const Vector& u, Vector& v) const override { Apply(u, v); }
  const Vector& GetScale() const { assert(scale); return *scale; }
  unsigned short GetnVar() const { return equations.GetnVar(); }
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
    std::unique_ptr<CPreconditioner<Mixed>> preconditioner;
    mutable CSysVector<Mixed> input, output;
  };
  const CNewtonLinearization& linearization;
  bool lowerTriangular;
  std::vector<Block> blocks;
  mutable Vector coupling;

 public:
  CNewtonBlockPreconditioner(const CNewtonLinearization&, CGeometry*, CConfig*, bool lower = false);
  void Build() override;
  void operator()(const Vector&, Vector&) const override;
};
#endif
