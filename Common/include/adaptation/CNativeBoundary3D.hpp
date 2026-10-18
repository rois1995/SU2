/*!
 * \file CNativeBoundary3D.hpp
 * \brief Coupled tetrahedral edge edits on immutable faceted or declared analytic physical references.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeCavity3D.hpp"
#include "CNativeAffineReference.hpp"
#include <map>
#include <limits>
#include <memory>
#include <set>
class CADTElemClass;
namespace SU2Native3D {
struct Facet {
  Id id = 0;
  int marker = 0;
  std::array<Node, 3> v;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, marker, v);
  }
};
struct SurfaceFace {
  Face v;
  Id facet = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(v, facet);
  }
};
struct Patch {
  std::vector<Cell> cells;
  std::vector<SurfaceFace> surface;
};
/*! Analytic authority is declared per original finite facet, never inferred from its marker.
 * Sphere: origin=center, extent.x=radius. Bump: z=origin.z+extent.z*u(1-u)*v(1-v),
 * u=(x-origin.x)/extent.x, v=(y-origin.y)/extent.y in [0,1]. Tolerance is a frozen
 * physical distance for representable projection and finite-chart boundary roundoff. Cylinder uses
 * extent.x=radius, extent.y=height and axis; Disk uses extent.x=radius and axis normal. */
struct AnalyticSurface {
  enum class Kind : unsigned { Sphere = 1, Bump = 2, Cylinder = 3, Disk = 4 };
  Kind kind = Kind::Sphere;
  Point origin, extent{1, 0, 0};
  double tolerance = 1e-12;
  Point axis{0,0,1};  // Cylinder/Disk axis; origin is cylinder base or disk center.
};
/*! Shared radial geometry for cylinder projection and frozen exterior source construction. */
Point ScaleCylinder(const AnalyticSurface& geometry, Point point, double factor);
double CylinderRadialFloor(const AnalyticSurface& geometry, const std::array<Point,3>& triangle);
long double CylinderFacing(const AnalyticSurface& geometry, const std::array<Point,3>& triangle);
/*! Bounded immutable imported facet authority. Exact finite planar containment; certified convex coplanar same-marker
 * seams are removable. Nonconvex finite patches retain oriented borders and holes. */
class PlanarReference {
 public:
  explicit PlanarReference(std::vector<Facet> facets, size_t ceiling = SIZE_MAX,
                           std::map<Id, AnalyticSurface> analytic = {},
                           std::map<Id,SU2NativeGeometry::AffineAuthority> affine = {});
  const std::map<Id, AnalyticSurface>& Analytic() const { return analytic; }
  const std::map<Id,SU2NativeGeometry::AffineAuthority>& Affine()const{return affine;}
  bool Declared(Id facet)const{return analytic.count(facet)||affine.count(facet);}
  const std::vector<Id>& ExtensionComponents()const{return declared_components;}
  bool CompleteAnalytic(Id facet) const { return complete_analytic.count(Canonical(facet)); }
  struct ClosestSurface { Point point, normal, direction; double distance=0; };
  /*! Exact radial/finite-axis closest point on a certified complete sphere, cylinder or disk.
   * Partial charts and graph surfaces reject; normal is physically oriented, direction points towards the query. */
  ClosestSurface ClosestAnalytic(Id facet,Point p) const;
  /*! Projection must remain in the original finite chart; unsupported feature intersections reject. */
  Point Project(Id facet, Point p, KernelStats* stats = nullptr) const;
  bool OnProjectedEdge(Id facet, Point a, Point b, Point p, KernelStats* stats = nullptr) const;
  const Facet& Get(Id id) const;
  const std::map<Id,Facet>& Facets() const { return facets; }
  /*! Requested resident map/vector storage; allocator metadata and constructor scratch are separate. */
  size_t ResidentBytes() const;
  /*! Conservative reference construction peak, including the house local box index. */
  static size_t ConstructionBytes(size_t facets, size_t analytic = 0, size_t affine = 0);
  bool Contains(Id facet, Point p, KernelStats* stats = nullptr) const;
  /*! Complete closed chord containment on a finite faceted patch; analytic paths require their own chart. */
  bool ContainsSegment(Id facet,Point a,Point b,KernelStats* stats=nullptr)const;
  Id Canonical(Id facet) const;
  /*! Finite complete triangle containment and exact orientation relative to the original physical facet. */
  bool ContainsFace(Id facet, const std::array<Point, 3>& triangle, KernelStats* stats = nullptr) const;
  /*! Conservative physical distance between a planar triangle and its analytic radial/graph authority.
   * Zero for exact faceted authority. Includes nodal projection residual and arithmetic envelope. */
  double SurfaceError(Id facet, const std::array<Point,3>& triangle, KernelStats* stats = nullptr) const;
  /*! Lowest canonical same-marker patch containing the complete oriented triangle. Broad phase never admits geometry.
   * Missing/invalid geometry leaves facet unchanged; nonfinite query coordinates throw. */
  bool FindFace(int marker, const std::array<Point, 3>& triangle, Id& facet, KernelStats* stats = nullptr) const;
  /*! Additional candidate/traversal storage bound for the serial per-rank binding query. */
  size_t SearchWorkspaceBytes() const;

 private:
  std::map<Id, Facet> facets;
  const std::map<Id, AnalyticSurface> analytic;
  const std::map<Id,SU2NativeGeometry::AffineAuthority> affine;
  std::vector<std::shared_ptr<const SU2NativeGeometry::FinitePlaneReference>> affine_domains;
  std::map<Id,size_t> affine_charts;
  std::vector<Id> declared_components;
  std::map<Id, Id> canonical;
  std::set<Id> complete_analytic, nonconvex;
  std::map<Id, std::vector<Edge>> boundaries;
  std::map<Id,std::vector<std::array<Id,3>>> boundary_corners;
  std::map<Id, std::shared_ptr<const PlanarReference>> bump_charts;
  std::map<Id, Node> nodes;
  std::map<int, std::vector<Id>> marker_facets;
  std::vector<Id> indexed;
  std::shared_ptr<CADTElemClass> search;
};
struct FirstHeightReport {
  size_t wall_faces=0, height_failures=0, footprint_failures=0, missing_cells=0;
  size_t reference_clearance_failures=0;
  double minimum_reference_clearance_ratio=std::numeric_limits<double>::infinity();
  Id reference_clearance_facet=0,reference_clearance_cell=0;
  Point reference_clearance_apex{};
  double maximum_relative_error=0, maximum_tilt=0, minimum_barycentric=1./3;
  Id footprint_facet=0, footprint_cell=0;
  Id worst_facet=0, worst_cell=0;
  Point worst_apex{};
};
/*! Independent geometric first-row certificate on physical triangle children. Empty heights release
 *  the BL contract. The normal foot must lie within its wall triangle; centroid tilt is reported
 *  separately. Metric quality/lengths and global coverage are caller duties. */
bool CheckFirstHeight(const Patch& patch,const std::map<int,double>& heights,const PlanarReference& reference,
                      FirstHeightReport& report,std::string& reason,KernelStats* stats=nullptr);
/*! Insert prescribed-height wall points into their retained incident tetrahedra. All new identities/ranges
 * are caller-reserved; complete dependencies, target metric, external clearance and commit are separate.
 * An obstructed normal point needs coordinated reconstruction, never a relaxed height. Existing valid
 * wall children stay intact; output remains unchanged on rejection. */
bool ConstructFirstLayer(const Patch& old,Id first_wall_node,Id first_cell,
                         const std::map<int,double>& heights,const PlanarReference& reference,
                         Patch& fresh,FirstHeightReport& report,std::string& reason,KernelStats* stats=nullptr);

/*! Project a displacement onto incident planar boundary/first-height tangent freedoms.
 * Analytic surface projection, complete dependencies and final geometry/height checks remain caller duties. */
Node ConstrainMovePoint(const Patch& old,Id vertex,Node proposal,const std::map<int,double>& heights,
                        const PlanarReference& reference);
/*! Geometric private proposals only. Require the globally admitted complete edge/vertex star and authoritative
 * physical skin. Input point is exact on-edge binary64, or the actual rounded midpoint of a curved edge;
 * declared analytic physical facets project it before
 * rebuilding all children. Inherited child connectivity must remain positive; planar points never snap.
 * first_cell is a caller-reserved ID range. Distributed IDs, metric gates and external collision checks are separate.
 * Both mesh and physical-face output remain unchanged on rejection. */
bool SplitEdge(const Patch& old, Edge edge, Node point, Id first_cell, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats = nullptr);
/*! Inserts a representable point by coning a complete edge-star cavity, without exact on-edge placement.
 * Interior interface skin is unchanged. A surface seam can be rebuilt only when its two physical triangles
 * share one canonical finite chart; analytic surface points are projected before reconstruction; marker junctions and unadmitted interface faces reject. The point must see
 * every retained oriented skin face and lie in the original surface fan. Metric gates are separate.
 * Fresh output is unchanged on rejection; callers reserve identities and admit complete dependencies. */
bool InsertEdgeCavity(const Patch& old, Edge edge, Node point, Id first_cell, const PlanarReference& reference,
                      Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Inverse edge bisection: all removed-vertex incident cells must form paired children of the supplied endpoints.
 * Preserves original facets/markers and feature edges; this is not general boundary-vertex collapse. */
/*! Simultaneous bounded (at most eight) complete-star vertex moves and a physical surface split.
 * Physical splits retain their finite chart; interior insertions may leave the old diagonal only inside a complete edge star.
 * Empty movement retains all old coordinates. All child faces preserve chart orientation;
 * artificial interface vertices stay fixed;
 * moved identities are caller-reserved and each incident reference/marker feature is preserved.
 * Inherited volume topology is split atomically; actual metric/progress and nonlocal clearance are caller duties. */
bool SplitSurfaceStrip(const Patch& old,Edge edge,Node point,const std::vector<std::pair<Id,Node>>& moves,
                       Id first_cell,const PlanarReference& reference,Patch& fresh,std::string& reason,
                       KernelStats* stats = nullptr);
/*! Project a physical wall split and jointly refit its existing free apices to all new height planes.
 * Height planes and normal-foot half-spaces are fitted together; visibility closure may reconnect the
 * admitted transition and requests an obstructing artificial face when a dependency is missing.
 * Complete edge/apex stars are required. Stationary identities are retained; moved apices use consecutive
 * reserved IDs. Joint fits must pass all normal-foot, height and geometry checks; caller still
 * certifies the actual metric, strict size progress, nonlocal clearance and versioned dependencies.
 * Enclosed original points are restored; a compound repair need not increase tet count. Failure leaves fresh unchanged. */
bool RefitWallSplit(const Patch& old,Edge edge,Node point,Id first_apex,Id first_cell,
                    const std::map<int,double>& heights,const PlanarReference& reference,
                    Patch& fresh,Feature& obstruction,std::string& reason,KernelStats* stats = nullptr);
/*! Physical split and coordinated normal wall-point insertion into retained volume topology.
 * Visibility cavities grow only inside the complete admitted patch, across obstructing faces.
 * Completed wall children are protected; no common background point is required. Reserve one node
 * per requested physical wall face from first_wall_node. With positive surface_tolerance, also split
 * unresolved curved perimeter edges of the two incident wall faces before constructing normal points.
 * Perimeter points plus wall points share the128-node bound. Caller certifies actual metric/progress,
 * external clearance and versioned dependencies. Failed proposals leave fresh unchanged. */
bool ReconstructWallSplit(const Patch& old,Edge edge,Node point,Id first_wall_node,Id first_cell,
                          const std::map<int,double>& heights,const PlanarReference& reference,
                          Patch& fresh,Face& obstruction,std::string& reason,KernelStats* stats = nullptr,
                          double surface_tolerance = 0);
/*! Complete edge star plus imported face neighbours; joint projected surface split and interior reconnection.
 * Placement tensor is a proposal proxy only. Caller must certify actual-query metric, geometry progress,
 * nonlocal clearance and complete versioned dependencies. Requested wall children and transition are carved
 * together at prescribed normal height. Reserve interior.id through interior.id+1+wall-face count, including
 * the private background replacement. Obstruction names a retained interface for bounded growth. */
bool ReconstructSplitPatch(const Patch& old,Edge edge,Node point,Node interior,const Tensor& placement,
                           Id first_cell,const PlanarReference& reference,Patch& fresh,Face& obstruction,
                           std::string& reason,KernelStats* stats = nullptr,
                           const std::map<int,double>& heights = {});
class FrozenField;
/*! Tentative bounded cover of split cells by non-endpoint vertices; physical vertices are excluded by default.
 * Optional costs contain only globally admitted movable candidates, including smooth physical vertices; missing entries are unavailable.
 * Minimum total cost within eight apices; at most511 binary search states. Complete stars remain caller duties.
 * Empty means no bounded cover. */
std::vector<Id> SplitPatchApices(const Patch& patch,Edge edge,const std::map<Id,size_t>* costs=nullptr);
struct JointSplitReport {
  size_t trials=0, geometry_rejected=0, height_rejected=0, metric_rejected=0;
  size_t connectivity_trials=0, flips=0, moves=0;
  size_t linearizations=0, metric_probes=0, linear_iterations=0, solve_trials=0;
  Id blocked_interior_vertex=UINT64_MAX;
  double initial_loss=0, best_loss=0, best_shape_loss=0;
  Patch best;  // Geometric/height-valid private proposal, including failed shape; never an accepted mesh.
};
/*! Private compound replacement of shared first-height ownership using metric-scaled wall-patch directions.
 * Complete interior vertex stars, six new node IDs and replacement IDs must be globally reserved.
 * Intermediate cuts enforce positive fixed-interface topology; only the final patch is height/full-field
 * certified. Geometry, wall skin and original donors are unchanged. No output change on rejection. */
bool ReconstructHeightStar(const Patch& old,Id vertex,Id first_node,Id first_cell,FrozenField& target,
                           const PlanarReference& reference,const std::map<int,double>& heights,
                           Patch& fresh,JointSplitReport& report,std::string& reason,size_t maximum_trials=128);
/*! Bounded actual-field-directed placement of one split and up to eight complete-star surface/interior vertices.
 * Surface points retain their finite authority; the split may leave a same-chart edge. Coupled shape derivatives
 * coordinate all admitted freedoms. All trials reconstruct from the original patch; artificial interfaces
 * and first-height constraints remain fixed. Proposal scores use midpoint lengths only; success requires full composed Refine
 * validation. Placement reserves half of the shared trial bound for optional point-preserving private
 * reconnection; only the split and admitted apices may move, using their reserved identities.
 * Output is unchanged on failure; distributed admission and external clearance are caller duties. */
bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std::pair<Id,Id>>& apices,Id first_cell,
                       FrozenField& target,const PlanarReference& reference,const std::map<int,double>& heights,
                       Patch& fresh,JointSplitReport& report,std::string& reason,size_t maximum_trials=128);
constexpr size_t MetricPatchSolveScratchBytes = MaximumReplacementCells * 8192;
struct ConnectivityRepairReport {
  size_t trials=0, flips=0, moves=0, removals=0, coordinated=0;
  size_t linearizations=0, metric_probes=0, linear_iterations=0, solve_trials=0, solve_rejected=0;
  size_t move_geometry_rejected=0, move_height_rejected=0, move_loss_rejected=0;
  double before_loss=0, after_loss=0;
  bool complete_shape=false;
  Id blocked_interior_vertex=UINT64_MAX;  // Actual-field deficit at an artificial interface; dependency hint, not infeasibility.
};
/*! Up to128 constructed private cells; bounded two/three/four-tet flips, unconstrained interior removal and movement in the actual frozen field. Returns a strictly
 * improved private replacement, possibly still inadmissible; unchanged output on failure.
 * Fixed interface coordinates and requested first height/footprints remain mandatory. Caller admits
 * the complete original dependencies and validates full composed edges/progress before publication.
 * Disable retirement when backtracking a shape repair that destroys frozen size resolution.
 * An optional movable set restricts point displacement/removal, without restricting fixed-skin flips. */
bool RepairMetricConnectivity(const Patch& old,FrozenField& target,const PlanarReference& reference,
                              const std::map<int,double>& heights,Id first_cell,Patch& fresh,
                              ConnectivityRepairReport& report,std::string& reason,size_t maximum_trials=64,bool retire_points=true,const std::set<Id>* movable=nullptr);
bool UndoEdgeSplit(const Patch& old, Id point, Edge endpoints, Id first_cell, const PlanarReference& reference,
                   Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Contracts a vertex to an existing neighbor in its complete admitted vertex star, without split history.
 * Boundary motion must lie on every incident finite reference patch; corners/junctions cannot drift.
 * Positive cells and oriented physical faces preserve the embedded domain through the contraction homotopy.
 * Metric/quality gates and global dependency admission remain caller requirements. Fresh cells follow old
 * cell order excluding cells containing both endpoints. Rejection leaves both output vectors unchanged. */
bool CollapseVertex(const Patch& old, Id point, Id neighbor, Id first_cell, const PlanarReference& reference,
                    Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Moves one complete vertex star, replacing its vertex by a caller-reserved distinct identity.
 * Vertex/cell counts are unchanged. Analytic boundary proposals are projected to their incident chart;
 * positive endpoint determinants and chart-oriented faces are required; global clearance remains a caller duty;
 * exact planar motion preserves its domain; interface-star omissions, corner drift and inversion reject atomically.
 * New identity permits surface-coordinate changes without weakening fixed-interface coordinate checks. */
bool MoveVertex(const Patch& old, Id vertex, Node point, Id first_cell, const PlanarReference& reference, Patch& fresh,
                std::string& reason, KernelStats* stats = nullptr);
/*! Fixed-skin reconnection to an existing admitted apex (including face2->3 and edge3->2/4->4).
 * Geometry/topology only: frozen metric progress, complete dependencies and global IDs remain caller duties. */
bool Reconnect(const Patch& old, Id vertex, Id first_cell, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats = nullptr);
}  // namespace SU2Native3D
