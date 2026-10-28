/*!
 * \file CNativeGeometryBackend3D.hpp
 * \brief Immutable geometry queries; whole-patch/source certificates remain backend-specific.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeAffineReference.hpp"
#include <optional>

namespace SU2NativeGeometry {
enum class GeometryBackendKind {NativeMapped};
struct GeometryBackendIdentity {
  GeometryBackendKind kind=GeometryBackendKind::NativeMapped;
  uint64_t version=0,association_schema=1;
  const void* declaration=nullptr; // In-process cache token only; never serialized or compared across restart.
};
struct GeometryEvaluation {
  Point point{},normal{},tangent{},direction1{},direction2{};
  double curvature1=0,curvature2=0,realization_error=0;
  bool differential_available=false,tangent_available=false; // Local evaluations, not curvature/embedding certificates.
};
struct GeometrySupport {
  Id body=UINT64_MAX,surface=UINT64_MAX,sheet=UINT64_MAX,feature=UINT64_MAX;
  unsigned mobility=0; // Physical dimension; SurfaceBinding::dimension remains a parameter-support dimension.
  int marker=0;
};
struct GeometryProjectionScope {
  Id body=UINT64_MAX,hint=UINT64_MAX;
  SurfaceBinding previous;
  std::optional<Id> sheet;
  std::optional<std::pair<unsigned,Id>> feature; // Physical dimension and policy feature identity.
  double absolute_error=0; // Model-search tolerance; final whole-surface acceptance is independent.
};
struct GeometryProjection {
  SurfaceBinding binding;
  GeometryEvaluation evaluation;
  Id surface=UINT64_MAX;
  double distance_lower=0,distance_upper=0; // Conservative closest-distance enclosure; numerical feet need not give tight bounds.
};
enum class GeometryErrorDirection { Both, ModelToMesh };
struct GeometryRegionBounds {std::array<Point,2> bounds;double maximum_error=0;};
class GeometryBackend3D {
 public:
  virtual ~GeometryBackend3D()=default;
  /*! Metadata/token do not establish model equality: persistence/agreement retains the full authority declaration. */
  virtual GeometryBackendIdentity Identity()const=0;
  /*! null side requests position only; a crease normal/curvature requires an explicit incident surface. */
  virtual bool Evaluate(const SurfaceBinding&,std::optional<Id> side,GeometryEvaluation&,AtlasBudget&,std::string&)const=0;
  virtual bool AssociationValid(const SurfaceBinding&,Point,std::string&)const=0;
  virtual bool Support(const SurfaceBinding&,Id side,GeometrySupport&,AtlasBudget&,std::string&)const=0;
  /*! Necessary body/feature incidence only; false allowed is a valid veto, not a query failure. */
  virtual bool ContractionAllowed(const SurfaceBinding&,const SurfaceBinding&,bool& allowed,AtlasBudget&,std::string&)const=0;
  /*! Numerical constrained model foot; no globally optimal distance or uniqueness claim. */
  virtual bool Project(Point,const GeometryProjectionScope&,GeometryProjection&,AtlasBudget&,std::string&)const=0;
  /*! Bounds the supplied piece union; finite coverage/embedding admission remains the specialized caller's duty. */
  virtual bool RegionErrorBounds(const FaceCoverage&,const std::array<Point,3>&,GeometryRegionBounds&,AtlasBudget&,std::string&)const=0;
  /*! Complete physical mesh of one semantic model body/sheet/marker, without face partitions.
   * ModelToMesh supplies source enclosure only; within_tolerance requires Both. */
  virtual bool PatchErrorBounds(const GeometrySupport&,const std::vector<std::array<Point,3>>&,double tolerance,
      MappedFacetReference::SurfaceUnionBounds&,AtlasBudget&,std::string&,GeometryErrorDirection=GeometryErrorDirection::Both)const=0;
};
/*! Shared immutable cubic/faceted authority; validated parameter-state policy is borrowed for the stage's lifetime.
 * Existing v5-v8 declarations/SurfaceBinding encoding remain the persistent source of truth.
 * Projection searches only facets on the requested body and semantic sheet. */
class NativeMappedGeometryBackend3D final:public GeometryBackend3D {
 public:
  NativeMappedGeometryBackend3D(std::shared_ptr<const MappedFacetReference>,const MobilityPolicy&);
  GeometryBackendIdentity Identity()const override;
  bool Evaluate(const SurfaceBinding&,std::optional<Id>,GeometryEvaluation&,AtlasBudget&,std::string&)const override;
  bool AssociationValid(const SurfaceBinding&,Point,std::string&)const override;
  bool Support(const SurfaceBinding&,Id,GeometrySupport&,AtlasBudget&,std::string&)const override;
  bool ContractionAllowed(const SurfaceBinding&,const SurfaceBinding&,bool&,AtlasBudget&,std::string&)const override;
  bool Project(Point,const GeometryProjectionScope&,GeometryProjection&,AtlasBudget&,std::string&)const override;
  bool RegionErrorBounds(const FaceCoverage&,const std::array<Point,3>&,GeometryRegionBounds&,AtlasBudget&,std::string&)const override;
  bool PatchErrorBounds(const GeometrySupport&,const std::vector<std::array<Point,3>>&,double,
      MappedFacetReference::SurfaceUnionBounds&,AtlasBudget&,std::string&,GeometryErrorDirection=GeometryErrorDirection::Both)const override;
 private:
  std::shared_ptr<const MappedFacetReference> authority;
  const MobilityPolicy& policy;
};
} // namespace SU2NativeGeometry
