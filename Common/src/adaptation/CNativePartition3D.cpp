/*!
 * \file CNativePartition3D.cpp
 * \brief Bounded triangular-face dual graph, two-constraint partition and atomic ownership migration.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativePartition3D.hpp"
#include "../../include/adaptation/CNativeEngine3D.hpp"
#if defined(HAVE_MPI) && defined(HAVE_PARMETIS)
#include "parmetis.h"
#endif
#include <cmath>
#include <numeric>
namespace SU2Native3D {
std::vector<uint32_t> WorkWeights(const std::vector<Cell>& cells,const std::function<FieldSample(Point)>& query) {
  if (cells.size() > MaximumReplacementCells) throw std::invalid_argument("Native 3D weight sample batch exceeds budget.");
  std::vector<uint32_t> result;
  result.reserve(cells.size());
  for (const auto& cell : cells) {
    const Tetrahedron tet{cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p};
    if (!(Orientation(tet[0], tet[1], tet[2], tet[3]) > 0))
      throw std::invalid_argument("Native 3D work estimate requires positive cells.");
    const auto center = query(Centroid(cell));
    long double work = 1 + MetricDensity(center.sensor)*TetrahedronVolume(tet)*6*std::sqrt(2.L);
    std::array<Tensor, 4> nodal;
    for (size_t i = 0; i < 4; ++i) nodal[i] = query(cell.v[i].p).target;
    for (size_t i = 0; i < 4; ++i) for (size_t j = i + 1; j < 4; ++j) {
      const auto a = cell.v[i].p, b = cell.v[j].p;
      const auto length = std::max({MetricNorm(a, b, center.target), MetricNorm(a, b, nodal[i]), MetricNorm(a, b, nodal[j])});
      if (!(length > 0)) throw std::invalid_argument("Native 3D work estimate has a zero metric edge.");
      work += std::max(0.L, std::log2(length / MaximumMetricEdge));
      work += std::max(0.L, std::log2(static_cast<long double>(CoarseningMetricEdge) / length));
    }
    if (!(std::isfinite(work) && work > 0)) throw std::runtime_error("Unrepresentable native 3D work estimate.");
    // shortcut: static operation-demand proxy; calibrate only against recorded tetrahedral rank costs.
    result.push_back(uint32_t(std::ceil(std::min(static_cast<long double>(MaximumPartitionWeight), work))));
  }
  return result;
}
std::vector<uint32_t> WorkWeights(const std::vector<Cell>& cells,FrozenField& field) {
  return WorkWeights(cells,[&](Point p){return field.Query(p);});
}
namespace {
struct RankWork {
  int rank = 0;
  uint64_t cells = 0, work = 0;
  template <class S> void Fields(S& s) { s(rank, cells, work); }
};
struct DualFace {
  Face face{};
  uint64_t local = 0, global = 0;
  int owner = 0, orientation = 0;
  bool physical = false;
  template <class S> void Fields(S& s) { s(face, local, global, owner, orientation, physical); }
};
struct Neighbor {
  uint64_t local = 0, global = 0;
  template <class S> void Fields(S& s) { s(local, global); }
};
struct WeightedCell {
  OwnedCell cell;
  uint32_t weight = 0;
  template <class S> void Fields(S& s) { s(cell, weight); }
};
#if defined(HAVE_MPI) && defined(HAVE_PARMETIS)
using GraphIndex = idx_t;
#else
using GraphIndex = int64_t;
#endif
int Rendezvous(Face face, int ranks) {
  uint64_t hash = 0;
  for (Id id : face) {
    id ^= id >> 30; id *= 0xbf58476d1ce4e5b9ULL;
    id ^= id >> 27; id *= 0x94d049bb133111ebULL; id ^= id >> 31;
    hash ^= id + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  }
  return hash % ranks;
}
int Parity(Face f) {
  unsigned inversions = 0;
  for (size_t i = 0; i < 3; ++i) for (size_t j = i + 1; j < 3; ++j) inversions += f[i] > f[j];
  return inversions % 2 ? -1 : 1;
}
size_t MapBytes(size_t n) { return transfer_memory::Mul(n, sizeof(Owned::value_type) + 4 * sizeof(void*)); }
}  // namespace
bool Repartition(World& world, Owned& input, const std::vector<uint32_t>& weights, int parts, PartitionStats& stats,
                 std::string& reason, size_t ceiling, size_t caller_bytes) {
  const double started = world.seconds();
  stats = {};
  struct Envelope {
    World& world; PartitionStats& stats; double started;
    ~Envelope() {
      stats.total_seconds = world.seconds() - started;
      stats.other_seconds = std::max(0., stats.total_seconds - stats.graph_seconds - stats.partition_seconds - stats.migration_seconds);
    }
  } envelope{world, stats, started};  // Includes release of local graph/directory/staging buffers at function return.
  CLocalFailure failure;
  auto vote = [&]() { const auto elected = world.elect(failure); reason = elected.message; return !elected.any; };
  auto finish = [&](bool valid) {
    if (valid) reason.clear();
    return valid;
  };
  auto admit = [&](size_t bytes) {
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, bytes);
    if (bytes > ceiling || bytes == SIZE_MAX) failure.Set(3, 0, "Native 3D partition storage budget exceeded.");
    return vote();
  };
  const double graph_started = world.seconds();
  auto graph_failure = [&]() { stats.graph_seconds = world.seconds() - graph_started; return finish(false); };
  const int common_parts = world.maximum(parts);
  if (parts != common_parts || parts < 1 || parts > world.size || weights.size() != input.size())
    failure.Set(1, 0, "Invalid or inconsistent native 3D partition controls.");
  for (auto weight : weights) if (!weight || weight > MaximumPartitionWeight)
    failure.Set(1, 0, "Native 3D partition weights must be between 1 and 1024.");
  for (const auto& row : input) if (row.first != row.second.cell.id)
    failure.Set(1, row.first, "Native 3D working cell map identity mismatch.");
  if (!vote()) return graph_failure();
  const size_t baseline = transfer_memory::Add(caller_bytes, MapBytes(input.size()), transfer_memory::Bytes(weights),
      transfer_memory::Mul(input.size(), sizeof(int)), transfer_memory::Mul(world.size, 8 * sizeof(uint64_t)));
  if (!admit(baseline)) return graph_failure();
  auto loads = [&](size_t cells, uint64_t work, std::vector<uint64_t>& counts, std::vector<uint64_t>& costs, size_t live) {
    std::vector<std::vector<RankWork>> to;
    try {
      counts.assign(world.size, 0); costs.assign(world.size, 0);
      to.assign(world.size, {{world.rank, cells, work}});
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    if (!vote()) return false;
    bool admitted = true;
    const auto rows = world.exchange(to, &admitted, ceiling, live);
    if (!admitted) { failure.Set(3, 0, "Native 3D partition load exchange exceeds storage budget."); return vote(); }
    for (auto row : rows) { counts.at(row.rank) = row.cells; costs.at(row.rank) = row.work; }
    return true;
  };
  const uint64_t local_work = std::accumulate(weights.begin(), weights.end(), uint64_t(0));
  if (!loads(input.size(), local_work, stats.cells_before, stats.work_before, baseline)) return graph_failure();
  const uint64_t total_cells = std::accumulate(stats.cells_before.begin(), stats.cells_before.end(), uint64_t(0));
  const uint64_t total_work = std::accumulate(stats.work_before.begin(), stats.work_before.end(), uint64_t(0));
  const auto index_max = uint64_t(std::numeric_limits<GraphIndex>::max());
  if (total_cells < uint64_t(parts) || total_cells > index_max / 4 || total_work > index_max)
    failure.Set(1, 0, "Native 3D working graph cell/weight range is not representable.");
  if (!vote()) return graph_failure();
  Directory directory(world);
  const double directory_started = world.seconds();
  if (!directory.Build(input, reason, ceiling, baseline)) {
    stats.directory_seconds = world.seconds() - directory_started;
    return graph_failure();
  }
  stats.directory_seconds = world.seconds() - directory_started;
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, directory.Statistics().maximum_work_bytes);
  const size_t retained = transfer_memory::Add(baseline, directory.ResidentBytes());
  const auto prefix = std::accumulate(stats.cells_before.begin(), stats.cells_before.begin() + world.rank, uint64_t(0));
  std::vector<std::vector<DualFace>> to;
  std::vector<int> colors;
  if (!admit(transfer_memory::Add(retained, transfer_memory::Mul(world.size, sizeof(std::vector<DualFace>)),
      transfer_memory::GrowthBound(4 * input.size(), sizeof(DualFace))))) return graph_failure();
  try {
    colors.assign(input.size(), 0); to.resize(world.size);
    uint64_t local = 0;
    for (const auto& entry : input) {
      const auto& v = entry.second.cell.v;
      const std::array<Face, 4> faces{{{v[1].id,v[2].id,v[3].id}, {v[0].id,v[3].id,v[2].id},
                                     {v[0].id,v[1].id,v[3].id}, {v[0].id,v[2].id,v[1].id}}};
      for (size_t i = 0; i < 4; ++i) {
        auto face = faces[i]; const auto parity = Parity(face); std::sort(face.begin(), face.end());
        to[Rendezvous(face, world.size)].push_back({face, local, prefix + local, world.rank, parity, entry.second.physical[i].present});
      }
      ++local;
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!vote()) return graph_failure();
  bool admitted = true;
  auto faces = world.exchange(to, &admitted, ceiling, retained);
  if (!admitted) { failure.Set(3,0,"Native 3D dual-face exchange exceeds storage budget."); vote(); return graph_failure(); }
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, world.max_exchange_work_bytes);
  std::sort(faces.begin(), faces.end(), [](const auto& a, const auto& b) { return std::tie(a.face,a.global) < std::tie(b.face,b.global); });
  std::vector<std::vector<Neighbor>> replies;
  size_t face_bytes = transfer_memory::Add(retained, transfer_memory::Bytes(to), transfer_memory::Bytes(faces));
  if (!admit(transfer_memory::Add(face_bytes, transfer_memory::Mul(world.size, sizeof(std::vector<Neighbor>)),
      transfer_memory::GrowthBound(faces.size(), sizeof(Neighbor))))) return graph_failure();
  try {
    replies.resize(world.size);
    for (size_t first = 0; first < faces.size();) {
      size_t last = first + 1;
      while (last < faces.size() && faces[last].face == faces[first].face) ++last;
      if (last - first == 1) {
        if (!faces[first].physical) failure.Set(1,faces[first].face[0],"Native 3D dual graph has an unmarked exterior face.");
      } else if (last - first == 2) {
        const auto& a = faces[first]; const auto& b = faces[first+1];
        if (a.global == b.global || a.orientation == b.orientation || a.physical || b.physical)
          failure.Set(1,a.face[0],"Invalid native 3D interior dual face orientation/binding.");
        else { replies[a.owner].push_back({a.local,b.global}); replies[b.owner].push_back({b.local,a.global}); }
      } else failure.Set(1,faces[first].face[0],"Nonmanifold native 3D dual face.");
      first = last;
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!vote()) return graph_failure();
  auto neighbors = world.exchange(replies, &admitted, ceiling, face_bytes);
  if (!admitted) { failure.Set(3,0,"Native 3D dual-neighbor exchange exceeds storage budget."); vote(); return graph_failure(); }
  std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) { return std::tie(a.local,a.global)<std::tie(b.local,b.global); });
  const size_t graph_bytes = transfer_memory::Add(face_bytes, transfer_memory::Bytes(replies), transfer_memory::Bytes(neighbors),
      transfer_memory::Mul(transfer_memory::Add(transfer_memory::Mul(9, input.size()), 1), sizeof(GraphIndex)));
  if (!admit(graph_bytes)) return graph_failure();
  std::vector<GraphIndex> xadj, adjacency, vwgt, part;
  try {
    xadj.resize(input.size()+1); part.resize(input.size()); adjacency.reserve(neighbors.size()); vwgt.reserve(2*input.size());
    size_t next = 0;
    for (size_t i=0;i<input.size();++i) {
      while (next<neighbors.size() && neighbors[next].local==i) { adjacency.push_back(GraphIndex(neighbors[next].global)); ++next; }
      xadj[i+1]=GraphIndex(adjacency.size()); vwgt.push_back(GraphIndex(weights[i])); vwgt.push_back(1);
    }
    if (next!=neighbors.size()) failure.Set(1,0,"Invalid native 3D dual local ordinal.");
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!vote()) return graph_failure();
  stats.graph_arcs = CPassiveComm::Allreduce(uint64_t(neighbors.size()), CPassiveComm::Op::SUM, world.comm);
  stats.graph_seconds = world.seconds() - graph_started;
  const double partition_started = world.seconds();
  if (parts > 1) {
#if defined(HAVE_MPI) && defined(HAVE_PARMETIS)
    std::vector<idx_t> vtxdist;
    std::vector<real_t> tpwgts;
    const size_t active = std::count_if(stats.cells_before.begin(), stats.cells_before.end(), [](uint64_t n) { return n != 0; });
    if (!admit(transfer_memory::Add(graph_bytes, transfer_memory::Mul(active + 1, sizeof(idx_t)),
        transfer_memory::Mul(transfer_memory::Mul(2, size_t(parts)), sizeof(real_t))))) {
      stats.partition_seconds = world.seconds() - partition_started;
      return finish(false);
    }
    try {
      vtxdist.reserve(active + 1); vtxdist.push_back(0);
      for (auto n:stats.cells_before) if (n) vtxdist.push_back(vtxdist.back()+idx_t(n));
      tpwgts.assign(2*size_t(parts),real_t(1./parts));
    } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
    if (!vote()) { stats.partition_seconds=world.seconds()-partition_started; return finish(false); }
    MPI_Comm graph_comm=MPI_COMM_NULL;
    MPI_Comm_split(world.comm,input.empty() ? MPI_UNDEFINED : 0,world.rank,&graph_comm);
    idx_t ncon=2,nparts=parts,edgecut=0; real_t tolerance[2]{1.15,1.15}; int error=METIS_OK;
    if (!input.empty()) {
      if (vtxdist.size()==2) {
        idx_t nvertices=input.size(),options[METIS_NOPTIONS]; METIS_SetDefaultOptions(options); options[METIS_OPTION_SEED]=0;
        error=METIS_PartGraphKway(&nvertices,&ncon,xadj.data(),adjacency.data(),vwgt.data(),nullptr,nullptr,
                                &nparts,tpwgts.data(),tolerance,options,&edgecut,part.data());
      } else {
        idx_t wgtflag=2,numflag=0,options[3]{1,0,0};
        error=ParMETIS_V3_PartKway(vtxdist.data(),xadj.data(),adjacency.data(),vwgt.data(),nullptr,
                                 &wgtflag,&numflag,&ncon,&nparts,tpwgts.data(),tolerance,options,&edgecut,part.data(),&graph_comm);
      }
      MPI_Comm_free(&graph_comm);
    }
    if (error!=METIS_OK) failure.Set(1,0,"Native 3D weighted graph partitioning failed.");
    for (size_t i=0;i<part.size();++i) {
      if (part[i]<0 || part[i]>=parts) failure.Set(1,0,"Invalid native 3D partition destination.");
      colors[i]=int(part[i]);
    }
    stats.edgecut=CPassiveComm::Allreduce(uint64_t(edgecut),CPassiveComm::Op::MAX,world.comm);
#else
    failure.Set(1,0,"Native 3D weighted partition requires MPI and ParMETIS.");
#endif
  }
  stats.partition_seconds=world.seconds()-partition_started;
  if (!vote()) return finish(false);
  const double migration_started=world.seconds();
  auto migration_failure=[&]() { stats.migration_seconds=world.seconds()-migration_started; return finish(false); };
  std::vector<std::vector<WeightedCell>> send;
  if (!admit(transfer_memory::Add(graph_bytes,transfer_memory::GrowthBound(input.size(),sizeof(WeightedCell)),
      transfer_memory::Mul(world.size,sizeof(std::vector<WeightedCell>))))) return migration_failure();
  uint64_t moved=0;
  try {
    send.resize(world.size);size_t i=0;
    for (const auto& row:input) { send[colors[i]].push_back({row.second,weights[i]});moved+=colors[i++]!=world.rank; }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!vote()) return migration_failure();
  auto received=world.exchange(send,&admitted,ceiling,graph_bytes);
  if (!admitted) { failure.Set(3,0,"Native 3D partition migration exceeds storage budget."); vote(); return migration_failure(); }
  const size_t staging=transfer_memory::Add(graph_bytes,transfer_memory::Bytes(send),transfer_memory::Bytes(received),MapBytes(received.size()));
  if (!admit(staging)) return migration_failure();
  Owned replacement;uint64_t received_work=0;
  try {
    for (auto& row:received) {
      if (!replacement.emplace(row.cell.cell.id,std::move(row.cell)).second) failure.Set(1,0,"Duplicate native 3D migrated cell identity.");
      received_work+=row.weight;
    }
  } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
  if (!vote()) return migration_failure();
  if (!loads(replacement.size(),received_work,stats.cells_after,stats.work_after,staging)) return migration_failure();
  if (std::accumulate(stats.cells_after.begin(),stats.cells_after.end(),uint64_t(0))!=total_cells ||
      std::accumulate(stats.work_after.begin(),stats.work_after.end(),uint64_t(0))!=total_work)
    failure.Set(1,0,"Native 3D partition changed total cells/work.");
  if (!vote()) return migration_failure();
  stats.moved_cells=CPassiveComm::Allreduce(moved,CPassiveComm::Op::SUM,world.comm);
  input.swap(replacement);stats.destinations.swap(colors);  // No allocation follows the common publication vote.
  stats.migration_seconds=world.seconds()-migration_started;
  return finish(true);
}
}  // namespace SU2Native3D
