/*!
 * \file CNativeLocalCore3DMesh.cpp
 * \brief Flat tetrahedral storage, oriented face links and in-place publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace SU2NativeLocal3D {
Face OrientedFace(Vertices v, unsigned face) {
  constexpr unsigned order[4][3]{{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
  return {v[order[face][0]], v[order[face][1]], v[order[face][2]]};
}

namespace {
bool FaceSign(Face v) { return ((v[0] > v[1]) + (v[0] > v[2]) + (v[1] > v[2])) % 2 == 0; }

Face Sorted(Face face) {
  std::sort(face.begin(), face.end());
  return face;
}

bool BitEqual(double a, double b) { return std::memcmp(&a, &b, sizeof(double)) == 0; }

bool SamePoint(const Node& a, const Node& b, const Tensor& left, const Tensor& right) {
  return a.id == b.id && BitEqual(a.p.x, b.p.x) && BitEqual(a.p.y, b.p.y) && BitEqual(a.p.z, b.p.z) &&
         BitEqual(left.xx, right.xx) && BitEqual(left.xy, right.xy) && BitEqual(left.xz, right.xz) &&
         BitEqual(left.yy, right.yy) && BitEqual(left.yz, right.yz) && BitEqual(left.zz, right.zz);
}

struct FaceUse {
  Face key;
  Slot cell;
  unsigned face;
  bool sign;
};

struct OuterFace {
  int32_t cell;
  unsigned face;
  FacetBinding binding;
  bool sign;
};

unsigned ReverseSlot(const std::array<int32_t, 4>& neighbors, Slot cell) {
  const auto found = std::find(neighbors.begin(), neighbors.end(), static_cast<int32_t>(cell));
  if (found == neighbors.end()) throw std::logic_error("Native local core has an asymmetric neighbor.");
  return found - neighbors.begin();
}
}  // namespace

IdSequence::IdSequence(Id base, int rank, int ranks) : last_(base), stride_(0) {
  if (ranks <= 0 || rank < 0 || rank >= ranks) throw std::invalid_argument("Invalid native local core rank.");
  stride_ = static_cast<Id>(rank) + 1;
  // Once the first id is taken, the sequence advances by the communicator size.
  ranks_ = static_cast<Id>(ranks);
}

Id IdSequence::Peek() const {
  if (last_ > std::numeric_limits<Id>::max() - stride_)
    throw std::overflow_error("Native local core exhausted global ids.");
  return last_ + stride_;
}

Id IdSequence::Take() {
  last_ = Peek();
  stride_ = ranks_;
  return last_;
}

Mesh::Mesh(const Owned& owned, bool allow_border, CoreOptions options) : live_cells_(owned.size()), options_(options) {
  if (owned.size() > size_t(INT32_MAX)) throw std::length_error("Too many native local core cells.");
  cells_.reserve(owned.size());
  {
    std::unordered_map<Id, Slot> index;
    for (const auto& entry : owned) {
      const auto& row = entry.second;
      if (entry.first != row.cell.id || !row.sensor_present)
        throw std::invalid_argument("Native local core requires keyed cells with nodal tensors.");
      CellRow cell;
      cell.id = row.cell.id;
      cell.version = row.version;
      cell.binding = SaveBindings(row.physical);
      for (unsigned k = 0; k < 4; ++k) {
        const auto& node = row.cell.v[k];
        if (points_.size() == NoSlot) throw std::length_error("Too many native local core points.");
        const auto result = index.emplace(node.id, static_cast<Slot>(points_.size()));
        auto& slot = cell.vertices[k];
        slot = result.first->second;
        if (result.second)
          points_.push_back({node, row.sensor[k], static_cast<Slot>(cells_.size())});
        else if (!SamePoint(points_[slot].node, node, points_[slot].tensor, row.sensor[k]))
          throw std::invalid_argument("Native local core found unequal copies of a nodal value.");
      }
      if (!Positive(cell.vertices)) throw std::invalid_argument("Native local core received a nonpositive cell.");
      cells_.push_back(cell);
    }
  }
  BuildFaces(allow_border);
  const auto unresolved = UnresolvedSurfacePoints(owned);
  for (auto& point : points_)
    point.unresolved = std::binary_search(unresolved.begin(), unresolved.end(), point.node.id);
  for (size_t k = 0; k < points_.size(); ++k)
    if (!points_[k].Free()) frozen_.push_back({static_cast<Slot>(k), points_[k].node, points_[k].tensor});
}

int32_t Mesh::SaveBindings(const std::array<FacetBinding, 4>& binding) {
  if (std::none_of(binding.begin(), binding.end(), [](FacetBinding b) { return b.present; })) return -1;
  if (!free_bindings_.empty()) {
    const auto slot = free_bindings_.back();
    free_bindings_.pop_back();
    bindings_[slot] = binding;
    return static_cast<int32_t>(slot);
  }
  if (bindings_.size() == size_t(INT32_MAX)) throw std::length_error("Too many native local core bindings.");
  bindings_.push_back(binding);
  return static_cast<int32_t>(bindings_.size() - 1);
}

std::array<FacetBinding, 4> Mesh::Bindings(Slot cell) const {
  const auto binding = cells_[cell].binding;
  return binding < 0 ? std::array<FacetBinding, 4>{} : bindings_[binding];
}

void Mesh::BuildFaces(bool allow_border) {
  std::vector<FaceUse> faces;
  faces.reserve(4 * cells_.size());
  for (Slot c = 0; c < cells_.size(); ++c)
    for (unsigned k = 0; k < 4; ++k) {
      const auto oriented = OrientedFace(cells_[c].vertices, k);
      faces.push_back({Sorted(oriented), c, k, FaceSign(oriented)});
    }
  std::sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
  for (size_t i = 0; i < faces.size();) {
    size_t last = i + 1;
    while (last < faces.size() && faces[last].key == faces[i].key) ++last;
    const auto& a = faces[i];
    const auto physical = Bindings(a.cell)[a.face].present;
    if (last == i + 2) {
      const auto& b = faces[i + 1];
      if (a.sign == b.sign || physical || Bindings(b.cell)[b.face].present)
        throw std::invalid_argument("Native local core found an invalid internal face.");
      cells_[a.cell].neighbors[a.face] = static_cast<int32_t>(b.cell);
      cells_[b.cell].neighbors[b.face] = static_cast<int32_t>(a.cell);
    } else if (last == i + 1 && (physical || allow_border)) {
      cells_[a.cell].neighbors[a.face] = physical ? Physical : Border;
      for (auto slot : a.key) {
        if (physical)
          points_[slot].surface = true;
        else
          points_[slot].border = true;
      }
    } else {
      throw std::invalid_argument("Native local core found an unpaired or nonmanifold face.");
    }
    i = last;
  }
}

Mesh::Plan Mesh::Prepare(const std::vector<Slot>& old, const std::vector<Vertices>& fresh,
                         const std::vector<ReplacedFace>& replaced) const {
  Plan plan;
  plan.refusal = Refusal::Topology;
  if (old.empty() || fresh.empty()) return plan;
  auto old_slots = old;
  std::sort(old_slots.begin(), old_slots.end());
  if (std::adjacent_find(old_slots.begin(), old_slots.end()) != old_slots.end()) return plan;

  std::map<Face, OuterFace> outer;
  for (auto slot : old) {
    if (slot >= cells_.size() || !cells_[slot].live) return plan;
    const auto& row = cells_[slot];
    if (replaced.empty() && Blocked(row.vertices)) {
      plan.refusal = Refusal::Blocked;
      return plan;
    }
    const auto bindings = Bindings(slot);
    for (unsigned k = 0; k < 4; ++k) {
      const auto neighbor = row.neighbors[k];
      if (neighbor >= 0 && std::binary_search(old_slots.begin(), old_slots.end(), static_cast<Slot>(neighbor)))
        continue;
      const auto face = OrientedFace(row.vertices, k);
      const auto reverse = neighbor < 0 ? 0 : ReverseSlot(cells_[neighbor].neighbors, slot);
      if (!outer.emplace(Sorted(face), OuterFace{neighbor, reverse, bindings[k], FaceSign(face)}).second) return plan;
    }
  }

  // Match the new oriented boundary. A split replaces each end in turn, and a collapse replaces
  // only the removed end. A collapse parent on the edge disappears with its cell.
  bool collapse_faces = false;
  for (const auto& edit : replaced) {
    if (!std::binary_search(old_slots.begin(), old_slots.end(), edit.cell) || edit.face >= 4 ||
        cells_[edit.cell].neighbors[edit.face] != Physical || edit.a == edit.b)
      return plan;
    const auto parent = OrientedFace(cells_[edit.cell].vertices, edit.face);
    const auto entry = outer.find(Sorted(parent));
    if (entry == outer.end() || entry->second.cell != Physical || entry->second.sign != FaceSign(parent)) return plan;
    const auto binding = entry->second.binding;
    outer.erase(entry);
    collapse_faces |= edit.a == NoSlot || edit.b == NoSlot;
    if (std::find(parent.begin(), parent.end(), edit.m) != parent.end()) continue;
    for (auto end : {edit.a, edit.b}) {
      if (end == NoSlot) continue;
      auto child = parent;
      const auto vertex = std::find(child.begin(), child.end(), end);
      if (vertex == child.end()) return plan;
      *vertex = edit.m;
      if (!outer.emplace(Sorted(child), OuterFace{Physical, 0, binding, FaceSign(child)}).second) return plan;
    }
  }

  std::map<Face, FaceUse> internal;
  plan.fresh.reserve(fresh.size());
  for (const auto& vertices : fresh) {
    if (std::any_of(vertices.begin(), vertices.end(),
                    [this](Slot point) { return point >= points_.size() || points_[point].removed; }))
      return plan;
    if (!(collapse_faces ? FloatPositive(vertices) : TrialPositive(vertices))) {
      plan.refusal = Refusal::Validity;
      return plan;
    }
    plan.fresh.push_back({vertices});
    const auto slot = static_cast<Slot>(plan.fresh.size() - 1);
    auto& row = plan.fresh[slot];
    for (unsigned k = 0; k < 4; ++k) {
      const auto face = OrientedFace(vertices, k);
      const auto key = Sorted(face);
      const auto sign = FaceSign(face);
      const auto bound = outer.find(key);
      if (bound != outer.end()) {
        const auto& previous = bound->second;
        if (previous.sign != sign || previous.face == 4) return plan;
        row.links[k] = {previous.cell, previous.face, false};
        row.bindings[k] = previous.binding;
        bound->second.face = 4;
        continue;
      }
      const auto result = internal.emplace(key, FaceUse{key, slot, k, sign});
      if (result.second) continue;
      auto& other = result.first->second;
      if (other.sign == sign || other.face == 4) return plan;
      row.links[k] = {static_cast<int32_t>(other.cell), other.face, true};
      plan.fresh[other.cell].links[other.face] = {static_cast<int32_t>(slot), k, true};
      other.face = 4;
    }
  }

  const auto incomplete = [](const auto& entry) { return entry.second.face != 4; };
  if (std::any_of(outer.begin(), outer.end(), incomplete) || std::any_of(internal.begin(), internal.end(), incomplete))
    return plan;

  // Matching oriented boundaries and positive cells fill the cavity with multiplicity one.
  // Physical and rank-border faces are always on this boundary, so their bindings follow the face unchanged.
  plan.old = old;
  std::sort(plan.old.begin(), plan.old.end(),
            [this](Slot a, Slot b) { return Key(cells_[a].vertices) < Key(cells_[b].vertices); });
  plan.refusal = Refusal::None;
  return plan;
}

void Mesh::MarkChanged(CellRow& row, uint32_t stamp) {
  if (!row.changed) erased_ids_.push_back(row.id);
  row.changed = true;
  row.stamp = stamp;
}

void Mesh::Commit(const Plan& plan, uint32_t stamp) {
  ++change_;
  std::vector<Slot> touched, slots;
  slots.reserve(plan.fresh.size());
  for (auto cell : plan.old) {
    auto& row = cells_[cell];
    MarkChanged(row, stamp);
    row.live = false;
    free_cells_.push_back(cell);
    if (row.binding >= 0) free_bindings_.push_back(static_cast<Slot>(row.binding));
    for (auto point : row.vertices) {
      points_[point].changed_at = change_;
      if (points_[point].incident != cell) continue;
      points_[point].incident = NoSlot;
      touched.push_back(point);
    }
  }
  for (const auto& fresh : plan.fresh) {
    Slot slot;
    if (free_cells_.empty()) {
      if (cells_.size() == size_t(INT32_MAX)) throw std::length_error("Too many native local core cells.");
      slot = static_cast<Slot>(cells_.size());
      cells_.emplace_back();
    } else {
      slot = free_cells_.back();
      free_cells_.pop_back();
    }
    slots.push_back(slot);
    auto& row = cells_[slot];
    row = {};
    row.vertices = fresh.vertices;
    row.binding = SaveBindings(fresh.bindings);
    row.stamp = stamp;
    row.changed = true;
    for (auto point : row.vertices) {
      points_[point].incident = slot;
      points_[point].changed_at = change_;
    }
  }
  for (size_t i = 0; i < slots.size(); ++i) {
    const auto slot = slots[i];
    for (unsigned k = 0; k < 4; ++k) {
      const auto& link = plan.fresh[i].links[k];
      cells_[slot].neighbors[k] = link.fresh ? static_cast<int32_t>(slots[link.cell]) : link.cell;
      if (!link.fresh && link.cell >= 0) cells_[link.cell].neighbors[link.face] = static_cast<int32_t>(slot);
    }
  }
  // A touched point with no fresh incident cell had no surviving star across the fixed cavity boundary.
  for (auto point : touched) {
    if (points_[point].incident != NoSlot) continue;
    points_[point].removed = true;
    free_points_.push_back(point);
  }
  live_cells_ = live_cells_ - plan.old.size() + plan.fresh.size();
  if (collapse_refusals_.size() > 6 * live_cells_) collapse_refusals_.clear();
}

Refusal Mesh::ReplaceCells(const std::vector<Slot>& old, const std::vector<Vertices>& fresh, uint32_t stamp) {
  const auto plan = Prepare(old, fresh);
  if (plan.Accepted() && options_.float_trials && !ExactlyPositive(plan)) return Refusal::Validity;
  if (plan.Accepted()) Commit(plan, stamp);
  return plan.refusal;
}

bool Mesh::Blocked(Vertices vertices) const {
  // The engine resolves the surface before the adjoining volume can reach its target.
  return options_.surface_first &&
         std::any_of(vertices.begin(), vertices.end(), [this](Slot p) { return points_[p].unresolved; });
}

bool Mesh::Blocked(const std::vector<Slot>& cells) const {
  return options_.surface_first &&
         std::any_of(cells.begin(), cells.end(), [this](Slot c) { return Blocked(cells_[c].vertices); });
}

void Mesh::Check() const {
  for (const auto& saved : frozen_) {
    const auto& point = points_[saved.slot];
    if (point.removed || !SamePoint(saved.node, point.node, saved.tensor, point.tensor))
      throw std::runtime_error("Native local core changed a frozen point.");
  }
  size_t live = 0;
  for (Slot slot = 0; slot < cells_.size(); ++slot) {
    const auto& row = cells_[slot];
    if (!row.live) continue;
    ++live;
    if (!Positive(row.vertices)) throw std::runtime_error("Native local core left a nonpositive cell.");
    for (unsigned k = 0; k < 4; ++k) {
      const auto p = row.vertices[k];
      const auto& point = points_[p];
      if (point.removed || point.incident >= cells_.size() || !cells_[point.incident].live ||
          std::find(cells_[point.incident].vertices.begin(), cells_[point.incident].vertices.end(), p) ==
              cells_[point.incident].vertices.end())
        throw std::runtime_error("Native local core left a stale point incident cell.");
      const auto neighbor = row.neighbors[k];
      if (Bindings(slot)[k].present != (neighbor == Physical))
        throw std::runtime_error("Native local core lost a physical binding.");
      if (neighbor == Physical)
        for (auto vertex : OrientedFace(row.vertices, k))
          if (!points_[vertex].surface) throw std::runtime_error("Native local core lost a physical point mark.");
      if (neighbor >= 0) {
        if (static_cast<size_t>(neighbor) >= cells_.size() || !cells_[neighbor].live)
          throw std::runtime_error("Native local core linked a dead cell.");
        const auto reverse = ReverseSlot(cells_[neighbor].neighbors, slot);
        const auto face = OrientedFace(row.vertices, k);
        const auto other = OrientedFace(cells_[neighbor].vertices, reverse);
        if (Sorted(face) != Sorted(other) || FaceSign(face) == FaceSign(other))
          throw std::runtime_error("Native local core linked unequal oriented faces.");
      } else if (neighbor != Physical && neighbor != Border) {
        throw std::runtime_error("Native local core left an unknown face kind.");
      }
    }
  }
  if (live != live_cells_) throw std::runtime_error("Native local core has an inconsistent live cell count.");
}

void Mesh::WriteBack(Owned& owned, IdSequence& ids) const {
  for (auto id : erased_ids_) {
    if (owned.erase(id) != 1) throw std::logic_error("Native local core lost an original cell at writeback.");
  }
  for (Slot slot = 0; slot < cells_.size(); ++slot) {
    const auto& cell = cells_[slot];
    if (!cell.live || !cell.changed) continue;
    SU2Native3D::OwnedCell row;
    row.cell.id = ids.Take();
    row.physical = Bindings(slot);
    row.sensor_present = true;
    for (unsigned k = 0; k < 4; ++k) {
      const auto& point = points_[cell.vertices[k]];
      row.cell.v[k] = point.node;
      row.sensor[k] = point.tensor;
    }
    if (!owned.emplace(row.cell.id, row).second) throw std::logic_error("Duplicate native local core output id.");
  }
}
}  // namespace SU2NativeLocal3D
