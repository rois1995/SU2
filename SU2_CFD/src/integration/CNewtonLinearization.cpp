/*!
 * \file CNewtonLinearization.cpp
 * \brief Matrix-free grouped derivatives retain all physical cross blocks.
 * \author SU2 Contributors
 */
#include "../../include/integration/CNewtonLinearization.hpp"

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
CNewtonPseudoTime::CNewtonPseudoTime(const CNewtonEquationSet& e, CGeometry* g, CConfig* c)
    : equations(e), geometry(g), config(c), diagonal(g->GetnPoint(), g->GetnPointDomain(), e.GetnVar(), 0.0) {
  if (c->GetnMarker_Periodic()) {
    periodicInput.Initialize(g->GetnPoint(), g->GetnPointDomain(), e.GetnVar(), 0.0);
    periodicProduct.Initialize(g->GetnPoint(), g->GetnPointDomain(), e.GetnVar(), 0.0);
  }
  for (const auto& block : equations.GetBlocks())
    if (block.slot == FLOW_SOL && (c->GetKind_Regime() == ENUM_REGIME::INCOMPRESSIBLE ||
                                  c->Low_Mach_Preconditioning() || c->GetKind_Upwind_Flow() == UPWIND::TURKEL)) {
      flowOffset = block.offset;
      flowVariables = block.variables;
      flowMatrix.resize(g->GetnPointDomain(), flowVariables * flowVariables) = 0.0;
    }
}

bool CNewtonPseudoTime::Build(const Vector& scale, const std::vector<unsigned char>& free) {
  assert(scale.GetNVar() == equations.GetnVar() && scale.GetNBlk() == geometry->GetnPoint());
  assert(free.size() == scale.GetLocSize());
  SU2_OMP_SAFE_GLOBAL_ACCESS(invalid = 0; scaling = &scale;)
  /*--- Freeze local partial-cell masses; the two periodic averages are applied in Apply, not in the volume. ---*/
  for (const auto& block : equations.GetBlocks()) {
    su2activematrix native(block.variables, block.variables);
    int localInvalid = 0;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      const double step = SU2_TYPE::GetValue(block.solver->GetNodes()->GetDelta_Time(p));
      const double volume = SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) +
          (config->GetnMarker_Periodic() ? su2double(0) : geometry->nodes->GetPeriodicVolume(p)));
      const double delta = step > 0 ? volume / step : 0;
      if (!(delta > 0) || !Armijo::IsFinite(delta)) localInvalid = 1;
      for (auto v = 0u; v < block.variables; ++v)
        diagonal(p, block.offset + v) = free[p * equations.GetnVar() + block.offset + v] ? delta : 0;
      if (block.slot != FLOW_SOL || !flowVariables) continue;
      block.solver->GetPseudoTimeMatrix(config, p, delta, native);
      for (auto i = 0u; i < block.variables; ++i)
        for (auto j = 0u; j < block.variables; ++j) {
          const auto row = block.offset + i, column = block.offset + j;
          const double value = SU2_TYPE::GetValue(native(i, j)) * scale(p, column) / scale(p, row);
          if (!Armijo::IsFinite(value)) localInvalid = 1;
          flowMatrix(p, i * block.variables + j) =
              free[p * equations.GetnVar() + row] && free[p * equations.GetnVar() + column] ? value : 0;
          if (i == j) diagonal(p, row) = flowMatrix(p, i * block.variables + j);
        }
    }
    END_SU2_OMP_FOR
    atomicMax(localInvalid, invalid);
  }
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    int global = 0;
    SU2_MPI::Allreduce(&invalid, &global, 1, MPI_INT, MPI_MAX, SU2_MPI::GetComm());
    invalid = global;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  return invalid == 0;
}

void CNewtonPseudoTime::Apply(const Vector& u, Vector& v, bool add) const {
  assert(u.GetNVar() == equations.GetnVar() && v.GetNVar() == equations.GetnVar());
  SU2_OMP_BARRIER
  const bool periodic = config->GetnMarker_Periodic();
  /*--- Project before and after the native mass: S^-1*P*D*P*S adds no mass to periodic consistency modes. ---*/
  if (periodic) {
    periodicInput = u;
    equations.ProjectPeriodic(periodicInput, scaling);
  }
  const auto& input = periodic ? periodicInput : u;
  auto& output = periodic ? periodicProduct : v;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
    for (auto i = 0u; i < equations.GetnVar(); ++i) {
      double value = diagonal(p, i) * input(p, i);
      if (i >= flowOffset && i < flowOffset + flowVariables) {
        value = 0;
        const auto row = i - flowOffset;
        for (auto j = 0u; j < flowVariables; ++j)
          value += flowMatrix(p, row * flowVariables + j) * input(p, flowOffset + j);
      }
      output(p, i) = (add && !periodic ? v(p, i) : 0.0) + value;
    }
  END_SU2_OMP_FOR
  if (periodic) {
    equations.ProjectPeriodic(periodicProduct, scaling);
    if (add) v += periodicProduct;
    else v = periodicProduct;
    SU2_OMP_BARRIER
  }
}

CNewtonLinearization::CNewtonLinearization(CNewtonEquationSet& e, CGeometry* g, CConfig* c)
    : equations(e),
      geometry(g),
      config(c),
      baseResidual(g->GetnPoint(), g->GetnPointDomain(), e.GetnVar(), 0.0),
      baseState(baseResidual),
      free(g->GetnPoint() * e.GetnVar(), true) {}

bool CNewtonLinearization::Apply(const Vector& u, Vector& v, bool pseudoTime, bool verifyStep,
                                 unsigned short residualRuntime) const {
  assert(scale && diagonal && u.GetNVar() == equations.GetnVar() && v.GetNVar() == equations.GetnVar());
  assert(&u != &v);
  const auto& blocks = equations.GetBlocks();
  const auto target = std::find_if(blocks.begin(), blocks.end(),
      [=](const CNewtonEquationSet::Block& b) { return b.runtime == residualRuntime; });
  assert(!residualRuntime || (!pseudoTime && !verifyStep && target != blocks.end()));
  const auto first = residualRuntime ? target->offset : 0;
  const auto last = residualRuntime ? first + target->variables : equations.GetnVar();
  const bool refine = refineStep && verifyStep;
  const auto flow = std::find_if(blocks.begin(), blocks.end(),
                                [](const CNewtonEquationSet::Block& block) { return block.slot == FLOW_SOL; });
  /*--- Size probes globally, retaining anisotropic state scales and representable changes at the base. ---*/
  double norm;
  if (physicalStep) {
    SU2_OMP_BARRIER
    v = u * (*scale);
    norm = v.norm();
  } else if (probeReference && flow != blocks.end() && config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE) {
    /*--- Use the same physical momentum reference for the absolute and relative limits.
     *     Reuse the output as scratch; the operator still differentiates along S*u. ---*/
    SU2_OMP_BARRIER
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto j = 0u; j < equations.GetnVar(); ++j) {
        v(p, j) = u(p, j);
        if (j > flow->offset && j <= flow->offset + geometry->GetnDim() &&
            free[p * equations.GetnVar() + j] && (*probeReference)(p, j) > 0)
          v(p, j) = u(p, j) * (*scale)(p, j) / (*probeReference)(p, j);
      }
    END_SU2_OMP_FOR
    norm = v.norm();
  } else
    norm = u.norm();
  if (norm == 0) {
    v = passivedouble(0);
    SU2_OMP_BARRIER
    return true;
  }
  double local = 0, resolution = 0;
  SU2_OMP_SAFE_GLOBAL_ACCESS(relative = 0; stepResolution = 0;)
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < u.GetNElmDomain(); ++i) {
    if (!free[i] || u[i] == 0) continue;
    const double change = u[i] * (*scale)[i];
    const double spacing = std::max(std::abs(std::nextafter(baseState[i], INFINITY) - baseState[i]),
                                    std::abs(std::nextafter(baseState[i], -INFINITY) - baseState[i]));
    resolution = std::max(resolution, std::abs(change) / spacing);
    /*--- Zero gauge pressure and zero species are valid coordinates; use their physical scale for relative probes. ---*/
    const auto reference =
        probeReference ? std::abs((*probeReference)[i]) : std::max(std::abs(baseState[i]), std::abs((*scale)[i]));
    if (reference > 0) local = std::max(local, std::abs(change) / reference);
  }
  END_SU2_OMP_FOR
  atomicMax(local, relative);
  atomicMax(resolution, stepResolution);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    const double localMax[] = {relative, stepResolution};
    double globalMax[2];
    SU2_MPI::Allreduce(localMax, globalMax, 2, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    relative = globalMax[0];
    stepResolution = globalMax[1];
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  double initialStep = absoluteStep / norm * (refine ? probeFactor : 1);
  if (relativeStep > 0 && relative > 0) initialStep = std::min(initialStep, relativeStep / relative);
  if (!(initialStep > 0) || !Armijo::IsFinite(initialStep)) {
    v = passivedouble(std::numeric_limits<double>::quiet_NaN());
    SU2_OMP_BARRIER
    return false;
  }
  /*--- Difference complete refreshed residuals, including all cross responses and explicit periodic constraints. ---*/
  auto probe = [&](double step) {
    bool shifted = false;
    if (!equations.FeasibleProbes(u, *scale, step, free, v, probeWork, shifted)) return false;
    auto isFree = [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short variable) {
      return free[p * equations.GetnVar() + b.offset + variable];
    };
    if (shifted) {
      equations.SetTrial(probeWork, step, *scale, isFree);
      if (!equations.EvaluateResiduals(false, false, reconstruction, residualRuntime)) return false;
      equations.CopyResidual(probeWork);
    }
    /*--- Directions are no longer needed after SetTrial, so their storage can hold residuals. ---*/
    equations.SetTrial(v, step, *scale, isFree);
    if (!equations.EvaluateResiduals(false, false, reconstruction, residualRuntime)) return false;
    equations.CopyResidual(v);
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto j = 0u; j < equations.GetnVar(); ++j) {
        const auto i = p * equations.GetnVar() + j;
        /*--- All input columns remain live; only unused output rows are omitted. ---*/
        if (j < first || j >= last)
          v[i] = 0.0;
        else
          v[i] = free[i] ? (v[i] - (shifted ? probeWork[i] : baseResidual[i])) / (step * (*scale)[i]) : u[i];
      }
    END_SU2_OMP_FOR
    return true;
  };
  auto differentiate = [&](double h) {
    bool sampled = false;
    for (auto attempt = 0u; attempt <= 16; ++attempt) {
      if (stepResolution > 0 && std::abs(h) * stepResolution < 1) break;
      if (probe(h)) { sampled = true; break; }
      if (probe(-h)) { h = -h; sampled = true; break; }
      h *= 0.5;
    }
    bool converged = false;
    if (refine && pseudoTime) {
      SU2_OMP_SAFE_GLOBAL_ACCESS(
          if (probeWork.GetLocSize() != u.GetLocSize())
            probeWork.Initialize(u.GetNBlk(), u.GetNBlkDomain(), u.GetNVar(), 0.0);)
    }
    for (unsigned refinement = 0; sampled && refinement <= (refine ? 16u : 0u); ++refinement) {
      /*--- A rounded-away perturbation cannot establish a zero derivative. ---*/
      if (stepResolution > 0 && std::abs(h) * stepResolution < 1) break;
      if (refinement && !probe(h)) break;
      SU2_OMP_FOR_STAT(1024)
      for (auto i = 0ul; i < u.GetNElmDomain(); ++i) {
        const double fine = v[i];
        if (refine) {
          v[i] = refinement ? 2 * fine - coarseProduct[i] : fine;
          coarseProduct[i] = fine;
          previousProduct[i] = v[i] - (refinement ? previousProduct[i] : 0.0);
        } else
          v[i] = fine;
      }
      END_SU2_OMP_FOR
      const double error = refine ? previousProduct.norm() : 0;
      double magnitude = refine && !pseudoTime ? v.norm() : 0;
      if (refine && pseudoTime) {
        /*--- Verify the operator being solved: near-null physical modes are resolved by the frozen mass action. ---*/
        if (nativeMass) {
          nativeMass->Apply(u, probeWork);
          probeWork += v;
        } else {
          SU2_OMP_FOR_STAT(1024)
          for (auto i = 0ul; i < u.GetNElmDomain(); ++i) probeWork[i] = v[i] + (*diagonal)[i] * u[i];
          END_SU2_OMP_FOR
        }
        magnitude = probeWork.norm();
      }
      if (!refine || (refinement > 1 && error <= 5e-4 * magnitude)) {
        converged = true;
        break;
      }
      /*--- Richardson extrapolation removes the leading forward-probe error without crossing physical bounds. ---*/
      previousProduct = v;
      SU2_OMP_BARRIER
      h *= 0.5;
    }
    return converged;
  };
  bool converged = differentiate(initialStep);
  if (!converged && refine && !physicalStep && probeFactor == 1 && flow != blocks.end() &&
      config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE) {
    /*--- Halving a cancellation-dominated probe cannot resolve its derivative. Try one coarser
     *     starting step, retaining feasibility, relative limits and the same verification tolerance. ---*/
    double coarserStep = 100 * initialStep;
    if (relativeStep > 0 && relative > 0) coarserStep = std::min(coarserStep, relativeStep / relative);
    if (coarserStep > initialStep && Armijo::IsFinite(coarserStep)) {
      converged = differentiate(coarserStep);
      /*--- Reuse only the starting scale for this base; every product is verified and SetBase resets it. ---*/
      if (converged) {
        SU2_OMP_SAFE_GLOBAL_ACCESS(probeFactor = 100;)
      }
    }
  }
  if (!converged) {
    v = passivedouble(std::numeric_limits<double>::quiet_NaN());
    SU2_OMP_BARRIER
    return false;
  }
  if (pseudoTime && nativeMass)
    nativeMass->Apply(u, v, true);
  else if (pseudoTime) {
    SU2_OMP_FOR_STAT(1024)
    for (auto i = 0ul; i < u.GetNElmDomain(); ++i) v[i] += (*diagonal)[i] * u[i];
    END_SU2_OMP_FOR
  }
  CSysMatrixComms::Initiate(v, geometry, config);
  CSysMatrixComms::Complete(v, geometry, config);
  return true;
}

CNewtonBlockPreconditioner::CNewtonBlockPreconditioner(const CNewtonLinearization& l, CGeometry* geometry,
                                                       CConfig* config, bool lower, unsigned iterations,
                                                       double tolerance)
    : linearization(l),
      lowerTriangular(lower),
      innerIterations(iterations),
      innerTolerance(tolerance),
      geometry(geometry),
      config(config) {
  const auto kind = static_cast<ENUM_LINEAR_SOLVER_PREC>(config->GetKind_Linear_Solver_Prec());
  blocks.reserve(l.GetBlocks().size());
  for (const auto& b : l.GetBlocks()) {
    blocks.emplace_back();
    auto& block = blocks.back();
    block.runtime = b.runtime;
    block.offset = b.offset;
    block.solver = b.solver;
    block.variables = b.variables;
    if (b.solver->Jacobian.HasCoefficients()) {
      block.linearSolver = std::make_unique<CSysSolve<Mixed>>();
      block.linearSolver->SetxIsZero(true);
      block.preconditioner.reset(CPreconditioner<Mixed>::Create(kind, b.solver->Jacobian, geometry, config));
    }
    block.input.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), b.variables, 0.0);
    block.output.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), b.variables, 0.0);
  }
  if (lower) coupling.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), l.GetnVar(), 0.0);
  if (config->GetnMarker_Periodic())
    periodicRHS.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), l.GetnVar(), 0.0);
}

void CNewtonBlockPreconditioner::Build() {
  SU2_OMP_SAFE_GLOBAL_ACCESS(work = 0;)
  for (auto& block : blocks) {
    if (linearization.UsesNativeMass() && block.solver->Jacobian.HasCoefficients()) {
      /*--- Native pseudo-time assembly can reintroduce couplings into a prescribed row. ---*/
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
        for (auto v = 0u; v < block.variables; ++v)
          if (!linearization.IsFree(p, block.offset + v)) block.solver->Jacobian.DeleteValsRowi(p, v);
      END_SU2_OMP_FOR
    }
    if (block.preconditioner) block.preconditioner->Build();
  }
}

unsigned long CNewtonBlockPreconditioner::Solve(const Vector& u, Vector& v, unsigned iterations,
                                                double tolerance) const {
  /*--- Vector producers use nowait with a different partition than the native block loops. ---*/
  SU2_OMP_BARRIER
  const auto& scale = linearization.GetScale();
  const bool periodic = config->GetnMarker_Periodic();
  if (periodic) {
    /*--- Native partial-cell solves approximate physical modes; keep consistency modes for the identity below. ---*/
    periodicRHS = u;
    linearization.ProjectPeriodic(periodicRHS);
  }
  const auto& rightHandSide = periodic ? periodicRHS : u;
  unsigned long total = 0;
  v = passivedouble(0);
  for (auto index = 0u; index < blocks.size(); ++index) {
    const auto& block = blocks[index];
    /*--- Lower coupling is approximate; the outer operator and true defect use verified physical products. ---*/
    if (lowerTriangular && index && !linearization.Apply(v, coupling, false, false, block.runtime)) {
      v = passivedouble(std::numeric_limits<double>::quiet_NaN());
      SU2_OMP_BARRIER
      SU2_OMP_SAFE_GLOBAL_ACCESS(work += total;)
      return total;
    }
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j) {
        const auto column = block.offset + j;
        const double cross = lowerTriangular && index ? coupling(p, column) : 0;
        block.input(p, j) = (rightHandSide(p, column) - cross) * scale(p, column);
      }
    END_SU2_OMP_FOR
    const auto inner = config->GetKind_Linear_Solver_Inner();
    if (!block.solver->Jacobian.HasCoefficients()) {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < u.GetNBlk(); ++p) {
        const double delta = p < u.GetNBlkDomain()
                                 ? SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
                                       SU2_TYPE::GetValue(block.solver->GetNodes()->GetDelta_Time(p))
                                 : 1.0;
        for (auto j = 0u; j < block.variables; ++j)
          block.output(p, j) = block.input(p, j) /
                               (linearization.IsFree(p, block.offset + j) ? delta : 1.0);
      }
      END_SU2_OMP_FOR
    } else if (!block.preconditioner)
      block.output = block.input;
    else if (!iterations || (iterations == 1 && inner == LINEAR_SOLVER_INNER::SMOOTHER))
      (*block.preconditioner)(block.input, block.output);
    else {
      CSysMatrixVectorProduct<Mixed> matrix(block.solver->Jacobian, geometry, config);
      block.output = Mixed(0);
      Mixed residual = tolerance;
      switch (inner) {
        case LINEAR_SOLVER_INNER::NONE:
          total += block.linearSolver->FGMRES_LinSolver(block.input, block.output, matrix, *block.preconditioner,
                                                        tolerance, iterations, residual, false, config);
          break;
        case LINEAR_SOLVER_INNER::BCGSTAB:
          total += block.linearSolver->BCGSTAB_LinSolver(block.input, block.output, matrix, *block.preconditioner,
                                                         tolerance, iterations, residual, false, config);
          break;
        case LINEAR_SOLVER_INNER::SMOOTHER:
          total += block.linearSolver->Smoother_LinSolver(block.input, block.output, matrix, *block.preconditioner, 0,
                                                          iterations, residual, false, config);
          break;
      }
    }
    SU2_OMP_BARRIER
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j)
        v(p, block.offset + j) = SU2_TYPE::GetValue(block.output(p, j)) / scale(p, block.offset + j);
    END_SU2_OMP_FOR
    /*--- Lower coupling must consume the consistent corrections of the preceding blocks. ---*/
    if (periodic && lowerTriangular && index + 1 < blocks.size()) linearization.ProjectPeriodic(v);
  }
  if (periodic) {
    /*--- Apply S^-1*(P*M^-1*P+(I-P))*S, retaining all arbitrary input directions. ---*/
    linearization.ProjectPeriodic(v);
    v += u - periodicRHS;
    SU2_OMP_BARRIER
  }
  SU2_OMP_SAFE_GLOBAL_ACCESS(work += total;)
  return total;
}

void CNewtonBlockPreconditioner::Multiply(const Vector& u, Vector& v) const {
  SU2_OMP_BARRIER
  const auto& scale = linearization.GetScale();
  for (const auto& block : blocks) {
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j)
        block.input(p, j) = u(p, block.offset + j) * scale(p, block.offset + j);
    END_SU2_OMP_FOR
    if (block.solver->Jacobian.HasCoefficients()) {
      CSysMatrixVectorProduct<Mixed> matrix(block.solver->Jacobian, geometry, config);
      matrix(block.input, block.output);
    } else {
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < u.GetNBlk(); ++p) {
        const double delta = p < u.GetNBlkDomain()
                                 ? SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
                                       SU2_TYPE::GetValue(block.solver->GetNodes()->GetDelta_Time(p))
                                 : 1.0;
        for (auto j = 0u; j < block.variables; ++j)
          block.output(p, j) = block.input(p, j) *
                               (linearization.IsFree(p, block.offset + j) ? delta : 1.0);
      }
      END_SU2_OMP_FOR
    }
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < u.GetNBlk(); ++p)
      for (auto j = 0u; j < block.variables; ++j)
        v(p, block.offset + j) = SU2_TYPE::GetValue(block.output(p, j)) / scale(p, block.offset + j);
    END_SU2_OMP_FOR
  }
}

#endif
