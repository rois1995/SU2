/*!
 * \file CNativeAffineReference.hpp
 * \brief Finite geometry in audited affine charts, independent of boundary physics.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeCavity3D.hpp"
#include "CNativePointConstruction3D.hpp"
#include <map>
#include <memory>
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
/*! Explicit numerical mapping contract on immutable finite triangles.
 * shortcut: original edges remain protected; extend after feature/accuracy classification.
 * No near-coplanar merging, inferred body identity,
 * geometric inverse recovery or per-volume-node state. Strict facets have zero mapping freedom.
 * Caller must supply/persist boundary bindings and retain actual-volume/embedding/metric gates.
 * This class alone does not enable a mapped mesh in the production lifecycle. */
class MappedFacetReference {
 public:
  MappedFacetReference(std::shared_ptr<const SU2Native3D::PlanarReference> reference,
                       const std::map<Id,double>& mapping_errors,uint64_t version,size_t ceiling=SIZE_MAX);
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
  size_t ResidentBytes()const;
  uint64_t Version()const{return version;}
 private:
  std::shared_ptr<const SU2Native3D::PlanarReference> reference;
  std::map<Id,double> errors;
  uint64_t version;
  std::map<Id,Point> nodes;
  std::map<Id,std::vector<Id>> vertex_facets;
  std::vector<SU2Native3D::Edge> edges;
  std::vector<std::vector<Id>> edge_facets;
  void CheckBinding(const SurfaceBinding& binding)const;
  Id EdgeIdentity(SU2Native3D::Edge edge)const;
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
  bool Contains(Point p)const;
  bool ContainsSegment(Point a,Point b)const;
  bool ContainsFace(const std::array<Point,3>& face)const;
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
