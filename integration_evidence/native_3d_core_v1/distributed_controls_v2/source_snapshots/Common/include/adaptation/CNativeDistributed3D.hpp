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
  template <class S> void Fields(S& s) { s(cell, version, physical); }
};
using Owned = std::map<Id, OwnedCell>;
struct Dependency {
  Id cell = 0;
  uint64_t version = 0;
  int owner = 0;
  template <class S> void Fields(S& s) { s(cell, version, owner); }
};
struct Feature {
  std::array<Id, 3> vertices{};
  unsigned count = 0;  // 1 vertex, 2 edge, 3 face. Empty feature lists are inactive callers.
  template <class S> void Fields(S& s) { s(vertices, count); }
};
struct Dependencies {
  std::vector<Dependency> cells;
  bool valid = false;
  std::string reason;
};
struct Imported {
  std::vector<OwnedCell> cells;
  bool valid = false;
  std::string reason;
};
struct DirectoryStats {
  uint64_t builds = 0, lookups = 0, fetches = 0, reservations = 0, publications = 0, rejected = 0;
  uint64_t metadata_scanned = 0, payloads = 0, elections = 0;
  size_t maximum_dependencies = 0, maximum_work_bytes = 0;
  double build_seconds = 0, lookup_seconds = 0, fetch_seconds = 0, reservation_seconds = 0, publication_seconds = 0;
  double election_seconds = 0;  // Nested detail; not added to the exclusive public-call phases.
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
  explicit Directory(World& world) : world(world) {}
  bool Build(const Owned& owned, std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  Dependencies Lookup(const std::vector<Feature>& features, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  Imported Fetch(const Dependencies& dependencies, const Owned& owned, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  bool Reserve(const Imported& imported, bool active, std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  /*! Owner-routed prevalidated replacement. Stages all new mesh/directory nodes before the common vote;
   * version/budget/rank veto leaves both accepted stores unchanged. No allocation after publication vote.
   * Reused cell IDs must advance version. Caller reserves globally unique new IDs and validates complete geometry. */
  bool Publish(Owned& owned, const std::vector<Dependency>& removed, const std::vector<OwnedCell>& added,
               bool prevalidated, std::string& reason, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0);
  size_t ResidentBytes() const;
  const DirectoryStats& Statistics() const { return stats; }
  std::vector<char> Snapshot() const;

 private:
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
    bool valid = false;
    std::string reason;
  };
  World& world;
  Stars stars;
  DirectoryStats stats;
  size_t resident_payload_bytes = 0;
  int Rendezvous(Key key) const;
  bool Vote(const CLocalFailure& failure, std::string& reason);
  Prepared Prepare(const std::vector<OwnedCell>& removed, const std::vector<OwnedCell>& added,
                   size_t ceiling, size_t baseline);
  void Commit(Prepared&& prepared) noexcept;
};
}  // namespace SU2Native3D
