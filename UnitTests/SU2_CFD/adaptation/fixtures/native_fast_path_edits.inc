// FP0 edit prototypes reuse flat connectivity, float kernels and the A5 scan.
// Include in the anonymous namespace after the FP0 flat-mesh definitions.
// Requires <algorithm>, <array>, <cstdint>, <iostream>, <limits>, <stdexcept> and <vector>.

// One original tetrahedron's use of a canonical interior edge.
struct PrototypeEdgeUse {
  uint64_t edge;
  uint32_t cell;
};

// A current edge and its contiguous ring in the sorted edge uses.
struct PrototypeEdge {
  uint32_t first, second;
  size_t begin, end;
  double length;
};

// Indexes interior edge rings without looking at the optional face metadata.
std::vector<PrototypeEdgeUse> PrototypeEdgeUses(const FlatMesh& mesh) {
  if (mesh.cells.size() > std::numeric_limits<uint32_t>::max())
    throw std::overflow_error("FP0 edit cell index exceeds 32 bits.");
  std::vector<PrototypeEdgeUse> uses;
  uses.reserve(6 * mesh.cells.size());
  for (uint32_t cell = 0; cell < mesh.cells.size(); ++cell) {
    const auto& points = mesh.cells[cell];
    for (size_t i = 0; i < points.size(); ++i)
      for (size_t j = i + 1; j < points.size(); ++j) {
        const auto endpoints = std::minmax(points[i], points[j]);
        if (mesh.points[endpoints.first].surface || mesh.points[endpoints.second].surface) continue;
        const uint64_t edge = (uint64_t(endpoints.first) << 32) | endpoints.second;
        uses.push_back({edge, cell});
      }
  }
  std::sort(uses.begin(), uses.end(), [](const PrototypeEdgeUse& left, const PrototypeEdgeUse& right) {
    return left.edge != right.edge ? left.edge < right.edge : left.cell < right.cell;
  });
  return uses;
}

// Measures each unique interior edge once and keeps the requested lengths.
template <class Accept>
std::vector<PrototypeEdge> PrototypeCandidates(const FlatMesh& mesh, const std::vector<PrototypeEdgeUse>& uses,
                                               Accept accept) {
  std::vector<PrototypeEdge> candidates;
  for (size_t begin = 0; begin < uses.size();) {
    size_t end = begin + 1;
    while (end < uses.size() && uses[end].edge == uses[begin].edge) ++end;
    const auto first = static_cast<uint32_t>(uses[begin].edge >> 32);
    const auto second = static_cast<uint32_t>(uses[begin].edge);
    const double length = FloatLength(mesh.points[first], mesh.points[second]);
    if (accept(length)) candidates.push_back({first, second, begin, end, length});
    begin = end;
  }
  return candidates;
}

// Measures the trial tetrahedron in its original orientation order.
bool PrototypePositive(const FlatMesh& mesh, const TetIndices& cell, uint32_t replaced, Point replacement) {
  std::array<Point, 4> points;
  for (size_t i = 0; i < points.size(); ++i)
    points[i] = cell[i] == replaced ? replacement : mesh.points[cell[i]].node.p;
  return FloatOrientation(points[0], points[1], points[2], points[3]) > 0;
}

// Makes an interior point using the prescribed arithmetic mean of both endpoint tensors.
FlatPoint PrototypeMidpoint(const FlatPoint& first, const FlatPoint& second, Id id) {
  const auto& a = first.node.p;
  const auto& b = second.node.p;
  const auto& left = first.metric;
  const auto& right = second.metric;
  return {{id, {(a.x + b.x) * .5, (a.y + b.y) * .5, (a.z + b.z) * .5}},
          {(left.xx + right.xx) * .5, (left.xy + right.xy) * .5, (left.xz + right.xz) * .5, (left.yy + right.yy) * .5,
           (left.yz + right.yz) * .5, (left.zz + right.zz) * .5},
          false};
}

// Splits the entire edge ring after both halves of every incident cell tested positive.
bool PrototypeSplitRing(FlatMesh& mesh, const std::vector<PrototypeEdgeUse>& uses, const PrototypeEdge& edge,
                        Id next_id, std::vector<uint8_t>& touched) {
  const FlatPoint midpoint = PrototypeMidpoint(mesh.points[edge.first], mesh.points[edge.second], next_id);
  for (size_t i = edge.begin; i < edge.end; ++i) {
    const auto& cell = mesh.cells[uses[i].cell];
    if (!PrototypePositive(mesh, cell, edge.first, midpoint.node.p) ||
        !PrototypePositive(mesh, cell, edge.second, midpoint.node.p))
      return false;
  }
  if (mesh.points.size() >= std::numeric_limits<uint32_t>::max())
    throw std::overflow_error("FP0 split point index exceeds 32 bits.");
  const auto inserted = static_cast<uint32_t>(mesh.points.size());
  mesh.points.push_back(midpoint);
  for (size_t i = edge.begin; i < edge.end; ++i) {
    const auto index = uses[i].cell;
    auto& first = mesh.cells[index];
    auto second = first;
    *std::find(first.begin(), first.end(), edge.first) = inserted;
    *std::find(second.begin(), second.end(), edge.second) = inserted;
    mesh.cells.push_back(second);
    touched[index] = 1;
  }
  return true;
}

// Accumulates pass totals without counting conflicts as geometric refusals.
struct PrototypeSplitResults {
  size_t passes = 0, splits = 0, refused = 0, deferred = 0;
};

// Runs the longest edges first using only the untouched rings of one pass.
bool PrototypeSplitPass(FlatMesh& mesh, Id& next_id, PrototypeSplitResults& result) {
  const auto uses = PrototypeEdgeUses(mesh);
  auto candidates = PrototypeCandidates(mesh, uses, [](double length) { return length > 1.8; });
  if (candidates.empty()) return false;
  ++result.passes;
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const PrototypeEdge& left, const PrototypeEdge& right) { return left.length > right.length; });
  std::vector<uint8_t> touched(mesh.cells.size());
  for (const auto& edge : candidates) {
    bool clean = true;
    for (size_t i = edge.begin; i < edge.end; ++i) clean = clean && !touched[uses[i].cell];
    if (!clean) {
      ++result.deferred;
    } else if (PrototypeSplitRing(mesh, uses, edge, next_id, touched)) {
      ++result.splits;
      ++next_id;
    } else {
      ++result.refused;
    }
  }
  return true;
}

// Stores a point's incident cell indices in a compact row.
struct PrototypePointStars {
  std::vector<size_t> offsets;
  std::vector<uint32_t> cells;
};

// Builds the endpoint stars once for a conservative single collapse pass.
PrototypePointStars PrototypeBuildStars(const FlatMesh& mesh) {
  PrototypePointStars stars;
  stars.offsets.resize(mesh.points.size() + 1);
  for (const auto& cell : mesh.cells)
    for (const auto point : cell) ++stars.offsets[point + 1];
  for (size_t i = 1; i < stars.offsets.size(); ++i) stars.offsets[i] += stars.offsets[i - 1];
  stars.cells.resize(stars.offsets.back());
  auto next = stars.offsets;
  for (uint32_t cell = 0; cell < mesh.cells.size(); ++cell)
    for (const auto point : mesh.cells[cell]) stars.cells[next[point]++] = cell;
  return stars;
}

// Disposition of a directional collapse under the prototype's two numerical checks.
enum class PrototypeCollapseCheck { Accepted, Orientation, Length };

// Checks cells that survive the replacement and only the edges affected by it.
PrototypeCollapseCheck PrototypeCheckCollapse(const FlatMesh& mesh, const PrototypePointStars& stars, uint32_t keep,
                                              uint32_t remove) {
  for (size_t i = stars.offsets[remove]; i < stars.offsets[remove + 1]; ++i) {
    const auto& cell = mesh.cells[stars.cells[i]];
    if (std::find(cell.begin(), cell.end(), keep) != cell.end()) continue;
    if (!PrototypePositive(mesh, cell, remove, mesh.points[keep].node.p)) return PrototypeCollapseCheck::Orientation;
    for (const auto point : cell)
      if (point != remove && !(FloatLength(mesh.points[keep], mesh.points[point]) <= 1.8))
        return PrototypeCollapseCheck::Length;
  }
  return PrototypeCollapseCheck::Accepted;
}

// Holds pass-local marks until final compaction so old cell and point indices remain stable.
struct PrototypeCollapseMarks {
  std::vector<uint8_t> touched, removed_cells, removed_points;
};

// Publishes one admitted collapse into the private arrays without changing index positions.
void PrototypeCollapseEdge(FlatMesh& mesh, const PrototypePointStars& stars, uint32_t keep, uint32_t remove,
                           PrototypeCollapseMarks& marks) {
  for (size_t i = stars.offsets[remove]; i < stars.offsets[remove + 1]; ++i) {
    const auto index = stars.cells[i];
    auto& cell = mesh.cells[index];
    marks.touched[index] = 1;
    if (std::find(cell.begin(), cell.end(), keep) != cell.end())
      marks.removed_cells[index] = 1;
    else
      *std::find(cell.begin(), cell.end(), remove) = keep;
  }
  marks.removed_points[remove] = 1;
}

// Compacts the pass's removed rows and rewrites surviving connectivity.
void PrototypeCompact(FlatMesh& mesh, const PrototypeCollapseMarks& marks) {
  std::vector<uint32_t> indices(mesh.points.size());
  uint32_t points = 0;
  for (size_t i = 0; i < mesh.points.size(); ++i) {
    if (marks.removed_points[i]) continue;
    indices[i] = points;
    mesh.points[points++] = mesh.points[i];
  }
  mesh.points.resize(points);
  size_t cells = 0;
  for (size_t i = 0; i < mesh.cells.size(); ++i) {
    if (marks.removed_cells[i]) continue;
    auto cell = mesh.cells[i];
    for (auto& vertex : cell) vertex = indices[vertex];
    mesh.cells[cells++] = cell;
  }
  mesh.cells.resize(cells);
}

// A retained or removed endpoint must still have its original incident star in this pass.
bool PrototypeUntouched(const PrototypePointStars& stars, const PrototypeCollapseMarks& marks, uint32_t point) {
  for (size_t i = stars.offsets[point]; i < stars.offsets[point + 1]; ++i)
    if (marks.touched[stars.cells[i]]) return false;
  return true;
}

// Counts refused candidate edges; their directional refusal reasons can overlap.
struct PrototypeCollapseResults {
  size_t collapses = 0, refused = 0, refused_orientation = 0, refused_length = 0, deferred = 0;
};

// Tests the lower index as the retained endpoint first, and the other direction on refusal.
PrototypeCollapseResults PrototypeCollapsePass(FlatMesh& mesh) {
  auto candidates = PrototypeCandidates(mesh, PrototypeEdgeUses(mesh), [](double length) { return length < .5; });
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const PrototypeEdge& left, const PrototypeEdge& right) { return left.length < right.length; });
  const auto stars = PrototypeBuildStars(mesh);
  PrototypeCollapseMarks marks;
  marks.touched.resize(mesh.cells.size());
  marks.removed_cells.resize(mesh.cells.size());
  marks.removed_points.resize(mesh.points.size());
  PrototypeCollapseResults result;
  for (const auto& edge : candidates) {
    if (!PrototypeUntouched(stars, marks, edge.first) || !PrototypeUntouched(stars, marks, edge.second)) {
      ++result.deferred;
      continue;
    }
    const auto forward = PrototypeCheckCollapse(mesh, stars, edge.first, edge.second);
    if (forward == PrototypeCollapseCheck::Accepted) {
      PrototypeCollapseEdge(mesh, stars, edge.first, edge.second, marks);
      ++result.collapses;
      continue;
    }
    const auto reverse = PrototypeCheckCollapse(mesh, stars, edge.second, edge.first);
    if (reverse == PrototypeCollapseCheck::Accepted) {
      PrototypeCollapseEdge(mesh, stars, edge.second, edge.first, marks);
      ++result.collapses;
      continue;
    }
    ++result.refused;
    result.refused_orientation +=
        forward == PrototypeCollapseCheck::Orientation || reverse == PrototypeCollapseCheck::Orientation;
    result.refused_length += forward == PrototypeCollapseCheck::Length || reverse == PrototypeCollapseCheck::Length;
  }
  PrototypeCompact(mesh, marks);
  return result;
}

// Measures split and collapse sequentially; the post-edit scans sit outside the timed passes.
void RunPrototypeEdits(FlatMesh mesh) {
  Id next_id = 0;
  for (const auto& row : mesh.points) next_id = std::max(next_id, row.node.id);
  ++next_id;
  const auto split_begin = Clock::now();
  PrototypeSplitResults split;
  while (split.passes < 20 && PrototypeSplitPass(mesh, next_id, split)) {
  }
  const double split_seconds = Seconds(split_begin);
  std::cout << "FP0 A7 {\"splits\":" << split.splits << ",\"passes\":" << split.passes
            << ",\"refused_orientation\":" << split.refused << ",\"deferred\":" << split.deferred
            << ",\"seconds\":" << split_seconds << ",\"microseconds_per_split\":"
            << (split.splits ? split_seconds * 1e6 / static_cast<double>(split.splits) : 0) << "}\n";
  PrintScan("A7_post", mesh);

  const auto collapse_begin = Clock::now();
  const auto collapse = PrototypeCollapsePass(mesh);
  const double collapse_seconds = Seconds(collapse_begin);
  std::cout << "FP0 A8 {\"collapses\":" << collapse.collapses << ",\"passes\":1,\"refused\":" << collapse.refused
            << ",\"refused_orientation\":" << collapse.refused_orientation
            << ",\"refused_length\":" << collapse.refused_length << ",\"deferred\":" << collapse.deferred
            << ",\"seconds\":" << collapse_seconds << ",\"microseconds_per_collapse\":"
            << (collapse.collapses ? collapse_seconds * 1e6 / static_cast<double>(collapse.collapses) : 0) << "}\n";
  PrintScan("A8_post", mesh);
}
