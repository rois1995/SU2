/*!
 * \file CNativeLocalContact3D.cpp
 * \brief Local and partition-wide contact tests for the native local surface core.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalContact3D.hpp"
#include "../../../include/adt/CADTElemClass.hpp"
#include "../../../include/option_structure.hpp"
#include "NativeLocalSurfaceExchange.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <ostream>
#include <tuple>

namespace SU2NativeLocal3D {
namespace {
using SU2Native3D::Cell;
using SU2Native3D::ConformingDisjoint;
using transfer_memory::Add;
using transfer_memory::GrowthBound;
using transfer_memory::Mul;

double Clock(bool report) { return report ? SU2_MPI::Wtime() : 0.; }

ContactBox PointBox(Point p) { return {p.x, p.y, p.z, p.x, p.y, p.z}; }

void Extend(ContactBox& box, const ContactBox& other) {
  for (unsigned i = 0; i < 3; ++i) {
    box[i] = std::min(box[i], other[i]);
    box[i + 3] = std::max(box[i + 3], other[i + 3]);
  }
}

ContactBox BoundsOf(const Cell& cell, unsigned omit = 4) {
  auto box = PointBox(cell.v[omit == 0 ? 1 : 0].p);
  for (unsigned i = 0; i < 4; ++i)
    if (i != omit) Extend(box, PointBox(cell.v[i].p));
  return box;
}

bool Meets(const ContactBox& a, const ContactBox& b) {
  for (unsigned i = 0; i < 3; ++i)
    if (a[i] > b[i + 3] || b[i] > a[i + 3]) return false;
  return true;
}

constexpr size_t TreeInputPerBox = 6 * sizeof(su2double) + 3 * sizeof(unsigned long) + 2 * sizeof(unsigned short);

size_t TreePeak(size_t n) {
  if (!n) return 0;
  // PredictBytes uses unchecked arithmetic internally. Bound its input first.
  if (n > std::numeric_limits<unsigned long>::max() / 6 ||
      Mul(Add(n, omp_get_max_threads()), 65536) == SIZE_MAX) return SIZE_MAX;
  size_t retained = 0, constructor_peak = 0;
  CADTElemClass::PredictBytes(3, 2 * n, n, 2 * n, &retained, &constructor_peak);
  // The box query retains two serial traversal fronts. Charge the transient growth as well.
  const auto query = Add(retained, Mul(n, sizeof(unsigned long)),
                         n > 200 ? GrowthBound(n, 2 * sizeof(unsigned long)) : 0);
  return Add(sizeof(CADTElemClass), std::max(Add(constructor_peak, Mul(n, TreeInputPerBox)), query));
}

template <class BoxAt>
std::unique_ptr<CADTElemClass> BuildTree(size_t n, BoxAt box_at) {
  if (!n) return {};
  std::vector<su2double> coordinates;
  coordinates.reserve(6 * n);
  for (size_t i = 0; i < n; ++i) {
    const auto& box = box_at(i);
    coordinates.insert(coordinates.end(), box.begin(), box.end());
  }
  std::vector<unsigned long> connectivity(2 * n), ids(n);
  std::iota(connectivity.begin(), connectivity.end(), 0UL);
  std::iota(ids.begin(), ids.end(), 0UL);
  std::vector<unsigned short> types(n, LINE), markers(n, 0);
  return std::make_unique<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
}

void Query(CADTElemClass* tree, const ContactBox& box, std::vector<unsigned long>& hits) {
  if (!tree) {
    hits.clear();
    return;
  }
  std::array<su2double, 6> target;
  std::copy(box.begin(), box.end(), target.begin());
  tree->DetermineIntersectingElements(target.data(), target.data() + 3, hits);
}

size_t TreeBytes(const std::unique_ptr<CADTElemClass>& tree) {
  return tree ? Add(sizeof(CADTElemClass), tree->GetAllocatedBytes()) : 0;
}

size_t ContactFixedBytes(size_t n) {
  const auto fresh = Add(std::max(ContactFreshLimit, n / 16), n);
  // Two direction trials retain caps; one direction converts its new cells for the exact checks.
  const auto trials = Add(GrowthBound(n, 2 * sizeof(Vertices)), Mul(n, sizeof(Vertices)));
  const auto entries = LocalContactIndex::EntryBytes(fresh);
  const auto slot_work = GrowthBound(Add(n, Mul(2, fresh)), sizeof(Slot));
  return Add(sizeof(LocalContactIndex), TreePeak(n), GrowthBound(entries, 1), trials, slot_work);
}

struct ScreenRow {
  Cell cell;
  uint32_t face = 0;
  int32_t source = 0;
  template <class Stream> void Fields(Stream& stream) { stream(cell, face, source); }
};

struct OwnerRow {
  Cell cell;
  int32_t source = 0;
  template <class Stream> void Fields(Stream& stream) { stream(cell, source); }
};

template <class Action>
void ContactStep(const World& world, Action action) {
  CLocalFailure local;
  try {
    action();
  } catch (const std::exception& e) {
    local.Set(2, 0, std::string("Native local surface contact: ") + e.what());
  }
  const auto elected = world.elect(local);
  if (elected.any) throw elected;
}

void Admit(World& world, MemoryBudget& budget, size_t bytes, size_t& peak) {
  budget.Require(world, bytes, "Native local surface contact: working-storage admission exceeded.");
  peak = std::max(peak, bytes);
}

template <class Row>
std::vector<Row> ShareRows(World& world, std::vector<Row> local, MemoryBudget& budget, size_t caller, size_t& peak) {
  if (world.size == 1) return local;
  const auto live = Add(caller, transfer_memory::Bytes(local));
  // Report the peak of this exchange without inheriting a previous pass's high-water mark.
  const auto previous = world.max_exchange_work_bytes;
  world.max_exchange_work_bytes = 0;
  try {
    auto rows = SendEverywhere(world, local, budget, live);
    peak = std::max(peak, world.max_exchange_work_bytes);
    world.max_exchange_work_bytes = std::max(previous, world.max_exchange_work_bytes);
    return rows;
  } catch (const CElectedFailure& e) {
    world.max_exchange_work_bytes = std::max(previous, world.max_exchange_work_bytes);
    ContactStep(world, [&] { throw std::runtime_error(e.message); });
    throw;
  }
}
}  // namespace

ContactPassAdmission AdmittedContactPass(size_t ceiling, size_t fixed, size_t bytes_per_cell) {
  const auto contact = [](size_t n) {
    return Add(LocalContactIndex::EntryBytes(n), ContactFixedBytes(n));
  };
  const auto peak = [&](size_t n) { return Add(fixed, Mul(n, bytes_per_cell), contact(n)); };
  size_t lo = 0, hi = INT32_MAX;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo + 1) / 2;
    const auto required = peak(mid);
    if (required != SIZE_MAX && required <= ceiling)
      lo = mid;
    else
      hi = mid - 1;
  }
  return {lo, peak(lo), contact(lo)};
}

double SurfaceContactMargin(double tolerance) {
  return std::nextafter(ContactMargin * tolerance, std::numeric_limits<double>::infinity());
}

LocalContactIndex::LocalContactIndex(const Mesh& mesh, size_t rebuild_limit, SurfaceCounters* counters)
    : mesh_(mesh), rebuild_limit_(rebuild_limit) {
  SurfaceCounters unused;
  Rebuild(counters ? *counters : unused, counters != nullptr);
}
LocalContactIndex::~LocalContactIndex() = default;

size_t LocalContactIndex::EntryBytes(size_t cells) { return Mul(cells, sizeof(Entry)); }

Cell LocalContactIndex::CellAt(Vertices vertices) const {
  Cell cell;
  for (unsigned i = 0; i < 4; ++i) cell.v[i] = mesh_.Points()[vertices[i]].node;
  return cell;
}

ContactBox LocalContactIndex::Bounds(Vertices vertices) const { return BoundsOf(CellAt(vertices)); }

LocalContactIndex::Entry LocalContactIndex::At(Slot slot) const {
  const auto vertices = mesh_.Cells()[slot].vertices;
  return {slot, vertices, Bounds(vertices)};
}

bool LocalContactIndex::Current(const Entry& entry) const {
  const auto& row = mesh_.Cells()[entry.slot];
  return row.live && row.vertices == entry.vertices;
}

void LocalContactIndex::Rebuild(SurfaceCounters& counters, bool report) {
  const auto start = Clock(report);
  tree_.reset();
  fresh_.clear();
  const auto& cells = mesh_.Cells();
  const size_t live = std::count_if(cells.begin(), cells.end(), [](const auto& cell) { return cell.live; });
  entries_.clear();
  entries_.reserve(live);
  hits_.reserve(live);
  for (Slot slot = 0; slot < cells.size(); ++slot)
    if (cells[slot].live) entries_.push_back(At(slot));
  tree_ = BuildTree(entries_.size(), [&](size_t i) -> const ContactBox& { return entries_[i].box; });
  if (report) {
    ++counters.c_contact_rebuilds;
    counters.seconds_c_contact_build += Clock(true) - start;
  }
}

void LocalContactIndex::Committed(const std::vector<Slot>& slots, SurfaceCounters& counters, bool report) {
  for (auto slot : slots) fresh_.push_back(At(slot));
  if (report) counters.c_contact_fresh_max = std::max(counters.c_contact_fresh_max, fresh_.size());
  if (fresh_.size() > std::max(rebuild_limit_, entries_.size() / 16)) Rebuild(counters, report);
}

void LocalContactIndex::TreeCandidates(const ContactBox& box, std::vector<Slot>& candidates) {
  Query(tree_.get(), box, hits_);
  for (auto hit : hits_) {
    const auto& entry = entries_[hit];
    if (Meets(box, entry.box) && Current(entry)) candidates.push_back(entry.slot);
  }
}

std::vector<Slot> LocalContactIndex::Candidates(const ContactBox& box) {
  std::vector<Slot> slots;
  TreeCandidates(box, slots);
  for (const auto& entry : fresh_)
    if (Meets(box, entry.box) && Current(entry)) slots.push_back(entry.slot);
  std::sort(slots.begin(), slots.end());
  slots.erase(std::unique(slots.begin(), slots.end()), slots.end());
  return slots;
}

bool LocalContactIndex::NewCellsDisjoint(const std::vector<Vertices>& cells, SurfaceCounters& counters,
                                        bool report) const {
  // (N1) Only pairs involving a new cell can change. Separate closed boxes cannot touch.
  for (size_t i = 0; i < cells.size(); ++i) {
    const auto cell = CellAt(cells[i]);
    const auto box = BoundsOf(cell);
    for (size_t j = 0; j < i; ++j) {
      const auto other = CellAt(cells[j]);
      if (!Meets(box, BoundsOf(other))) continue;
      if (report) {
        ++counters.c_contact_candidates;
        ++counters.c_contact_calls;
      }
      if (!ConformingDisjoint(cell, other)) return false;
    }
  }
  return true;
}

bool LocalContactIndex::KeptCellsDisjoint(const std::vector<Vertices>& cells, const std::vector<Slot>& old,
                                         SurfaceCounters& counters, bool report) {
  // (N2) Use the current mesh, including cells created since the tree was built.
  // Exact disjointness permits contact only on the simplex with shared point ids.
  if (cells.empty()) return true;
  auto combined = Bounds(cells.front());
  for (const auto& cell : cells) Extend(combined, Bounds(cell));
  fresh_hits_.clear();
  for (const auto& entry : fresh_)
    if (Meets(combined, entry.box) && Current(entry)) fresh_hits_.push_back(entry.slot);
  for (const auto& vertices : cells) {
    const auto cell = CellAt(vertices);
    const auto box = BoundsOf(cell);
    candidates_.clear();
    TreeCandidates(box, candidates_);
    for (auto slot : fresh_hits_)
      if (Meets(box, Bounds(mesh_.Cells()[slot].vertices))) candidates_.push_back(slot);
    std::sort(candidates_.begin(), candidates_.end());
    candidates_.erase(std::unique(candidates_.begin(), candidates_.end()), candidates_.end());
    for (auto slot : candidates_) {
      if (std::find(old.begin(), old.end(), slot) != old.end()) continue;
      if (report) {
        ++counters.c_contact_candidates;
        ++counters.c_contact_calls;
      }
      if (!ConformingDisjoint(cell, CellAt(mesh_.Cells()[slot].vertices))) return false;
    }
  }
  return true;
}

struct RemoteSurface::Data {
  std::vector<ScreenRow> rows;
  std::vector<ContactBox> boxes;
  std::unique_ptr<CADTElemClass> tree;
  std::vector<unsigned long> hits;
  size_t reservation = 0;
};

RemoteSurface::RemoteSurface() = default;
RemoteSurface::~RemoteSurface() = default;
RemoteSurface::RemoteSurface(RemoteSurface&&) noexcept = default;
RemoteSurface& RemoteSurface::operator=(RemoteSurface&&) noexcept = default;

size_t RemoteSurface::Bytes() const {
  // Include the serial query fronts' growth for the lifetime of the pass.
  return data_ ? data_->reservation : 0;
}

bool RemoteSurface::Near(const ContactBox& box) {
  if (!data_) return false;
  Query(data_->tree.get(), box, data_->hits);
  return std::any_of(data_->hits.begin(), data_->hits.end(),
                     [&](size_t hit) { return Meets(box, data_->boxes[hit]); });
}

RemoteSurface GatherRemoteSurface(World& world, const Owned& owned, double margin, MemoryBudget& budget,
                                   size_t caller, bool report) {
  RemoteSurface result;
  const auto start = Clock(report);
  size_t count = 0;
  for (const auto& row : owned)
    for (const auto& binding : row.second.physical) count += binding.present;
  std::vector<ScreenRow> local;
  Admit(world, budget, Add(caller, Mul(count, sizeof(ScreenRow))), result.peak_bytes_);
  ContactStep(world, [&] {
    local.reserve(count);
    for (const auto& row : owned)
      for (unsigned k = 0; k < 4; ++k)
        if (row.second.physical[k].present) local.push_back({row.second.cell, k, world.rank});
  });
  auto received = ShareRows(world, std::move(local), budget, caller, result.peak_bytes_);
  received.erase(std::remove_if(received.begin(), received.end(),
                                [&](const auto& row) { return row.source == world.rank; }), received.end());
  const auto n = received.size();
  const auto rows_boxes = Add(sizeof(RemoteSurface::Data), transfer_memory::Bytes(received), Mul(n, sizeof(ContactBox)));
  Admit(world, budget, Add(caller, rows_boxes, TreePeak(n)), result.peak_bytes_);
  ContactStep(world, [&] {
    result.data_ = std::make_unique<RemoteSurface::Data>();
    auto& data = *result.data_;
    data.rows = std::move(received);
    std::sort(data.rows.begin(), data.rows.end(), [](const auto& a, const auto& b) {
      return std::tie(a.source, a.cell.id, a.face) < std::tie(b.source, b.cell.id, b.face);
    });
    data.boxes.reserve(n);
    for (const auto& row : data.rows) {
      auto box = BoundsOf(row.cell, row.face);
      const auto infinity = std::numeric_limits<double>::infinity();
      for (unsigned i = 0; i < 3; ++i) {
        box[i] = std::nextafter(box[i] - margin, -infinity);
        box[i + 3] = std::nextafter(box[i + 3] + margin, infinity);
      }
      data.boxes.push_back(box);
    }
    data.tree = BuildTree(n, [&](size_t i) -> const ContactBox& { return data.boxes[i]; });
    data.hits.reserve(n);
    data.reservation = Add(rows_boxes, TreeBytes(data.tree), transfer_memory::Bytes(data.hits),
                           n > 200 ? GrowthBound(n, 2 * sizeof(unsigned long)) : 0);
  });
  if (report) result.seconds_gather_ = Clock(true) - start;
  return result;
}

PassContactCheck VerifyPassContact(World& world, const Owned& owned, MemoryBudget& budget, size_t caller,
                                   bool report) {
  // What this check proves: Check gives positive cells and matched inner faces on every rank.
  // Assume the input pairs each partition face once with opposite orientations and matching ids and
  // coordinates. The pass and WriteBack preserve those faces. Where the covering count first
  // exceeds one starting outside the mesh, an entered physical-face cell overlaps another cell:
  // matched inner faces cancel in the count. A forbidden zero-volume contact outside the shared
  // simplex on an inner simplex would have an overlapping full neighbourhood. Otherwise a cell
  // with a physical face witnesses the contact. In both cases the closed boxes meet. This also
  // catches contacts made by L or D, whose split trials have no contact check.
  PassContactCheck result;
  const auto start = Clock(report);
  const auto owner = [](const auto& row) {
    const auto& faces = row.second.physical;
    return std::any_of(faces.begin(), faces.end(), [](const auto& binding) { return binding.present; });
  };
  const size_t count = std::count_if(owned.begin(), owned.end(), owner);
  std::vector<OwnerRow> local;
  Admit(world, budget, Add(caller, Mul(count, sizeof(OwnerRow))), result.peak_bytes);
  ContactStep(world, [&] {
    local.reserve(count);
    for (const auto& row : owned)
      if (owner(row)) local.push_back({row.second.cell, world.rank});
  });
  auto rows = ShareRows(world, std::move(local), budget, caller, result.peak_bytes);
  const auto n = rows.size();
  Admit(world, budget, Add(caller, transfer_memory::Bytes(rows), Mul(n, sizeof(ContactBox)), TreePeak(n)),
        result.peak_bytes);
  std::vector<ContactBox> boxes;
  std::vector<unsigned long> hits;
  std::unique_ptr<CADTElemClass> tree;
  ContactStep(world, [&] {
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
      return std::tie(a.source, a.cell.id) < std::tie(b.source, b.cell.id);
    });
    boxes.reserve(n);
    for (const auto& row : rows) boxes.push_back(BoundsOf(row.cell));
    tree = BuildTree(n, [&](size_t i) -> const ContactBox& { return boxes[i]; });
    hits.reserve(n);
  });
  if (report) {
    result.owner_rows = n;
    result.owner_bytes = transfer_memory::Bytes(rows);
    result.seconds_gather = Clock(true) - start;
  }
  const auto check = Clock(report);
  ContactStep(world, [&] {
    for (const auto& entry : owned) {
      const auto& cell = entry.second.cell;
      if (report) ++result.verify_cells;
      const auto box = BoundsOf(cell);
      Query(tree.get(), box, hits);
      std::sort(hits.begin(), hits.end());
      for (auto hit : hits) {
        const auto& row = rows[hit];
        if ((row.source == world.rank && row.cell.id == cell.id) || !Meets(box, boxes[hit])) continue;
        if (report) {
          ++result.verify_candidates;
          ++result.verify_calls;
        }
        if (!ConformingDisjoint(cell, row.cell))
          throw std::runtime_error("cell " + std::to_string(cell.id) + " contacts cell " +
                                   std::to_string(row.cell.id) + " from rank " + std::to_string(row.source));
      }
    }
  });
  if (report) result.seconds_verify = Clock(true) - check;
  return result;
}

void PrintRemoteSurfaceFields(std::ostream& out, const RemoteSurface& result) {
  out << " kind=screen remote_rows=" << result.data_->rows.size() << " remote_bytes=" << result.Bytes()
      << " peak_bytes=" << result.peak_bytes_ << " seconds_gather=" << result.seconds_gather_;
}

void PrintPassContactFields(std::ostream& out, const PassContactCheck& result) {
  out << " kind=verify owner_rows=" << result.owner_rows << " owner_bytes=" << result.owner_bytes
      << " peak_bytes=" << result.peak_bytes << " verify_cells=" << result.verify_cells
      << " verify_candidates=" << result.verify_candidates << " verify_calls=" << result.verify_calls
      << " seconds_gather=" << result.seconds_gather << " seconds_verify=" << result.seconds_verify;
}
}  // namespace SU2NativeLocal3D
