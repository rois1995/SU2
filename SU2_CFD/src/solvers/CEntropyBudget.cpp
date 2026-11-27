/*!
 * \file CEntropyBudget.cpp
 * \brief Read-only diagnostic of the spatial JST entropy balance.
 * \copyright Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * \license GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/solvers/CEntropyBudget.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/variables/CEulerVariable.hpp"
#include "../../include/numerics/flow/p1_viscous_entropy.hpp"
#include "../../../Common/include/toolboxes/boundary_closure.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace ebt = EntropyBudgetTerms;

CEntropyBudget::CEntropyBudget(const CGeometry& geometry, const CConfig& config, const CEulerVariable& nodes,
                               const CSysVector<su2double>& residual)
    : geometry(geometry), config(config), nodes(nodes), residual(residual),
      dim(geometry.GetnDim()), nVar(dim + 2), owned(geometry.GetnPointDomain()), p1(config.GetEdge_Form_P1()),
      nodal(geometry.GetnPoint()), previous(owned), convection(owned), reconstructed(owned), beta(owned),
      normals(owned), volume(owned) {}

void CEntropyBudget::BeginIteration() {
  terms.fill(0);
  sums.fill(0);
  maxima.fill(0);
  maxima[WALL_RATIO] = -std::numeric_limits<passivedouble>::max();
  counts.fill(0);
  stage = CONV;
  for (auto* rows : {&previous, &convection, &reconstructed}) std::fill(rows->begin(), rows->end(), State{});
  std::fill(normals.begin(), normals.end(), Normal{});
  std::fill(volume.begin(), volume.end(), std::array<double, VOLUME_COUNT>{});

  // eq:complete: use the same current (including halo) primitives that the residual reads.
  for (auto point = 0ul; point < nodal.size(); ++point) {
    P1BoundaryClosure::EsState state;
    state.rho = nodes.GetDensity(point);
    state.p = nodes.GetPressure(point);
    for (unsigned d = 0; d < dim; ++d) state.vel[d] = nodes.GetVelocity(point, d);
    auto& row = nodal[point];
    row.w = ebt::Variables(dim, config.GetGamma(), state);
    row.psi = ebt::Potential(dim, state);
    for (unsigned k = 0; k < nVar; ++k) {
      row.u[k] = ebt::Value(nodes.GetSolution(point, k));
      row.lapl[k] = ebt::Value(nodes.GetUndivided_Laplacian(point, k));
    }
    row.ut[0] = ebt::Value(state.rho);
    std::copy_n(row.psi.begin(), dim, row.ut.begin() + 1);
    row.ut[nVar - 1] = ebt::Value(state.rho * nodes.GetEnthalpy(point));
  }

  pairs.assign(geometry.edges->GetSplitNodes().size(), PairSample{});
  walls.clear();
  if (first) {
    PrepareBoundaryShares();
    if (config.GetVisc_P1_Entropy()) {
      elements.resize(geometry.GetnElem());
      for (auto elem = 0ul; elem < elements.size(); ++elem) {
        const auto& cell = *geometry.elem[elem];
        elements[elem].resize(cell.GetnNodes() == dim + 1 ? 1 : geometry.GetMixedSimplices(cell.GetVTK_Type()).size());
      }
    }
  }
  for (auto& marker : farViscous) std::fill(marker.begin(), marker.end(), State{});
}

void CEntropyBudget::BeginStage(Term next) {
  CloseStage();
  stage = next;
}

void CEntropyBudget::CloseStage() {
  for (auto point = 0ul; point < owned; ++point) {
    SnapshotRow(point, stage);
    if (stage == CONV) convection[point] = previous[point];
  }
}

// eq:production: SU2 assembles R, with V du/dt = -R.
void CEntropyBudget::SnapshotRow(unsigned long point, Term term, bool strongWall) {
  for (unsigned k = 0; k < nVar; ++k) {
    const double current = ebt::Value(residual(point, k));
    const auto contribution = -nodal[point].w[k] * (current - previous[point][k]);
    const auto target = strongWall && k > 0 && k <= dim ? PROJ : term;
    terms[target] += contribution;
    sums[STAGE_ABS] += std::abs(contribution);
    if (term == CONV) sums[CONV_ABS] += std::abs(contribution);
    previous[point][k] = current;
  }
}

CEntropyBudget::Term CEntropyBudget::TermOfMarker(unsigned short kind) {
  switch (kind) {
    case FAR_FIELD: return BC_FAR;
    case EULER_WALL:
    case SYMMETRY_PLANE: return BC_SLIP;
    case HEAT_FLUX:
    case ISOTHERMAL:
    case HEAT_TRANSFER:
    case CHT_WALL_INTERFACE:
    case SMOLUCHOWSKI_MAXWELL: return BC_WALL;
    default: return BC_OTHER;
  }
}

bool CEntropyBudget::IsNodalMarker(unsigned short kind, bool strong) {
  if (strong) return TermOfMarker(kind) == BC_WALL || kind == CUSTOM_BOUNDARY;
  switch (kind) {
    case ACTDISK_INLET: case ENGINE_INFLOW: case INLET_FLOW: case ACTDISK_OUTLET: case ENGINE_EXHAUST:
    case SUPERSONIC_INLET: case OUTLET_FLOW: case SUPERSONIC_OUTLET: case GILES_BOUNDARY:
    case RIEMANN_BOUNDARY: case FAR_FIELD: return true;
    default: return false;
  }
}

void CEntropyBudget::CloseMarker(unsigned short marker, unsigned short kind, bool projection, bool strongWall) {
  const auto term = TermOfMarker(kind);
  double standardFar = 0;
  for (auto v = 0ul; v < geometry.GetnVertex(marker); ++v) {
    const auto point = geometry.vertex[marker][v]->GetNode();
    if (!geometry.nodes->GetDomain(point)) continue;
    if (!p1 && (term == BC_FAR || projection)) {
      // thm:closure: the MD slip sample is the projection delta plus psi . beta.
      auto value = ebt::Dot(dim, nodal[point].psi, markerBeta[marker][v]).value;
      for (unsigned k = 0; k < nVar; ++k)
        value -= nodal[point].w[k] * (ebt::Value(residual(point, k)) - previous[point][k]);
      AddSignedPart(projection ? SignedParts::SLIP_VERTEX : SignedParts::FAR_VERTEX, value);
    }
    if (p1 && term == BC_FAR && config.GetBC_Entropy_P1_Standard_FarField()) {
      for (unsigned k = 0; k < nVar; ++k)
        standardFar -= nodal[point].w[k] * (ebt::Value(residual(point, k)) - previous[point][k]);
    }
    SnapshotRow(point, projection ? PROJ : term, strongWall);
  }
  sums[FAR_STD] += standardFar;
}

bool CEntropyBudget::OwnsItem(const unsigned long* points, unsigned count) const {
  return ebt::OwnsItem(points, count, [&](auto p) { return geometry.nodes->GetGlobalIndex(p); },
                       [&](auto p) { return geometry.nodes->GetDomain(p); });
}

unsigned CEntropyBudget::OwnedIncidences(const unsigned long* points, unsigned count) const {
  const auto ownedNodes = std::count_if(points, points + count,
                                         [&](auto point) { return geometry.nodes->GetDomain(point); });
  return ownedNodes * (IncidenceDenominator / count);
}

void CEntropyBudget::PrepareBoundaryShares() {
  markerBeta.resize(config.GetnMarker_All());
  farViscous.resize(markerBeta.size());
  for (unsigned short m = 0; m < markerBeta.size(); ++m) {
    const auto kind = config.GetMarker_All_KindBC(m);
    if (kind == SEND_RECEIVE || kind == INTERNAL_BOUNDARY) continue;

    auto& shares = markerBeta[m];
    shares.resize(geometry.GetnVertex(m));
    if (!p1 && kind == FAR_FIELD && config.GetViscous()) farViscous[m].resize(shares.size());
    if (p1) {
      su2activematrix p1Shares;
      p1Shares.resize(shares.size(), dim) = su2double(0.0);
      BoundaryClosure::AddMarkerFlux(geometry, m, [](unsigned long) { return su2double(1.0); }, p1Shares);
      for (auto v = 0ul; v < shares.size(); ++v)
        for (unsigned d = 0; d < dim; ++d) shares[v][d] = ebt::Value(p1Shares(v, d));
    } else {
      for (auto v = 0ul; v < shares.size(); ++v)
        for (unsigned d = 0; d < dim; ++d) shares[v][d] = -ebt::Value(geometry.vertex[m][v]->GetNormal()[d]);
    }
    for (auto v = 0ul; v < shares.size(); ++v) {
      const auto point = geometry.vertex[m][v]->GetNode();
      if (geometry.nodes->GetDomain(point))
        for (unsigned d = 0; d < dim; ++d) beta[point][d] += shares[v][d];
    }
  }
}

void CEntropyBudget::BoundaryPostPass() {
  for (unsigned short m = 0; m < markerBeta.size(); ++m) {
    const auto term = TermOfMarker(config.GetMarker_All_KindBC(m));
    for (auto v = 0ul; v < markerBeta[m].size(); ++v) {
      const auto point = geometry.vertex[m][v]->GetNode();
      if (!geometry.nodes->GetDomain(point)) continue;
      const auto potential = ebt::Dot(dim, nodal[point].psi, markerBeta[m][v]).value;
      sums[PSI_ALL] += potential;
      if (term == BC_FAR) sums[PSI_FAR] += potential;
      if (term == BC_SLIP) sums[PSI_SLIP] += potential;
      if (term == BC_WALL) sums[PSI_WALL] += potential;
      if (!farViscous[m].empty()) sums[FAR_VISC] += ebt::Dot(nVar, nodal[point].w, farViscous[m][v]).value;
    }
  }
  for (auto point = 0ul; point < owned; ++point) {
    if (!geometry.nodes->GetPhysicalBoundary(point)) continue;
    double error2 = 0, scale2 = 0;
    for (unsigned d = 0; d < dim; ++d) {
      const auto error = beta[point][d] + normals[point][d];
      error2 += error * error;
      scale2 += beta[point][d] * beta[point][d];
    }
    maxima[BETA_ERROR] = std::max(maxima[BETA_ERROR], std::sqrt(error2));
    maxima[BETA_SCALE] = std::max(maxima[BETA_SCALE], std::sqrt(scale2));
  }
}

void CEntropyBudget::AddSignedPart(unsigned group, double value) {
  if (value > 0) {
    sums[POS_PART + group] += value;
    ++counts[POS_COUNT + group];
  } else if (value < 0) {
    sums[NEG_PART + group] += value;
    ++counts[NEG_COUNT + group];
  }
}

void CEntropyBudget::TapPair(unsigned long pair, const su2double* central, const su2double* chi) {
  for (unsigned k = 0; k < nVar; ++k) pairs[pair].central[k] = ebt::Value(central[k]);
  for (unsigned k = 0; k < 2; ++k) pairs[pair].chi[k] = ebt::Value(chi[k]);
}

void CEntropyBudget::EdgePostPass() {
  const auto& edges = *geometry.edges;
  const auto& taps = nodes.EntropyBudgetEdge;
  for (auto edge = 0ul; edge < geometry.GetnEdge(); ++edge) {
    const unsigned long point[] = {edges.GetNode(edge, 0), edges.GetNode(edge, 1)};
    const auto* normal = p1 ? edges.GetFormNormal()[edge] : edges.GetNormal(edge);
    PairSample sample;
    State viscous{};
    for (unsigned k = 0; k < nVar; ++k) {
      sample.central[k] = ebt::Value(taps(edge, 3 + k));
      if (!p1 && config.GetViscous()) viscous[k] = ebt::Value(taps(edge, 3 + nVar + k));
    }
    for (unsigned k = 0; k < 2; ++k) sample.chi[k] = ebt::Value(taps(edge, k)) * ebt::Value(taps(edge, 2));
    AccumulatePair(point, normal, sample, viscous, false);
    if (first && p1) {
      const auto* e = edges.GetFormE()[edge];
      maxima[MAX_FORM_E] = std::max(maxima[MAX_FORM_E],
          ebt::Relative(std::sqrt(ebt::Dot(dim, e, e).value), std::sqrt(ebt::Dot(dim, normal, normal).value)));
    }
  }
}

void CEntropyBudget::PairPostPass() {
  const auto& edges = *geometry.edges;
  for (auto pair = 0ul; pair < pairs.size(); ++pair) {
    const auto& ends = edges.GetSplitNodes()[pair];
    const unsigned long point[] = {ends[0], ends[1]};
    AccumulatePair(point, edges.GetSplitNormal()[pair], pairs[pair], State{}, true);
  }
}

void CEntropyBudget::AccumulatePair(const unsigned long* point, const su2double* normal, const PairSample& sample,
                                    const State& viscous, bool split) {
  const auto& i = nodal[point[0]];
  const auto& j = nodal[point[1]];
  const auto dw = ebt::Jump(i.w, j.w);
  const auto dut = ebt::Jump(i.ut, j.ut);
  const auto dlapl = ebt::Jump(i.lapl, j.lapl);
  const auto [central, eps2, eps4, enthalpy] =
      ebt::ProduceEdge(dim, dw, ebt::Jump(i.psi, j.psi), normal, sample.central, dut, dlapl, sample.chi);
  const auto add = [](double value) { return value > 0 ? 1ul : 0ul; };

  counts[split ? PAIR_INCIDENCE : EDGE_INCIDENCE] += OwnedIncidences(point, 2);
  if (OwnsItem(point, 2)) {
    ++counts[split ? PAIR_COUNT : EDGE_COUNT];
    sums[OWNER_CENTRAL] += ebt::Dot(nVar, dw, sample.central).value;
    sums[OWNER_EPS2] += eps2;
    sums[OWNER_EPS4] += eps4;
    counts[EPS2_POS] += add(eps2);
    counts[EPS2_POS_RO] += add(eps2 - ebt::Roundoff(nVar, std::abs(sample.chi[0]) * enthalpy.scale));
    maxima[MAX_CENTRAL] = std::max(maxima[MAX_CENTRAL], std::abs(central));
    maxima[MAX_EPS2] = std::max(maxima[MAX_EPS2], std::abs(eps2));
    if (!p1) AddSignedPart(SignedParts::CENTRAL_EDGE, central);
    ratioSamples.push_back({{ebt::Dot(nVar, dw, ebt::Jump(i.u, j.u)), enthalpy}});
  }

  // eq:tadmor: the edge flux is added to R_i and subtracted from R_j.
  for (unsigned end = 0; end < 2; ++end) {
    const auto p = point[end];
    if (!geometry.nodes->GetDomain(p)) continue;
    const double orientation = end == 0 ? 1 : -1;
    const auto& w = nodal[p].w;
    for (unsigned k = 0; k < nVar; ++k) {
      const double flux[] = {sample.central[k], -sample.chi[0] * dut[k], sample.chi[1] * dlapl[k], viscous[k]};
      for (unsigned part = 0; part < 4; ++part) {
        sums[CENTRAL_SUM + part] -= orientation * w[k] * flux[part];
        sums[CENTRAL_ABS + part] += std::abs(w[k] * flux[part]);
        reconstructed[p][k] += orientation * flux[part];
        maxima[ROW_SCALE] = std::max(maxima[ROW_SCALE], std::abs(flux[part]));
      }
    }
    for (unsigned d = 0; d < dim; ++d) normals[p][d] += orientation * ebt::Value(normal[d]);
    volume[p][POINT_EPS2] += 0.5 * eps2;
    volume[p][POINT_EPS4] += 0.5 * eps4;
    volume[p][POINT_CENTRAL] += 0.5 * central;
    volume[p][POINT_VISC] -= orientation * ebt::Dot(nVar, w, viscous).value;
  }
}

void CEntropyBudget::CopyElement(ElementSample& sample, const unsigned long* point,
                                const su2double (*res)[ebt::MaxVar], double weight, double galerkin) {
  sample.weight = weight;
  sample.galerkin = galerkin;
  for (unsigned a = 0; a <= dim; ++a) {
    sample.point[a] = point[a];
    for (unsigned k = 0; k < nVar; ++k) sample.res[a][k] = ebt::Value(res[a][k]);
  }
}

void CEntropyBudget::TapViscousElement(unsigned long elem, unsigned sub, const unsigned long* point,
                                      const su2double (*res)[ebt::MaxVar], const su2double& weight,
                                      const P1ViscousEntropy::Element& simplex) {
  const auto galerkin = first ? ebt::Value(simplex.GalerkinDissipation()) : 0.0;
  CopyElement(elements[elem][sub], point, res, ebt::Value(weight), galerkin);
}

void CEntropyBudget::TapWallElement(unsigned long elem, short sub, const unsigned long* point,
                                   const su2double (*res)[ebt::MaxVar], double weight, const su2double& production,
                                   const su2double& galerkin, bool symmetry, bool isothermal) {
  walls.push_back({{}, elem, sub, ebt::Value(production), symmetry, isothermal});
  CopyElement(walls.back().sample, point, res, weight, ebt::Value(galerkin));
}

ebt::Contraction CEntropyBudget::ContractElement(const ElementSample& sample, bool ownedOnly) const {
  ebt::Contraction result;
  for (unsigned a = 0; a <= dim; ++a) {
    const auto point = sample.point[a];
    if (ownedOnly && !geometry.nodes->GetDomain(point)) continue;
    const auto work = ebt::Dot(nVar, nodal[point].w, sample.res[a]);
    result.value += sample.weight * work.value;
    result.scale += sample.weight * work.scale;
  }
  return result;
}

void CEntropyBudget::ElementPostPass() {
  for (auto elem = 0ul; elem < elements.size(); ++elem) {
    const auto& cell = *geometry.elem[elem];
    unsigned long point[N_POINTS_MAXIMUM];
    for (unsigned a = 0; a < cell.GetnNodes(); ++a) point[a] = cell.GetNode(a);
    const bool own = OwnsItem(point, cell.GetnNodes());
    const auto incidence = OwnedIncidences(point, cell.GetnNodes());
    counts[VISC_INCIDENCE] += incidence * elements[elem].size();
    for (const auto& sample : elements[elem]) {
      if (sample.weight == 0) continue;
      const auto work = ContractElement(sample, false);
      const auto ownedWork = ContractElement(sample, true);
      sums[ELEM_VISC] -= ownedWork.value;
      sums[VISC_ABS] += ownedWork.scale;
      // thm:visc: count sub-simplices individually and distribute -omega_t D_t.
      for (unsigned a = 0; a <= dim; ++a)
        if (geometry.nodes->GetDomain(sample.point[a])) volume[sample.point[a]][POINT_VISC] -= work.value / (dim + 1);
      if (!own) continue;
      ++counts[VISC_COUNT];
      sums[OWNER_VISC] -= work.value;
      counts[VISC_NEG] += work.value < 0;
      counts[VISC_NEG_RO] += work.value < -ebt::Roundoff(nVar * (dim + 1), work.scale);
      if (first) {
        const auto galerkin = sample.weight * sample.galerkin;
        maxima[MAX_VISC_CHECK] = std::max(maxima[MAX_VISC_CHECK],
            ebt::Relative(work.value - galerkin, work.scale + std::abs(galerkin)));
      }
    }
  }
}

void CEntropyBudget::WallPostPass() {
  struct WallElement {
    double production = 0, dissipation = 0, scale = 0;
    bool owned = false;
  };
  std::map<std::pair<unsigned long, short>, WallElement> grouped;
  for (const auto& wall : walls) {
    const auto& sample = wall.sample;
    const auto ownedWork = ContractElement(sample, true);
    sums[wall.symmetry ? SYM_SUM : WALL_SUM] -= ownedWork.value;
    const auto [entry, inserted] = grouped.try_emplace({wall.elem, wall.sub});
    auto& group = entry->second;
    if (inserted) {
      counts[WALL_INCIDENCE] += OwnedIncidences(sample.point.data(), dim + 1);
      group.owned = OwnsItem(sample.point.data(), dim + 1);
    }
    if (!group.owned) continue;
    const auto production = sample.weight * wall.production;
    group.production += production;
    group.dissipation = sample.weight * sample.galerkin;
    group.scale += std::abs(production);
    if (!wall.symmetry) sums[WALL_PI] += production;
    if (first && !wall.isothermal) {
      const auto work = ContractElement(sample, false);
      maxima[MAX_ADIABATIC_CHECK] = std::max(maxima[MAX_ADIABATIC_CHECK],
          ebt::Relative(-work.value - production, work.scale + std::abs(production)));
    }
  }
  // thm:wall: sum all SAT faces before testing Pi_T^w - D_T/c, including D_T = 0.
  for (const auto& entry : grouped) {
    const auto& group = entry.second;
    if (!group.owned) continue;
    const auto penalty = P1ViscousEntropy::NOSLIP_PENALTY;
    const auto defect = ebt::WallDefect(group.production, group.dissipation, penalty);
    ++counts[WALL_COUNT];
    counts[WALL_BAD] += defect > 0;
    counts[WALL_BAD_RO] += defect > ebt::Roundoff(nVar, group.scale + std::abs(group.dissipation / penalty));
    if (group.dissipation > 0) {
      maxima[WALL_RATIO] = std::max(maxima[WALL_RATIO], group.production / group.dissipation);
      ++counts[WALL_RATIO_COUNT];
    }
  }
}

void CEntropyBudget::TapFarFace(const unsigned long* point, unsigned short count,
                                const P1BoundaryClosure::EsFaceSample* samples) {
  if (!OwnsItem(point, count)) return;
  for (unsigned q = 0; q < count; ++q) {
    const auto& sample = samples[q];
    if (!sample.ghost) continue;

    // thm:closure: psi_q.N_q - w_q.F* is bounded by the ghost numerical entropy flux.
    const auto wGhost = ebt::Variables(dim, config.GetGamma(), sample.gq);
    const auto psi = ebt::Jump(ebt::Potential(dim, sample.gq), ebt::Potential(dim, sample.sq));
    const auto w = ebt::Jump(wGhost, sample.wq);
    const auto potential = ebt::Dot(dim, psi, sample.normal);
    const auto flux = ebt::Dot(nVar, w, sample.flux);
    const auto defect = potential.value - flux.value;
    ++counts[GHOST_COUNT];
    counts[GHOST_BAD] += defect > ebt::Roundoff(nVar, potential.scale + flux.scale);
    maxima[GHOST_DEFECT] = std::max(maxima[GHOST_DEFECT], defect);
  }
}

void CEntropyBudget::TapFarViscous(unsigned short marker, unsigned long vertex, const su2double* flux) {
  for (unsigned k = 0; k < nVar; ++k) farViscous[marker][vertex][k] = ebt::Value(flux[k]);
}

void CEntropyBudget::RatioHistogram() {
  passivedouble localMax = 0, globalMax = 0;
  for (const auto& ratio : ratioSamples) localMax = std::max(localMax, static_cast<passivedouble>(ratio[0].value));
  SU2_MPI::Allreduce(&localMax, &globalMax, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());

  const auto bounds = ebt::RatioBounds(ebt::Value(config.GetGamma()));
  ratioMin = std::numeric_limits<passivedouble>::max();
  for (const auto& sample : ratioSamples) {
    if (!ebt::ResolvedRatio(nVar, sample[0], globalMax)) continue;
    const double ratio = sample[1].value / sample[0].value;
    const auto tolerance = ebt::Roundoff(nVar, sample[1].scale + bounds[1] * sample[0].scale) / sample[0].value;
    ratioMin = std::min(ratioMin, ratio);
    maxima[RATIO_MAX] = counts[RATIO_COUNT] == 0 ? ratio : std::max(maxima[RATIO_MAX], ratio);
    ++counts[RATIO_COUNT];
    counts[RATIO_BAD] += ratio < bounds[0] - tolerance || ratio > bounds[1] + tolerance;
    ++counts[HISTOGRAM + ebt::RatioBin(ratio)];
  }
  if (counts[RATIO_COUNT] == 0) maxima[RATIO_MAX] = -std::numeric_limits<passivedouble>::max();
  ratioSamples.clear();
}

void CEntropyBudget::Reduce() {
  const auto comm = SU2_MPI::GetComm();
  // MPI_DOUBLE follows SU2's passive precision, even in a single-precision build.
  std::array<passivedouble, unsigned(SUM_COUNT) + unsigned(TERM_COUNT)> local{}, global{};
  std::copy(sums.begin(), sums.end(), local.begin());
  std::copy(terms.begin(), terms.end(), local.begin() + SUM_COUNT);
  SU2_MPI::Allreduce(local.data(), global.data(), global.size(), MPI_DOUBLE, MPI_SUM, comm);
  std::copy_n(global.begin(), SUM_COUNT, sums.begin());
  std::copy_n(global.begin() + SUM_COUNT, TERM_COUNT, terms.begin());

  std::array<passivedouble, MAX_COUNT> maxLocal, maxGlobal;
  std::copy(maxima.begin(), maxima.end(), maxLocal.begin());
  decltype(counts) countGlobal;
  passivedouble minLocal = ratioMin, minGlobal;
  SU2_MPI::Allreduce(maxLocal.data(), maxGlobal.data(), maxGlobal.size(), MPI_DOUBLE, MPI_MAX, comm);
  SU2_MPI::Allreduce(&minLocal, &minGlobal, 1, MPI_DOUBLE, MPI_MIN, comm);
  SU2_MPI::Allreduce(counts.data(), countGlobal.data(), countGlobal.size(), MPI_UNSIGNED_LONG, MPI_SUM, comm);
  std::copy(maxGlobal.begin(), maxGlobal.end(), maxima.begin());
  ratioMin = minGlobal;
  counts = countGlobal;
}

void CEntropyBudget::FillHistory() {
  const auto visc = p1 ? terms[VISC] : terms[VISC] + sums[MD_VISC];
  history[CENTRAL] = sums[CENTRAL_SUM];
  history[CENTRAL_INT] = sums[CENTRAL_SUM] - sums[PSI_ALL];
  history[PSI_BETA] = sums[PSI_ALL];
  history[EPS2] = sums[EPS2_SUM];
  history[EPS4] = sums[EPS4_SUM];
  history[VISC_RAW] = visc;
  history[WALL_RAW] = sums[WALL_SUM];
  history[FAR] = terms[BC_FAR];
  history[SLIP] = terms[BC_SLIP];
  history[WALL_BC] = terms[BC_WALL] - sums[WALL_SUM] - sums[SYM_SUM];
  history[OTHER] = terms[BC_OTHER];
  history[SRC] = terms[SOURCE];
  history[PROJECTION] = terms[PROJ];
  history[SYM_SAT] = sums[SYM_SUM];
  history[WALL_PRODUCTION] = sums[WALL_PI];
  history[WALL_HEAT] = sums[WALL_SUM] - sums[WALL_PI];
  history[FAR_VISCOUS] = sums[FAR_VISC];
  history[FAR_STANDARD] = sums[FAR_STD];
  history[FAR_THM] = history[FAR] + sums[PSI_FAR];
  history[SLIP_THM] = history[SLIP] + sums[PSI_SLIP];
  history[WALL_THM] = history[WALL_BC] + sums[PSI_WALL];
  history[TOTAL] = sums[TOTAL_SUM];

  const auto conv = sums[CENTRAL_SUM] + sums[EPS2_SUM] + sums[EPS4_SUM] + sums[MD_VISC];
  const auto convError = conv - terms[CONV];
  const auto convScale = sums[CENTRAL_ABS] + sums[EPS2_ABS] + sums[EPS4_ABS] + sums[MD_VISC_ABS];
  double total = 0;
  for (auto field : {CENTRAL, EPS2, EPS4, VISC_RAW, WALL_RAW, FAR, SLIP, WALL_BC, OTHER, SRC, PROJECTION, SYM_SAT})
    total += history[field];
  history[CLOSURE] = ebt::Relative(total - history[TOTAL], sums[STAGE_ABS] + convScale);
  history[CONV_CLOSURE] = ebt::Relative(convError, sums[CONV_ABS] + convScale);
  history[CONV_ROW_CHECK] = ebt::Relative(maxima[ROW_ERROR], maxima[ROW_SCALE]);
  history[OWNER_CHECK] = std::max({
      ebt::Relative(sums[OWNER_CENTRAL] - sums[CENTRAL_SUM], sums[CENTRAL_ABS]),
      ebt::Relative(sums[OWNER_EPS2] - sums[EPS2_SUM], sums[EPS2_ABS]),
      ebt::Relative(sums[OWNER_EPS4] - sums[EPS4_SUM], sums[EPS4_ABS]),
      ebt::Relative(sums[OWNER_VISC] - sums[ELEM_VISC], sums[VISC_ABS]),
      ebt::Relative(terms[VISC] - sums[ELEM_VISC], sums[VISC_ABS] + std::abs(terms[VISC]))});
  history[BETA_CHECK] = ebt::Relative(maxima[BETA_ERROR], maxima[BETA_SCALE]);

  const auto bounds = ebt::RatioBounds(ebt::Value(config.GetGamma()));
  history[R_MIN] = counts[RATIO_COUNT] ? ratioMin : 0;
  history[R_MAX] = counts[RATIO_COUNT] ? maxima[RATIO_MAX] : 0;
  history[R_LOWER] = bounds[0];
  history[R_UPPER] = bounds[1];
  history[MAX_WALL_RATIO] = counts[WALL_RATIO_COUNT] ? maxima[WALL_RATIO] : 0;
  history[CENTRAL_RATIO] = ebt::Relative(maxima[MAX_CENTRAL], maxima[MAX_EPS2]);
  history[MAX_FAR_GHOST_DEFECT] = maxima[GHOST_DEFECT];
  const Count historyCounts[] = {EPS2_POS, EPS2_POS_RO, VISC_NEG, VISC_NEG_RO, WALL_BAD, WALL_BAD_RO};
  for (unsigned k = 0; k < sizeof(historyCounts) / sizeof(*historyCounts); ++k)
    history[NPOS_EPS2 + k] = counts[historyCounts[k]];
  const Count sampleCounts[] = {RATIO_COUNT, RATIO_BAD, EDGE_COUNT, PAIR_COUNT, VISC_COUNT, WALL_COUNT, GHOST_COUNT, GHOST_BAD};
  for (unsigned k = 0; k < sizeof(sampleCounts) / sizeof(*sampleCounts); ++k)
    history[N_RATIO + k] = counts[sampleCounts[k]];

  const auto* velocity = config.GetVelocity_FreeStreamND();
  const auto rho = ebt::Value(config.GetDensity_FreeStreamND());
  const auto speed2 = ebt::Dot(dim, velocity, velocity).value;
  const auto qArea = 0.5 * rho * speed2 * ebt::Value(config.GetRefArea());
  /*--- Oswatitsch: D U_inf = T_inf dS/dt and rho s = -R U, so D = (p_inf/rho_inf) Pi / U_inf with Pi the
   * production of -U; CD = D / (q_inf A). ---*/
  const auto dragFactor =
      qArea > 0 ? ebt::Value(config.GetPressure_FreeStreamND()) / (rho * sqrt(speed2) * qArea) : 0;
  const auto dissipation = history[EPS2] + history[EPS4];
  history[DRAG_CD] = -dragFactor * dissipation;
  history[DRAG_CD_ALL] = -dragFactor * (history[CENTRAL_INT] + dissipation);
  const auto farBalance = history[CENTRAL] + dissipation + history[FAR];
  const auto scale = std::abs(history[CENTRAL]) + std::abs(history[EPS2]) + std::abs(history[EPS4]) + std::abs(history[FAR]);
  history[FARFIELD_CHECK] = ebt::Relative(farBalance, scale);
  history[FARFIELD_CHECK_PROJ] = ebt::Relative(farBalance + history[PROJECTION], scale + std::abs(history[PROJECTION]));

  // Check the minimum-global-ID rule against the independent count from all owned endpoints.
  history[ITEM_COUNT_CHECK] = 0;
  for (unsigned k = 0; k < 4; ++k) {
    const double expected = static_cast<double>(counts[EDGE_INCIDENCE + k]) / IncidenceDenominator;
    history[ITEM_COUNT_CHECK] = std::max(history[ITEM_COUNT_CHECK], std::abs(counts[EDGE_COUNT + k] - expected));
  }
}

void CEntropyBudget::Finalize() {
  CloseStage();
  EdgePostPass();
  PairPostPass();
  BoundaryPostPass();
  ElementPostPass();
  WallPostPass();
  for (auto point = 0ul; point < owned; ++point) {
    const auto work = ebt::Dot(nVar, nodal[point].w, previous[point]);
    sums[TOTAL_SUM] -= work.value;
    for (unsigned k = 0; k < nVar; ++k) {
      maxima[ROW_ERROR] = std::max(maxima[ROW_ERROR], std::abs(convection[point][k] - reconstructed[point][k]));
      maxima[ROW_SCALE] = std::max(maxima[ROW_SCALE], std::abs(convection[point][k]));
    }
  }
  RatioHistogram();
  Reduce();
  FillHistory();
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    WriteCsv();
    if (first && p1) {
      std::ostringstream msg;
      msg << "Entropy budget first iteration: relative Galerkin check=" << maxima[MAX_VISC_CHECK]
          << ", relative adiabatic SAT check=" << maxima[MAX_ADIABATIC_CHECK] << ", max |formE|/|formNormal|="
          << maxima[MAX_FORM_E] << '\n';
      std::cout << msg.str();
    }
  }
  first = false;
}

double CEntropyBudget::PointField(unsigned long point, unsigned field) const {
  if (point >= owned) return 0;
  const auto cellVolume = ebt::Value(geometry.nodes->GetVolume(point) + geometry.nodes->GetPeriodicVolume(point));
  if (cellVolume <= 0) return 0;
  const auto& values = volume[point];
  return (field == POINT_DISSIP ? values[POINT_EPS2] + values[POINT_EPS4] : values[field]) / cellVolume;
}

void CEntropyBudget::WriteCsv() const {
  std::ofstream file("entropy_budget.csv");
  file << "ITER";
  for (auto name : HistoryNames) file << ',' << name;
  file << ",R_UNDER";
  for (unsigned k = 0; k < ebt::RatioBins - 2; ++k) file << ",R_BIN_" << k;
  file << ",R_OVER";
  const char* groups[] = {"MD_CENTRAL_EDGE", "MD_FAR_VERTEX", "MD_SLIP_PROJECTION_VERTEX"};
  for (auto group : groups)
    file << ',' << group << "_POS_SUM," << group << "_NEG_SUM," << group << "_POS_COUNT," << group << "_NEG_COUNT";
  file << '\n' << config.GetInnerIter() << std::setprecision(17);
  for (auto value : history) file << ',' << value;
  for (unsigned k = 0; k < ebt::RatioBins; ++k) file << ',' << counts[HISTOGRAM + k];
  for (unsigned k = 0; k < SignedParts::SIZE; ++k)
    file << ',' << sums[POS_PART + k] << ',' << sums[NEG_PART + k] << ',' << counts[POS_COUNT + k] << ','
         << counts[NEG_COUNT + k];
  file << '\n';
  file.close();
  if (!file) std::cerr << "ENTROPY_BUDGET_DIAGNOSTIC: could not write entropy_budget.csv\n";
}

bool CEntropyBudget::HashEnabled(const CConfig& config, unsigned short mesh) {
  static const bool enabled = std::getenv("SU2_ENTROPY_BUDGET_HASH") != nullptr;
  const auto solver = config.GetKind_Solver();
  return enabled && std::is_same_v<su2double, passivedouble> && mesh == MESH_0 &&
         (solver == MAIN_SOLVER::EULER || solver == MAIN_SOLVER::NAVIER_STOKES) &&
         config.GetKind_TimeIntScheme() == EULER_IMPLICIT;
}

void CEntropyBudget::PrintHash(HashPhase phase, const CConfig& config, const CSolver& solver) {
  constexpr std::uint64_t seed = 14695981039346656037ull;
  std::uint64_t rows = seed, jacobian = seed;
  const auto hashBytes = [](auto* data, std::size_t count, std::uint64_t& hash) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(data);
    for (std::size_t k = 0; k < count * sizeof(*data); ++k) hash = (hash ^ bytes[k]) * 1099511628211ull;
  };
  const auto nVar = solver.GetnVar();
  const auto points = solver.LinSysRes.GetNBlkDomain();
  const auto& matrix = solver.Jacobian;
  for (auto p = 0ul; p < points; ++p) {
    const auto* row = phase == HashPhase::Solution ? solver.GetNodes()->GetSolution()[p] : solver.LinSysRes.GetBlock(p);
    hashBytes(row, nVar, rows);
    if (phase != HashPhase::Solution) {
      const auto block = [&](unsigned long col) {
        const auto data = matrix.GetBlockView(p, col);
        hashBytes(&col, 1, jacobian);
        if (data.ptr) {
          hashBytes(data.ptr, nVar * nVar, jacobian);
        } else if (data.qs) {
          hashBytes(data.qs, nVar, jacobian);
          hashBytes(data.qv, nVar * nVar, jacobian);
        }
      };
      block(p);
      matrix.ForEachOffDiagColumn(p, block);
    }
  }
  const char* label = phase == HashPhase::Spatial ? "spatial" : phase == HashPhase::ImplicitRhs ? "rhs" : "solution";
  std::ostringstream line;
  line << "SU2_ENTROPY_BUDGET_HASH rank=" << SU2_MPI::GetRank() << " iter=" << config.GetInnerIter() << ' ' << label
       << '=' << std::hex << rows;
  if (phase != HashPhase::Solution) line << " jacobian=" << std::hex << jacobian;
  std::cout << (line.str() + '\n') << std::flush;
}
