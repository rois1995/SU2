/*!
 * \file CNativeLocalCore3DShift.cpp
 * \brief Worker-only rendezvous and deterministic migration of intact tetrahedron rows.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace SU2NativeLocal3D {
namespace {
void Elect(const World& world, const CLocalFailure& local) {
  const auto elected = world.elect(local);
  if (elected.any) throw elected;
}

template <class Action>
void LocalStep(const World& world, Action action) {
  CLocalFailure failure;
  try {
    action();
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  Elect(world, failure);
}

template <class Row>
std::vector<Row> Exchange(World& world, const std::vector<std::vector<Row>>& outgoing, MemoryBudget& budget,
                          size_t baseline, const char* reason) {
  std::vector<Row> result;
  LocalStep(world, [&] {
    bool admitted = false;
    result = world.exchange(outgoing, &admitted, budget.ceiling, baseline);
    budget.maximum_bytes = std::max(budget.maximum_bytes, world.max_exchange_work_bytes);
    if (!admitted) throw std::length_error(reason);
  });
  return result;
}

struct FaceMark {
  std::array<Id, 3> key;
  bool physical;
};

struct Announcement {
  Id id = 0;
  int rank = 0;
  bool surface = false;

  template <class Stream>
  void Fields(Stream& stream) {
    stream(id, rank, surface);
  }
};

std::vector<Announcement> LocalBorder(const Owned& owned, int rank) {
  std::vector<FaceMark> faces;
  faces.reserve(4 * owned.size());
  for (const auto& entry : owned) {
    const auto& row = entry.second;
    for (unsigned k = 0; k < 4; ++k) {
      std::array<Id, 3> key{};
      for (unsigned i = 0, j = 0; i < 4; ++i)
        if (i != k) key[j++] = row.cell.v[i].id;
      std::sort(key.begin(), key.end());
      faces.push_back({key, row.physical[k].present});
    }
  }
  std::sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
  std::vector<Id> physical, border;
  for (size_t i = 0; i < faces.size();) {
    size_t last = i + 1;
    while (last < faces.size() && faces[last].key == faces[i].key) ++last;
    const auto& face = faces[i];
    if (last == i + 1) {
      auto& ids = face.physical ? physical : border;
      ids.insert(ids.end(), face.key.begin(), face.key.end());
    } else if (last != i + 2 || face.physical || faces[i + 1].physical) {
      throw std::invalid_argument("Native local border meeting found invalid face incidence.");
    }
    i = last;
  }
  std::sort(physical.begin(), physical.end());
  std::sort(border.begin(), border.end());
  border.erase(std::unique(border.begin(), border.end()), border.end());
  std::vector<Announcement> result;
  result.reserve(border.size());
  for (auto id : border) result.push_back({id, rank, std::binary_search(physical.begin(), physical.end(), id)});
  return result;
}

void BorderReplies(const std::vector<Announcement>& received, std::vector<std::vector<BorderPoint>>& replies,
                   std::vector<Id>& rendezvous, bool /*report*/) {
  for (size_t i = 0; i < received.size();) {
    const auto id = received[i].id;
    bool surface = received[i].surface;
    size_t last = i + 1;
    while (last < received.size() && received[last].id == id) {
      if (received[last].rank == received[last - 1].rank)
        throw std::invalid_argument("Native local border meeting received a duplicate rank.");
      surface |= received[last++].surface;
    }
    if (last == i + 1) throw std::invalid_argument("Native local border point has no partner rank.");
    const BorderPoint value{id, received[i].rank, received[last - 1].rank, surface};
    for (size_t j = i; j < last; ++j) replies.at(received[j].rank).push_back(value);
    // Keep the list in both modes because it is part of the admission accounting.
    rendezvous.push_back(id);
    i = last;
  }
}

int Destination(const SU2Native3D::OwnedCell& cell, const BorderMeeting& meeting, Direction direction, int rank) {
  int destination = rank;
  for (const auto& node : cell.cell.v) {
    const auto* border = meeting.Find(node.id);
    if (!border) continue;
    if (direction == Direction::Lower)
      destination = std::min(destination, border->lower);
    else
      destination = std::max(destination, border->upper);
  }
  return destination;
}

size_t CommonPoints(const std::vector<Id>& first, const std::vector<Id>& second) {
  size_t count = 0;
  auto a = first.begin(), b = second.begin();
  while (a != first.end() && b != second.end()) {
    if (*a < *b)
      ++a;
    else if (*b < *a)
      ++b;
    else {
      ++a;
      ++b;
      ++count;
    }
  }
  return count;
}
}  // namespace

size_t MemoryBudget::OwnedBytes(size_t cells) const { return transfer_memory::Mul(cells, owned_row_bytes); }

void MemoryBudget::Require(const World& world, size_t bytes, const char* reason) {
  CLocalFailure failure;
  if (bytes == SIZE_MAX || bytes > ceiling) failure.Set(3, 0, reason);
  Elect(world, failure);
  maximum_bytes = std::max(maximum_bytes, bytes);
}

const BorderPoint* BorderMeeting::Find(Id id) const {
  const auto found =
      std::lower_bound(points.begin(), points.end(), id, [](const auto& point, Id key) { return point.id < key; });
  return found != points.end() && found->id == id ? &*found : nullptr;
}

size_t BorderMeeting::Bytes() const {
  return transfer_memory::Add(transfer_memory::Bytes(points), transfer_memory::Bytes(rendezvous));
}

BorderMeeting MeetBorderPoints(World& world, const Owned& owned, MemoryBudget& budget, size_t retained, bool report) {
  if (world.size == 1) return {};
  using transfer_memory::Add;
  using transfer_memory::Bytes;
  using transfer_memory::GrowthBound;
  using transfer_memory::Mul;
  const auto baseline = Add(budget.caller_bytes, budget.OwnedBytes(owned.size()), retained);
  const auto local_scratch = Mul(owned.size(), 4 * sizeof(FaceMark) + 12 * (3 * sizeof(Id) + sizeof(Announcement)));
  budget.Require(world, Add(baseline, local_scratch), "Native local border incidence exceeds storage admission.");
  std::vector<Announcement> local;
  LocalStep(world, [&] { local = LocalBorder(owned, world.rank); });

  std::vector<Announcement> received;
  {
    budget.Require(world,
                   Add(baseline, Bytes(local), GrowthBound(local.size(), sizeof(Announcement)),
                       Mul(world.size, sizeof(std::vector<Announcement>))),
                   "Native local border requests exceed storage admission.");
    std::vector<std::vector<Announcement>> outgoing;
    LocalStep(world, [&] {
      outgoing.resize(world.size);
      for (const auto& point : local) outgoing[point.id % world.size].push_back(point);
    });
    received = Exchange(world, outgoing, budget, Add(baseline, Bytes(local)),
                        "Native local border request exchange exceeds storage admission.");
  }
  std::sort(received.begin(), received.end(),
            [](const auto& a, const auto& b) { return std::tie(a.id, a.rank) < std::tie(b.id, b.rank); });
  BorderMeeting meeting;
  const auto response_baseline = Add(baseline, Bytes(local), Bytes(received));
  budget.Require(world,
                 Add(response_baseline, GrowthBound(received.size(), sizeof(BorderPoint)),
                     Mul(world.size, sizeof(std::vector<BorderPoint>)), GrowthBound(received.size(), sizeof(Id))),
                 "Native local border replies exceed storage admission.");
  std::vector<std::vector<BorderPoint>> replies;
  LocalStep(world, [&] {
    replies.resize(world.size);
    BorderReplies(received, replies, meeting.rendezvous, report);
  });
  meeting.points = Exchange(world, replies, budget, Add(response_baseline, Bytes(meeting.rendezvous)),
                            "Native local border reply exchange exceeds storage admission.");
  std::sort(meeting.points.begin(), meeting.points.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
  LocalStep(world, [&] {
    if (meeting.points.size() != local.size() || !std::equal(local.begin(), local.end(), meeting.points.begin(),
                                                             [](const auto& a, const auto& b) { return a.id == b.id; }))
      throw std::runtime_error("Native local border meeting lost an announced point.");
  });
  return meeting;
}

void Mesh::MarkBorderSurface(const BorderMeeting& meeting) {
  for (auto& point : points_) {
    if (!point.border) continue;
    const auto* mark = meeting.Find(point.node.id);
    if (!mark) throw std::runtime_error("Native local pass has a border point missing from the meeting.");
    point.surface |= mark->surface;
  }
}

ShiftStats ShiftBorders(World& world, Owned& owned, const BorderMeeting& meeting, Direction direction,
                        MemoryBudget& budget, const BorderMeeting* previous, bool report) {
  if (world.size == 1) return {};
  using transfer_memory::Add;
  using transfer_memory::Bytes;
  using transfer_memory::GrowthBound;
  using transfer_memory::Mul;
  using Row = SU2Native3D::OwnedCell;
  const auto baseline =
      Add(budget.caller_bytes, budget.OwnedBytes(owned.size()), meeting.Bytes(), previous ? previous->Bytes() : 0);
  budget.Require(world,
                 Add(baseline, GrowthBound(owned.size(), sizeof(Row)), Mul(world.size, sizeof(std::vector<Row>))),
                 "Native local border migration send storage exceeds admission.");
  ShiftStats stats;
  if (report) {
    stats.cells_before = owned.size();
    stats.border_points = meeting.points.size();
    if (previous) stats.frozen_in_both = CommonPoints(previous->rendezvous, meeting.rendezvous);
  }
  std::vector<Row> received;
  {
    std::vector<std::vector<Row>> outgoing;
    LocalStep(world, [&] {
      outgoing.resize(world.size);
      for (const auto& entry : owned) {
        const auto destination = Destination(entry.second, meeting, direction, world.rank);
        outgoing.at(destination).push_back(entry.second);
        if (report && destination != world.rank) ++stats.sent;
      }
    });
    received = Exchange(world, outgoing, budget, baseline, "Native local border migration exceeds storage admission.");
  }
  budget.Require(world, Add(baseline, Bytes(received), budget.OwnedBytes(received.size())),
                 "Native local border migration replacement map exceeds admission.");
  Owned next;
  LocalStep(world, [&] {
    for (auto& row : received) {
      if (report && !owned.count(row.cell.id)) ++stats.received;
      if (!next.emplace(row.cell.id, std::move(row)).second)
        throw std::invalid_argument("Native local border shift received a duplicate cell id.");
    }
  });
  owned.swap(next);
  if (report) stats.cells_after = owned.size();
  return stats;
}
}  // namespace SU2NativeLocal3D
