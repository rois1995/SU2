/*!
 * \file b0_e0.cpp
 * \brief E0 full-state snapshots, protocol hashes and damage-location artifacts.
 */
#include "b0_driver.hpp"

#include <cstring>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <type_traits>

namespace b0 {
namespace {
/*--- Versioned local binary archive; no struct padding or pointer values on disk. ---*/
struct Archive {
  std::iostream& f;
  bool read;
  template<class T> void io(T& x) {
    static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value, "archive scalar");
    if (read) f.read(reinterpret_cast<char*>(&x), sizeof(x));
    else f.write(reinterpret_cast<const char*>(&x), sizeof(x));
    if (!f) throw std::runtime_error("truncated or unwritable E0 snapshot");
  }
  void io(string& s) {
    uint64_t n = s.size(); io(n);
    if (n > (1ull << 30)) throw std::runtime_error("invalid snapshot string length");
    if (read) s.resize(n);
    if (read) f.read(s.data(), n); else f.write(s.data(), n);
    if (!f) throw std::runtime_error("snapshot string IO failed");
  }
  template<class T> void io(vector<T>& v) {
    uint64_t n = v.size(); io(n);
    if (n > (1ull << 30)) throw std::runtime_error("invalid snapshot vector length");
    if (read) v.resize(n);
    if constexpr (std::is_arithmetic<T>::value) {
      if (read) f.read(reinterpret_cast<char*>(v.data()), n * sizeof(T));
      else f.write(reinterpret_cast<const char*>(v.data()), n * sizeof(T));
      if (!f) throw std::runtime_error("snapshot vector IO failed");
    } else for (auto& x : v) io(x);
  }
  template<class T, size_t N> void io(std::array<T, N>& a) { for (auto& x : a) io(x); }
};
void MeshIO(Archive& a, Mesh& m) {
  a.io(m.dim); a.io(m.X); a.io(m.M); a.io(m.gid); a.io(m.vflag); a.io(m.T); a.io(m.Tref);
  a.io(m.F); a.io(m.Fref); a.io(m.Fstate); a.io(m.markerName); a.io(m.markerRef); a.io(m.gidEnd);
  if (m.dim != 2 && m.dim != 3) throw std::runtime_error("invalid snapshot dimension");
  if (m.X.size() % m.dim || m.M.size() != (size_t)m.nv() * m.nm() || m.gid.size() != (size_t)m.nv() ||
      m.vflag.size() != (size_t)m.nv() || m.T.size() % m.nn() || m.Tref.size() != (size_t)m.ne() ||
      m.F.size() % m.dim || m.Fref.size() != (size_t)m.nf() || m.Fstate.size() != (size_t)m.nf() ||
      m.markerName.size() != m.markerRef.size()) throw std::runtime_error("invalid snapshot mesh arrays");
  for (i64 v : m.T) if (v < 0 || v >= m.nv()) throw std::runtime_error("invalid snapshot connectivity");
  for (i64 v : m.F) if (v < 0 || v >= m.nv()) throw std::runtime_error("invalid snapshot face");
}
void ParamsIO(Archive& a, Params& p) {
  a.io(p.hmin); a.io(p.hmax); a.io(p.hgrad); a.io(p.hausd); a.io(p.angle); a.io(p.surface); a.io(p.bl);
}
void EngineIO(Archive& a, EngineOpts& p) {
  a.io(p.floorArt); a.io(p.background); a.io(p.mmgMemMB); a.io(p.mmgMemFactor); a.io(p.mmgBytesPerElem);
  a.io(p.mmgMemMin); a.io(p.verbosity); a.io(p.injectStage); a.io(p.injectStep); a.io(p.injectPiece);
  a.io(p.injectLabel); a.io(p.injectTarget);
}
void RunIO(Archive& a, RunOpts& p) {
  a.io(p.placement); a.io(p.P); a.io(p.sched); a.io(p.L); a.io(p.a); a.io(p.b); a.io(p.capMemFactor);
  a.io(p.maxLevels); a.io(p.batchBudgetC); a.io(p.batchBudgetD); a.io(p.cE); a.io(p.memBudgetMB);
  a.io(p.margin); a.io(p.alphaIn); a.io(p.alphaOut); a.io(p.level0Stats); a.io(p.out); a.io(p.dumpVtu);
  a.io(p.artifacts); a.io(p.snapshotStep); a.io(p.allow); a.io(p.serialRefs); a.io(p.verbose);
}
void RegistryIO(Archive& a, FormerFaces& f) {
  a.io(f.dim);
  if (f.dim != 2 && f.dim != 3) throw std::runtime_error("invalid registry dimension");
  a.io(f.fx); a.io(f.gids); a.io(f.step); a.io(f.kind); a.io(f.piece); a.io(f.beforeQ); a.io(f.afterQ);
  if (f.fx.size() % (f.dim * f.dim) || f.gids.size() != (size_t)f.nFaces() * f.dim ||
      f.step.size() != (size_t)f.nFaces() || f.kind.size() != f.step.size() || f.piece.size() != f.step.size() ||
      f.beforeQ.size() != f.step.size() || f.afterQ.size() != f.step.size()) throw std::runtime_error("invalid registry arrays");
}
uint64_t BytesHash(const string& s) {
  uint64_t h = 1469598103934665603ull;
  for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
  return h;
}
uint64_t SnapshotHash(const string& file, uint64_t length) {
  std::ifstream f(file, std::ios::binary);
  uint64_t h = 1469598103934665603ull;
  char bytes[8192];
  while (length) {
    const auto n = std::min<uint64_t>(length, sizeof(bytes));
    f.read(bytes, n);
    if (!f) throw std::runtime_error("truncated snapshot checksum");
    for (uint64_t i = 0; i < n; ++i) { h ^= (unsigned char)bytes[i]; h *= 1099511628211ull; }
    length -= n;
  }
  return h;
}
vector<i64> Gids(const Mesh& m, const vector<i64>& vertices) {
  vector<i64> g;
  for (i64 v : vertices) g.push_back(m.gid.at(v));
  std::sort(g.begin(), g.end()); g.erase(std::unique(g.begin(), g.end()), g.end());
  return g;
}
}  // namespace

uint64_t Driver::StateHash() const {
  std::stringstream buf;
  Archive a{buf, false};
  auto mh = MeshHash(mesh); a.io(mh);
  /*--- Registry order is canonical; sample/k-d/BVH acceleration structures are derived. ---*/
  vector<string> registry;
  for (i64 f = 0; f < ff.nFaces(); ++f) {
    std::stringstream row;
    Archive r{row, false};
    vector<vector<double>> vertices;
    for (int v = 0; v < ff.dim; ++v) vertices.push_back(vector<double>(ff.fx.begin() + f * ff.dim * ff.dim + v * ff.dim,
                                                                      ff.fx.begin() + f * ff.dim * ff.dim + (v + 1) * ff.dim));
    std::sort(vertices.begin(), vertices.end()); r.io(vertices);
    int step = ff.step.at(f), kind = ff.kind.at(f); i64 piece = ff.piece.at(f);
    r.io(step); r.io(kind); r.io(piece);
    vector<i64> gids(ff.gids.begin() + f * ff.dim, ff.gids.begin() + (f + 1) * ff.dim);
    std::sort(gids.begin(), gids.end()); r.io(gids);
    double before = ff.beforeQ.at(f), after = ff.afterQ.at(f); r.io(before); r.io(after);
    registry.push_back(row.str());
  }
  std::sort(registry.begin(), registry.end()); a.io(registry);
  auto c = Gids(mesh, CoverageSet()), d = Gids(mesh, DefectSet()); a.io(c); a.io(d);
  auto k = K, w = W; std::sort(k.begin(), k.end()); std::sort(w.begin(), w.end()); a.io(k); a.io(w);
  std::map<i64, std::array<double, 6>> floor(eng.physFloor.begin(), eng.physFloor.end());
  for (auto x : floor) { auto gid = x.first; a.io(gid); a.io(x.second); }
  return BytesHash(buf.str());
}

uint64_t Driver::PlanHash(const vector<PieceIn>& pieces) const {
  std::stringstream buf;
  Archive a{buf, false};
  auto state = StateHash(); a.io(state);
  vector<string> records;
  for (const auto& p : pieces) {
    std::stringstream row; Archive r{row, false};
    auto label = p.label; auto rank = p.vrank; auto kind = p.kind;
    r.io(label); r.io(rank); r.io(kind);
    vector<vector<i64>> keys;
    for (i64 e : p.elems) {
      vector<i64> key;
      for (int v = 0; v < mesh.nn(); ++v) key.push_back(mesh.gid[mesh.T[mesh.nn() * e + v]]);
      std::sort(key.begin(), key.end()); key.push_back(mesh.Tref[e]); keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end()); r.io(keys);
    records.push_back(row.str());
  }
  std::sort(records.begin(), records.end()); a.io(records);
  return BytesHash(buf.str());
}

static void StateIO(Archive& a, Driver& d, vector<PieceIn>& pieces, StepRecord& rec) {
  string magic = "B0STATE2"; a.io(magic);
  if (magic != "B0STATE2") throw std::runtime_error("unsupported E0 snapshot version");
  uint32_t endian = 0x01020304; a.io(endian);
  if (endian != 0x01020304) throw std::runtime_error("snapshot endian mismatch");
  uint64_t expected = a.read ? 0 : d.StateHash(), inputHash = a.read ? 0 : MeshHash(d.input);
  uint64_t planHash = a.read ? 0 : d.PlanHash(pieces);
  a.io(expected); a.io(inputHash); a.io(planHash);
  ParamsIO(a, d.prm); MeshIO(a, d.input); MeshIO(a, d.mesh);
  if (MeshHash(d.input) != inputHash) throw std::runtime_error("immutable input/background hash mismatch");
  EngineIO(a, d.eng.opt); RunIO(a, d.ro);
  a.io(d.eng.refArt); a.io(d.eng.domainSize);
  uint64_t n = d.eng.physFloor.size(); a.io(n);
  if (n > (uint64_t)d.input.nv()) throw std::runtime_error("invalid floor count");
  if (a.read) d.eng.physFloor.clear();
  if (a.read) for (uint64_t i = 0; i < n; ++i) {
    i64 gid; std::array<double, 6> mt; a.io(gid); a.io(mt);
    if (!d.eng.physFloor.emplace(gid, mt).second) throw std::runtime_error("duplicate physical floor gid");
  } else {
    std::map<i64, std::array<double, 6>> floor(d.eng.physFloor.begin(), d.eng.physFloor.end());
    for (auto x : floor) { auto gid = x.first; a.io(gid); a.io(x.second); }
  }
  RegistryIO(a, d.ff); a.io(d.K); a.io(d.W); a.io(d.level0Junction);
  auto c = a.read ? vector<i64>() : Gids(d.mesh, d.CoverageSet());
  auto defect = a.read ? vector<i64>() : Gids(d.mesh, d.DefectSet());
  a.io(c); a.io(defect);
  a.io(d.stepCounter); a.io(d.status); a.io(d.stopReason); a.io(d.planMismatches); a.io(d.injectionsObserved);
  a.io(rec.kind); a.io(rec.step); a.io(rec.L); a.io(rec.C); a.io(rec.D); a.io(rec.capLoad); a.io(rec.capMem); a.io(rec.nExcluded); a.io(rec.note);
  n = pieces.size(); a.io(n);
  if (n > (uint64_t)d.mesh.ne()) throw std::runtime_error("invalid piece count");
  if (a.read) pieces.resize(n);
  for (auto& p : pieces) {
    a.io(p.elems); a.io(p.label); a.io(p.vrank); a.io(p.kind); a.io(p.predOut);
    vector<vector<i64>> actual;
    if (p.elems.empty() || p.vrank < 0 || p.vrank >= d.ro.P) throw std::runtime_error("invalid snapshot piece");
    for (i64 e : p.elems) {
      if (e < 0 || e >= d.mesh.ne()) throw std::runtime_error("invalid snapshot piece element");
      vector<i64> key;
      for (int v = 0; v < d.mesh.nn(); ++v) key.push_back(d.mesh.gid[d.mesh.T[d.mesh.nn() * e + v]]);
      std::sort(key.begin(), key.end()); actual.push_back(key);
    }
    auto keys = actual; a.io(keys);
    if (keys != actual) throw std::runtime_error("snapshot piece element-key mismatch");
  }
  n = d.references.size(); a.io(n);
  if (n > 1000) throw std::runtime_error("invalid reference count");
  if (a.read) d.references.resize(n);
  for (auto& m : d.references) {
    uint64_t h = a.read ? 0 : MeshHash(m); a.io(h); MeshIO(a, m);
    if (MeshHash(m) != h) throw std::runtime_error("immutable serial reference hash mismatch");
  }
  if (a.read) {
    if (Gids(d.mesh, d.CoverageSet()) != c || Gids(d.mesh, d.DefectSet()) != defect)
      throw std::runtime_error("snapshot obligation mismatch");
    if (d.StateHash() != expected || d.PlanHash(pieces) != planHash) throw std::runtime_error("snapshot state/plan hash mismatch");
    d.eng.prm = d.prm; d.adj.Build(d.mesh); d.ff.Index();
    d.bg.mesh = &d.input; d.bg.loc.Build(d.input.dim, d.input.X.data(), d.input.T.data(), d.input.ne()); d.eng.bg = &d.bg;
  }
}

void Driver::SaveState(const string& file, const vector<PieceIn>& pieces, const StepRecord& rec) const {
  /*--- Serialization takes mutable references but the writer does not alter the state. ---*/
  auto& self = const_cast<Driver&>(*this);
  std::fstream f(file, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!f) throw std::runtime_error("cannot write snapshot " + file);
  Archive a{f, false}; auto ps = pieces; auto sr = rec;
  StateIO(a, self, ps, sr);
  f.flush();
  if (!f) throw std::runtime_error("snapshot flush failed");
  const uint64_t length = f.tellp();
  f.close();
  if (!f) throw std::runtime_error("snapshot close failed");
  const uint64_t checksum = SnapshotHash(file, length);
  std::ofstream trailer(file, std::ios::binary | std::ios::app);
  trailer.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
  trailer.close();
  if (!trailer) throw std::runtime_error("snapshot checksum write failed");
}
void Driver::LoadState(const string& file, vector<PieceIn>& pieces, StepRecord& rec) {
  CommRank(MPI_COMM_WORLD, &rank); CommSize(MPI_COMM_WORLD, &size); tStart = Now();
  std::fstream f(file, std::ios::binary | std::ios::in);
  if (!f) throw std::runtime_error("cannot read snapshot " + file);
  f.seekg(0, std::ios::end);
  const auto size = f.tellg();
  if (size < (std::streamoff)sizeof(uint64_t)) throw std::runtime_error("truncated snapshot");
  f.seekg(size - (std::streamoff)sizeof(uint64_t));
  uint64_t checksum; f.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
  if (checksum != SnapshotHash(file, (uint64_t)size - sizeof(uint64_t))) throw std::runtime_error("snapshot checksum mismatch");
  f.seekg(0);
  Archive a{f, true}; StateIO(a, *this, pieces, rec);
  a.io(checksum);
  if (f.peek() != std::char_traits<char>::eof()) throw std::runtime_error("trailing snapshot data");
}

namespace {
// Packed little-endian rows, no padding. Batch IO instead of formatting millions of numbers.
struct CellBuffer {
  std::ostream& out;
  string bytes;
  explicit CellBuffer(std::ostream& f) : out(f) { bytes.reserve(1 << 20); }
  template<class T> void put(T value) { bytes.append(reinterpret_cast<const char*>(&value), sizeof(value)); }
  void flush() {
    out.write(bytes.data(), bytes.size()); bytes.clear();
    if (!out) throw std::runtime_error("cannot write cell artifact payload");
  }
};
void WriteCellRows(std::ostream& out, const vector<CellStat>& cells, int dim) {
  CellBuffer b(out);
  const int edges = dim * (dim + 1) / 2;
  for (const auto& c : cells) {
    if ((int)c.edges.size() != edges) throw std::runtime_error("invalid cell edge count");
    b.put(uint8_t(c.cls)); b.put(uint8_t(c.ar)); b.put(uint8_t(c.db));
    // Quality and distance retain float64: gate/tail thresholds, distance bins and 1e300 sentinel.
    b.put(c.quality); b.put(c.distance); b.put(int64_t(c.face));
    // Edges are descriptive only; float32 never enters calibration or attribution.
    for (double edge : c.edges) {
      if (!std::isfinite(edge) || edge > std::numeric_limits<float>::max())
        throw std::runtime_error("cell edge outside float32 range");
      b.put(float(edge));
    }
    if (b.bytes.size() >= (1 << 20)) b.flush();
  }
  b.flush();
}
}  // namespace

void Driver::WriteArtifacts(const string& prefix, bool includeReferences) const {
  if (rank != 0) return;
  const uint32_t endian = 1;
  if (*reinterpret_cast<const uint8_t*>(&endian) != 1) throw std::runtime_error("cell artifact requires little endian");
  StepRecord rec; rec.kind = "final"; rec.step = stepCounter;
  if (includeReferences) SaveState(prefix + "_final.b0state", {}, rec);
  else WriteB0In(prefix + ".b0in", mesh, prm);
  std::ofstream data(prefix + "_cells.bin.tmp", std::ios::binary);
  if (!data) throw std::runtime_error("cannot open cell artifact payload");
  std::ostringstream header; header << std::setprecision(17);
  header << "{\"format\":\"B0CELLS1\",\"dim\":" << mesh.dim << ",\"hash\":\"" << StateHash() << "\",\"candidate\":";
  auto write = [&](const Mesh& m, const vector<CellStat>& cells) {
    header << "{\"nv\":" << m.nv() << ",\"ne\":" << m.ne() << ",\"offset\":" << data.tellp() << "}";
    WriteCellRows(data, cells, m.dim);
  };
  const auto cells = CellStats(mesh, bg, &ff);
  write(mesh, cells);
  header << ",\"serial\":[";
  if (includeReferences) for (size_t i = 0; i < references.size(); ++i) {
    if (i) header << ",";
    write(references[i], CellStats(references[i], bg, &ff));
  }
  header << "],\"faces\":{\"offset\":" << data.tellp() << ",\"count\":" << ff.nFaces() << "}";
  const auto finalQ = FaceQualityMap(mesh, bg);
  const auto geometricQ = FrozenFaceFinalQuality(mesh, ff, cells);
  CellBuffer faces(data);
  for (i64 f = 0; f < ff.nFaces(); ++f) {
    const auto it = finalQ.find(MakeKey(ff.dim, &ff.gids[ff.dim * f]));
    faces.put(int32_t(ff.step[f])); faces.put(int32_t(ff.kind[f])); faces.put(int64_t(ff.piece[f]));
    faces.put(ff.beforeQ[f]); faces.put(ff.afterQ[f]); faces.put(geometricQ[f]);
    faces.put(uint8_t(it != finalQ.end()));
    if (faces.bytes.size() >= (1 << 20)) faces.flush();
  }
  faces.flush();
  header << ",\"bytes\":" << data.tellp() << "}\n";
  data.close();
  if (!data) throw std::runtime_error("cannot close cell artifact payload");
  // Publish the small header only after the complete payload has been closed.
  WriteText(prefix + "_cells.json.tmp", header.str());
  if (std::rename((prefix + "_cells.bin.tmp").c_str(), (prefix + "_cells.bin").c_str()) ||
      std::rename((prefix + "_cells.json.tmp").c_str(), (prefix + "_cells.json").c_str()))
    throw std::runtime_error("cannot publish cell artifacts");
}

}  // namespace b0

namespace b0 {
void E0SelfTest() {
  auto check = [](bool ok, const char* what) { if (!ok) throw std::runtime_error(what); };
  Mesh m;
  m.dim = 2; m.X = {0, 0, 1, 0, 0, 1}; m.M = {1, 0, 1, 1, 0, 1, 1, 0, 1};
  m.gid = {7, 8, 9}; m.gidEnd = 10; m.vflag = {0, 0, 0}; m.T = {0, 1, 2}; m.Tref = {3};
  m.F = {0, 1, 1, 2, 2, 0}; m.Fref = {1, 1, 1}; m.Fstate = {F_FIXED, F_FIXED, F_FIXED};
  m.markerName = {"wall"}; m.markerRef = {1};
  Mesh keys = m; keys.gid.push_back(10); keys.T.insert(keys.T.end(), {0, 1, 3});
  check(CanonicalKeyHash(keys, 0) != CanonicalKeyHash(keys, 1), "piece ids distinguish shared minimum gid");
  const auto original = MeshHash(m);
  auto changed = m; ++changed.gidEnd; check(MeshHash(changed) != original, "gid counter hash");
  changed = m; ++changed.Tref[0]; check(MeshHash(changed) != original, "element ref hash");
  changed = m; changed.Fstate[0] = F_DONE; check(MeshHash(changed) != original, "face state hash");
  changed = m; changed.vflag[0] = V_DEFECT; check(MeshHash(changed) != original, "vertex flag hash");
  changed = m; changed.markerName[0] = "other"; check(MeshHash(changed) != original, "marker hash");
  changed = m; changed.M[0] = 2; check(MeshHash(changed) != original, "carried metric hash");
  changed = m;
  for (int v = 0; v < 3; ++v) {
    changed.gid[v] = m.gid[2 - v];
    for (int d = 0; d < 2; ++d) changed.X[2 * v + d] = m.X[2 * (2 - v) + d];
  }
  for (auto& v : changed.T) v = 2 - v;
  for (auto& v : changed.F) v = 2 - v;
  check(MeshHash(changed) == original, "canonical numbering hash");
  Driver d; d.Setup(m, Params());
  // Geometry and target-metric faults must still fail in every scoring path.
  auto checkInvalidCells = [&](const Mesh& base) {
    Adjacency ad; ad.Build(base);
    for (int fault = 0; fault < 7; ++fault) {
      Mesh invalid = base;
      Mesh field = base;
      if (fault == 0) invalid.X.back() = 0;  // zero volume
      if (fault == 1) std::swap(invalid.T[1], invalid.T[2]);  // inverted
      if (fault == 2) invalid.X.back() = std::numeric_limits<double>::quiet_NaN();
      if (fault >= 3 && fault <= 5)
        for (i64 v = 0; v < field.nv(); ++v)
          field.M[field.nm() * v] = fault == 3 ? std::numeric_limits<double>::quiet_NaN() : fault == 4 ? 0 : -1;
      Background target; target.mesh = &field;
      if (fault != 6) target.loc.Build(base.dim, base.X.data(), base.T.data(), base.ne());
      bool cellsRejected = false, statsRejected = false, classifierRejected = false;
      try { CellStats(invalid, target, nullptr); } catch (const std::runtime_error&) { cellsRejected = true; }
      try { ComputeStats(invalid, target, StatsOpts()); } catch (const std::runtime_error&) { statsRejected = true; }
      try { Classify(invalid, target, ad, nullptr, 0, nullptr); } catch (const std::runtime_error&) { classifierRejected = true; }
      if (!cellsRejected || !statsRejected || !classifierRejected)
        throw std::runtime_error(Fmt("invalid %dD cell fault %d not rejected: cells=%d stats=%d classifier=%d",
                                    base.dim, fault, cellsRejected, statsRejected, classifierRejected));
    }
    Mesh carried = base;
    std::fill(carried.M.begin(), carried.M.end(), 0);  // Synthetic non-SPD ridge tensors.
    const auto carriedHash = MeshHash(carried), targetHash = MeshHash(base);
    Background target; target.mesh = &base;
    target.loc.Build(base.dim, base.X.data(), base.T.data(), base.ne());
    check(CellStats(carried, target, nullptr)[0].quality > 0, "ridge cell target quality");
    check(ComputeStats(carried, target, StatsOpts()).qmin > 0, "ridge statistics target quality");
    const auto classified = Classify(carried, target, ad, nullptr, 0, nullptr);
    check(std::accumulate(classified.allCells, classified.allCells + 8, i64(0)) == base.ne(), "ridge classifier population");
    check(MeshHash(carried) == carriedHash && MeshHash(base) == targetHash, "scoring preserves carried and target tensors");
  };
  checkInvalidCells(m);
  auto hash = d.StateHash(); d.K = {7}; check(d.StateHash() != hash, "K obligation hash");
  d.K.clear(); d.W = {8}; check(d.StateHash() != hash, "W obligation hash"); d.W.clear();
  d.ff.Add(m, {7, 8}); d.ff.step = {0}; d.ff.kind = {0}; d.ff.piece = {11};
  d.ff.beforeQ = {0.5}; d.ff.afterQ = {0.4}; d.ff.Index();
  check(d.StateHash() != hash, "registry hash");
  hash = d.StateHash(); d.ff.kind[0] = 2; check(d.StateHash() != hash, "registry provenance hash");
  PieceIn p; p.elems = {0}; p.label = 11;
  const auto plan = d.PlanHash({p}); p.vrank = 1; check(d.PlanHash({p}) != plan, "plan assignment hash");
  Sym M; M.a[0][0] = 4; M.a[1][1] = 9;
  double x[2] = {0.37, 0.2};
  check(std::fabs(d.ff.Dist(x, M) - 0.6) < 1e-12, "exact segment interior projection");
  check(d.ff.DistApprox(x, M) > d.ff.Dist(x, M), "old sample distance differs");
  FormerFaces many;
  double coords[4];
  for (int f = 0; f < 80; ++f) {
    coords[0] = 0.01 * f; coords[1] = 0; coords[2] = 0.01 * f; coords[3] = 1;
    many.AddCoords(2, coords, 1);
  }
  many.Index(); M.a[0][1] = M.a[1][0] = 2;
  for (int i = 0; i < 30; ++i) {
    x[0] = i * 0.037; x[1] = i * 0.019;
    check(std::fabs(many.Dist(x, M) - many.DistBrute(x, M)) < 1e-10, "conservative exact distance pruning");
  }
  FormerFaces tri;
  double t[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0}; tri.AddCoords(3, t, 1); tri.Index();
  Sym M3; M3.a[0][0] = M3.a[1][1] = 1; M3.a[2][2] = 4;
  double x3[3] = {0.2, 0.2, 0.3};
  check(std::fabs(tri.Dist(x3, M3) - 0.6) < 1e-12, "exact triangle interior projection");
  vector<CellStat> cells(1); cells[0].quality = 0.37;
  check(std::fabs(FrozenFaceFinalQuality(m, d.ff, cells)[0] - 0.37) < 1e-12, "retained-face adjacent quality");
  d.ff.fx = {0.2, 0.15, 0.7, 0.15};
  check(std::fabs(FrozenFaceFinalQuality(m, d.ff, cells)[0] - 0.37) < 1e-12, "removed-face geometric adjacent quality");
  d.ff.fx = {1.2, 1.15, 1.7, 1.15};
  check(FrozenFaceFinalQuality(m, d.ff, cells)[0] == -1, "outside former face has no adjacent cell");
  Mesh tet; tet.dim = 3; tet.X = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}; tet.T = {0, 1, 2, 3};
  tet.gid = {0, 1, 2, 3}; tet.gidEnd = 4; tet.vflag.assign(4, 0); tet.Tref = {0};
  tet.M = {1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1};
  checkInvalidCells(tet);
  tri.fx = {0.1, 0.1, 0.1, 0.5, 0.1, 0.1, 0.1, 0.5, 0.1};
  check(std::fabs(FrozenFaceFinalQuality(tet, tri, cells)[0] - 0.37) < 1e-12, "3D removed-face intersection quality");
  std::puts("E0 C++ selftest PASS: full/canonical state, plan hashes, exact distances, invalid 2D/3D cells");
}
}  // namespace b0
