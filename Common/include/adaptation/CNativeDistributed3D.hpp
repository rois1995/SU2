/*!
 * \file CNativeDistributed3D.hpp
 * \brief Complete tetrahedral dependency metadata and atomic versioned publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeBoundary3D.hpp"
#include "CNativeDistributed2D.hpp"

namespace SU2Native3D {
// Reuse passive scalar serialization, bounded transport and worker-communicator failure election unchanged.
using World = SU2NativeBoundary2D::World;
using RecordStream = SU2NativeBoundary2D::RecordStream;
struct FacetBinding {
  bool present = false;
  Id facet = 0;
  template <class S> void Fields(S& s) { s(present, facet); }
};
struct OwnedCell {
  Cell cell;
  uint64_t version = 0;
  std::array<FacetBinding, 4> physical{};  // Opposite vertex slots; reference owns marker/geometry.
  std::array<Tensor, 4> sensor{};
  bool sensor_present = false;
  template <class S> void Fields(S& s) {
    uint64_t format = 0x4e334443454c4c02ULL;
    s(format);
    if (format != 0x4e334443454c4c02ULL) throw std::invalid_argument("Unsupported native3D cell payload version.");
    s(cell, version, physical, sensor_present, sensor);
  }
};
using Owned = std::map<Id, OwnedCell>;
struct Dependency {
  Id cell = 0;
  uint64_t version = 0;
  int owner = 0;
  template <class S> void Fields(S& s) { s(cell, version, owner); }
};
struct Dependencies {
  std::vector<Dependency> cells;
  bool valid = false;
  bool oversized = false;  // Batch refusal of a complete star above the cavity bound; a block, not a protocol failure.
  std::string reason;
  Feature coordinate_star{};  // Complete-vertex movement only; topology cavities retain their bound.
};
struct DependencyBatch {
  std::vector<Dependencies> candidates;
  bool valid = false;  // Common storage/protocol success; each candidate still requires its own valid flag.
  std::vector<size_t> pass_deferred;  // Original candidate slots touching an accepted cavity in this construction pass.
  std::vector<size_t> refused;  // PlanBatch: input indexes refused for an oversized complete star (blocked by caller).
  std::string reason;
};
struct Imported {
  std::vector<OwnedCell> cells;
  bool valid = false;
  std::string reason;
  Feature coordinate_star{};
};
struct ImportedBatch {
  std::vector<Imported> candidates;
  bool valid = false;
  std::string reason;
};
struct ReservationBatch {
  std::vector<bool> chosen,pass_deferred;
  bool valid = false;
  std::string reason;
};
struct DirectoryStats {
  uint64_t builds = 0, lookups = 0, fetches = 0, reservations = 0, publications = 0, rejected = 0;
  uint64_t metadata_scanned = 0, payloads = 0, elections = 0;
  uint64_t collision_builds=0,collision_candidates=0,collision_checks=0;
  double collision_seconds=0;
  uint64_t change_scanned = 0, change_sent = 0, change_received = 0;
  double change_seconds = 0;  // Nested in publication; only affected-star owner notifications.
  size_t maximum_dependencies = 0, maximum_work_bytes = 0;
  double build_seconds = 0, lookup_seconds = 0, fetch_seconds = 0, reservation_seconds = 0, publication_seconds = 0;
  double election_seconds = 0;  // Nested detail; not added to the exclusive public-call phases.
  double parameter_agreement_seconds=0,parameter_publication_seconds=0; // Nested, boundary-only geometry.
  uint64_t parameter_edits=0,parameter_faces=0,parameter_payload_bytes=0; // Logical replicated payload, not wire/RSS.
};
constexpr size_t MaximumDirectoryFeatures = 4 * MaximumCavityCells;
constexpr size_t MaximumDirectoryScan = 4096;
/*! Metadata completeness/version/storage protocol, not geometric or metric approval.
 * All ranks of world participate in matching calls. Accepted source ownership must cover every global cell.
 * Vertex records hold full simplex IDs, so edge/face queries filter an authoritative complete star.
 * caller_bytes includes other live caller buffers; byte admission models requested storage, not process RSS.
 * Geometry/embedding/marker/metric certificates and nonconflicting cavity selection remain caller duties. */
class Directory {
 public:
  explicit Directory(World& world, bool track_changes = false,BoundaryParameterState* parameters=nullptr);
  ~Directory();
  // Initial construction only; create a new directory after ownership migration.
  bool Build(const Owned& owned, std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  Dependencies Lookup(const std::vector<Feature>& features, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! One complete star per feature, in input order, sharing the same exchanges. Missing/oversized
   * stars refuse only their candidate; protocol/storage failure refuses the whole batch without mutation. */
  DependencyBatch LookupBatch(const std::vector<Feature>& features, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0,
                              bool coordinate_stars = false);
  /*! Complete-star metadata selection before payload admission. slots maps compacted candidates
   * to input features; deferred candidates are not failures or reservations. Oversized complete stars are
   * listed in refused (never selected); a missing feature still refuses the batch. Final Fetch/Reserve
   * still verify versions and ownership. Selection must be byte/work/time bounded by its caller. */
  DependencyBatch PlanBatch(const std::vector<Feature>& features, std::vector<size_t>& slots,
                           size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  Imported Fetch(const Dependencies& dependencies, const Owned& owned, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! Shared cells are fetched once; a stale payload refuses every candidate that requires it.
   * Common protocol/storage failures refuse the batch. Neither method mutates the accepted stores. */
  ImportedBatch FetchBatch(const DependencyBatch& dependencies, const Owned& owned,
                          size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  ReservationBatch ReserveBatch(const ImportedBatch& imported, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0,
                                const std::vector<unsigned>& priorities = {});
  /*! Collective cell-only reservations for one-cell interior replacements. All callers must use this
   * phase together, and certify an unchanged complete oriented skin/old vertices before publication.
   * Adjacent cells may share read-only vertices; general surface/movement edits must use ReserveBatch. */
  ReservationBatch ReserveInteriors(const ImportedBatch& imported, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! Lower priority values win conflicting claims; ownership breaks equal-priority ties. */
  /*! All workers begin/end together after Build. Claims are transient and installed only by Publish.
   * Vertex-disjoint construction passes defer conflicts without vetoing the next pass. */
  void BeginConstructionPass() noexcept {construction_pass=true;pass_vertices.clear();}
  void EndConstructionPass() noexcept {construction_pass=false;pass_vertices.clear();}
  bool PassDeferred()const{return last_pass_deferred;}
  bool Reserve(const Imported& imported, bool active, std::string& reason, size_t ceiling = SIZE_MAX,
               size_t caller_bytes = 0, unsigned priority = 0);
  /*! Owner-routed prevalidated replacement. Stages all new mesh/directory nodes before the common vote;
   * version/budget/rank veto leaves both accepted stores unchanged. No allocation after publication vote.
   * Reused cell IDs must advance version. Caller reserves globally unique new IDs and validates complete geometry.
   * Coverage deltas may also update retained physical faces together with all incident covers of a changed trace. */
  bool Publish(Owned& owned, const std::vector<Dependency>& removed, const std::vector<OwnedCell>& added,
               bool prevalidated, std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0,
               const std::vector<BoundaryParameterEdit>& parameter_edits = {},
               const SU2NativeGeometry::BoundaryCoverage& coverage = {},SU2NativeGeometry::AtlasBudget* reservation=nullptr);
  /*! After successful tracked publication, sorted changed vertices for every old/new star owner.
   * Staged before the final vote; rejection preserves both stores and this list. Mode must match across workers. */
  const std::vector<Id>& ChangedVertices() const { return changed_vertices; }
  /*! Collective exact contact veto against the current published mesh. Private replacement cells are broadcast;
   * local cached boxes plus published additions cover every current cell. False return is a protocol/resource
   * failure; candidate=false is a geometric veto or a deferred peer conflict. Eligible peers are selected
   * in priority/owner order after current-mesh vetoes. Publication invalidates stale cell IDs/versions. */
  bool Clearance(const Owned& owned,const Patch& old,const Patch& fresh,bool& candidate,std::string& reason,
                 size_t ceiling=SIZE_MAX,size_t caller_bytes=0,bool* deferred=nullptr,unsigned priority=0,
                 Feature coordinate_star = {});
  size_t ResidentBytes() const;
  const DirectoryStats& Statistics() const { return stats; }
  std::vector<char> Snapshot() const;

 private:
  struct CollisionIndex;
  std::unique_ptr<CollisionIndex> collision;
  BoundaryParameterState* parameters=nullptr; // Borrowed for the directory lifetime; never copied per edit.
  const SU2NativeGeometry::MappedFacetReference* parameter_geometry=nullptr;
  size_t parameter_geometry_bytes=0;
  size_t ParameterBytes()const{return parameters?transfer_memory::Add(parameter_geometry_bytes,parameters->ResidentBytes()):0;}
  bool AgreeParameters(std::string& reason,size_t ceiling,size_t caller_bytes);
  bool PrepareParameters(const std::vector<OwnedCell>& old,const std::vector<OwnedCell>& added,
                         const std::vector<BoundaryParameterEdit>& edits,BoundaryParameterState::Publication& staged,
                         std::string& reason,size_t ceiling,size_t caller_bytes,const SU2NativeGeometry::BoundaryCoverage& coverage,SU2NativeGeometry::AtlasBudget* reservation);
  struct Entry {
    Id key = 0;
    bool identity = false;
    Dependency dependency;
    std::array<Id, 4> nodes{};
    Point point;
    bool erase = false;
    template <class S> void Fields(S& s) { s(key, identity, dependency, nodes, point, erase); }
  };
  using Key = std::pair<bool, Id>;
  using Star = std::map<Id, Entry>;
  using Stars = std::map<Key, Star>;
  struct Prepared {
    Stars changed;
    std::set<Id> pass_vertices;
    bool valid = false;
    std::string reason;
  };
  World& world;
  const bool track_changes;
  std::vector<Id> changed_vertices;
  Stars stars;
  bool construction_pass=false,last_pass_deferred=false;
  std::set<Id> pass_vertices;
  DirectoryStats stats;
  size_t resident_payload_bytes = 0;
  bool initialized = false;
  int Rendezvous(Key key) const;
  bool Vote(const CLocalFailure& failure, std::string& reason);
  bool LookupImpl(const std::vector<Feature>& features, Dependencies* merged,
                  DependencyBatch* batch, size_t ceiling, size_t caller_bytes, std::string& reason,
                  std::vector<std::vector<Id>>* footprints = nullptr, bool coordinate_stars = false);
  ReservationBatch ReserveBatchImpl(const ImportedBatch& imported, size_t ceiling, size_t caller_bytes, bool interiors,
                                    const std::vector<unsigned>& priorities = {});
  ReservationBatch ChooseBatch(const std::vector<std::vector<Id>>& footprints, size_t ceiling, size_t caller_bytes,
                               bool vertices=true, const std::vector<unsigned>& priorities = {}, bool coordinate_stars=false);
  Prepared Prepare(const std::vector<OwnedCell>& removed, const std::vector<OwnedCell>& added,
                   size_t ceiling, size_t baseline);
  bool NotifyChanges(const Prepared& prepared, std::vector<Id>& changed, std::string& reason,
                     size_t ceiling, size_t baseline);
  size_t PreparedBytes(const Prepared& prepared) const;
  void Commit(Prepared&& prepared) noexcept;
};
}  // namespace SU2Native3D
