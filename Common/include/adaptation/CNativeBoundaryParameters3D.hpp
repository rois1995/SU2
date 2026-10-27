/*!
 * \file CNativeBoundaryParameters3D.hpp
 * \brief Sparse boundary geometry records and staged, allocation-free publication.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeAffineReference.hpp"
#include <set>
#include <tuple>
namespace SU2Native3D {
struct Patch;
class Directory;
struct SurfaceFace {
  Face v;
  Id facet = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(v, facet);
  }
};

struct BoundaryPoint {
  Node node;
  SU2NativeGeometry::SurfaceBinding binding;
  template<class S>void Fields(S& s){s(node,binding);}
};
using BoundaryPoints=std::map<Id,BoundaryPoint>;
size_t BoundaryCoverageBytes(const SU2NativeGeometry::BoundaryCoverage&);
size_t BoundaryAtlasBytes(const SU2NativeGeometry::BoundaryCoverage&,const SU2NativeGeometry::MobilityPolicy&);
std::vector<char> EncodeBoundaryAtlas(const SU2NativeGeometry::BoundaryCoverage&,const SU2NativeGeometry::MobilityPolicy&,
                                    size_t ceiling=SIZE_MAX,size_t caller_bytes=0);
void DecodeBoundaryAtlas(const std::vector<char>&,SU2NativeGeometry::BoundaryCoverage&,SU2NativeGeometry::MobilityPolicy&,
                         size_t ceiling=SIZE_MAX,size_t caller_bytes=0);
SU2NativeGeometry::AtlasBudget BoundaryAtlasBudget(size_t encoded_bytes,size_t ceiling=SIZE_MAX,size_t caller_bytes=0);
struct BoundaryParameterEdit {
  BoundaryPoint point;
  uint64_t revision=0;
  bool insert=false;
  template<class S>void Fields(S& s){s(point,revision,insert);}
};
struct BoundaryFaceChange {
  Face nodes{};
  Id facet=0;
  bool insert=false;
  template<class S>void Fields(S& s){s(nodes,facet,insert);}
};
/*! Immutable point identities inside a publication epoch: movement replaces the identity.
 * Only boundary records are retained. Geometry/source values and volume nodes are separate. */
class BoundaryParameterState {
 public:
  struct Entry {
    BoundaryPoint point;
    uint64_t faces=0;
    template<class S>void Fields(S& s){s(point,faces);}
  };
  using Coordinate=std::array<double,3>;
  using Entries=std::map<Id,Entry>;
  using Support=std::tuple<unsigned,Id,double,double>;
  using FacetFaceIndex=std::set<std::pair<Id,Face>>;
  using FacetFaceRange=std::pair<FacetFaceIndex::const_iterator,FacetFaceIndex::const_iterator>;
  struct Publication {
    size_t ResidentBytes() const;
    Entries added;
    std::map<Coordinate,Id> coordinates;
    std::map<Support,Id> supports;
    std::set<Id> removed;
    std::map<Id,uint64_t> final_counts;
    std::map<Face,SurfaceFace> surface_added;
    std::set<Face> surface_removed;
    SU2NativeGeometry::BoundaryCoverage coverage_added;
    std::set<Face> coverage_removed;
    std::set<Edge> traces_removed;
    std::map<Edge,uint64_t> edge_counts;
    std::map<Edge,SurfaceFace> edge_incident;
    std::set<std::pair<Edge,Face>> edge_faces_added,edge_faces_removed;
    std::set<std::pair<Id,Face>> facet_faces_added,facet_faces_removed;
    uint64_t before=0,after=0;
    bool valid=false;
  };
  BoundaryParameterState(std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> geometry,
      const BoundaryPoints& points,const std::vector<SurfaceFace>& surface,
      uint64_t revision=1,size_t ceiling=SIZE_MAX,size_t caller_bytes=0,
      const SU2NativeGeometry::BoundaryCoverage& coverage={},const SU2NativeGeometry::MobilityPolicy& mobility={},SU2NativeGeometry::AtlasBudget* reservation=nullptr);
  const Entries& Records()const{return records;}
  const std::map<Face,SurfaceFace>& Surface()const{return surface;}
  const SU2NativeGeometry::BoundaryCoverage& Coverage()const{return coverage;}
  const SU2NativeGeometry::MobilityPolicy& Mobility()const{return mobility;}
  bool ResolveCoverage(const SurfaceFace&,SU2NativeGeometry::FaceCoverage&,SU2NativeGeometry::AtlasBudget&,std::string&)const;
  bool ResolveTrace(Edge,const SurfaceFace&,SU2NativeGeometry::EdgeTrace&,SU2NativeGeometry::AtlasBudget&,std::string&)const;
  const SurfaceFace* IncidentFace(Edge edge)const;
  std::array<const SurfaceFace*,2> IncidentFaces(Edge edge)const;
  /*! Borrowed views remain valid until Commit, assignment, replacement or destruction.
   * Component-mode facet queries conservatively return the complete canonical component. */
  FacetFaceRange FacesOnOriginalFacet(Id facet)const;
  const BoundaryPoint* Find(Id id)const;
  const BoundaryPoint* Find(Point point)const;
  const SU2NativeGeometry::MappedFacetReference& Geometry()const{return *geometry;}
  uint64_t Revision()const{return revision;}
  std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> Authority()const{return geometry;}
  /*! Metadata-only policy setup over this validated state; geometry, topology and revision stay unchanged. */
  std::shared_ptr<BoundaryParameterState> WithMobilityPolicy(double angle,const std::map<int,int>& roles,
      SU2NativeGeometry::AtlasBudget& reservation,size_t ceiling=SIZE_MAX,size_t caller_bytes=0)const;
  /*! Boundary-only transport, sharing already agreed immutable authority/indexes. */
  size_t PayloadBytes()const;
  std::vector<char> Payload()const;
  static std::shared_ptr<BoundaryParameterState> FromPayload(
      std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> geometry,const std::vector<char>& bytes,
      size_t ceiling=SIZE_MAX,size_t caller_bytes=0);
  /*! Preserve forward parameters/coordinates; only current mesh identities change. */
  std::shared_ptr<BoundaryParameterState> Rebind(const std::map<Id,Id>& numbering,
      size_t ceiling=SIZE_MAX,size_t caller_bytes=0)const;
  size_t ResidentBytes()const;
  /*! Canonical declaration/record bytes for collective agreement and snapshots, not a new sidecar. */
  std::vector<char> Snapshot()const;
  size_t SnapshotBytes()const;
  /*! Global boundary-only deltas; caller transports/admit counts collectively before invoking.
   * Prepares changed map nodes only, never copies the entire accepted table per transaction.
   * Old/new ideal facet boundaries must agree, including exact feature subdivisions.
   * Covers may update retained faces; changing a retained trace requires all accepted incident faces together.
   * Actual tetrahedra, embedding, metric/height and full-star reservations remain caller duties. */
  Publication Prepare(const std::vector<BoundaryParameterEdit>& edits,const std::vector<BoundaryFaceChange>& faces,
      std::string& reason,size_t ceiling=SIZE_MAX,size_t caller_bytes=0,
      const SU2NativeGeometry::BoundaryCoverage& coverage_delta={},SU2NativeGeometry::AtlasBudget* reservation=nullptr)const;
  static std::vector<BoundaryParameterEdit> Changes(const Patch& old,const Patch& fresh,uint64_t revision);
 private:
  friend class Directory;
  void Commit(Publication&& publication)noexcept;
  std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> geometry;
  Entries records;
  std::map<Face,SurfaceFace> surface;
  std::map<Coordinate,Id> coordinates;
  std::map<Support,Id> supports;
  SU2NativeGeometry::BoundaryCoverage coverage;
  SU2NativeGeometry::MobilityPolicy mobility;
  std::map<Edge,uint64_t> edge_counts;
  std::map<Edge,SurfaceFace> edge_incident;
  std::set<std::pair<Edge,Face>> edge_faces;
  FacetFaceIndex facet_faces;
  uint64_t revision;
};
} // namespace SU2Native3D
