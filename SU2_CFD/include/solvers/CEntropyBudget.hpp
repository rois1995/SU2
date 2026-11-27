/*!
 * \file CEntropyBudget.hpp
 * \brief Read-only diagnostic of the spatial JST entropy balance.
 * \copyright Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * \license GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "../numerics/flow/entropy_budget_terms.hpp"
#include <vector>

class CConfig;
class CGeometry;
class CEulerVariable;
class CSolver;
template <class ScalarType> class CSysVector;
namespace P1ViscousEntropy { class Element; }

class CEntropyBudget {
 public:
  enum Term { CONV, VISC, BC_FAR, BC_SLIP, BC_WALL, BC_OTHER, SOURCE, PROJ, TERM_COUNT };
  enum HistoryField {
    CENTRAL, CENTRAL_INT, PSI_BETA, EPS2, EPS4, VISC_RAW, WALL_RAW, FAR, SLIP, WALL_BC, OTHER, SRC, PROJECTION, SYM_SAT,
    WALL_PRODUCTION, WALL_HEAT, FAR_VISCOUS, FAR_STANDARD, FAR_THM, SLIP_THM, WALL_THM, TOTAL, CLOSURE,
    CONV_CLOSURE, CONV_ROW_CHECK, OWNER_CHECK, BETA_CHECK, NPOS_EPS2, NPOS_EPS2_RO, NNEG_VISC, NNEG_VISC_RO,
    NBAD_WALL, NBAD_WALL_RO, MAX_WALL_RATIO, CENTRAL_RATIO, R_MIN, R_MAX, R_LOWER, R_UPPER, N_RATIO, NBAD_RATIO,
    N_EDGE, N_PAIR, N_VISC, N_WALL, N_FAR_GHOST, NBAD_FAR_GHOST, MAX_FAR_GHOST_DEFECT,
    DRAG_CD, DRAG_CD_ALL, FARFIELD_CHECK, FARFIELD_CHECK_PROJ, ITEM_COUNT_CHECK, HISTORY_COUNT
  };
  inline static constexpr const char* HistoryNames[] = {
    "ENT_CENTRAL", "ENT_CENTRAL_INT", "ENT_PSI_BETA", "ENT_EPS2", "ENT_EPS4", "ENT_VISC", "ENT_WALL",
    "ENT_BC_FAR", "ENT_BC_SLIP", "ENT_BC_WALL", "ENT_BC_OTHER", "ENT_SOURCE", "ENT_PROJ", "ENT_SYM_SAT",
    "ENT_WALL_PRODUCTION", "ENT_WALL_HEAT", "ENT_BC_FAR_VISC", "ENT_BC_FAR_STANDARD",
    "ENT_BC_FAR_THM", "ENT_BC_SLIP_THM", "ENT_BC_WALL_THM", "ENT_TOTAL", "ENT_CLOSURE", "ENT_CONV_CLOSURE",
    "ENT_CONV_ROW_CHECK", "ENT_OWNER_CHECK", "ENT_BETA_CHECK", "ENT_NPOS_EPS2", "ENT_NPOS_EPS2_RO", "ENT_NNEG_VISC",
    "ENT_NNEG_VISC_RO", "ENT_NBAD_WALL", "ENT_NBAD_WALL_RO", "ENT_MAX_WALL_RATIO", "ENT_CENTRAL_RATIO",
    "ENT_R_MIN", "ENT_R_MAX", "ENT_R_LOWER", "ENT_R_UPPER", "ENT_N_RATIO", "ENT_NBAD_RATIO", "ENT_N_EDGE",
    "ENT_N_PAIR", "ENT_N_VISC", "ENT_N_WALL", "ENT_N_FAR_GHOST", "ENT_NBAD_FAR_GHOST", "ENT_MAX_FAR_GHOST_DEFECT",
    "ENT_DRAG_CD", "ENT_DRAG_CD_ALL", "ENT_FARFIELD_CHECK", "ENT_FARFIELD_CHECK_PROJ", "ENT_ITEM_COUNT_CHECK"
  };
  static_assert(sizeof(HistoryNames) / sizeof(*HistoryNames) == HISTORY_COUNT);
  enum VolumeField { POINT_EPS2, POINT_EPS4, POINT_DISSIP, POINT_CENTRAL, POINT_VISC, VOLUME_COUNT };
  inline static constexpr const char* VolumeNames[] = {
    "ENT_EPS2", "ENT_EPS4", "ENT_DISSIP", "ENT_CENTRAL", "ENT_VISC"
  };
  static_assert(sizeof(VolumeNames) / sizeof(*VolumeNames) == VOLUME_COUNT);

  CEntropyBudget(const CGeometry&, const CConfig&, const CEulerVariable&, const CSysVector<su2double>&);
  void BeginIteration();
  void BeginStage(Term next);
  void CloseMarker(unsigned short marker, unsigned short kind, bool projection = false, bool strongWall = false);
  static Term TermOfMarker(unsigned short kind);
  static bool IsNodalMarker(unsigned short kind, bool strong);
  void TapPair(unsigned long pair, const su2double* central, const su2double* chi);
  void TapViscousElement(unsigned long elem, unsigned sub, const unsigned long* point,
                         const su2double (*res)[EntropyBudgetTerms::MaxVar], const su2double& weight,
                         const P1ViscousEntropy::Element& simplex);
  void TapWallElement(unsigned long elem, short sub, const unsigned long* point,
                      const su2double (*res)[EntropyBudgetTerms::MaxVar], double weight,
                      const su2double& production, const su2double& galerkin, bool symmetry, bool isothermal);
  void TapFarFace(const unsigned long* point, unsigned short count,
                  const P1BoundaryClosure::EsFaceSample* samples);
  void TapFarViscous(unsigned short marker, unsigned long vertex, const su2double* flux);
  void Finalize();
  double History(unsigned field) const { return history[field]; }
  double PointField(unsigned long point, unsigned field) const;

  enum class HashPhase { Spatial, ImplicitRhs, Solution };
  static bool HashEnabled(const CConfig&, unsigned short mesh);
  static void PrintHash(HashPhase, const CConfig&, const CSolver&);

 private:
  using State = EntropyBudgetTerms::State;
  using Normal = EntropyBudgetTerms::Normal;
  struct NodalState {
    State w{}, u{}, ut{}, lapl{};
    Normal psi{};
  };
  struct PairSample {
    State central{};
    double chi[2]{};
  };
  struct ElementSample {
    std::array<unsigned long, 4> point{};
    std::array<State, 4> res{};
    double weight = 0, galerkin = 0;
  };
  struct WallSample {
    ElementSample sample;
    unsigned long elem;
    short sub;
    double production;
    bool symmetry, isothermal;
  };
  struct SignedParts {
    enum { CENTRAL_EDGE, FAR_VERTEX, SLIP_VERTEX, SIZE };
  };

  // One packed reduction per operation at the end of spatial assembly.
  enum Sum { CENTRAL_SUM, EPS2_SUM, EPS4_SUM, MD_VISC, OWNER_CENTRAL, OWNER_EPS2, OWNER_EPS4, OWNER_VISC,
             ELEM_VISC, WALL_SUM, SYM_SUM, WALL_PI, FAR_VISC, FAR_STD, PSI_ALL, PSI_FAR, PSI_SLIP, PSI_WALL,
             TOTAL_SUM, STAGE_ABS, CONV_ABS, CENTRAL_ABS, EPS2_ABS, EPS4_ABS, MD_VISC_ABS, VISC_ABS, POS_PART, NEG_PART = POS_PART + SignedParts::SIZE,
             SUM_COUNT = NEG_PART + SignedParts::SIZE };
  enum Max { MAX_CENTRAL, MAX_EPS2, BETA_ERROR, BETA_SCALE, ROW_ERROR, ROW_SCALE, MAX_VISC_CHECK, MAX_ADIABATIC_CHECK,
             MAX_FORM_E, WALL_RATIO, RATIO_MAX, GHOST_DEFECT, MAX_COUNT };
  enum Count { EDGE_COUNT, PAIR_COUNT, VISC_COUNT, WALL_COUNT, GHOST_COUNT, GHOST_BAD, EPS2_POS, EPS2_POS_RO,
               VISC_NEG, VISC_NEG_RO, WALL_BAD, WALL_BAD_RO, RATIO_COUNT, RATIO_BAD, WALL_RATIO_COUNT,
               EDGE_INCIDENCE, PAIR_INCIDENCE, VISC_INCIDENCE, WALL_INCIDENCE, POS_COUNT,
               NEG_COUNT = POS_COUNT + SignedParts::SIZE, HISTOGRAM = NEG_COUNT + SignedParts::SIZE,
               COUNT_SIZE = HISTOGRAM + EntropyBudgetTerms::RatioBins };

  const CGeometry& geometry;
  const CConfig& config;
  const CEulerVariable& nodes;
  const CSysVector<su2double>& residual;
  const unsigned short dim, nVar;
  const unsigned long owned;
  const bool p1;
  Term stage = CONV;
  std::vector<NodalState> nodal;
  std::vector<State> previous, convection, reconstructed;
  std::vector<Normal> beta, normals;
  std::vector<std::array<double, VOLUME_COUNT>> volume;
  std::vector<std::vector<Normal>> markerBeta;
  std::vector<std::vector<State>> farViscous;
  std::vector<PairSample> pairs;
  std::vector<std::vector<ElementSample>> elements;
  std::vector<WallSample> walls;
  std::array<double, TERM_COUNT> terms{};
  std::array<double, SUM_COUNT> sums{};
  std::array<double, MAX_COUNT> maxima{};
  std::array<unsigned long, COUNT_SIZE> counts{};
  std::array<double, HISTORY_COUNT> history{};
  std::vector<std::array<EntropyBudgetTerms::Contraction, 2>> ratioSamples;
  double ratioMin = 0;
  bool first = true;

  // Count fractions via vertex incidences with a common denominator for 2, 3, 4, 5, 6 and 8 nodes.
  static constexpr unsigned IncidenceDenominator = 120;
  bool OwnsItem(const unsigned long*, unsigned count) const;
  unsigned OwnedIncidences(const unsigned long*, unsigned count) const;
  void CloseStage();
  void SnapshotRow(unsigned long, Term, bool strongWall = false);
  void PrepareBoundaryShares();
  void BoundaryPostPass();
  void AddSignedPart(unsigned group, double value);
  void EdgePostPass();
  void PairPostPass();
  void AccumulatePair(const unsigned long*, const su2double* normal, const PairSample&, const State& viscous,
                      bool split);
  void ElementPostPass();
  void WallPostPass();
  void CopyElement(ElementSample&, const unsigned long*, const su2double (*res)[EntropyBudgetTerms::MaxVar],
                   double weight, double galerkin);
  EntropyBudgetTerms::Contraction ContractElement(const ElementSample&, bool ownedOnly) const;
  void RatioHistogram();
  void Reduce();
  void FillHistory();
  void WriteCsv() const;
};
