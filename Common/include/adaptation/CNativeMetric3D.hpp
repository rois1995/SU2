/*!
 * \file CNativeMetric3D.hpp
 * \brief Frozen-target private progress checks and strict final tetrahedral metric gates.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeField3D.hpp"
#include "CNativeCavity3D.hpp"
#include <map>
namespace SU2Native3D {
constexpr double MaximumMetricEdge = 1.8;
/*! Signed planar cross product with fma compensation, compiled in the strict-arithmetic library. */
long double CompensatedCross2D(long double ax,long double ay,long double bx,long double by);
struct EdgeMeasure {
  Point a, b;
  long double length = 0;
  std::shared_ptr<const FrozenMetricIdentity> identity;
  bool Reusable(Point from,Point to,const std::shared_ptr<const FrozenMetricIdentity>& field)const;
};
constexpr size_t MaximumCampaignEdges=6*MaximumReplacementCells;
/*! Bit0..5 follow local (01,02,03,12,13,23) edges. Only exact endpoint/field certificates skip
 * imports. Needed campaign entries are protected in reuse before bounded cache eviction can occur. */
std::vector<unsigned char> MetricEdgeRequests(const std::vector<Cell>& cells,std::map<Edge,EdgeMeasure>& reuse,
    const std::shared_ptr<const FrozenMetricIdentity>& identity,const std::map<Edge,EdgeMeasure>* campaign=nullptr);

struct PatchMetric {
  double minimum_mean_ratio = 1, minimum_scaled_jacobian = 1;
  Id weakest_cell = 0, sliver_cell = 0;
  Edge longest_edge{};
  long double maximum_length = 0;
  std::map<Edge, EdgeMeasure> edges;
};
/*! Proposal only: minimise the midpoint-metric spring energy around an original vertex. */
Point MetricMeanPoint(Point origin,const std::vector<std::pair<Point,Tensor>>& neighbors);
struct MetricShapeLinearization {
  std::array<double,2> residual{};
  std::array<std::array<std::array<double,3>,4>,2> derivative{};
};
/*! Proposal Jacobian of q/J hinges with a 1% interior aim at one actual centroid. Optional spatial
 * tensor derivatives are dM/dx,dM/dy,dM/dz at that centroid; null means a constant tensor.
 * Requires positive geometry/SPD metric. Final acceptance always remeasures the actual frozen field. */
MetricShapeLinearization LinearizeMetricShape(const Tetrahedron& tet,const Tensor& metric,
                                            const std::array<Tensor,3>* spatial=nullptr);
struct MetricStats {
  uint64_t patches = 0, cells = 0, edges = 0, changes = 0, failures = 0;
  uint64_t edge_evaluations = 0, edge_reused = 0, edge_campaign_reused=0;
  size_t maximum_cells = 0, maximum_nodes = 0, maximum_edges = 0;
};
// Geometry intents require independently certified geometric progress at the engine caller.
enum class MetricChange { Refine, Coarsen, Repair, GeometryRefine, GeometryRepair };
enum class MetricFailure { None, Measurement, Shape, Progress };
/*! Refine admission (user decision 2026-10-23: the 2D split rule, CNativeMesh2D.hpp SplitPatch sizeProgress).
 * SplitInsertion: a refinement that removes no vertex of the old cells and replaces a changed old edge longer than
 * SplitInsertionLength (a size insertion) needs no changed-edge excess progress; one that removes a vertex keeps the
 * strict excess decrease; a non-size insertion must add no oversized changed edge and strictly raise the minimum mean
 * ratio. A vertex is removed when it is absent from the fresh cells; `relocated` (certified by the caller) old vertices
 * may be absent because a refit replaced them one for one by a new identity. Fresh shape stays strict.
 * StrictExcess: the previous rule (strict excess decrease for every refinement), kept for A/B comparisons and the
 * reproducibility of older harness evidence. Default: SplitInsertion (user decision 2026-10-23).
 * Process-wide; final mesh gates are independent of it. */
enum class RefineAcceptance { StrictExcess, SplitInsertion };
constexpr double SplitInsertionLength = 1.6;
void SetRefineAcceptance(RefineAcceptance acceptance);
RefineAcceptance GetRefineAcceptance();
/*! Per-operation shape admission (user decision 2026-10-23: q >= .20 / J >= .05 are FINAL acceptance gates only).
 * Absolute: every fresh cell of a metric change must pass the final shape gates (previous behaviour, production default).
 * ValidityOnly (build-then-optimize, OPTIMIZE_PLAN): fresh cells need only q > 0 and J >= ValiditySliverJacobian, J the
 * scaled Jacobian measured in the metric (a cell below it is a numerically flat sliver whose quality gradient vanishes).
 * It governs ValidateMetricChange and MeasureMetricCells(require_shape) only; progress rules are unchanged and
 * ValidateMetricPatch (final completion) stays absolute in both modes. Process-wide; callers restore the previous mode. */
enum class ShapeAdmission { Absolute, ValidityOnly };
constexpr double ValiditySliverJacobian = 1e-3;
void SetShapeAdmission(ShapeAdmission admission);
ShapeAdmission GetShapeAdmission();
struct MetricDiagnosis {
  MetricFailure failure = MetricFailure::None;
  bool size_progress = false, quality_progress = false, cell_removal = false;
  bool size_insertion = false, vertex_removal = false;  // Refine classification (SplitInsertion rule)
  long double removed_excess = 0, added_excess = 0, longest_removed = 0;
  double minimum_mean_ratio = 1, minimum_scaled_jacobian = 1;
  Id weakest_cell = 0, sliver_cell = 0;
};
/*! Minimum normalized q/J at actual cell centroids, with the same geometry/source validation
 * and arithmetic as MeasureMetricCells. This scalar is a proposal-ranking score, never an edge/mesh certificate. */
double MetricShapeQuality(const std::vector<Cell>& cells,FrozenField& target);
/*! Measures only actual centroids and source-resolved edges of the supplied known cells; throws on
 * invalid geometry/source coverage. A caller can merge bounded batches by cell minima and unique edges.
 * The source must remain the same original frozen target throughout, including any scalar-edge reuse. */
PatchMetric MeasureMetricCells(const std::vector<Cell>& cells, FrozenField& target, MetricStats* stats = nullptr,
                              const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr,
                              bool require_shape = false, MetricDiagnosis* diagnosis = nullptr,
                              std::map<Edge,EdgeMeasure>* campaign = nullptr);
/*! Same progress gate on complete measurements assembled from the same frozen target. The caller must
 * retain all cells/edges and verify geometry/dependencies separately; this does not certify source coverage.
 * GeometryRefine/GeometryRepair need caller-certified geometric-error progress; metric shape remains strict and changed
 * oversized-edge excess must not increase. It does not authorize geometric progress by itself. */
bool ValidateMetricChange(size_t old_count, const PatchMetric& before, size_t fresh_count,
                          const PatchMetric& measured, MetricChange kind, std::string& reason,
                          MetricDiagnosis* diagnosis = nullptr, size_t relocated = 0);
/*! Requires separately validated geometry/topology/dependencies. Actual centroid target and source-resolved
 * target edge lengths; bounded128 cells. Final completion requires the strict gate on the whole mesh.
 * Reuse requires the same frozen full-field identity, edge IDs and exact endpoint coordinates.
 * Successful full-target integrals retain their fixed accuracy policy; missing composed integration rejects. */
bool ValidateMetricPatch(const std::vector<Cell>& cells, FrozenField& target, PatchMetric& measured,
                         std::string& reason, MetricStats* stats = nullptr,
                         const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr);
/*! Private progress may retain pre-existing oversized edges. Fresh shape/sliver gates are strict;
 * refinement follows GetRefineAcceptance() (see RefineAcceptance), coarsening must introduce no oversized edges and remove cells.
 * Common unchanged edges cancel exactly by identity+coordinates, not subtraction of large residual sums.
 * Repair must reduce changed-edge excess or improve quality/sliver bounds without increasing excess.
 * This is not final acceptance. Unsupported composed integration rejects, never checks the sensor substitute. */
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason,
                          MetricStats* stats = nullptr,
                          const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr,
                          MetricDiagnosis* diagnosis = nullptr, size_t relocated = 0);
}  // namespace SU2Native3D
