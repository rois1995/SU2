/*!
 * \file CNativeAffineReference.hpp
 * \brief Finite geometry in audited affine charts, independent of boundary physics.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include <functional>
#include "CNativeCavity3D.hpp"
#include "CNativePointConstruction3D.hpp"
#include "CNativeAtlas3D.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace SU2Native3D { class PlanarReference; struct Facet; struct SurfaceFace; }

namespace SU2NativeGeometry {
using SU2Native3D::Point;
using SU2Native3D::Id;
/*! Immutable definition; identity/version belong in every dependent cache key.
 * Distance is a physical error contract, never a tolerance for volume predicates.
 * The error contract must be at least the smallest normal binary64 value. */
struct AffineAuthority {
  Id identity=0;
  uint64_t version=1;
  unsigned dimension=3;
  Point origin{}, normal{0,0,1};
  double maximum_error=0;
  template<class S>void Fields(S& s){s(identity,version,dimension,origin,normal,maximum_error);}
};
/*! Drop the dominant normal coordinate: chart coordinates retain original binary64 bits.
 * Strict-arithmetic lifting audits an outward residual bound; exact global coplanarity is unnecessary.
 * Rejects scales/errors not representable by the host, rather than increasing the contract. */
class AffineChart {
 public:
  explicit AffineChart(AffineAuthority authority);
  const AffineAuthority& Authority()const{return authority;}
  Point Coordinates(Point p)const;
  Point Lift(Point chart)const;
  Point Project(Point p)const;
  double ErrorBound(Point p)const;
 private:
  AffineAuthority authority;
  unsigned dependent=0;
};
/*! Audited L1 displacement upper bound (also bounds Euclidean displacement).
 * Uses strict outward arithmetic; equal coordinates return zero. */
double DisplacementBound(Point a,Point b);
/*! Idempotent simultaneous projection onto one to three independent affine constraints.
 * Actual mapped points must satisfy every declared error contract. Dependent or ill-conditioned
 * unequal declarations refuse explicitly; no sequential projection or tolerance enlargement. */
Point ProjectIntersection(Point point,const std::vector<AffineAuthority>& constraints);
/*! Proposed shared original-source column, not a geometry/CFD vertex projection.
 * Oriented physical faces meet at origin. A bounded normal-cone active-set search proposes
 * a four-radius shell; callers must certify actual positivity, conformity and coverage.
 * Work is accumulated across vertices; a missing/ill-conditioned cone or exhaustion refuses. */
Point SourceColumn(Point origin,const std::vector<std::array<Point,3>>& faces,double radius,
                   uint64_t& work,uint64_t maximum_work);
/*! Sufficient outward-rounded separation certificate for two finite triangles.
 * False includes uncertainty. No vertex-only membership or tolerance enlargement. */
bool SourceFacesSeparated(const std::array<Point,3>& a,const std::array<Point,3>& b,double distance);
/*! Boundary-only latent geometry; version and support identity survive coordinate rounding.
 * dimension0: original vertex; dimension1: canonical original edge index and u;
 * dimension2: original oriented facet and u,v, with dependent coefficient exactly1-u-v.
 * These are geometry parameters, never source-metric interpolation weights. */
struct SurfaceBinding {
  Id feature=0;
  uint64_t version=0;
  unsigned dimension=3;
  double u=0,v=0;
  template<class S>void Fields(S& s){s(feature,version,dimension,u,v);}
};
struct MappedSurfaceAudit {
  size_t nodes=0,faces=0,feature_segments=0,requested_bytes=0;
  double maximum_mapping_error=0;
};
/*! Geometry policy, independent of whether a BL metric is requested. Edge keys use original IDs.
 * roles: 0 reflective carrier, 2 other physical boundary; marker interfaces remain protected.
 * Unresolved whole-edge normal comparisons are protected, never classified from samples. */
struct MobilityPolicy {
  double angle=0;
  uint64_t version=1,identity=0;
  std::map<int,int> roles;
  std::map<SU2Native3D::Edge,Id> features; // Protected edge -> canonical curve chain.
  std::set<Id> corners;
  std::map<Id,Id> bodies,sheets;          // Original facet -> edge-connected root.
};
struct RegionSegment {Id facet=0;ExactBary first,last;};
/*! Explicit numerical mapping contract on immutable finite triangles.
 * Facet-local policy protects original edges. Reference-component policy releases only
 * the immutable reference's certified planar seams, preserving physical border chains.
 * No near-coplanar merging, inferred body identity,
 * geometric inverse recovery or per-volume-node state. Strict facets have zero mapping freedom.
 * Caller must supply/persist boundary bindings and retain actual-volume/embedding/metric gates.
 * This class alone does not enable a mapped mesh in the production lifecycle. */
class MappedFacetReference {
 public:
  /*! Cubic Bernstein order: 300,030,003,210,120,021,012,102,201,111.
   * Controls are immutable binary64 geometry, not a recipe to rebuild on restart. */
  using CubicNet=std::array<Point,10>;
  using CurvedNets=std::map<Id,CubicNet>;
  MappedFacetReference(std::shared_ptr<const SU2Native3D::PlanarReference> reference,
                       const std::map<Id,double>& mapping_errors,uint64_t version,size_t ceiling=SIZE_MAX,
                       bool reference_components=false,const CurvedNets& curved_nets={},size_t query_cache_bytes=SIZE_MAX);
  /*! Fixed-geometry cache quota from separately available storage; no allocation or working-mesh history. */
  static size_t SurfaceQueryStorageBytes(size_t curved_facets,size_t available);
  const CurvedNets& Curves()const{return curves;}
  struct CurvedClosest {
    Id facet=0;
    SurfaceBinding binding;
    Point point{},normal{};
    double lower=0,upper=0;
    /*! Lower bound on -dot(query-surface, outward unit normal) at every possible nearest point.
     * Positive certifies fluid-side normal clearance; nonpositive includes unresolved sign. */
    double clearance_lower=0;
    /*! Local differential geometry at the returned binding; these are not global curvature bounds. */
    double curvature1=0,curvature2=0;
    Point direction1{},direction2{};
    size_t work=0;
  };
  struct SurfaceEvaluation {
    Point point{},normal{},first{},second{},direction1{},direction2{};
    double curvature1=0,curvature2=0;
  };
  /*! Forward sided differential geometry, shared with ClosestCurved; never performs a closest-point search. */
  SurfaceEvaluation EvaluateSurface(Id side,const SurfaceBinding&)const;
  /*! Outward interval proof of physical-face orientation at an incident model association. */
  bool OrientedFace(Id side,const SurfaceBinding&,const std::array<Point,3>&)const;
  SU2Native3D::Edge OriginalEdge(Id feature)const{return edges.at(feature);}
  Id Body(Id facet)const{return body_roots.at(facet);}
  /*! Closest distance to the edge-connected, same-marker curved component containing hint.
   * Outward control-hull subdivision certifies [lower,upper] to the requested absolute error.
   * The returned binding/normal describe the candidate point, not a certified unique minimizer.
   * Ambiguous accuracy or resource exhaustion throws; no other body/marker is searched. */
  CurvedClosest ClosestCurved(Id hint,Point query,double absolute_error,size_t maximum_work,
                              size_t ceiling=SIZE_MAX, const std::function<bool(Id)>& allowed_facets={},
                              size_t* attempted_work=nullptr)const;
  /*! Fixed-model broad phase within the hinted body/marker; candidates are not associations. */
  std::vector<Id> NearbySurfaceFacets(Id hint,const std::array<Point,2>& box)const;
  static constexpr size_t ClearanceWork=16384;
  /*! Additional scratch bound for one active query; immutable authority storage is separate. */
  size_t CurvedQueryWorkspaceBytes(size_t maximum_work)const;
  struct SurfaceUnionBounds {
    double model_to_mesh=0,mesh_to_model=0;
    size_t model_pieces=0,physical_pieces=0;
    bool within_tolerance=false,outside_tolerance=false;
  };
  /*! Bounds both directed Hausdorff distances between the supplied complete unions.
   * No face correspondence is assumed. A successful query certifies the tolerance or supplies an outside witness.
   * Topology, feature incidence and body separation require independent admission. */
  SurfaceUnionBounds BoundSurfaceUnion(const std::vector<Id>& model_facets,
      const std::vector<std::array<Point,3>>& physical,double tolerance,AtlasBudget&)const;
  /*! Model enclosure for source tubes; does not certify the reverse distance or final tolerance. */
  SurfaceUnionBounds BoundModelSurface(const std::vector<Id>& model_facets,
      const std::vector<std::array<Point,3>>& physical,double tolerance,AtlasBudget&)const;
  /*! One-sided bound for a physical patch against a model union; omitted supports only weaken admission. */
  double BoundPhysicalSurface(const std::vector<Id>& model_facets,
      const std::vector<std::array<Point,3>>& physical,double tolerance,AtlasBudget&)const;
  /*! Convex hull of an immutable curved patch and current source face, including realization error. */
  class SourcePatch {
   public:
    /*! Tube of a physical triangle. Caller must certify model containment in the union of tubes. */
    static SourcePatch Tube(const std::array<Point,3>& source,double radius);
    std::array<Point,2> Bounds()const;
    double OutwardDisplacement()const;
    bool Separated(const std::array<Point,3>& outer,uint64_t& work,uint64_t maximum_work)const;
    static size_t WorkspaceBytes();
    size_t ResidentBytes()const;
   private:
    friend class MappedFacetReference;
    SourcePatch()=default;
    struct Piece {
      std::array<Point,10> lower{},upper{};
      std::array<Point,3> source_lower{},source_upper{};
      double margin=0;
    };
    std::vector<Piece> pieces;
    std::array<Point,3> source{};
  };
  SourcePatch PrepareSourcePatch(Id facet,const std::array<SurfaceBinding,3>& binding,
                                 const std::array<Point,3>& source)const;
  /*! Additional resident/preparation storage, admitted before constructing restricted pieces. */
  static size_t SourcePreparationBytes(const FaceCoverage& coverage);
  static size_t SourcePreparationBytes(size_t piece_count);
  SourcePatch PrepareSourcePatch(const FaceCoverage&,const std::array<Point,3>& source,AtlasBudget&)const;
  double SurfaceError(const FaceCoverage&,const std::array<Point,3>& realized,AtlasBudget&)const;
  bool ResolveSingleChart(Id facet,const std::array<SurfaceBinding,3>& binding,FaceCoverage&,
                          AtlasBudget&,std::string& reason)const;
  bool BindingParameters(Id facet,const SurfaceBinding&,ExactBary&,AtlasBudget&,std::string& reason)const;
  bool ReferencePointKey(Id facet,const ExactBary&,std::vector<uint64_t>&,AtlasBudget&,std::string& reason)const;
  /*! Canonical point key in a supporting original chart; unsupported returns false with empty reason. */
  bool ReferenceParameters(Id facet,const std::vector<uint64_t>&,ExactBary&,AtlasBudget&,std::string& reason)const;
  bool CoverageTraces(const FaceCoverage&,std::array<EdgeTrace,3>&,AtlasBudget&,std::string& reason)const;
  /*! False reports malformed input/resource refusal and leaves equal unchanged. */
  bool CompareTraces(const EdgeTrace&,const EdgeTrace&,bool& equal,AtlasBudget&,std::string& reason)const;
  bool EqualTraces(const EdgeTrace&,const EdgeTrace&,AtlasBudget&,std::string& reason)const;
  /*! Traces are oriented 0->1,1->2,2->0; current-ID canonicalization belongs to the parameter layer. */
  bool ValidateCoverage(Id anchor,const std::array<SurfaceBinding,3>& binding,const std::array<Point,3>& realized,
                        const FaceCoverage&,const std::array<EdgeTrace,3>&,AtlasBudget&,std::string& reason)const;
  /*! Exact one-dimensional genuine-curve coverage; directed=false compares unsigned arc multiplicity. */
  bool ValidateFeatureReplacement(const std::vector<std::pair<const EdgeTrace*,int>>& uses,
      const MobilityPolicy&,bool directed,AtlasBudget&,std::string&)const;
  bool ValidateCoverageReplacement(const std::vector<FaceCoverage>& old,const std::vector<FaceCoverage>& fresh,
                                   AtlasBudget&,std::string& reason)const;
  /*! Exact per-original-chart oriented region boundary, including holes/components.
   * Mandatory canonical ReferencePointKeys retain current bindings and retained-trace parameter breaks. */
  bool RegionBoundary(const std::vector<CoveragePiece>& accepted_region,
                      const std::set<std::vector<uint64_t>>& mandatory_reference_keys,
                      std::vector<RegionSegment>&,AtlasBudget&,std::string& reason)const;
  /*! Sufficient real C0 surface embedding certificate for a complete reference disk.
   * The supplied normal proposes a general projection basis; all actual basis arithmetic is enclosed.
   * Boundary arcs, chart gluing and vertex fan are certified, never replaced by corner chords. */
  bool CertifyEmbedding(const std::vector<CoveragePiece>& region,Point normal,AtlasBudget&,
                        std::string& reason)const;
  bool BuildMobilityPolicy(double angle,const std::map<int,int>& roles,MobilityPolicy&,
                           AtlasBudget&,std::string& reason)const;
  unsigned MobilityDimension(const SurfaceBinding&,const MobilityPolicy&)const;
  /*! Physical displacement from the original flat facet, including realization error. */
  double EnvelopeError(Id facet)const;
  /*! Outward bound between a flat realized child and its curved parameter patch. */
  double SurfaceError(Id facet,const std::array<SurfaceBinding,3>& binding,
                      const std::array<Point,3>& realized)const;
  bool ReferenceComponents()const{return bool(components);}
  Id Canonical(Id facet)const;
  const std::vector<Id>& ComponentFacets(Id facet)const;
  const std::vector<SU2Native3D::Edge>& ComponentBorder(Id facet)const;
  unsigned ComponentAxis(Id facet)const;
  std::vector<Id> CandidateFacets(Id facet,Point proposal)const;
  bool PhysicalSupport(const SurfaceBinding& binding)const;
  bool ContainsSegment(Id facet,const SurfaceBinding& a,const SurfaceBinding& b,SU2Native3D::KernelStats* stats=nullptr)const;
  SurfaceBinding Vertex(Id vertex)const;
  /*! Input endpoints must already be in canonical original-ID order; no rounded1-t conversion. */
  SurfaceBinding EdgePoint(SU2Native3D::Edge original_edge,double t)const;
  /*! Strict facet interior only; edge/corner parameters must use their shared support. */
  SurfaceBinding FacetPoint(Id facet,double u,double v)const;
  SU2Native3D::ParametricPoint Ideal(const SurfaceBinding& binding)const;
  double MaximumError(const SurfaceBinding& binding)const;
  double ErrorBound(const SurfaceBinding& binding,Point realized)const;
  Point Realize(const SurfaceBinding& binding)const;
  bool Supports(Id facet,const SurfaceBinding& binding)const;
  bool ContainsFace(Id facet,const std::array<SurfaceBinding,3>& binding,
                    const std::array<Point,3>& realized,SU2Native3D::KernelStats* stats=nullptr)const;
  /*! Complete ideal-facet tiling/feature chains and closed actual surface topology.
   * Does not establish actual embedding or tetrahedron positivity. Boundary-only work/storage
   * is admitted before preparation; report and accepted state stay unchanged on refusal. */
  bool ValidateSurface(const std::vector<SU2Native3D::SurfaceFace>& surface,
                       const std::map<Id,SurfaceBinding>& bindings,const std::map<Id,Point>& realized,
                       MappedSurfaceAudit& report,std::string& reason,size_t maximum_faces,size_t ceiling=SIZE_MAX)const;
  template<class F>void ForFacets(const SurfaceBinding& b,F use)const {
    CheckBinding(b);if(b.dimension==2){use(b.feature);return;}
    const auto& rows=b.dimension==0?vertex_facets.at(b.feature):edge_facets.at(b.feature);for(Id id:rows)use(id);
  }
  bool Admissible(const SurfaceBinding& binding,Point realized)const;
  bool FeatureSegment(const SurfaceBinding& a,const SurfaceBinding& b,Id& edge,double& ta,double& tb)const;
  bool BoundarySegment(Id facet,const SurfaceBinding& a,const SurfaceBinding& b,Id& edge,double& ta,double& tb)const;
  const SU2Native3D::PlanarReference& Reference()const{return *reference;}
  const std::map<Id,double>& Errors()const{return errors;}
  size_t ResidentBytes()const;
  uint64_t Version()const{return version;}
 private:
  SurfaceUnionBounds QuerySurfaceUnion(const std::vector<Id>&,
      const std::vector<std::array<Point,3>>&,double,AtlasBudget&,bool include_model,bool include_mesh)const;
  SurfaceUnionBounds QuerySurfaceUnionAtResolution(const std::vector<Id>&,
      const std::vector<std::array<Point,3>>&,double,AtlasBudget&,bool include_model,bool include_mesh,double resolution)const;

  std::shared_ptr<const SU2Native3D::PlanarReference> reference;
  std::map<Id,double> errors;
  CurvedNets curves;
  uint64_t version;
  std::map<Id,Point> nodes;
  std::map<Id,std::vector<Id>> vertex_facets;
  std::map<Id,Id> body_roots;
  std::vector<SU2Native3D::Edge> edges;
  std::vector<std::vector<Id>> edge_facets;
  struct ComponentData;
  std::shared_ptr<const ComponentData> components;
  struct CurvedSearch;
  std::shared_ptr<CurvedSearch> curved_search;
  /*! Memo of component containment or curved surface error, keyed by immutable geometry, bitwise bindings and
   *  realized points. Containment statistics bypass it; full bounded storage is admitted before use. */
  struct FaceMemo;
  std::shared_ptr<FaceMemo> face_memo;
  static size_t FaceMemoBytes();
  double ComponentPointError(const SurfaceBinding& binding,Point realized)const;
  bool ComponentContainsFace(Id facet,const std::array<SurfaceBinding,3>& binding,
                             const std::array<Point,3>& realized,SU2Native3D::KernelStats* stats)const;
  void CheckBinding(const SurfaceBinding& binding)const;
  Id EdgeIdentity(SU2Native3D::Edge edge)const;
  SU2Native3D::ParametricPoint FacetParameters(Id facet,const SurfaceBinding& binding)const;
  std::array<Point,2> CurvedBounds(const SurfaceBinding& binding)const;
};
struct LineEdge {Id identity=0;SU2Native3D::Node a,b;};
/*! A union of directed connected intervals, including multiple body-footprint gaps.
 * Endpoints retain input feature IDs. Coincident disconnected endpoints and overlaps reject. */
class FiniteLineReference {
 public:
  FiniteLineReference(AffineAuthority authority,const std::vector<LineEdge>& edges,size_t ceiling=SIZE_MAX);
  const AffineChart& Chart()const{return chart;}
  bool Contains(Point p)const;
  bool ContainsSegment(Point a,Point b)const;
  Point Project(Point p)const;
  std::vector<Id> FeaturesIntersecting(Point a,Point b)const;
  size_t Components()const{return intervals.size();}
  size_t ResidentBytes()const;
 private:
  struct Interval {double lo,hi;Id component,first,last;};
  AffineChart chart;
  std::vector<Interval> intervals;
};
/*! Audited plane plus the existing finite planar kernel in exact chart coordinates.
 * Oriented outer/inner loops and disconnected regions are prepared once. Input facets
 * must form a conforming single-cover triangulation with straight footprint edges.
 * Finite topology is checked; analytic body-intersection curves need a separate authority.
 * The shared 3D backend carries these domains; arbitrary initial adoption remains separate. */
class FinitePlaneReference {
 public:
  FinitePlaneReference(AffineAuthority authority,const std::vector<SU2Native3D::Facet>& facets,size_t ceiling=SIZE_MAX);
  const AffineChart& Chart()const{return chart;}
  Id Component(Id facet)const;
  bool Contains(Id facet,Point p,SU2Native3D::KernelStats* stats=nullptr)const;
  bool ContainsSegment(Id facet,Point a,Point b,SU2Native3D::KernelStats* stats=nullptr)const;
  bool ContainsFace(Id facet,const std::array<Point,3>& face,SU2Native3D::KernelStats* stats=nullptr)const;
  bool Contains(Point p,SU2Native3D::KernelStats* stats=nullptr)const;
  bool ContainsSegment(Point a,Point b,SU2Native3D::KernelStats* stats=nullptr)const;
  bool ContainsFace(const std::array<Point,3>& face,SU2Native3D::KernelStats* stats=nullptr)const;
  Point Project(Point p)const;
  std::vector<SU2Native3D::Edge> FeaturesIntersecting(Point a,Point b)const;
  size_t Components()const{return components.size();}
  size_t ResidentBytes()const;
 private:
  AffineChart chart;
  std::shared_ptr<const SU2Native3D::PlanarReference> reference;
  std::vector<Id> components;
  std::vector<SU2Native3D::Edge> borders;
  std::map<Id,Point> nodes;
};
} // namespace SU2NativeGeometry
