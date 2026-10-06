/*!
 * \file b0_main.cpp
 * \brief B0 spike: command line, analytic cases, serial references, runs, JSON output.
 *
 * Usage:
 *   b0spike gen <box2d|box2x|box3d> <out.b0in> [n]
 *   b0spike serial <in.b0in> <outprefix> <nrenum> [first]      serial references: identity + renumbered runs
 *   b0spike run <in.b0in> [--key value ...]              partitioned run (see RunOpts)
 *   b0spike statecheck <state.b0state> <out.b0state>   full-state roundtrip
 *   b0spike replay <state.b0state> <all|piece-id> [outprefix]
 *   b0spike measure <state.b0state> <outprefix>        final state/cell/face artifacts
 *   b0spike mask <in.b0in> --serial <csv> --artifacts <prefix> [--P 8 --placement Pb]
 *   b0spike perturb <mask.b0state> <serial-index> <amplitude> <seed> <outprefix>
 *   b0spike distcheck <state.b0state> [sample-count]
 *   b0spike selftest unused
 *   b0spike mmgcap <in.b0in> <MB>                         MMG at a memory cap (one call, whole mesh)
 */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "b0_driver.hpp"

using namespace b0;

namespace {

/*--- minimal JSON writer ---*/
struct J {
  std::ostringstream s;
  J() { s.precision(17); }
  bool first = true;
  void sep() {
    if (!first) s << ",";
    first = false;
  }
  void key(const string& k) {
    sep();
    s << "\"" << k << "\":";
  }
  void num(const string& k, double v) {
    key(k);
    if (std::isfinite(v))
      s << v;
    else
      s << "null";
  }
  void str(const string& k, const string& v) {
    key(k);
    s << "\"";
    for (char c : v) {
      if (c == '"' || c == '\\') s << '\\';
      if (c == '\n') { s << "\\n"; continue; }
      s << c;
    }
    s << "\"";
  }
  void arr(const string& k, const vector<double>& v) {
    key(k);
    s << "[";
    for (size_t i = 0; i < v.size(); ++i) s << (i ? "," : "") << (std::isfinite(v[i]) ? v[i] : -1.0);
    s << "]";
  }
  void open(const string& k) {
    key(k);
    s << "{";
    first = true;
  }
  void openArr(const string& k) {
    key(k);
    s << "[";
    first = true;
  }
  void openAnon() {
    sep();
    s << "{";
    first = true;
  }
  void close() {
    s << "}";
    first = false;
  }
  void closeArr() {
    s << "]";
    first = false;
  }
  void raw(const string& k, const string& v) {
    key(k);
    s << v;
  }
};

void WriteStats(J& j, const string& name, const MeshStats& st) {
  j.open(name);
  j.num("nv", st.nv);
  j.num("ne", st.ne);
  j.num("q1", st.q1global);
  j.num("q5", st.q5global);
  j.num("qmin", st.qmin);
  j.num("edgeIn", st.edgeIn);
  j.num("edgeInNear", st.edgeInNear);
  j.num("nEdge", st.nEdge);
  j.num("nEdgeNear", st.nEdgeNear);
  j.num("badFrac", st.badFrac);
  j.num("nBad", st.nBad);
  j.num("complexityTarget", st.complexityTarget);
  j.num("complexityCarried", st.complexityCarried);
  j.arr("classQ1", vector<double>(st.classQ1, st.classQ1 + NAR));
  j.arr("classQ5", vector<double>(st.classQ5, st.classQ5 + NAR));
  j.arr("devMean", st.devMean);
  j.arr("devMax", st.devMax);
  j.num("depthP95", st.depthP95);
  j.num("depthP99", st.depthP99);
  j.num("depthMax", st.depthMax);
  j.num("nFacesDegraded", st.nFacesDegraded);
  j.openArr("regions");
  for (int a = 0; a < NAR; ++a)
    for (int d = 0; d < NDB; ++d) {
      const auto& r = st.reg[a][d];
      j.openAnon();
      j.num("ar", a);
      j.num("db", d);
      j.num("nCell", r.nCell);
      j.num("q1", r.q1);
      j.num("q5", r.q5);
      j.num("qmin", r.qmin);
      j.num("bad", r.bad);
      if (!st.regionQuality.empty()) j.arr("quality", st.regionQuality[a * NDB + d]);
      j.num("nEdge", r.nEdge);
      j.num("edgeIn", r.nEdge ? (double)r.nEdgeIn / r.nEdge : -1);
      j.close();
    }
  j.closeArr();
  j.close();
}

/*--- analytic cases ---*/
Mesh GenBox(int dim, int n, bool two, Params& prm, const string& kind) {
  Mesh m;
  m.dim = dim;
  const int nm = m.nm();
  auto addBox2 = [&](double x0) {
    const i64 base = m.nv();
    for (int j = 0; j <= n; ++j)
      for (int i = 0; i <= n; ++i) {
        m.X.push_back(x0 + (double)i / n);
        m.X.push_back((double)j / n);
      }
    auto id = [&](int i, int j) { return base + (i64)j * (n + 1) + i; };
    for (int j = 0; j < n; ++j)
      for (int i = 0; i < n; ++i) {
        const i64 a = id(i, j), b = id(i + 1, j), c = id(i + 1, j + 1), d = id(i, j + 1);
        if ((i + j) % 2 == 0) {
          m.T.insert(m.T.end(), {a, b, c, a, c, d});
        } else {
          m.T.insert(m.T.end(), {a, b, d, b, c, d});
        }
        m.Tref.push_back(0);
        m.Tref.push_back(0);
      }
    for (int i = 0; i < n; ++i) {
      m.F.insert(m.F.end(), {id(i, 0), id(i + 1, 0)});
      m.Fref.push_back(1);
      m.F.insert(m.F.end(), {id(n, i), id(n, i + 1)});
      m.Fref.push_back(2);
      m.F.insert(m.F.end(), {id(i, n), id(i + 1, n)});
      m.Fref.push_back(3);
      m.F.insert(m.F.end(), {id(0, i), id(0, i + 1)});
      m.Fref.push_back(4);
    }
  };
  if (dim == 2) {
    addBox2(0.0);
    if (two) addBox2(1.5);
    m.markerName = {"bottom", "right", "top", "left"};
    m.markerRef = {1, 2, 3, 4};
  } else {
    auto id = [&](int i, int j, int k) { return (i64)(k * (n + 1) + j) * (n + 1) + i; };
    for (int k = 0; k <= n; ++k)
      for (int j = 0; j <= n; ++j)
        for (int i = 0; i <= n; ++i) {
          m.X.push_back((double)i / n);
          m.X.push_back((double)j / n);
          m.X.push_back((double)k / n);
        }
    /*--- Kuhn subdivision: 6 tets per cube along the main diagonal ---*/
    static const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (int k = 0; k < n; ++k)
      for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i)
          for (int p = 0; p < 6; ++p) {
            int c[3] = {i, j, k};
            i64 v[4];
            v[0] = id(c[0], c[1], c[2]);
            for (int s = 0; s < 3; ++s) {
              ++c[perm[p][s]];
              v[s + 1] = id(c[0], c[1], c[2]);
            }
            m.T.insert(m.T.end(), v, v + 4);
            m.Tref.push_back(0);
          }
    /*--- boundary faces from the tets (faces without a neighbour) ---*/
    std::map<FKey, std::pair<int, std::array<i64, 3>>> cnt;
    for (i64 e = 0; e < m.ne(); ++e)
      for (int j = 0; j < 4; ++j) {
        i64 f[3];
        ElemFace(3, &m.T[4 * e], j, f);
        auto& c = cnt[MakeKey(3, f)];
        ++c.first;
        c.second = {f[0], f[1], f[2]};
      }
    for (auto& kv : cnt) {
      if (kv.second.first != 1) continue;
      const auto& f = kv.second.second;
      double c[3] = {0, 0, 0};
      for (int a = 0; a < 3; ++a)
        for (int d = 0; d < 3; ++d) c[d] += m.X[3 * f[a] + d] / 3;
      int ref = 0;
      if (c[0] < 1e-9) ref = 1;
      else if (c[0] > 1 - 1e-9) ref = 2;
      else if (c[1] < 1e-9) ref = 3;
      else if (c[1] > 1 - 1e-9) ref = 4;
      else if (c[2] < 1e-9) ref = 5;
      else ref = 6;
      m.F.insert(m.F.end(), f.begin(), f.end());
      m.Fref.push_back(ref);
    }
    m.markerName = {"xmin", "xmax", "ymin", "ymax", "zmin", "zmax"};
    m.markerRef = {1, 2, 3, 4, 5, 6};
  }
  /*--- fix orientation ---*/
  for (i64 e = 0; e < m.ne(); ++e)
    if (SignedVolume(dim, m.X.data(), &m.T[m.nn() * e]) < 0) std::swap(m.T[m.nn() * e], m.T[m.nn() * e + 1]);
  /*--- analytic metric ---*/
  m.M.resize(m.nv() * nm);
  for (i64 v = 0; v < m.nv(); ++v) {
    const double* x = &m.X[dim * v];
    double val[3], vec[3][3] = {{0}};
    if (dim == 2) {
      /*--- oblique anisotropic band through (0.5,0.5) (local x for the second box), normal at 30 deg ---*/
      const double xl = x[0] > 1.25 ? x[0] - 1.5 : x[0];
      const double theta = kind == "box2r" ? M_PI / 3 : M_PI / 6;
      const double nx = std::cos(theta), ny = std::sin(theta);
      const double d = (xl - 0.5) * nx + (x[1] - 0.5) * ny;
      const double h0 = kind == "imbal" ? (xl < 0.5 ? 0.004 : 0.01) : kind == "box2r" ? (xl < 0.4 ? 0.01 : 0.025) : 0.01;
      const double hn = h0 * (0.02 + 0.98 * (1 - std::exp(-std::fabs(d) / 0.05)));
      val[0] = 1 / (hn * hn);
      val[1] = 1 / (h0 * h0);
      vec[0][0] = nx; vec[0][1] = ny;
      vec[1][0] = -ny; vec[1][1] = nx;
    } else {
      /*--- oblique band (normal (1,1,0.5)) + an isotropic size jump across x = 0.5 ---*/
      const double nr = std::sqrt(2.25);
      double nvec[3] = {1 / nr, 1 / nr, 0.5 / nr};
      double radius = 0;
      if (kind == "box3c") {
        const double center[3] = {0.5, 0.5, -0.3};
        for (int d = 0; d < 3; ++d) { nvec[d] = x[d] - center[d]; radius += nvec[d] * nvec[d]; }
        radius = std::sqrt(radius);
        for (double& d : nvec) d /= radius;
      }
      const double d = kind == "box3c" ? radius - 0.9 : (x[0] - 0.5) * nvec[0] + (x[1] - 0.5) * nvec[1] + (x[2] - 0.5) * nvec[2];
      const double h0 = kind == "box3c" ? 0.08 : (x[0] >= 0.5 ? 0.04 : 0.08) * (kind == "box3s" ? 1.6 : 1.0);
      const double hn = h0 * (0.05 + 0.95 * (1 - std::exp(-std::fabs(d) / 0.05)));
      // orthonormal basis
      double t1[3] = {-nvec[1], nvec[0], 0};
      if (std::hypot(t1[0], t1[1]) < 1e-12) { t1[0] = 1; t1[1] = 0; t1[2] = 0; }
      const double l1 = std::sqrt(t1[0] * t1[0] + t1[1] * t1[1]);
      for (auto& c : t1) c /= l1;
      const double t2[3] = {nvec[1] * t1[2] - nvec[2] * t1[1], nvec[2] * t1[0] - nvec[0] * t1[2],
                            nvec[0] * t1[1] - nvec[1] * t1[0]};
      val[0] = 1 / (hn * hn);
      val[1] = val[2] = 1 / (h0 * h0);
      for (int d2 = 0; d2 < 3; ++d2) {
        vec[0][d2] = nvec[d2];
        vec[1][d2] = t1[d2];
        vec[2][d2] = t2[d2];
      }
    }
    const Sym s = Recompose(dim, val, vec);
    ToUpper(dim, s, &m.M[nm * v]);
  }
  m.gid.resize(m.nv());
  std::iota(m.gid.begin(), m.gid.end(), 0);
  m.gidEnd = m.nv();
  m.vflag.assign(m.nv(), 0);
  prm.hmin = 1e-6;
  prm.hmax = 1.0;
  prm.hgrad = 1.3;
  prm.hausd = 0.01;
  prm.angle = 45;
  prm.surface = dim == 3;  // 2D boxes with a fixed surface (ADAP_SURFACE= NO), 3D box with surface adaptation
  prm.bl = false;
  m.Fstate.assign(m.nf(), prm.surface ? F_UNTOUCHED : F_FIXED);
  return m;
}

/*--- random renumbering of vertices, elements and faces (serial noise, N0) ---*/
Mesh Renumber(const Mesh& m, uint64_t seed) {
  auto rnd = [&seed]() {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return seed >> 17;
  };
  vector<i64> pv(m.nv()), pe(m.ne()), pf(m.nf());
  std::iota(pv.begin(), pv.end(), 0);
  std::iota(pe.begin(), pe.end(), 0);
  std::iota(pf.begin(), pf.end(), 0);
  for (i64 i = m.nv() - 1; i > 0; --i) std::swap(pv[i], pv[rnd() % (i + 1)]);
  for (i64 i = m.ne() - 1; i > 0; --i) std::swap(pe[i], pe[rnd() % (i + 1)]);
  for (i64 i = m.nf() - 1; i > 0; --i) std::swap(pf[i], pf[rnd() % (i + 1)]);
  vector<i64> inv(m.nv());
  for (i64 i = 0; i < m.nv(); ++i) inv[pv[i]] = i;
  Mesh r = m;
  const int dim = m.dim, nm = m.nm(), nn = m.nn();
  for (i64 i = 0; i < m.nv(); ++i) {
    for (int d = 0; d < dim; ++d) r.X[dim * i + d] = m.X[dim * pv[i] + d];
    for (int c = 0; c < nm; ++c) r.M[nm * i + c] = m.M[nm * pv[i] + c];
    r.vflag[i] = m.vflag[pv[i]];
  }
  for (i64 i = 0; i < m.ne(); ++i) {
    for (int a = 0; a < nn; ++a) r.T[nn * i + a] = inv[m.T[nn * pe[i] + a]];
    r.Tref[i] = m.Tref[pe[i]];
  }
  /*--- faces stay grouped by marker (the serial path gives MMG the faces marker by marker) ---*/
  vector<i64> order;
  for (int ref : m.markerRef)
    for (i64 i = 0; i < m.nf(); ++i)
      if (m.Fref[pf[i]] == ref) order.push_back(pf[i]);
  for (i64 i = 0; i < m.nf(); ++i) {
    for (int a = 0; a < dim; ++a) r.F[dim * i + a] = inv[m.F[dim * order[i] + a]];
    r.Fref[i] = m.Fref[order[i]];
    r.Fstate[i] = m.Fstate[order[i]];
  }
  std::iota(r.gid.begin(), r.gid.end(), 0);
  return r;
}

bool ParseOpts(int argc, char** argv, int first, RunOpts& ro, EngineOpts& eo) {
  for (int i = first; i + 1 < argc; i += 2) {
    const string k = argv[i], v = argv[i + 1];
    if (k == "--placement") ro.placement = v;
    else if (k == "--P") ro.P = std::stoi(v);
    else if (k == "--sched") ro.sched = v;
    else if (k == "--L") ro.L = std::stoi(v);
    else if (k == "--a") ro.a = std::stod(v);
    else if (k == "--b") ro.b = std::stod(v);
    else if (k == "--capmem") ro.capMemFactor = std::stod(v);
    else if (k == "--levels") ro.maxLevels = std::stoi(v);
    else if (k == "--budgetC") ro.batchBudgetC = std::stoi(v);
    else if (k == "--cE") ro.cE = std::stod(v);
    else if (k == "--budget") ro.memBudgetMB = std::stod(v);
    else if (k == "--margin") ro.margin = std::stod(v);
    else if (k == "--alphaIn") ro.alphaIn = std::stod(v);
    else if (k == "--alphaOut") ro.alphaOut = std::stod(v);
    else if (k == "--out") ro.out = v;
    else if (k == "--artifacts") ro.artifacts = v;
    else if (k == "--snapshot-step") ro.snapshotStep = std::stoi(v);
    else if (k == "--budgetD") ro.batchBudgetD = std::stoi(v);
    else if (k == "--vtu") ro.dumpVtu = v;
    else if (k == "--level0stats") ro.level0Stats = std::stoi(v);
    else if (k == "--floor") eo.floorArt = std::stoi(v);
    else if (k == "--background") eo.background = std::stoi(v);
    else if (k == "--mmgmem") eo.mmgMemMB = std::stod(v);
    else if (k == "--mmgcapfactor") eo.mmgMemFactor = std::stod(v);
    else if (k == "--mmgbytes") eo.mmgBytesPerElem = std::stod(v);
    else if (k == "--mmgmin") eo.mmgMemMin = std::stod(v);
    else if (k == "--allow") {
      std::stringstream ss(v);
      string t;
      while (std::getline(ss, t, ',')) ro.allow.push_back(std::stod(t));
    } else if (k == "--serial") {
      std::stringstream ss(v);
      string t;
      while (std::getline(ss, t, ',')) ro.serialRefs.push_back(t);
    } else if (k == "--inject") {
      // stage:first|largest|piece-label:step (an unobserved request exits 2)
      std::stringstream ss(v);
      string s, a, b;
      std::getline(ss, s, ':');
      std::getline(ss, a, ':');
      std::getline(ss, b, ':');
      const char* names[] = {"none", "planning", "allocation", "migration", "mmg", "interpolation", "validation", "splice", "commit"};
      for (int q = 0; q < 9; ++q)
        if (s == names[q]) eo.injectStage = (Stage)q;
      if (eo.injectStage == Stage::NONE) throw std::runtime_error("unknown injection stage");
      eo.injectPiece = a.empty() ? "first" : a;
      eo.injectStep = b.empty() ? -1 : std::stoi(b);
    } else {
      std::cerr << "unknown option " << k << "\n";
      return false;
    }
  }
  if ((argc - first) % 2 || ro.P < 1 || ro.L < 1) throw std::runtime_error("invalid option pairs/P/L");
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (SerialExecution()) {
    const char* ranks = std::getenv("OMPI_COMM_WORLD_SIZE");
    if (ranks && std::atoi(ranks) > 1) {
      std::cerr << "B0_SERIAL_ONLY cannot run under MPI\n";
      return 1;
    }
  } else
    MPI_Init(&argc, &argv);
  int rank = 0, size = 1;
  CommRank(MPI_COMM_WORLD, &rank);
  CommSize(MPI_COMM_WORLD, &size);
  int ret = 0;
  try {
    string mode;
    int modeId = -1;
    CollectivePhase("command arguments", [&] {
      if (argc < 3) throw std::runtime_error("usage: b0spike gen|serial|run|mmgcap ...");
      mode = argv[1];
      const char* modes[] = {"selftest", "gen",       "statecheck", "replay",   "mask",   "perturb",
                             "measure",  "distcheck", "serial",     "classify", "mmgcap", "run"};
      for (int i = 0; i < 12; ++i)
        if (mode == modes[i]) modeId = i;
      int required = mode == "serial" ? 5
                                      : mode == "perturb" ? 7
                                                          : (mode == "gen" || mode == "statecheck" ||
                                                             mode == "replay" || mode == "measure" || mode == "mmgcap")
                                                                ? 4
                                                                : 3;
      if (argc < required) throw std::runtime_error("missing command arguments");
    });
    CollectiveDecision(modeId);
    if (mode == "selftest") {
      CollectivePhase("selftest command", [&] { E0SelfTest(); });
      // The same tests run in serial and with two ranks; only the last rank fails.
      CheckExchangeSize(INT_MAX);
      bool rejected = false;
      try {
        CheckExchangeSize(rank == size - 1 ? (size_t)INT_MAX + 1 : 0);
      } catch (const std::runtime_error&) {
        rejected = true;
      }
      CollectivePhase("exchange rejection selftest", [&] {
        if (!rejected) throw std::runtime_error("oversized exchange was not rejected");
      });
      rejected = false;
      try {
        CollectivePhase("asymmetric selftest", [&] {
          if (rank == size - 1) throw std::bad_alloc();
        });
      } catch (const std::runtime_error& e) {
        rejected = string(e.what()).find("failed on rank " + std::to_string(size - 1)) != string::npos;
      }
      CollectivePhase("failure propagation selftest", [&] {
        if (!rejected) throw std::runtime_error("local failure was not propagated");
      });
      if (size > 1) {
        rejected = false;
        try {
          CollectiveDecision(rank == 0);
        } catch (const std::runtime_error&) {
          rejected = true;
        }
        CollectivePhase("divergent decision selftest", [&] {
          if (!rejected) throw std::runtime_error("divergent decision was not rejected");
        });
      }
      if (rank == 0) std::cout << "E0 collective selftest PASS\n";
    } else if (mode == "gen") {
      CollectivePhase("gen command", [&] {
        Params prm;
        const string kind = argv[2];
        const int n =
            argc > 4 ? std::stoi(argv[4]) : (kind == "box3d" ? 16 : kind == "box3c" ? 12 : kind == "box3s" ? 10 : 60);
        Mesh m = (kind == "box3d" || kind == "box3s" || kind == "box3c")
                     ? GenBox(3, n, false, prm, kind)
                     : GenBox(2, n, kind == "box2x", prm, kind);  // box2i: imbalance
        if (rank == 0) WriteB0In(argv[3], m, prm);
      });
    } else if (mode == "statecheck" || mode == "replay") {
      Driver d;
      vector<PieceIn> pieces;
      StepRecord rec;
      uint64_t before = 0;
      CollectivePhase("snapshot load", [&] {
        d.LoadState(argv[2], pieces, rec);
        before = d.StateHash();
      });
      if (mode == "statecheck") {
        CollectivePhase("snapshot save", [&] {
          if (rank == 0) d.SaveState(argv[3], pieces, rec);
        });
        if (!SerialExecution()) MPI_Barrier(MPI_COMM_WORLD);
        CollectivePhase("snapshot reload", [&] {
          Driver again;
          vector<PieceIn> ps;
          StepRecord sr;
          again.LoadState(argv[3], ps, sr);
          if (again.StateHash() != before) throw std::runtime_error("snapshot roundtrip mismatch");
        });
        if (rank == 0) std::cout << "statecheck hash " << before << " PASS\n";
      } else {
        string selector, output;
        CollectivePhase("replay setup", [&] {
          if (pieces.empty()) throw std::runtime_error("final snapshot has no transaction to replay");
          selector = argv[3];
          output = argc > 4 ? argv[4] : "replay";
          d.ro.artifacts.clear();
        });
        if (CollectiveDecision(selector == "all")) {
          d.stepCounter = rec.step;
          vector<char> accepted;
          const bool committed = d.Transaction(pieces, rec, accepted);
          CollectivePhase("replay output", [&] {
            if (rank == 0) {
              WriteText(output + ".posthash", std::to_string(d.StateHash()) + "\n");
              std::cout << "replay committed " << committed << " hash " << d.StateHash() << "\n";
            }
            d.WriteArtifacts(output);
          });
        } else {
          CollectivePhase("piece replay", [&] {
            const auto it = std::find_if(pieces.begin(), pieces.end(),
                                         [&](const PieceIn& p) { return std::to_string(p.label) == selector; });
            if (it == pieces.end()) throw std::runtime_error("piece label absent from snapshot");
            if (rank == 0) {
              const auto out = d.eng.AdaptPiece(d.mesh, d.adj, *it, rec.step);
              std::ostringstream json;
              json << "{\"piece\":\"" << selector << "\",\"status\":" << out.st.code << ",\"nIn\":" << out.nInElem
                   << ",\"predOut\":" << it->predOut << ",\"nOut\":" << out.nOutElem << ",\"nArt\":" << out.nArtFaces
                   << "}\n";
              WriteText(output + ".json", json.str());
            }
          });
        }
      }
    } else if (mode == "mask") {
      CollectivePhase("mask command", [&] {
        Params prm, pp;
        Driver d;
        EngineOpts eo;
        const Mesh in = ReadB0In(argv[2], prm);
        d.Setup(in, prm);
        if (!ParseOpts(argc, argv, 3, d.ro, eo)) throw std::runtime_error("bad mask options");
        const auto cf = CutFaces(d.input, d.adj, d.Placement());
        vector<i64> faceGids;
        for (const auto& c : cf) faceGids.insert(faceGids.end(), c.key, c.key + d.input.dim);
        d.ff.Add(d.input, faceGids);
        for (const auto& c : cf) {
          (void)c;
          d.ff.step.push_back(0);
          d.ff.kind.push_back(0);
          d.ff.piece.push_back(0);
          d.ff.beforeQ.push_back(-1);
          d.ff.afterQ.push_back(-1);
        }
        d.ff.Index();
        for (const auto& file : d.ro.serialRefs) d.references.push_back(ReadB0In(file, pp));
        if (!d.references.empty()) {
          d.mesh = d.references[0];
          d.adj.Build(d.mesh);
        }
        d.WriteArtifacts(d.ro.artifacts.empty() ? d.ro.out : d.ro.artifacts);
      });
    } else if (mode == "perturb") {
      CollectivePhase("perturb command", [&] {
        if (argc != 7) throw std::runtime_error("usage: perturb mask.b0state serial-index amplitude seed outprefix");
        Driver d;
        vector<PieceIn> pieces;
        StepRecord rec;
        d.LoadState(argv[2], pieces, rec);
        d.mesh = d.references.at(std::stoi(argv[3]));
        /*--- Serial MMG's returned ridge tensors can be non-SPD; the power pilot
         *    uses the immutable target field, exactly as the evaluator does. ---*/
        for (i64 v = 0; v < d.mesh.nv(); ++v) {
          double mt[6];
          if (!InterpMetric(d.bg.loc, d.input.M.data(), d.mesh.dim, &d.mesh.X[d.mesh.dim * v], mt))
            throw std::runtime_error("power pilot vertex outside immutable target");
          std::copy_n(mt, d.mesh.nm(), &d.mesh.M[d.mesh.nm() * v]);
        }
        const double amplitude = std::stod(argv[4]);
        if (amplitude < 0) throw std::runtime_error("negative perturbation amplitude");
        uint64_t seed = std::stoull(argv[5]);
        auto rnd = [&]() {
          seed = seed * 6364136223846793005ull + 1442695040888963407ull;
          return (double)(seed >> 11) / 9007199254740992.0;
        };
        d.adj.Build(d.mesh);
        vector<char> boundary(d.mesh.nv(), 0);
        for (i64 v : d.mesh.F) boundary[v] = 1;
        int moved = 0, rejected = 0;
        for (i64 v = 0; v < d.mesh.nv(); ++v) {
          if (boundary[v]) continue;
          double mt[6];
          if (!InterpMetric(d.bg.loc, d.input.M.data(), d.mesh.dim, &d.mesh.X[d.mesh.dim * v], mt)) continue;
          const auto M = FromUpper(d.mesh.dim, mt);
          if (d.ff.Dist(&d.mesh.X[d.mesh.dim * v], M) >= 2) continue;
          double val[3], basis[3][3], old[3];
          Eig(d.mesh.dim, M, val, basis);
          for (int j = 0; j < d.mesh.dim; ++j) old[j] = d.mesh.X[d.mesh.dim * v + j];
          for (int j = 0; j < d.mesh.dim; ++j) {
            const double delta = amplitude * (2 * rnd() - 1) / std::sqrt(val[j]);
            for (int k = 0; k < d.mesh.dim; ++k) d.mesh.X[d.mesh.dim * v + k] += delta * basis[j][k];
          }
          bool valid = true;
          for (i64 q = d.adj.v2eStart[v]; q < d.adj.v2eStart[v + 1]; ++q)
            valid &= SignedVolume(d.mesh.dim, d.mesh.X.data(), &d.mesh.T[d.mesh.nn() * d.adj.v2e[q]]) > 0;
          if (!valid) {
            for (int j = 0; j < d.mesh.dim; ++j) d.mesh.X[d.mesh.dim * v + j] = old[j];
            ++rejected;
          } else {
            ++moved;
            if (InterpMetric(d.bg.loc, d.input.M.data(), d.mesh.dim, &d.mesh.X[d.mesh.dim * v], mt))
              std::copy_n(mt, d.mesh.nm(), &d.mesh.M[d.mesh.nm() * v]);
          }
        }
        const auto checked = AcceptanceChecks(d.mesh, nullptr, {}, d.mesh);
        if (!checked.ok) throw std::runtime_error("invalid perturbed mesh: " + checked.errors.front());
        if (rank == 0) std::cout << "perturb moved " << moved << " rejected " << rejected << "\n";
        d.WriteArtifacts(argv[6], false);
      });
    } else if (mode == "measure") {
      CollectivePhase("measure command", [&] {
        Driver d;
        vector<PieceIn> pieces;
        StepRecord rec;
        d.LoadState(argv[2], pieces, rec);
        d.WriteArtifacts(argv[3]);
      });
    } else if (mode == "distcheck") {
      CollectivePhase("distcheck command", [&] {
        Driver d;
        vector<PieceIn> pieces;
        StepRecord rec;
        d.LoadState(argv[2], pieces, rec);
        const int count = argc > 3 ? std::stoi(argv[3]) : 1000;
        double exactError = 0, approxError = 0;
        for (i64 k = 0; k < std::min<i64>(count, d.mesh.ne()); ++k) {
          const i64 e = k * d.mesh.ne() / std::min<i64>(count, d.mesh.ne());
          const auto x = ElemCentroid(d.mesh, e);
          double mt[6];
          InterpMetric(d.bg.loc, d.input.M.data(), d.mesh.dim, x.data(), mt);
          const auto M = FromUpper(d.mesh.dim, mt);
          const double exact = d.ff.DistBrute(x.data(), M);
          exactError = std::max(exactError, std::fabs(d.ff.Dist(x.data(), M) - exact) / std::max(exact, 1e-12));
          approxError = std::max(approxError, std::fabs(d.ff.DistApprox(x.data(), M) - exact) / std::max(exact, 1e-12));
        }
        if (rank == 0)
          std::cout << "{\"exactMaxRelativeError\":" << exactError << ",\"approxMaxRelativeError\":" << approxError
                    << "}\n";
        if (exactError > 1e-7) throw std::runtime_error("exact distance validation failed");
      });
    } else if (mode == "serial") {
      CollectivePhase("serial command", [&] {
        Params prm;
        Mesh in = ReadB0In(argv[2], prm);
        const string prefix = argv[3];
        const int nRen = std::stoi(argv[4]);
        Driver drv;
        drv.Setup(in, prm);
        J j;
        j.s << "{";
        j.num("nvIn", in.nv());
        j.num("neIn", in.ne());
        j.openArr("runs");
        for (int r = argc > 5 ? std::stoi(argv[5]) : 0; r <= nRen; ++r) {
          if (r % size != rank) continue;
          const bool twice = std::getenv("B0_TWICE") != nullptr;  // N1: the same call twice in one process
          const Mesh src = (r == 0 || twice) ? drv.input : Renumber(drv.input, 1000 + r);
          Engine eng = drv.eng;
          if (const char* mm = std::getenv("B0_MMGMEM")) eng.opt.mmgMemMB = std::atof(mm);  // N1: fixed MMG allowance
          // physFloor keyed by gid: renumbered meshes have new gids -> rebuild by coordinates
          if (!prm.surface && r > 0) {
            eng.physFloor.clear();
            Driver d2;
            d2.Setup(src, prm);
            eng.physFloor = d2.eng.physFloor;
          }
          Mesh srcFlag = src;
          for (i64 v = 0; v < srcFlag.nv(); ++v) srcFlag.vflag[v] = 0;
          {
            // corners
            vector<std::set<int>> refs(srcFlag.nv());
            for (i64 f = 0; f < srcFlag.nf(); ++f)
              for (int a = 0; a < srcFlag.dim; ++a) refs[srcFlag.F[srcFlag.dim * f + a]].insert(srcFlag.Fref[f]);
            for (i64 v = 0; v < srcFlag.nv(); ++v)
              if ((int)refs[v].size() >= srcFlag.dim) srcFlag.vflag[v] |= V_CORNER;
          }
          int st = 0;
          double t = 0, pk = 0;
          Mesh out = eng.SerialRemesh(srcFlag, st, t, pk);
          const string file = prefix + "_s" + std::to_string(r) + ".b0in";
          std::cout << "hash " << r << " " << MeshHash(out) << " nv " << out.nv() << std::endl;
          WriteB0In(file, out, prm);
          StatsOpts so;
          so.surfRef = prm.surface ? &drv.input : nullptr;
          const MeshStats ms = ComputeStats(out, drv.bg, so);
          Adjacency ad;
          ad.Build(out);
          const auto cl = Classify(out, drv.bg, ad, nullptr, 0, nullptr);
          J jr;
          jr.s << "{";
          jr.num("renum", r);
          jr.str("file", file);
          jr.num("status", st);
          jr.num("tMMG", t);
          jr.num("peakMB", pk);
          jr.str("hash", std::to_string(MeshHash(out)));
          WriteStats(jr, "stats", ms);
          jr.arr("classDensity", vector<double>(cl.allDensity, cl.allDensity + 8));
          jr.arr("classCells", vector<double>(cl.allCells, cl.allCells + 8));
          jr.s << "}";
          WriteText(prefix + "_s" + std::to_string(r) + ".json", jr.s.str() + "\n");
          if (rank == 0)
            std::cout << "serial run " << r << ": " << out.nv() << " vertices, MMG " << t << " s, peak " << pk
                      << " MB\n";
        }
        j.closeArr();
        j.s << "}";
      });
    } else if (mode == "classify") {
      CollectivePhase("classify command", [&] {
        /*--- classifier calibration on a serial mesh in the target metric (MMG's returned metric is not SPD at
         *    3D ridge points): class densities of bad cells ---*/
        Params prm, pp;
        Mesh in = ReadB0In(argv[2], prm);
        Driver drv;
        drv.Setup(in, prm);
        J jo;
        jo.s << "{";
        jo.openArr("meshes");
        for (int a = 3; a < argc; ++a) {
          const Mesh sm = ReadB0In(argv[a], pp);
          Adjacency ad;
          ad.Build(sm);
          const auto cl = Classify(sm, drv.bg, ad, nullptr, 0, nullptr);
          jo.openAnon();
          jo.str("file", argv[a]);
          jo.arr("classDensity", vector<double>(cl.allDensity, cl.allDensity + 8));
          jo.arr("classCells", vector<double>(cl.allCells, cl.allCells + 8));
          jo.close();
        }
        jo.closeArr();
        jo.s << "}";
        if (rank == 0) std::cout << jo.s.str() << std::endl;
      });
    } else if (mode == "mmgcap") {
      CollectivePhase("mmgcap command", [&] {
        Params prm;
        Mesh in = ReadB0In(argv[2], prm);
        Driver drv;
        drv.Setup(in, prm);
        Engine eng = drv.eng;
        eng.opt.mmgMemMB = std::stod(argv[3]);
        int st = -1;
        double t = 0, pk = 0;
        std::cout << "MMG with IPARAM_mem = " << argv[3] << " MB" << std::endl;
        try {
          Mesh out = eng.SerialRemesh(drv.input, st, t, pk);
          std::cout << "RESULT returned status " << st << " nv " << out.nv() << " t " << t << " peakMB " << pk
                    << " HWM " << HWMMB() << std::endl;
        } catch (const std::exception& e) {
          std::cout << "RESULT failure returned: " << e.what() << " status " << st << " HWM " << HWMMB() << std::endl;
        }
      });
    } else if (mode == "run") {
      Params prm, pp;
      Mesh in;
      Driver drv;
      EngineOpts eo;
      J j;
      vector<int> part;
      vector<double> pred;
      double t0 = 0, tPlace = 0;
      CollectivePhase("run setup", [&] {
        in = ReadB0In(argv[2], prm);
        if (!ParseOpts(argc, argv, 3, drv.ro, eo)) throw std::runtime_error("bad options");
        drv.Setup(in, prm);
        drv.eng.opt = eo;
        for (const auto& file : drv.ro.serialRefs) drv.references.push_back(ReadB0In(file, pp));
        t0 = Now();
        /*--- level-0 placement analysis (N2) ---*/
        j.s << "{";
        j.str("placement", drv.ro.placement);
        j.str("sched", drv.ro.sched);
        j.num("P", drv.ro.P);
        j.num("floor", eo.floorArt);
        j.num("background", eo.background);
        j.num("L", drv.ro.L);
        j.num("a", drv.ro.a);
        j.num("b", drv.ro.b);
        j.num("realRanks", size);
        j.num("nvIn", in.nv());
        j.num("neIn", in.ne());
        j.num("hausd", prm.hausd);
        j.num("surface", prm.surface);
        j.num("dim", in.dim);
        j.num("mmgmem", eo.mmgMemMB);
        j.num("capmem", drv.ro.capMemFactor);
        const double tp0 = Now();
        part = drv.Placement();
        tPlace = Now() - tp0;
        pred = drv.PredOut();
      });
      CollectiveDecision(drv.ro.sched == "sbase" ? 0
                                                 : drv.ro.sched == "sbatch"
                                                       ? 1
                                                       : drv.ro.sched == "sb4" ? 2 : drv.ro.sched == "level0" ? 3 : -1);
      {
        vector<CutFace> cf;
        CollectivePhase("partition statistics", [&] {
          vector<double> w(drv.ro.P, 0), nE(drv.ro.P, 0);
          for (i64 k = 0; k < drv.mesh.ne(); ++k) {
            w[part[k]] += 1 + pred[k];
            nE[part[k]] += 1;
          }
          double mx = 0, mean = 0;
          int nEmpty = 0;
          for (double x : w) {
            mx = std::max(mx, x);
            mean += x / drv.ro.P;
          }
          for (double x : nE) nEmpty += x == 0;
          cf = CutFaces(drv.mesh, drv.adj, part);
          const auto mr = MacroOrientation(drv.mesh, cf);
          /*--- nodal edge cut ---*/
          std::set<std::pair<i64, i64>> cutEdges;
          const int nn = drv.mesh.nn();
          for (auto& c : cf) {
            for (int a = 0; a < drv.mesh.dim; ++a)
              for (int b = a + 1; b < drv.mesh.dim; ++b)
                cutEdges.insert({std::min(c.key[a], c.key[b]), std::max(c.key[a], c.key[b])});
          }
          (void)nn;
          j.open("partition");
          j.num("tPlacement", tPlace);
          j.num("imbalanceWork", mean > 0 ? mx / mean : 0);
          j.num("emptyParts", nEmpty);
          j.num("cutFaces", cf.size());
          j.num("cutEdgesOnFaces", cutEdges.size());
          j.num("areaCut", mr.areaCut);
          j.num("areaAniso", mr.areaAniso);
          j.num("areaViol", mr.areaViol);
          j.num("areaUnres", mr.areaUnres);
          j.num("nAniso", mr.nAniso);
          j.num("nViol", mr.nViol);
          j.num("nUnres", mr.nUnres);
          j.num("longestViolRun", mr.longestViolRun);
          j.num("meanCf", mr.meanCf);
          j.num("p95Cf", mr.p95Cf);
          j.num("meanLogAR", mr.meanLogAR);
          j.close();
        });
        if (CollectiveDecision(drv.ro.level0Stats)) {
          /*--- keep the cut faces for the level-0 analysis after the step ---*/
          FormerFaces ff0;
          CollectivePhase("level 0 statistics setup", [&] {
            drv.log.str("");
            vector<i64> faceGids;
            for (const auto& c : cf) faceGids.insert(faceGids.end(), c.key, c.key + drv.mesh.dim);
            ff0.Add(drv.mesh, faceGids);
            ff0.Index();
          });
          drv.Level0();
          CollectivePhase("level 0 statistics", [&] {
            const double tL0 = Now() - t0;
            /*--- serial thresholds for "degraded": p1 per AR class of the identity serial mesh ---*/
            vector<Mesh> ser;
            vector<MeshStats> serSt;
            double degQ[NAR] = {0, 0, 0, 0};
            Params pp;
            for (size_t r = 0; r < drv.ro.serialRefs.size(); ++r) ser.push_back(ReadB0In(drv.ro.serialRefs[r], pp));
            if (!ser.empty()) {
              StatsOpts so;
              const MeshStats s0 = ComputeStats(ser[0], drv.bg, so);
              for (int a = 0; a < NAR; ++a) degQ[a] = s0.classQ1[a];
            }
            StatsOpts so;
            so.ff = &ff0;
            so.degradeQ = degQ;
            so.surfRef = prm.surface ? &drv.input : nullptr;
            const MeshStats cand = ComputeStats(drv.mesh, drv.bg, so);
            j.num("tLevel0", tL0);
            WriteStats(j, "level0", cand);
            j.openArr("level0Serial");
            for (auto& sm : ser) {
              const MeshStats s = ComputeStats(sm, drv.bg, so);
              j.sep();
              J jj;
              jj.s << "";
              WriteStats(jj, "x", s);
              string str = jj.s.str();
              j.s << str.substr(str.find(':') + 1);
            }
            j.closeArr();
            /*--- predictive value of macro-orientation: depth of violating vs lateral faces (aniso faces) ---*/
            double sumV = 0, sumL = 0, sumU = 0, sumI = 0;
            i64 nV = 0, nL = 0, nU = 0, nI = 0, degV = 0, degL = 0;
            for (size_t i = 0; i < cf.size() && i < cand.faceDepth.size(); ++i) {
              const double d = cand.faceDepth[i];
              if (cf[i].macro == 2) {
                sumV += d;
                ++nV;
                degV += d > 2;
              } else if (cf[i].macro == 1) {
                sumL += d;
                ++nL;
                degL += d > 2;
              } else if (cf[i].macro == 3) {
                sumU += d;
                ++nU;
              } else {
                sumI += d;
                ++nI;
              }
            }
            j.open("macroVsDepth");
            j.num("nViol", nV);
            j.num("meanDepthViol", nV ? sumV / nV : -1);
            j.num("fracDeepViol", nV ? (double)degV / nV : -1);
            j.num("nLateral", nL);
            j.num("meanDepthLateral", nL ? sumL / nL : -1);
            j.num("fracDeepLateral", nL ? (double)degL / nL : -1);
            j.num("nUnres", nU);
            j.num("meanDepthUnres", nU ? sumU / nU : -1);
            j.num("nIso", nI);
            j.num("meanDepthIso", nI ? sumI / nI : -1);
            j.close();
          });
        } else {
          drv.Level0();
        }
      }
      if (CollectiveDecision(drv.steps.empty() || drv.steps[0].committed)) {
        if (drv.ro.sched == "sbase")
          drv.SBase(1);
        else if (drv.ro.sched == "sbatch")
          drv.SBatch();
        else if (drv.ro.sched == "sb4")
          drv.SB4();
        else if (drv.ro.sched == "level0") {
          CollectivePhase("level 0 status",
                          [&] { drv.status = drv.CoverageSet().empty() ? "LEVEL0_ONLY_COMPLETE" : "LEVEL0_ONLY"; });
        }
      }
      CollectivePhase("run output", [&] {
        const auto finalCheck = AcceptanceChecks(drv.mesh, prm.surface ? nullptr : &drv.input, {}, drv.input);
        j.num("finalValid", finalCheck.ok);
        if (!finalCheck.ok) {
          drv.status = "INVALID_FINAL_STATE";
          ret = 2;
        }
        if (drv.planMismatches) drv.status = "PLAN_MISMATCH";
        if (eo.injectStage != Stage::NONE && !drv.injectionsObserved) drv.status = "INJECTION_MISSED";
        if (drv.status == "PLAN_MISMATCH" || drv.status == "INJECTION_MISSED") ret = 2;
        if (!drv.ro.artifacts.empty()) drv.WriteArtifacts(drv.ro.artifacts);
        const double tRun = Now() - t0;
        /*--- final statistics against the serial references, matched by the former interfaces of this run ---*/
        j.str("status", drv.status);
        j.str("stopReason", drv.stopReason);
        j.num("tRun", tRun);
        j.num("finalC", drv.CoverageSet().size());
        j.num("finalD", drv.DefectSet().size());
        j.str("hash", std::to_string(drv.StateHash()));
        j.num("planMismatches", drv.planMismatches);
        j.num("injectionsObserved", drv.injectionsObserved);
        j.num("formerFaces", drv.ff.nFaces());
        double tMMG = 0, crit = 0, tCommit = 0;
        j.openArr("steps");
        for (auto& s : drv.steps) {
          tMMG += s.tMMGsum;
          crit += s.critPath;
          tCommit += s.tCommit;
          j.openAnon();
          j.str("kind", s.kind);
          j.num("step", s.step);
          j.num("L", s.L);
          j.num("nPieces", s.nPieces);
          j.num("nAccepted", s.nAccepted);
          j.num("nRolled", s.nRolled);
          j.num("nUnadmitted", s.nUnadmitted);
          j.num("nExcluded", s.nExcluded);
          j.num("C", s.C);
          j.num("D", s.D);
          j.num("workElems", s.workElems);
          j.num("largestPiece", s.largestPiece);
          j.num("largestWork", s.largestWork);
          j.num("nvAfter", s.nvAfter);
          j.num("neAfter", s.neAfter);
          j.num("totalWork", s.totalWork);
          j.num("capLoad", s.capLoad);
          j.num("capMem", s.capMem);
          j.num("tMMGsum", s.tMMGsum);
          j.num("tMMGmax", s.tMMGmax);
          j.num("critPath", s.critPath);
          j.num("tCommit", s.tCommit);
          j.num("committed", s.committed);
          j.num("lowFailures", s.lowFailures);
          j.str("note", s.note);
          j.str("postHash", s.postHash);
          j.arr("freeInteriorRatio", s.freeRatio);
          j.openArr("pieceIds");
          for (const auto& id : s.pieceIds) {
            j.sep();
            j.s << "\"" << id << "\"";
          }
          j.closeArr();
          j.arr("vrankPredMB", s.vrankPred);
          j.arr("vrankMeasMB", s.vrankMeas);
          {
            vector<double> flat;
            for (auto& p : s.pieceRec) flat.insert(flat.end(), p.begin(), p.end());
            j.arr("pieces", flat);
          }
          j.openArr("pieceResults");
          for (size_t i = 0; i < s.pieceRec.size(); ++i) {
            j.openAnon();
            j.str("id", s.pieceIds.at(i));
            const char* names[] = {"nIn", "predOut", "nOut", "tMMG", "vrank", "nArt", "status", "capMB", "peakMB"};
            for (int k = 0; k < 9; ++k) j.num(names[k], s.pieceRec[i][k]);
            j.num("freeInteriorRatio", s.freeRatio.at(i));
            j.num("freeVertices", s.pieceVerts.at(i)[0]);
            j.num("frozenVertices", s.pieceVerts.at(i)[1]);
            j.num("outputOverPrediction", s.pieceRec[i][1] > 0 ? s.pieceRec[i][2] / s.pieceRec[i][1] : -1);
            j.num("artificialFacesPerElement", s.pieceRec[i][0] > 0 ? s.pieceRec[i][5] / s.pieceRec[i][0] : -1);
            j.close();
          }
          j.closeArr();
          j.openArr("failures");
          for (auto& f : s.failures) {
            j.sep();
            j.s << "\"";
            for (char c : f) j.s << (c == '"' ? '\'' : c);
            j.s << "\"";
          }
          j.closeArr();
          j.close();
        }
        j.closeArr();
        j.num("tMMGtotal", tMMG);
        j.num("critPathTotal", crit);
        j.num("tCommitTotal", tCommit);
        drv.ff.Index();
        StatsOpts so;
        so.ff = &drv.ff;
        so.surfRef = prm.surface ? &drv.input : nullptr;
        const bool skipFinal =
            drv.ro.sched == "level0" && drv.ro.level0Stats && drv.ro.artifacts.empty();  // N2: level-0 statistics only
        const MeshStats fin = skipFinal ? MeshStats() : ComputeStats(drv.mesh, drv.bg, so);
        WriteStats(j, "final", fin);
        j.openArr("serial");
        if (!skipFinal)
          for (auto& f : drv.ro.serialRefs) {
            const Mesh sm = ReadB0In(f, pp);
            const MeshStats s = ComputeStats(sm, drv.bg, so);
            J jj;
            WriteStats(jj, "x", s);
            string str = jj.s.str();
            j.sep();
            j.s << str.substr(str.find(':') + 1);
          }
        j.closeArr();
        j.s << "}";
        if (rank == 0) {
          WriteText(drv.ro.out, j.s.str() + "\n");
          std::cout << "status " << drv.status << " (" << drv.stopReason << "), steps " << drv.steps.size() << ", nv "
                    << drv.mesh.nv() << ", tRun " << tRun << " s, MMG " << tMMG << " s, crit " << crit << " s\n";
          for (auto& s : drv.steps) {
            std::cout << "  step " << s.step << " " << s.kind << " L" << s.L << " pieces " << s.nPieces << " acc "
                      << s.nAccepted << " rolled " << s.nRolled << " excl " << s.nExcluded << " C " << s.C << " D "
                      << s.D << " work " << s.workElems << " largest " << s.largestPiece << " MMG " << s.tMMGsum
                      << " crit " << s.critPath << " nv " << s.nvAfter << (s.committed ? "" : " ROLLED BACK") << "\n";
            for (auto& f : s.failures) std::cout << "    " << f << "\n";
          }
          if (!drv.ro.dumpVtu.empty()) WriteVTU(drv.ro.dumpVtu, drv.mesh);
        }
      });
    } else {
      throw std::runtime_error("unknown mode " + mode);
    }
  } catch (const std::exception& e) {
    std::cerr << "rank " << rank << ": error: " << e.what() << std::endl;
    ret = 1;
  }
  int globalRet = 0;
  Reduce(&ret, &globalRet, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  ret = globalRet;
  if (!SerialExecution()) MPI_Finalize();
  return ret;
}
