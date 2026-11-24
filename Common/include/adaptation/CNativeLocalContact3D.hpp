/*!
 * \file CNativeLocalContact3D.hpp
 * \brief Contact checks for native local tetrahedral adaptation.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "CNativeLocalCore3D.hpp"
#include <memory>

class CADTElemClass;

namespace SU2NativeLocal3D {
using ContactBox = std::array<double, 6>;
constexpr double ContactMargin = 3.0;
constexpr size_t ContactFreshLimit = 4096;

class LocalContactIndex {
 public:
  explicit LocalContactIndex(const Mesh&, size_t rebuild_limit = ContactFreshLimit,
                             SurfaceCounters* counters = nullptr);
  ~LocalContactIndex();
  size_t FreshSize() const { return fresh_.size(); }
  std::vector<Slot> Candidates(const ContactBox&);
  bool NewCellsDisjoint(const std::vector<Vertices>&, SurfaceCounters&, bool report) const;
  bool KeptCellsDisjoint(const std::vector<Vertices>&, const std::vector<Slot>& old,
                         SurfaceCounters&, bool report);
  void Rebuild(SurfaceCounters&, bool report);
  void Committed(const std::vector<Slot>&, SurfaceCounters&, bool report);
  ContactBox Bounds(Vertices) const;
  static size_t EntryBytes(size_t cells);

 private:
  struct Entry {
    Slot slot;
    Vertices vertices;
    ContactBox box;
  };

  SU2Native3D::Cell CellAt(Vertices) const;
  Entry At(Slot) const;
  bool Current(const Entry&) const;
  void TreeCandidates(const ContactBox&, std::vector<Slot>&);
  const Mesh& mesh_;
  size_t rebuild_limit_;
  std::vector<Entry> entries_, fresh_;
  std::unique_ptr<CADTElemClass> tree_;
  std::vector<unsigned long> hits_;
  std::vector<Slot> fresh_hits_, candidates_;
};

class RemoteSurface {
 public:
  RemoteSurface();
  ~RemoteSurface();
  RemoteSurface(RemoteSurface&&) noexcept;
  RemoteSurface& operator=(RemoteSurface&&) noexcept;
  size_t Bytes() const;
  bool Near(const ContactBox&);

 private:
  friend RemoteSurface GatherRemoteSurface(World&, const Owned&, double, MemoryBudget&, size_t, bool);
  friend void PrintRemoteSurfaceFields(std::ostream&, const RemoteSurface&);
  struct Data;
  std::unique_ptr<Data> data_;
  size_t peak_bytes_ = 0;
  double seconds_gather_ = 0;
};

struct ContactContext {
  LocalContactIndex& index;
  RemoteSurface* remote = nullptr;
};

struct ContactPassAdmission {
  size_t cell_limit, peak_bytes, contact_bytes;
};
ContactPassAdmission AdmittedContactPass(size_t ceiling, size_t fixed, size_t bytes_per_cell);
double SurfaceContactMargin(double tolerance);

struct PassContactCheck {
  size_t owner_rows = 0, owner_bytes = 0, peak_bytes = 0;
  size_t verify_cells = 0, verify_candidates = 0, verify_calls = 0;
  double seconds_gather = 0, seconds_verify = 0;
};
RemoteSurface GatherRemoteSurface(World&, const Owned&, double margin, MemoryBudget&, size_t caller, bool report);
PassContactCheck VerifyPassContact(World&, const Owned&, MemoryBudget&, size_t caller, bool report);
void PrintRemoteSurfaceFields(std::ostream&, const RemoteSurface&);
void PrintPassContactFields(std::ostream&, const PassContactCheck&);
}  // namespace SU2NativeLocal3D
