/*!
 * \file CNewtonLinearization.cpp
 * \brief Matrix-free grouped derivatives retain all physical cross blocks.
 * \author SU2 Contributors
 */
#include "../../include/integration/CNewtonLinearization.hpp"

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
CNewtonLinearization::CNewtonLinearization(CNewtonEquationSet& e, CGeometry* g, CConfig* c)
    : equations(e), geometry(g), config(c),
      baseResidual(g->GetnPoint(), g->GetnPointDomain(), e.GetnVar(), 0.0), baseState(baseResidual),
      free(g->GetnPoint() * e.GetnVar(), true) {}

bool CNewtonLinearization::Apply(const Vector& u, Vector& v, bool pseudoTime) const {
  assert(scale && diagonal && u.GetNVar() == equations.GetnVar() && v.GetNVar() == equations.GetnVar());
  const auto norm = u.norm();
  if (norm == 0) {
    v = passivedouble(0);
    SU2_OMP_BARRIER
    return true;
  }
  double local = 0;
  SU2_OMP_SAFE_GLOBAL_ACCESS(relative = 0;)
  if (relativeStep > 0) {
    SU2_OMP_FOR_STAT(1024)
    for (auto i = 0ul; i < u.GetNElmDomain(); ++i) {
      if (!free[i]) continue;
      // Zero gauge pressure and zero species are valid coordinates; use their physical scale for relative probes.
      const auto reference = probeReference ? std::abs((*probeReference)[i])
                                           : std::max(std::abs(baseState[i]), std::abs((*scale)[i]));
      if (reference > 0) local = std::max(local, std::abs(u[i] * (*scale)[i]) / reference);
    }
    END_SU2_OMP_FOR
    atomicMax(local, relative);
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      double global = 0;
      SU2_MPI::Allreduce(&relative, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      relative = global;
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS
  }
  double h = absoluteStep / norm;
  if (relative > 0) h = std::min(h, relativeStep / relative);
  if (!(h > 0) || !Armijo::IsFinite(h)) {
    v = passivedouble(std::numeric_limits<double>::quiet_NaN());
    SU2_OMP_BARRIER
    return false;
  }
  equations.SetTrial(u, h, *scale, [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short variable) {
    return free[p * equations.GetnVar() + b.offset + variable];
  });
  if (!equations.EvaluateResiduals()) {
    v = passivedouble(std::numeric_limits<double>::quiet_NaN());
    SU2_OMP_BARRIER
    return false;
  }
  equations.CopyResidual(v);
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < u.GetNElmDomain(); ++i) {
    v[i] = free[i] ? (v[i] - baseResidual[i]) / (h * (*scale)[i]) : u[i];
    if (pseudoTime) v[i] += (*diagonal)[i] * u[i];
  }
  END_SU2_OMP_FOR
  CSysMatrixComms::Initiate(v, geometry, config);
  CSysMatrixComms::Complete(v, geometry, config);
  return true;
}

CNewtonBlockPreconditioner::CNewtonBlockPreconditioner(const CNewtonLinearization& l, CGeometry* geometry,
                                                       CConfig* config, bool lower)
    : linearization(l), lowerTriangular(lower) {
  const auto kind = static_cast<ENUM_LINEAR_SOLVER_PREC>(config->GetKind_Linear_Solver_Prec());
  blocks.reserve(l.GetBlocks().size());
  for (const auto& b : l.GetBlocks()) {
    blocks.emplace_back();
    auto& block = blocks.back();
    block.offset = b.offset;
    block.variables = b.variables;
    block.preconditioner.reset(CPreconditioner<Mixed>::Create(kind, b.solver->Jacobian, geometry, config));
    block.input.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), b.variables, 0.0);
    block.output.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), b.variables, 0.0);
  }
  if (lower) coupling.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), l.GetnVar(), 0.0);
}

void CNewtonBlockPreconditioner::Build() {
  for (auto& block : blocks)
    if (block.preconditioner) block.preconditioner->Build();
}

void CNewtonBlockPreconditioner::operator()(const Vector& u, Vector& v) const {
  const auto& scale = linearization.GetScale();
  v = passivedouble(0);
  for (auto index = 0u; index < blocks.size(); ++index) {
    const auto& block = blocks[index];
    if (lowerTriangular && index && !linearization.Apply(v, coupling, false)) {
      v = passivedouble(std::numeric_limits<double>::quiet_NaN());
      SU2_OMP_BARRIER
      return;
    }
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j) {
        const auto column = block.offset + j;
        const double cross = lowerTriangular && index ? coupling(p, column) : 0;
        block.input(p, j) = (u(p, column) - cross) * scale(p, column);
      }
    END_SU2_OMP_FOR
    if (block.preconditioner)
      (*block.preconditioner)(block.input, block.output);
    else
      block.output = block.input;
    SU2_OMP_BARRIER
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j)
        v(p, block.offset + j) = SU2_TYPE::GetValue(block.output(p, j)) / scale(p, block.offset + j);
    END_SU2_OMP_FOR
  }
}
#endif
