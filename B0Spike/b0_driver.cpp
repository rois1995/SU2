/*!
 * \file b0_driver.cpp
 * \brief B0 spike driver: placements, transactions, schedulers.
 */
#include "b0_driver.hpp"

#include <cstring>
#include <iostream>
#include <fstream>
#include <queue>

#include "metis.h"

namespace b0 {

namespace {

/*--- packing of piece outputs for the exchange between real MPI ranks ---*/
struct Packer {
  vector<char> buf;
  template <class T>
  void put(const T& v) {
    const char* p = reinterpret_cast<const char*>(&v);
    buf.insert(buf.end(), p, p + sizeof(T));
  }
  template <class T>
  void putv(const vector<T>& v) {
    put<uint64_t>(v.size());
    const char* p = reinterpret_cast<const char*>(v.data());
    buf.insert(buf.end(), p, p + v.size() * sizeof(T));
  }
  void puts(const string& s) {
    put<uint64_t>(s.size());
    buf.insert(buf.end(), s.begin(), s.end());
  }
};
struct Unpacker {
  const char* p;
  const char* end;
  void need(uint64_t bytes) const {
    if (bytes > (uint64_t)(end - p)) throw std::runtime_error("truncated piece output");
  }
  template <class T>
  T get() {
    T v;
    need(sizeof(T));
    memcpy(&v, p, sizeof(T));
    p += sizeof(T);
    return v;
  }
  template <class T>
  void getv(vector<T>& v) {
    const auto n = get<uint64_t>();
    if (n > (uint64_t)(end - p) / sizeof(T)) throw std::runtime_error("invalid piece array size");
    v.resize(n);
    if (n) memcpy(v.data(), p, n * sizeof(T));
    p += n * sizeof(T);
  }
  string gets() {
    const auto n = get<uint64_t>();
    need(n);
    string s(p, n);
    p += n;
    return s;
  }
};
void PackOut(Packer& k, const PieceOut& o) {
  k.put(o.st.code);
  k.put((int)o.st.stage);
  k.puts(o.st.msg);
  k.put(o.mmgStatus);
  k.put(o.tMMG);
  k.put(o.tTotal);
  k.put(o.nInElem);
  k.put(o.nOutElem);
  k.put(o.nOutVert);
  k.put(o.nArtFaces);
  k.put(o.nFreeVerts);
  k.put(o.nFrozenVerts);
  k.putv(o.newX);
  k.putv(o.newM);
  k.putv(o.T);
  k.putv(o.Tref);
  k.putv(o.F);
  k.putv(o.Fref);
  k.putv(o.Fstate);
  k.putv(o.removedFaces);
  k.putv(o.artFaceKeysGid);
  k.putv(o.freedVerts);
  k.putv(o.frozenVerts);
  k.put(o.bytesIn);
  k.put(o.bytesOut);
  k.put(o.peakDeltaMB);
  k.put(o.capMB);
}
void UnpackOut(Unpacker& u, PieceOut& o) {
  o.st.code = u.get<int>();
  o.st.stage = (Stage)u.get<int>();
  o.st.msg = u.gets();
  o.mmgStatus = u.get<int>();
  o.tMMG = u.get<double>();
  o.tTotal = u.get<double>();
  o.nInElem = u.get<i64>();
  o.nOutElem = u.get<i64>();
  o.nOutVert = u.get<i64>();
  o.nArtFaces = u.get<i64>();
  o.nFreeVerts = u.get<i64>();
  o.nFrozenVerts = u.get<i64>();
  u.getv(o.newX);
  u.getv(o.newM);
  u.getv(o.T);
  u.getv(o.Tref);
  u.getv(o.F);
  u.getv(o.Fref);
  u.getv(o.Fstate);
  u.getv(o.removedFaces);
  u.getv(o.artFaceKeysGid);
  u.getv(o.freedVerts);
  u.getv(o.frozenVerts);
  o.bytesIn = u.get<size_t>();
  o.bytesOut = u.get<size_t>();
  o.peakDeltaMB = u.get<double>();
  o.capMB = u.get<double>();
}

struct ElemKey {
  i64 g[4];
  bool operator<(const ElemKey& o) const {
    for (int i = 0; i < 4; ++i)
      if (g[i] != o.g[i]) return g[i] < o.g[i];
    return false;
  }
};
ElemKey KeyOf(const Mesh& m, i64 k) {
  ElemKey key{{0, 0, 0, 0}};
  for (int a = 0; a < m.nn(); ++a) key.g[a] = m.gid[m.T[m.nn() * k + a]];
  std::sort(key.g, key.g + m.nn());
  if (m.nn() == 3) key.g[3] = -1;
  return key;
}

vector<idx_t> MetisPart(idx_t nv, idx_t ncon, vector<idx_t>& xadj, vector<idx_t>& adjncy, vector<idx_t>& vwgt,
                        vector<idx_t>& adjwgt, idx_t nparts) {
  vector<idx_t> part(nv, 0);
  if (nparts <= 1 || nv == 0) return part;
  if (nv <= nparts) {
    for (idx_t i = 0; i < nv; ++i) part[i] = i;
    return part;
  }
  idx_t options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);
  options[METIS_OPTION_SEED] = 1;
  options[METIS_OPTION_NUMBERING] = 0;
  vector<real_t> ub(ncon, 1.05);
  idx_t obj = 0;
  const int ret = METIS_PartGraphKway(&nv, &ncon, xadj.data(), adjncy.data(), vwgt.empty() ? nullptr : vwgt.data(),
                                      nullptr, adjwgt.empty() ? nullptr : adjwgt.data(), &nparts, nullptr, ub.data(),
                                      options, &obj, part.data());
  if (ret != METIS_OK) throw std::runtime_error("METIS failed");
  return part;
}

}  // namespace

/*----------------------------------------------------------------------------------------------------------------*/

void Driver::Setup(const Mesh& in, const Params& p) {
  CommRank(MPI_COMM_WORLD, &rank);
  CommSize(MPI_COMM_WORLD, &size);
  tStart = Now();
  prm = p;
  input = in;
  const int dim = input.dim, nm = input.nm();
  /*--- marker corners: vertices on >= dim markers ---*/
  {
    vector<vector<int>> refs(input.nv());
    for (i64 f = 0; f < input.nf(); ++f)
      for (int a = 0; a < dim; ++a) {
        auto& r = refs[input.F[dim * f + a]];
        if (std::find(r.begin(), r.end(), input.Fref[f]) == r.end()) r.push_back(input.Fref[f]);
      }
    for (i64 v = 0; v < input.nv(); ++v)
      if ((int)refs[v].size() >= dim) input.vflag[v] |= V_CORNER;
  }
  double lo[3] = {1e300, 1e300, 1e300}, hi[3] = {-1e300, -1e300, -1e300};
  for (i64 v = 0; v < input.nv(); ++v)
    for (int d = 0; d < dim; ++d) {
      lo[d] = std::min(lo[d], input.X[dim * v + d]);
      hi[d] = std::max(hi[d], input.X[dim * v + d]);
    }
  double diag = 0;
  for (int d = 0; d < dim; ++d) diag += std::pow(hi[d] - lo[d], 2);
  eng.prm = prm;
  eng.domainSize = std::sqrt(diag);
  eng.refArt = 1 + *std::max_element(input.markerRef.begin(), input.markerRef.end());
  /*--- physical floor (ADAP_SURFACE= NO), from complete boundary stars, edges in the serial order ---*/
  if (!prm.surface) {
    vector<vector<std::array<double, 3>>> edges(input.nv());
    for (size_t mk = 0; mk < input.markerRef.size(); ++mk)
      for (i64 f = 0; f < input.nf(); ++f) {
        if (input.Fref[f] != input.markerRef[mk]) continue;
        for (int a = 0; a < dim; ++a)
          for (int b = 0; b < dim; ++b) {
            if (a == b) continue;
            std::array<double, 3> e = {0, 0, 0};
            for (int d = 0; d < dim; ++d) e[d] = input.X[dim * input.F[dim * f + b] + d] - input.X[dim * input.F[dim * f + a] + d];
            edges[input.F[dim * f + a]].push_back(e);
          }
      }
    for (i64 v = 0; v < input.nv(); ++v) {
      if (edges[v].empty()) continue;
      std::array<double, 6> mt{};
      std::copy(&input.M[nm * v], &input.M[nm * v] + nm, mt.begin());
      FloorWithEdges(dim, mt.data(), edges[v]);
      eng.physFloor[input.gid[v]] = mt;
    }
  }
  mesh = input;
  adj.Build(mesh);
  bg.mesh = &input;
  bg.loc.Build(dim, input.X.data(), input.T.data(), input.ne());
  eng.bg = &bg;
  ff.dim = dim;
  ff.Index();
}

/*----------------------------------------------------------------------------------------------------------------*/

vector<double> Driver::PredOut() const {
  const double cE = ro.cE > 0 ? ro.cE : (mesh.dim == 2 ? 4.0 / std::sqrt(3.0) : 12.0 / std::sqrt(2.0));
  vector<double> p(mesh.ne());
  for (i64 k = 0; k < mesh.ne(); ++k) p[k] = cE * ElemComplexity(mesh, k);
  return p;
}

double Driver::Work(const vector<i64>& elems, const vector<double>& pred) const {
  double w = 0;
  for (i64 e : elems) {
    if (e < 0 || e >= (i64)pred.size()) throw std::runtime_error("Work: stale prediction or invalid element");
    w += 1 + pred[e];
  }
  return w;
}

vector<int> Driver::Placement() {
  const int dim = mesh.dim, nn = mesh.nn(), P = ro.P;
  const i64 ne = mesh.ne(), nv = mesh.nv();
  vector<int> part(ne, 0);
  const string& pl = ro.placement;
  auto centroid = [&](i64 k) { return ElemCentroid(mesh, k); };
  if (pl == "P0") {
    /*--- SU2: ParMETIS on the nodal graph (vertex weights only, here unit), elements by majority ---*/
    vector<vector<i64>> nb(nv);
    for (i64 k = 0; k < ne; ++k)
      for (int a = 0; a < nn; ++a)
        for (int b = 0; b < nn; ++b)
          if (a != b) nb[mesh.T[nn * k + a]].push_back(mesh.T[nn * k + b]);
    vector<idx_t> xadj(nv + 1, 0), adjncy, vw, aw;
    for (i64 v = 0; v < nv; ++v) {
      std::sort(nb[v].begin(), nb[v].end());
      nb[v].erase(std::unique(nb[v].begin(), nb[v].end()), nb[v].end());
      xadj[v + 1] = xadj[v] + nb[v].size();
      adjncy.insert(adjncy.end(), nb[v].begin(), nb[v].end());
    }
    const auto vp = MetisPart(nv, 1, xadj, adjncy, vw, aw, P);
    for (i64 k = 0; k < ne; ++k) {
      int cnt[64] = {0};
      std::map<int, int> c;
      for (int a = 0; a < nn; ++a) ++c[vp[mesh.T[nn * k + a]]];
      int best = -1, bc = 0;
      for (auto& kv : c) bc = std::max(bc, kv.second);
      i64 bestG = -1;
      for (int a = 0; a < nn; ++a) {
        const i64 v = mesh.T[nn * k + a];
        if (c[vp[v]] == bc && (bestG < 0 || mesh.gid[v] < bestG)) {
          bestG = mesh.gid[v];
          best = vp[v];
        }
      }
      (void)cnt;
      part[k] = best;
    }
  } else if (pl == "Pb" || pl == "P3" || pl == "emptymid") {
    const auto pred = PredOut();
    vector<idx_t> xadj(ne + 1, 0), adjncy, vw(2 * ne), aw;
    double maxW = 0, maxM = 0;
    for (i64 k = 0; k < ne; ++k) {
      maxW = std::max(maxW, 1 + pred[k]);
      maxM = std::max(maxM, 1 + 3 * pred[k]);
    }
    i64 f[3];
    for (i64 k = 0; k < ne; ++k) {
      vw[2 * k] = std::max<idx_t>(1, (idx_t)std::ceil(1000 * (1 + pred[k]) / maxW));
      vw[2 * k + 1] = std::max<idx_t>(1, (idx_t)std::ceil(1000 * (1 + 3 * pred[k]) / maxM));
      for (int j = 0; j < nn; ++j) {
        const i64 o = adj.nb[nn * k + j];
        if (o < 0) continue;
        adjncy.push_back(o);
        if (pl == "P3") {
          ElemFace(dim, &mesh.T[nn * k], j, f);
          const double w = FaceWeightB3(mesh, f, ro.a, ro.b);
          aw.push_back((idx_t)std::min(1e6, std::round(w)));
        } else {
          aw.push_back(1);
        }
      }
      xadj[k + 1] = adjncy.size();
    }
    const int np = pl == "emptymid" ? P - 1 : P;
    const auto ep = MetisPart(ne, 2, xadj, adjncy, vw, aw, np);
    for (i64 k = 0; k < ne; ++k) part[k] = ep[k];
    if (pl == "emptymid")
      for (auto& p : part)
        if (p >= P / 2) ++p;
  } else if (pl.rfind("cutx:", 0) == 0) {
    const double x0 = std::stod(pl.substr(5));
    for (i64 k = 0; k < ne; ++k) part[k] = centroid(k)[0] < x0 ? 0 : 1;
  } else if (pl.rfind("line:", 0) == 0 || pl.rfind("plane:", 0) == 0) {
    std::stringstream ss(pl.substr(pl.find(':') + 1));
    vector<double> v;
    string t;
    while (std::getline(ss, t, ',')) v.push_back(std::stod(t));
    for (i64 k = 0; k < ne; ++k) {
      const auto c = centroid(k);
      double s = 0;
      for (int d = 0; d < dim; ++d) s += (c[d] - v[d]) * v[dim + d];
      part[k] = s < 0 ? 0 : 1;
    }
  } else if (pl.rfind("random:", 0) == 0) {
    uint64_t seed = std::stoull(pl.substr(7));
    auto rnd = [&seed]() {
      seed = seed * 6364136223846793005ull + 1442695040888963407ull;
      return (seed >> 11) * (1.0 / 9007199254740992.0);
    };
    vector<vector<double>> sites;
    for (int p = 0; p < P; ++p) sites.push_back(centroid((i64)(rnd() * ne) % ne));
    for (i64 k = 0; k < ne; ++k) {
      const auto c = centroid(k);
      double best = 1e300;
      for (int p = 0; p < P; ++p) {
        double d2 = 0;
        for (int d = 0; d < dim; ++d) d2 += std::pow(c[d] - sites[p][d], 2);
        if (d2 < best) {
          best = d2;
          part[k] = p;
        }
      }
    }
  } else {
    throw std::runtime_error("unknown placement " + pl);
  }
  return part;
}

vector<vector<i64>> Driver::Components(const vector<i64>& elems) const {
  const int nn = mesh.nn();
  std::unordered_map<i64, i64> idx;
  idx.reserve(elems.size() * 2);
  for (size_t i = 0; i < elems.size(); ++i) idx[elems[i]] = i;
  vector<i64> par(elems.size());
  std::iota(par.begin(), par.end(), 0);
  std::function<i64(i64)> find = [&](i64 x) {
    while (par[x] != x) {
      par[x] = par[par[x]];
      x = par[x];
    }
    return x;
  };
  for (size_t i = 0; i < elems.size(); ++i)
    for (int j = 0; j < nn; ++j) {
      const i64 o = adj.nb[nn * elems[i] + j];
      if (o < 0) continue;
      const auto it = idx.find(o);
      if (it == idx.end()) continue;
      const i64 a = find(i), b = find(it->second);
      if (a != b) par[std::max(a, b)] = std::min(a, b);
    }
  std::map<i64, vector<i64>> groups;
  for (size_t i = 0; i < elems.size(); ++i) groups[find(i)].push_back(elems[i]);
  vector<std::pair<ElemKey, vector<i64>>> out;
  for (auto& g : groups) {
    std::sort(g.second.begin(), g.second.end());
    ElemKey mn = KeyOf(mesh, g.second[0]);
    for (i64 e : g.second) mn = std::min(mn, KeyOf(mesh, e));
    out.push_back({mn, std::move(g.second)});
  }
  std::sort(out.begin(), out.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
  vector<vector<i64>> r;
  for (auto& o : out) r.push_back(std::move(o.second));
  return r;
}

void Driver::DetectPinches(vector<i64>& elems, std::set<i64>& pinchV, std::set<std::pair<i64, i64>>& pinchE) const {
  const int dim = mesh.dim, nn = mesh.nn();

    std::sort(elems.begin(), elems.end());
    auto in = [&](i64 e) { return std::binary_search(elems.begin(), elems.end(), e); };
    /*--- boundary faces ---*/
    vector<std::array<i64, 3>> bf;
    i64 f[3];
    for (i64 e : elems)
      for (int j = 0; j < nn; ++j) {
        const i64 o = adj.nb[nn * e + j];
        if (o >= 0 && in(o)) continue;
        ElemFace(dim, &mesh.T[nn * e], j, f);
        bf.push_back({f[0], f[1], dim == 3 ? f[2] : -1});
      }
    if (dim == 2) {
      std::map<i64, int> cnt;
      for (auto& b : bf) {
        ++cnt[b[0]];
        ++cnt[b[1]];
      }
      for (auto& kv : cnt)
        if (kv.second > 2) pinchV.insert(kv.first);
    } else {
      std::map<std::pair<i64, i64>, int> ecnt;
      std::map<i64, vector<int>> vf;
      for (size_t q = 0; q < bf.size(); ++q)
        for (int a = 0; a < 3; ++a) {
          const i64 u = std::min(bf[q][a], bf[q][(a + 1) % 3]), w = std::max(bf[q][a], bf[q][(a + 1) % 3]);
          ++ecnt[{u, w}];
          vf[bf[q][a]].push_back(q);
        }
      for (auto& kv : ecnt)
        if (kv.second > 2) pinchE.insert(kv.first);
      for (auto& kv : vf) {
        const i64 v = kv.first;
        auto& fs = kv.second;
        /*--- link connectivity: faces around v connected through edges containing v ---*/
        vector<int> par(fs.size());
        std::iota(par.begin(), par.end(), 0);
        std::function<int(int)> fd = [&](int x) { return par[x] == x ? x : par[x] = fd(par[x]); };
        std::map<i64, int> other;
        for (size_t i = 0; i < fs.size(); ++i)
          for (int a = 0; a < 3; ++a) {
            const i64 w = bf[fs[i]][a];
            if (w == v) continue;
            auto it = other.find(w);
            if (it == other.end())
              other[w] = i;
            else
              par[fd(i)] = fd(it->second);
          }
        int nc = 0;
        for (size_t i = 0; i < fs.size(); ++i) nc += fd(i) == (int)i;
        if (nc > 1) pinchV.insert(v);
      }
    }
}

void Driver::RepairStep(vector<PieceIn>& pieces0, vector<PieceIn>& pieces, vector<i64>& excluded) const {
  vector<int> owner(mesh.ne(), -1);
  for (size_t i = 0; i < pieces0.size(); ++i)
    for (i64 e : pieces0[i].elems) owner[e] = i;
  for (size_t i = 0; i < pieces0.size(); ++i) FillPinches(pieces0[i].elems, owner, i);
  for (auto& pi : pieces0) {
    RepairPinches(pi.elems, excluded);
    for (auto& c : Components(pi.elems)) {
      PieceIn q;
      q.elems = std::move(c);
      q.vrank = pi.vrank;
      pieces.push_back(std::move(q));
    }
  }
}

void Driver::FillPinches(vector<i64>& elems, vector<int>& owner, int me) const {
  /*--- pinch repair by growth (closure, design 2.2): the missing elements of the star of a pinch vertex or edge are
   *    added if no other piece of the step holds them; what is left is excluded by RepairPinches ---*/
  const int nn = mesh.nn();
  for (int round = 0; round < 6; ++round) {
    std::set<i64> pinchV;
    std::set<std::pair<i64, i64>> pinchE;
    DetectPinches(elems, pinchV, pinchE);
    if (pinchV.empty() && pinchE.empty()) return;
    vector<i64> add;
    auto star = [&](i64 v) {
      for (i64 s = adj.v2eStart[v]; s < adj.v2eStart[v + 1]; ++s) {
        const i64 k = adj.v2e[s];
        if (owner[k] < 0) add.push_back(k);
      }
    };
    for (i64 v : pinchV) star(v);
    for (auto& e : pinchE) {
      star(e.first);
      star(e.second);
    }
    std::sort(add.begin(), add.end());
    add.erase(std::unique(add.begin(), add.end()), add.end());
    if (add.empty()) return;
    for (i64 k : add) owner[k] = me;
    elems.insert(elems.end(), add.begin(), add.end());
    std::sort(elems.begin(), elems.end());
    (void)nn;
  }
}

void Driver::RepairPinches(vector<i64>& elems, vector<i64>& excluded) const {
  const int dim = mesh.dim, nn = mesh.nn();
  for (int round = 0; round < 8; ++round) {
    std::sort(elems.begin(), elems.end());
    auto in = [&](i64 e) { return std::binary_search(elems.begin(), elems.end(), e); };
    /*--- boundary faces ---*/
    vector<std::array<i64, 3>> bf;
    i64 f[3];
    for (i64 e : elems)
      for (int j = 0; j < nn; ++j) {
        const i64 o = adj.nb[nn * e + j];
        if (o >= 0 && in(o)) continue;
        ElemFace(dim, &mesh.T[nn * e], j, f);
        bf.push_back({f[0], f[1], dim == 3 ? f[2] : -1});
      }
    std::set<i64> pinchV;
    std::set<std::pair<i64, i64>> pinchE;
    if (dim == 2) {
      std::map<i64, int> cnt;
      for (auto& b : bf) {
        ++cnt[b[0]];
        ++cnt[b[1]];
      }
      for (auto& kv : cnt)
        if (kv.second > 2) pinchV.insert(kv.first);
    } else {
      std::map<std::pair<i64, i64>, int> ecnt;
      std::map<i64, vector<int>> vf;
      for (size_t q = 0; q < bf.size(); ++q)
        for (int a = 0; a < 3; ++a) {
          const i64 u = std::min(bf[q][a], bf[q][(a + 1) % 3]), w = std::max(bf[q][a], bf[q][(a + 1) % 3]);
          ++ecnt[{u, w}];
          vf[bf[q][a]].push_back(q);
        }
      for (auto& kv : ecnt)
        if (kv.second > 2) pinchE.insert(kv.first);
      for (auto& kv : vf) {
        const i64 v = kv.first;
        auto& fs = kv.second;
        /*--- link connectivity: faces around v connected through edges containing v ---*/
        vector<int> par(fs.size());
        std::iota(par.begin(), par.end(), 0);
        std::function<int(int)> fd = [&](int x) { return par[x] == x ? x : par[x] = fd(par[x]); };
        std::map<i64, int> other;
        for (size_t i = 0; i < fs.size(); ++i)
          for (int a = 0; a < 3; ++a) {
            const i64 w = bf[fs[i]][a];
            if (w == v) continue;
            auto it = other.find(w);
            if (it == other.end())
              other[w] = i;
            else
              par[fd(i)] = fd(it->second);
          }
        int nc = 0;
        for (size_t i = 0; i < fs.size(); ++i) nc += fd(i) == (int)i;
        if (nc > 1) pinchV.insert(v);
      }
    }
    if (pinchV.empty() && pinchE.empty()) return;
    /*--- exclude all but the largest face-connected group of the star ---*/
    std::set<i64> drop;
    auto handle = [&](const vector<i64>& star, const vector<i64>& pivot) {
      const size_t ns = star.size();
      vector<int> par(ns);
      std::iota(par.begin(), par.end(), 0);
      std::function<int(int)> fd = [&](int x) { return par[x] == x ? x : par[x] = fd(par[x]); };
      for (size_t i = 0; i < ns; ++i)
        for (size_t j = i + 1; j < ns; ++j) {
          int shared = 0;
          bool hasPivot = true;
          for (int a = 0; a < nn; ++a)
            for (int b = 0; b < nn; ++b) shared += mesh.T[nn * star[i] + a] == mesh.T[nn * star[j] + b];
          for (i64 p : pivot) {
            bool x = false;
            for (int a = 0; a < nn; ++a) x |= mesh.T[nn * star[j] + a] == p;
            hasPivot &= x;
          }
          if (shared == dim && hasPivot) par[fd(i)] = fd(j);
        }
      std::map<int, vector<i64>> g;
      for (size_t i = 0; i < ns; ++i) g[fd(i)].push_back(star[i]);
      if (g.size() <= 1) return;
      const vector<i64>* keep = nullptr;
      for (auto& kv : g)
        if (!keep || kv.second.size() > keep->size()) keep = &kv.second;
      for (auto& kv : g)
        if (&kv.second != keep) drop.insert(kv.second.begin(), kv.second.end());
    };
    for (i64 v : pinchV) {
      vector<i64> star;
      for (i64 s = adj.v2eStart[v]; s < adj.v2eStart[v + 1]; ++s)
        if (in(adj.v2e[s])) star.push_back(adj.v2e[s]);
      handle(star, {v});
    }
    for (auto& e : pinchE) {
      vector<i64> star;
      for (i64 s = adj.v2eStart[e.first]; s < adj.v2eStart[e.first + 1]; ++s) {
        const i64 k = adj.v2e[s];
        if (!in(k)) continue;
        bool hasW = false;
        for (int a = 0; a < nn; ++a) hasW |= mesh.T[nn * k + a] == e.second;
        if (hasW) star.push_back(k);
      }
      handle(star, {e.first, e.second});
    }
    if (drop.empty()) return;
    vector<i64> keep;
    for (i64 e : elems)
      if (!drop.count(e)) keep.push_back(e);
      else excluded.push_back(e);
    elems.swap(keep);
  }
}

vector<i64> Driver::WorkSet(const vector<i64>& seeds, int L) const {
  const int nn = mesh.nn();
  vector<int> dist(mesh.nv(), -1);
  std::queue<i64> q;
  for (i64 v : seeds)
    if (dist[v] < 0) {
      dist[v] = 0;
      q.push(v);
    }
  while (!q.empty()) {
    const i64 v = q.front();
    q.pop();
    if (dist[v] >= L - 1) continue;
    for (i64 s = adj.v2eStart[v]; s < adj.v2eStart[v + 1]; ++s)
      for (int a = 0; a < nn; ++a) {
        const i64 w = mesh.T[nn * adj.v2e[s] + a];
        if (dist[w] < 0) {
          dist[w] = dist[v] + 1;
          q.push(w);
        }
      }
  }
  vector<i64> work;
  for (i64 k = 0; k < mesh.ne(); ++k) {
    bool w = false;
    for (int a = 0; a < nn && !w; ++a) w = dist[mesh.T[nn * k + a]] >= 0;
    if (w) work.push_back(k);
  }
  return work;
}

vector<i64> Driver::SurfaceExpand(const vector<i64>& seeds) const {
  if (!prm.surface) return seeds;
  const int dim = mesh.dim;
  vector<char> onPending(mesh.nv(), 0);
  for (i64 f = 0; f < mesh.nf(); ++f)
    if (mesh.Fstate[f] == F_PENDING)
      for (int a = 0; a < dim; ++a) onPending[mesh.F[dim * f + a]] = 1;
  std::set<i64> out(seeds.begin(), seeds.end());
  for (i64 v : seeds) {
    if (!onPending[v]) continue;
    std::set<i64> ring = {v};
    for (int r = 0; r < 3; ++r) {
      std::set<i64> nx = ring;
      for (i64 u : ring)
        for (i64 s = adj.v2fStart[u]; s < adj.v2fStart[u + 1]; ++s)
          for (int a = 0; a < dim; ++a) nx.insert(mesh.F[dim * adj.v2f[s] + a]);
      ring.swap(nx);
    }
    out.insert(ring.begin(), ring.end());
  }
  return vector<i64>(out.begin(), out.end());
}

vector<i64> Driver::CoverageSet() const {
  vector<char> c(mesh.nv(), 0);
  for (i64 v = 0; v < mesh.nv(); ++v) c[v] = (mesh.vflag[v] & V_CONSTRAINED) != 0;
  for (i64 f = 0; f < mesh.nf(); ++f)
    if (mesh.Fstate[f] == F_PENDING)
      for (int a = 0; a < mesh.dim; ++a) c[mesh.F[mesh.dim * f + a]] = 1;
  vector<i64> r;
  for (i64 v = 0; v < mesh.nv(); ++v)
    if (c[v]) r.push_back(v);
  return r;
}
vector<i64> Driver::DefectSet() const {
  vector<i64> r;
  for (i64 v = 0; v < mesh.nv(); ++v)
    if (mesh.vflag[v] & V_DEFECT) r.push_back(v);
  return r;
}

void Driver::UpdateDefects() {
  const auto out = Classify(mesh, bg, adj, &ff, 2.0 * ro.L, ro.allow.size() == 8 ? ro.allow.data() : nullptr);
  for (i64 v : out.defectVerts) mesh.vflag[v] |= V_DEFECT;
}

/*----------------------------------------------------------------------------------------------------------------*/

bool Driver::Transaction(vector<PieceIn>& pieces, StepRecord& rec, vector<char>& accepted) {
  const int step = stepCounter++;
  rec.step = step;
  rec.nPieces = pieces.size();
  const int P = ro.P, dim = mesh.dim;
  CollectiveDecision(P);  // Measurement reductions below must use the same count.
  vector<double> pred;
  vector<PieceOut> outs;
  int invalid = 0, anyInvalid = 0;
  /*--- Canonical piece identities and plan agreement precede all execution/exchange. ---*/
  CollectivePhase("transaction plan", [&] {
    pred = PredOut();
    outs.resize(pieces.size());
    std::set<i64> labels;
    vector<int> owner(mesh.ne(), -1);
    for (size_t i = 0; i < pieces.size(); ++i) {
      auto& pc = pieces[i];
      bool valid = !pc.elems.empty() && pc.vrank >= 0 && pc.vrank < P;
      for (i64 e : pc.elems) {
        if (e < 0 || e >= mesh.ne()) {
          valid = false;
          continue;
        }
        if (owner[e] >= 0) valid = false;
        owner[e] = i;
      }
      if (!valid) {
        invalid = 1;
        continue;
      }
      const i64 first = *std::min_element(pc.elems.begin(), pc.elems.end(),
                                          [&](i64 a, i64 b) { return KeyOf(mesh, a) < KeyOf(mesh, b); });
      pc.label = CanonicalKeyHash(mesh, first);
      if (!labels.insert(pc.label).second) invalid = 1;
      pc.predOut = 0;
      for (i64 e : pc.elems) pc.predOut += pred.at(e);
    }
  });
  Reduce(&invalid, &anyInvalid, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  if (anyInvalid) {
    CollectivePhase("invalid plan rollback", [&] {
      ++planMismatches;
      status = "PLAN_MISMATCH";
      rec.committed = false;
      rec.failures.push_back("PLAN_MISMATCH: invalid or duplicate piece plan");
      accepted.assign(pieces.size(), 0);
    });
    return false;
  }
  uint64_t ph = 0, lo = 0, hi = 0;
  CollectivePhase("plan hash", [&] {
    std::sort(pieces.begin(), pieces.end(), [](const auto& a, const auto& b) { return a.label < b.label; });
    ph = PlanHash(pieces);
  });
  Reduce(&ph, &lo, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);
  Reduce(&ph, &hi, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
  if (lo != hi) {
    CollectivePhase("plan mismatch rollback", [&] {
      ++planMismatches;
      status = "PLAN_MISMATCH";
      rec.committed = false;
      rec.failures.push_back("PLAN_MISMATCH before execution");
      accepted.assign(pieces.size(), 0);
    });
    return false;
  }
  CollectivePhase("injection selection", [&] {
    eng.opt.injectTarget = false;
    if (eng.opt.injectStage != Stage::NONE && (eng.opt.injectStep < 0 || eng.opt.injectStep == step) &&
        !pieces.empty()) {
      size_t target = 0;
      if (eng.opt.injectPiece == "largest") {
        for (size_t i = 1; i < pieces.size(); ++i)
          if (pieces[i].elems.size() > pieces[target].elems.size()) target = i;
      } else if (eng.opt.injectPiece != "first") {
        const i64 label = std::stoll(eng.opt.injectPiece);
        target = pieces.size();
        for (size_t i = 0; i < pieces.size(); ++i)
          if (pieces[i].label == label) target = i;
      }
      if (target < pieces.size()) {
        eng.opt.injectLabel = pieces[target].label;
        eng.opt.injectTarget = true;
      }
    }
  });
  auto injected = [&](Stage s, size_t i) {
    return eng.opt.injectStage == s && eng.opt.injectTarget && pieces[i].label == eng.opt.injectLabel;
  };
  if (CollectiveDecision(!ro.artifacts.empty() && (ro.snapshotStep < 0 || ro.snapshotStep == step))) {
    CollectivePhase("snapshot save", [&] {
      if (rank == 0) SaveState(ro.artifacts + Fmt("_step%d.b0state", step), pieces, rec);
    });
  }
  vector<char> preFail;
  vector<double> predPeak, meas;
  vector<vector<size_t>> byV;
  CollectivePhase("piece admission and execution", [&] {
    preFail.assign(pieces.size(), 0);
    for (size_t i = 0; i < pieces.size(); ++i)
      if (injected(Stage::PLANNING, i)) {
        preFail[i] = 1;
        outs[i].st = {1, Stage::PLANNING, "injected failure at stage planning"};
      }
    /*--- allocation: admission against the transaction peak of each virtual rank (2.4) ---*/
    const double aIn = ro.alphaIn > 0 ? ro.alphaIn : (dim == 2 ? 400 : 900);
    const double aOut = ro.alphaOut > 0 ? ro.alphaOut : (dim == 2 ? 400 : 900);
    const double bElemIn = dim == 2 ? 3 * 8 + 4 + 0.5 * (2 * 8 + 3 * 16) : 4 * 8 + 4 + 0.18 * (3 * 8 + 6 * 16);
    const double bElemOut = bElemIn;
    predPeak.assign(P, 0);
    byV.resize(P);
    for (size_t i = 0; i < pieces.size(); ++i) byV[pieces[i].vrank].push_back(i);
    for (int v = 0; v < P; ++v) {
      auto calc = [&](const vector<size_t>& list) {
        double retained = 0, mc = 0;
        for (size_t i : list) {
          if (preFail[i]) continue;
          double nOut = 0;
          for (i64 e : pieces[i].elems) nOut += pred[e];
          const double nIn = pieces[i].elems.size();
          retained += bElemIn * nIn + bElemOut * nOut;
          mc = std::max(mc, aIn * nIn + aOut * nOut + bElemIn * nIn + 64 * nIn);
          retained += bElemOut * nOut;  // splice preparation / communication buffers
        }
        return (retained + mc) / 1048576.0;
      };
      predPeak[v] = calc(byV[v]);
      if (ro.memBudgetMB > 0) {
        auto list = byV[v];
        while (!list.empty() && calc(list) > ro.memBudgetMB / ro.margin) {
          /*--- drop the largest piece of this rank ---*/
          size_t worst = 0;
          for (size_t q = 1; q < list.size(); ++q)
            if (pieces[list[q]].elems.size() > pieces[list[worst]].elems.size()) worst = q;
          const size_t i = list[worst];
          preFail[i] = 1;
          outs[i].st = {1, Stage::ALLOCATION, "not admitted: transaction peak above the budget"};
          ++rec.nUnadmitted;
          list.erase(list.begin() + worst);
        }
        predPeak[v] = calc(list);
      }
      for (size_t i : byV[v])
        if (!preFail[i] && injected(Stage::ALLOCATION, i)) {
          preFail[i] = 1;
          outs[i].st = {1, Stage::ALLOCATION, "injected failure at stage allocation"};
        }
    }
    /*--- local work: real rank r runs the virtual ranks v with v % size == r ---*/
    meas.assign(P, 0);
    for (int v = rank; v < P; v += size) {
      TrimHeap();
      const double rss0 = RSSMB();
      double peak = 0;
      for (size_t i : byV[v]) {
        if (preFail[i]) continue;
        TrimHeap();
        const double rssB = RSSMB();
        ResetHWM();
        outs[i] = eng.AdaptPiece(mesh, adj, pieces[i], step);
        outs[i].peakDeltaMB = std::max(0.0, HWMMB() - rssB);
        peak = std::max(peak, rssB - rss0 + outs[i].peakDeltaMB);
      }
      TrimHeap();
      meas[v] = std::max(peak, RSSMB() - rss0);
    }
  });
  /*--- status reductions: one Allreduce(MAX) per stage, entered by every rank (empty ones too) ---*/
  {
    const Stage stages[] = {Stage::PLANNING,      Stage::ALLOCATION, Stage::MIGRATION, Stage::MMG,
                            Stage::INTERPOLATION, Stage::VALIDATION, Stage::SPLICE};
    for (Stage s : stages) {
      int loc = 0, glob = 0;
      for (int v = rank; v < P; v += size)
        for (size_t i : byV[v]) loc = std::max(loc, (outs[i].st.code != 0 && outs[i].st.stage == s) ? 1 : 0);
      Reduce(&loc, &glob, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    }
  }
  /*--- exchange of the outputs (gathered B0: every rank splices identically) ---*/
  if (size > 1) {
    Packer k;
    vector<int> cnt, disp;
    CollectivePhase("output packing", [&] {
      vector<size_t> mine;
      for (int v = rank; v < P; v += size)
        for (size_t i : byV[v])
          if (!preFail[i]) mine.push_back(i);
      k.put<uint64_t>(mine.size());
      for (size_t i : mine) {
        k.put<i64>(pieces[i].label);
        PackOut(k, outs[i]);
      }
      cnt.resize(size);
      disp.assign(size, 0);
    });
    size_t bytes = k.buf.size();
    // Exercise asymmetric count rejection without allocating a multi-GB buffer.
    if (const char* testRank = std::getenv("B0_TEST_EXCHANGE_OVERSIZE_RANK"))
      if (rank == std::atoi(testRank)) bytes = (size_t)INT_MAX + 1;
    CheckExchangeSize(bytes);
    int n = k.buf.size();
    MPI_Allgather(&n, 1, MPI_INT, cnt.data(), 1, MPI_INT, MPI_COMM_WORLD);
    vector<char> all;
    vector<double> measAll;
    CollectivePhase("exchange buffer allocation", [&] {
      int64_t total = 0;
      for (int r = 0; r < size; ++r) {
        if (cnt[r] < 0 || total + cnt[r] > INT_MAX) throw std::runtime_error("MPI output exchange exceeds count limit");
        disp[r] = total;
        total += cnt[r];
      }
      all.resize(total);
      measAll.assign(P, 0);
    });
    MPI_Allgatherv(k.buf.data(), n, MPI_CHAR, all.data(), cnt.data(), disp.data(), MPI_CHAR, MPI_COMM_WORLD);
    Reduce(meas.data(), measAll.data(), P, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    CollectivePhase("output decoding", [&] {
      meas = std::move(measAll);
      std::map<i64, size_t> byLabel;
      for (size_t i = 0; i < pieces.size(); ++i) byLabel[pieces[i].label] = i;
      std::set<i64> received;
      for (int r = 0; r < size; ++r) {
        Unpacker u{all.data() + disp[r], all.data() + disp[r] + cnt[r]};
        const auto m = u.get<uint64_t>();
        if (m > pieces.size()) throw std::runtime_error("invalid output count");
        for (uint64_t q = 0; q < m; ++q) {
          const auto label = u.get<i64>();
          const auto it = byLabel.find(label);
          if (it == byLabel.end() || !received.insert(label).second || pieces[it->second].vrank % size != r ||
              preFail[it->second])
            throw std::runtime_error("invalid, duplicate or misrouted piece output");
          UnpackOut(u, outs[it->second]);
        }
        if (u.p != u.end) throw std::runtime_error("trailing piece output data");
      }
      for (size_t i = 0; i < pieces.size(); ++i)
        if (!preFail[i] && !received.count(pieces[i].label)) throw std::runtime_error("missing piece output");
    });
  }
  /*--- accepted pieces, step record ---*/
  double tc0 = 0;
  Mesh next;
  CheckReport chk;
  vector<i64> frozen;
  CollectivePhase("splice and acceptance", [&] {
    accepted.assign(pieces.size(), 0);
    vector<double> vrankT(P, 0);
    for (size_t i = 0; i < pieces.size(); ++i) {
      accepted[i] = outs[i].st.code == 0;
      if (outs[i].st.msg.find("injected") != string::npos) ++injectionsObserved;
      rec.pieceIds.push_back(std::to_string(pieces[i].label));
      const i64 nFrozen = outs[i].nFrozenVerts, nFree = outs[i].nFreeVerts;
      rec.pieceVerts.push_back({nFree, nFrozen});
      rec.freeRatio.push_back(nFrozen > 0 ? (double)nFree / nFrozen : -1);
      if (accepted[i])
        ++rec.nAccepted;
      else {
        ++rec.nRolled;
        rec.failures.push_back(Fmt("vrank %d piece %zu (%zu elems): stage %s: %s", pieces[i].vrank, i,
                                   pieces[i].elems.size(), StageName(outs[i].st.stage), outs[i].st.msg.c_str()));
      }
      rec.tMMGsum += outs[i].tMMG;
      rec.tMMGmax = std::max(rec.tMMGmax, outs[i].tMMG);
      vrankT[pieces[i].vrank] += outs[i].tTotal;
      rec.largestPiece = std::max<i64>(rec.largestPiece, pieces[i].elems.size());
      {
        double w = 0;
        for (i64 e : pieces[i].elems) w += 1 + pred[e];
        rec.largestWork = std::max(rec.largestWork, w);
        rec.totalWork += w;
        rec.pieceRec.push_back({(double)pieces[i].elems.size(), w - pieces[i].elems.size(), (double)outs[i].nOutElem,
                                outs[i].tMMG, (double)pieces[i].vrank, (double)outs[i].nArtFaces,
                                (double)(outs[i].st.code ? -1 : outs[i].mmgStatus), outs[i].capMB,
                                outs[i].peakDeltaMB});
      }
      rec.workElems += pieces[i].elems.size();
      if (outs[i].mmgStatus == 1) ++rec.lowFailures;
    }
    rec.critPath = *std::max_element(vrankT.begin(), vrankT.end());
    rec.vrankPred = predPeak;
    rec.vrankMeas = meas;
    /*--- commit: splice, gid allocation, acceptance checks, then release of the rollback state ---*/
    tc0 = Now();
    next = Splice(mesh, adj, pieces, outs, accepted);
    for (size_t i = 0; i < pieces.size(); ++i)
      if (accepted[i]) frozen.insert(frozen.end(), outs[i].artFaceKeysGid.begin(), outs[i].artFaceKeysGid.end());
    chk = AcceptanceChecks(next, prm.surface ? nullptr : &input, frozen, mesh);
  });
  {
    int loc = 0, glob = 0;
    for (size_t i = 0; i < pieces.size(); ++i)
      if (pieces[i].vrank % size == rank && injected(Stage::COMMIT, i) && accepted[i]) loc = 1;
    Reduce(&loc, &glob, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    CollectivePhase("commit injection", [&] {
      if (glob) {
        ++injectionsObserved;
        chk.Fail("injected acceptance failure at commit");
      }
    });
    int okLoc = chk.ok ? 0 : 1, okGlob = 0;
    Reduce(&okLoc, &okGlob, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if (okGlob) chk.ok = false;
  }
  if (!chk.ok) {
    CollectivePhase("acceptance rollback", [&] {
      rec.committed = false;
      rec.nRolled += rec.nAccepted;
      rec.nAccepted = 0;
      std::fill(accepted.begin(), accepted.end(), 0);
      for (auto& e : chk.errors) rec.failures.push_back("acceptance: " + e);
      rec.tCommit = Now() - tc0;
      /*--- the retained input (this->mesh) stays the current state ---*/
      rec.postHash = std::to_string(StateHash());
      if (rank == 0 && !ro.artifacts.empty()) {
        WriteText(ro.artifacts + Fmt("_step%d.posthash", step), rec.postHash + "\n");
      }
    });
    return false;
  }
  CollectivePhase("commit bookkeeping", [&] {
    /*--- level-0 coverage bookkeeping, rolled-back pieces keep their vertices as coverage seeds ---*/
    std::unordered_map<i64, i64> g2v;
    g2v.reserve(next.nv() * 2);
    for (i64 v = 0; v < next.nv(); ++v) g2v[next.gid[v]] = v;
    if (rec.kind == "level0") {
      for (size_t i = 0; i < pieces.size(); ++i) {
        if (accepted[i]) {
          for (i64 v : outs[i].frozenVerts) next.vflag[g2v.at(mesh.gid[v])] |= V_CONSTRAINED;
        } else {
          for (i64 e : pieces[i].elems)
            for (int a = 0; a < mesh.nn(); ++a)
              next.vflag[g2v.at(mesh.gid[mesh.T[mesh.nn() * e + a]])] |= V_CONSTRAINED;
        }
      }
    }
    /*--- DEFECT recomputed: old flags cleared, LOWFAILURE seeds of this step kept ---*/
    for (i64 v = 0; v < next.nv(); ++v)
      if (next.gid[v] < mesh.gidEnd) next.vflag[v] &= ~V_DEFECT;
    /*--- Registry records every accepted frozen face and its commit provenance. ---*/
    vector<int> plannedOwner(mesh.ne(), -1);
    for (size_t i = 0; i < pieces.size(); ++i)
      for (i64 e : pieces[i].elems) {
        if (plannedOwner[e] >= 0) throw std::runtime_error("overlapping transaction pieces");
        plannedOwner[e] = i;
      }
    std::set<FKey> cutKeys;
    for (i64 e = 0; e < mesh.ne(); ++e)
      if (plannedOwner[e] >= 0)
        for (int j = 0; j < mesh.nn(); ++j) {
          const i64 nb = adj.nb[mesh.nn() * e + j];
          if (nb < 0 || plannedOwner[nb] < 0 || plannedOwner[nb] == plannedOwner[e]) continue;
          i64 f[3];
          ElemFace(dim, &mesh.T[mesh.nn() * e], j, f);
          for (int a = 0; a < dim; ++a) f[a] = mesh.gid[f[a]];
          cutKeys.insert(MakeKey(dim, f));
        }
    const auto beforeQ = FaceQualityMap(mesh, bg), afterQ = FaceQualityMap(next, bg);
    ff.Add(mesh, frozen);
    for (size_t i = 0; i < pieces.size(); ++i)
      if (accepted[i]) {
        const auto& keys = outs[i].artFaceKeysGid;
        for (size_t f = 0; f < keys.size(); f += dim) {
          vector<i64> gids(keys.begin() + f, keys.begin() + f + dim);
          const int kind = rec.kind == "level0" ? 0 : cutKeys.count(MakeKey(dim, &keys[f])) ? 1 : 2;
          ff.step.push_back(step);
          ff.kind.push_back(kind);
          ff.piece.push_back(pieces[i].label);
          const auto key = MakeKey(dim, gids.data());
          ff.beforeQ.push_back(beforeQ.at(key));
          const auto it = afterQ.find(key);
          ff.afterQ.push_back(it == afterQ.end() ? -1 : it->second);
        }
      }
    mesh = std::move(next);
    adj.Build(mesh);
    ff.Index();
    UpdateDefects();
    rec.nvAfter = mesh.nv();
    rec.neAfter = mesh.ne();
    rec.tCommit = Now() - tc0;
    rec.postHash = std::to_string(StateHash());
    if (rank == 0 && !ro.artifacts.empty())
      WriteText(ro.artifacts + Fmt("_step%d.posthash", step), rec.postHash + "\n");
  });
  return true;
}

/*----------------------------------------------------------------------------------------------------------------*/

vector<PieceIn> Driver::AssignWhole(vector<vector<i64>>& comps, double capMem, StepRecord& rec,
                                    const vector<char>* seedVert, int forceCut) {
  const int P = ro.P, nn = mesh.nn(), dim = mesh.dim;
  const auto pred = PredOut();
  vector<PieceIn> pieces;
  vector<double> load(P, 0);
  vector<std::pair<double, size_t>> fits;
  vector<size_t> big;
  for (size_t c = 0; c < comps.size(); ++c) {
    const double w = Work(comps[c], pred);
    if (w <= capMem && !(forceCut && w > capMem / 4))
      fits.push_back({w, c});
    else
      big.push_back(c);
  }
  /*--- components that fit nowhere: cut (6.6 2a-2c) ---*/
  for (size_t c : big) {
    auto& comp = comps[c];
    std::sort(comp.begin(), comp.end());
    std::unordered_map<i64, idx_t> loc;
    for (size_t i = 0; i < comp.size(); ++i) loc[comp[i]] = i;
    /*--- seed clusters (face components of the union of element stars of coverage seeds) that fit: contracted ---*/
    vector<i64> starElems;
    if (seedVert)
      for (i64 e : comp) {
        bool s = false;
        for (int a = 0; a < nn && !s; ++a) s = (*seedVert)[mesh.T[nn * e + a]];
        if (s) starElems.push_back(e);
      }
    auto clusters = Components(starElems);
    vector<idx_t> node(comp.size(), -1);
    idx_t nNode = 0;
    for (auto& cl : clusters)
      if (Work(cl, pred) <= capMem) {
        for (i64 e : cl) node[loc[e]] = nNode;
        ++nNode;
      }
    for (size_t i = 0; i < comp.size(); ++i)
      if (node[i] < 0) node[i] = nNode++;
    vector<double> nw(nNode, 0);
    vector<std::map<idx_t, double>> ew(nNode);
    i64 f[3];
    for (size_t i = 0; i < comp.size(); ++i) {
      const i64 e = comp[i];
      nw[node[i]] += 1 + pred[e];
      for (int j = 0; j < nn; ++j) {
        const i64 o = adj.nb[nn * e + j];
        if (o < 0) continue;
        const auto it = loc.find(o);
        if (it == loc.end() || node[it->second] == node[i]) continue;
        ElemFace(dim, &mesh.T[nn * e], j, f);
        bool touch = false;
        if (seedVert)
          for (int a = 0; a < dim; ++a) touch |= (*seedVert)[f[a]];
        vector<const double*> mets;
        for (int a = 0; a < dim; ++a) mets.push_back(&mesh.M[mesh.nm() * f[a]]);
        double val[3], vec[3][3];
        Eig(dim, LogEuclidMean(dim, mets), val, vec);
        const double w =
            std::max(1.0, std::min(1e6, 1 + 1000.0 * touch + 100.0 * 0.5 * std::log10(val[dim - 1] / val[0])));
        ew[node[i]][node[it->second]] += w;
      }
    }
    vector<idx_t> xadj(nNode + 1, 0), adjncy, vw(nNode), aw;
    double maxW = *std::max_element(nw.begin(), nw.end());
    for (idx_t n = 0; n < nNode; ++n) {
      vw[n] = std::max<idx_t>(1, (idx_t)std::ceil(1e6 * nw[n] / maxW));
      for (auto& kv : ew[n]) {
        adjncy.push_back(kv.first);
        aw.push_back((idx_t)std::min(1e9, std::round(kv.second)));
      }
      xadj[n + 1] = adjncy.size();
    }
    const auto np = MetisPart(nNode, 1, xadj, adjncy, vw, aw, P);
    vector<vector<i64>> parts(P);
    for (size_t i = 0; i < comp.size(); ++i) parts[np[node[i]]].push_back(comp[i]);
    /*--- fragments below 5% of their part go to the neighbouring part sharing most faces, if it fits ---*/
    vector<double> pw(P, 0);
    for (int p = 0; p < P; ++p) pw[p] = Work(parts[p], pred);
    vector<int> partOf(comp.size());
    for (size_t i = 0; i < comp.size(); ++i) partOf[i] = np[node[i]];
    for (int p = 0; p < P; ++p) {
      auto frags = Components(parts[p]);
      if (frags.size() <= 1) continue;
      for (auto& fr : frags) {
        const double w = Work(fr, pred);
        if (w >= 0.05 * pw[p]) continue;
        std::map<int, int> shared;
        for (i64 e : fr)
          for (int j = 0; j < nn; ++j) {
            const i64 o = adj.nb[nn * e + j];
            if (o < 0) continue;
            const auto it = loc.find(o);
            if (it != loc.end() && partOf[it->second] != p) ++shared[partOf[it->second]];
          }
        int best = -1, bs = 0;
        for (auto& kv : shared)
          if (kv.second > bs && load[kv.first] + pw[kv.first] + w <= capMem) {
            bs = kv.second;
            best = kv.first;
          }
        if (best < 0) continue;
        for (i64 e : fr) partOf[loc[e]] = best;
        pw[best] += w;
        pw[p] -= w;
      }
    }
    for (auto& v : parts) v.clear();
    for (size_t i = 0; i < comp.size(); ++i) parts[partOf[i]].push_back(comp[i]);
    for (int p = 0; p < P; ++p) {
      if (parts[p].empty()) continue;
      load[p] += Work(parts[p], pred);
      for (auto& sub : Components(parts[p])) {
        PieceIn pi;
        pi.elems = std::move(sub);
        pi.vrank = p;
        pieces.push_back(std::move(pi));
      }
    }
    rec.note += Fmt("cut component of %zu elements; ", comp.size());
  }
  /*--- whole components: LPT by work, ties by label (component order) ---*/
  std::stable_sort(fits.begin(), fits.end(), [](const auto& x, const auto& y) { return x.first > y.first; });
  for (auto& fc : fits) {
    int best = -1;
    for (int p = 0; p < P; ++p)
      if (load[p] + fc.first <= capMem && (best < 0 || load[p] < load[best])) best = p;
    if (best < 0) {
      best = std::min_element(load.begin(), load.end()) - load.begin();
      if (load[best] + fc.first > 1.5 * capMem) rec.note += "capHard exceeded; ";
    }
    load[best] += fc.first;
    PieceIn pi;
    pi.elems = comps[fc.second];
    pi.vrank = best;
    pieces.push_back(std::move(pi));
  }
  return pieces;
}

/*----------------------------------------------------------------------------------------------------------------*/

void Driver::Level0() {
  StepRecord rec;
  vector<PieceIn> pieces;
  vector<i64> exclGids;
  CollectivePhase("level 0 planning", [&] {
    rec.kind = "level0";
    rec.L = ro.L;
    const auto part = Placement();
    const int P = ro.P;
    vector<vector<i64>> byPart(P);
    for (i64 k = 0; k < mesh.ne(); ++k) byPart[part[k]].push_back(k);
    /*--- junction vertices of the level-0 partition (for S-B4): >= 3 parts, or a cut vertex on the boundary ---*/
    {
      const int nn = mesh.nn();
      vector<i64> cutV;
      for (i64 v = 0; v < mesh.nv(); ++v) {
        std::set<int> ps;
        for (i64 s = adj.v2eStart[v]; s < adj.v2eStart[v + 1]; ++s) ps.insert(part[adj.v2e[s]]);
        const bool onB = adj.v2fStart[v + 1] > adj.v2fStart[v];
        if (ps.size() >= 3 || (ps.size() >= 2 && onB)) level0Junction.push_back(mesh.gid[v]);
      }
      (void)nn;
    }
    vector<i64> excluded;
    for (int p = 0; p < P; ++p) {
      if (byPart[p].empty()) continue;
      auto elems = byPart[p];
      RepairPinches(elems, excluded);
      for (auto& c : Components(elems)) {
        PieceIn pi;
        pi.elems = std::move(c);
        pi.vrank = p;
        pieces.push_back(std::move(pi));
      }
    }
    rec.nExcluded = excluded.size();
    {
      const auto pred = PredOut();
      double tw = 0;
      for (i64 k = 0; k < mesh.ne(); ++k) tw += 1 + pred[k];
      rec.capLoad = 1.1 * tw / P;
    }
    for (i64 e : excluded)
      for (int a = 0; a < mesh.nn(); ++a) exclGids.push_back(mesh.gid[mesh.T[mesh.nn() * e + a]]);
  });
  vector<char> acc;
  const bool committed = Transaction(pieces, rec, acc);
  CollectivePhase("level 0 bookkeeping", [&] {
    if (!committed) {
      status = "INCOMPLETE_COVERAGE";
      stopReason = "level 0 rolled back (acceptance)";
      steps.push_back(rec);
      return;
    }
    std::unordered_map<i64, i64> g2v;
    for (i64 v = 0; v < mesh.nv(); ++v) g2v[mesh.gid[v]] = v;
    for (i64 g : exclGids) {
      const auto it = g2v.find(g);
      if (it != g2v.end()) mesh.vflag[it->second] |= V_CONSTRAINED;
    }
    rec.C = CoverageSet().size();
    rec.D = DefectSet().size();
    steps.push_back(rec);
  });
}

static string ClassifyStatus(size_t C, size_t D) {
  return C ? "INCOMPLETE_COVERAGE" : D ? "INCOMPLETE_QUALITY" : "COMPLETE";
}

void Driver::SBase(int firstLevel) {
  long prevC = -1, prevD = -1;
  vector<i64> prevCg;
  int stagC = 0, stagD = 0, L = ro.L;
  double capMem = 0;
  CollectivePhase("SBase initialization", [&] {
    const auto pred0 = PredOut();
    double totalWork = 0;
    for (i64 k = 0; k < mesh.ne(); ++k) totalWork += 1 + pred0[k];
    capMem = ro.capMemFactor * totalWork / ro.P;
  });
  bool coverageDone = false;
  for (int level = firstLevel;; ++level) {
    vector<i64> C, D;
    StepRecord rec;
    vector<PieceIn> pieces;
    bool stop = false;
    CollectivePhase("SBase step planning", [&] {
      C = CoverageSet();
      D = DefectSet();
      if (C.empty() && D.empty()) {
        status = "COMPLETE";
        stop = true;
        return;
      }
      vector<i64> Cg;
      for (i64 v : C) Cg.push_back(mesh.gid[v]);
      std::sort(Cg.begin(), Cg.end());
      if (C.empty() && !coverageDone) {
        coverageDone = true;
        prevD = -1;
        stagD = 0;
      }
      bool sC = !C.empty() && prevC >= 0 && ((double)C.size() >= 0.9 * prevC || Cg == prevCg);
      bool sD = C.empty() && !D.empty() && prevD >= 0 && (double)D.size() >= 0.9 * prevD;
      stagC = sC ? stagC + 1 : 0;
      stagD = sD ? stagD + 1 : 0;
      if (stagC >= 2 || stagD >= 2 || level >= ro.maxLevels) {
        status = ClassifyStatus(C.size(), D.size());
        stopReason = stagC >= 2 ? "coverage stagnation" : stagD >= 2 ? "quality stagnation" : "level bound";
        stop = true;
        return;
      }
      if (stagC == 1 || stagD == 1) ++L;
      prevC = C.empty() ? -1 : (long)C.size();
      prevCg = Cg;
      prevD = C.empty() ? (long)D.size() : -1;
      rec.kind = "level";
      rec.L = L;
      rec.C = C.size();
      rec.D = D.size();
      vector<i64> seeds(C);
      seeds.insert(seeds.end(), D.begin(), D.end());
      std::sort(seeds.begin(), seeds.end());
      seeds.erase(std::unique(seeds.begin(), seeds.end()), seeds.end());
      auto work = WorkSet(SurfaceExpand(seeds), L);
      auto comps = Components(work);
      vector<char> seedVert(mesh.nv(), 0);
      for (i64 v : C) seedVert[v] = 1;
      const auto pred = PredOut();
      double wTot = Work(work, pred);
      rec.capLoad = 1.1 * (wTot + (mesh.ne() - (double)work.size())) / ro.P;
      rec.capMem = capMem;
      auto pieces0 = AssignWhole(comps, capMem, rec, &seedVert, 0);
      vector<i64> excluded;
      RepairStep(pieces0, pieces, excluded);
      rec.nExcluded = excluded.size();
    });
    if (CollectiveDecision(stop)) break;
    vector<char> acc;
    const bool committed = Transaction(pieces, rec, acc);
    CollectivePhase("SBase step bookkeeping", [&] {
      if (!committed) {
        status = "INCOMPLETE_" + string(C.empty() ? "QUALITY" : "COVERAGE");
        stopReason = "step rolled back (acceptance)";
        steps.push_back(rec);
        return;
      }
      steps.push_back(rec);
    });
    if (!committed) return;
  }
}

void Driver::SBatch() {
  const int P = ro.P, nn = mesh.nn();
  const int L = ro.L;
  for (int phase = 0; phase < 2; ++phase) {
    struct Item {
      vector<i64> seedGids;
    };
    vector<Item> queue;
    vector<i64> S;
    double capLoad = 0;
    CollectivePhase("batch phase planning", [&] {
      /*--- queue built once per phase from C (coverage) or D (quality) ---*/
      S = phase == 0 ? CoverageSet() : DefectSet();
      if (S.empty()) return;
      const auto pred = PredOut();
      auto work = WorkSet(S, L);
      auto clusters = Components(work);
      capLoad = 1.1 * (Work(work, pred) + (mesh.ne() - (double)work.size())) / P;
      vector<char> isS(mesh.nv(), 0);
      for (i64 v : S) isS[v] = 1;
      for (auto& cl : clusters) {
        const double w = Work(cl, pred);
        /*--- seeds of the cluster: vertices of S with an element in the cluster ---*/
        std::set<i64> seedsCl;
        for (i64 e : cl)
          for (int a = 0; a < nn; ++a)
            if (isS[mesh.T[nn * e + a]]) seedsCl.insert(mesh.T[nn * e + a]);
        if (w <= capLoad / 2 || seedsCl.size() < 4) {
          Item it;
          for (i64 v : seedsCl) it.seedGids.push_back(mesh.gid[v]);
          queue.push_back(it);
          continue;
        }
        /*--- split the sheet: METIS on the cluster's dual graph with the B3 weights ---*/
        const idx_t k = (idx_t)std::ceil(w / (capLoad / 2));
        std::unordered_map<i64, idx_t> loc;
        for (size_t i = 0; i < cl.size(); ++i) loc[cl[i]] = i;
        vector<idx_t> xadj(cl.size() + 1, 0), adjncy, vw(cl.size()), aw;
        i64 f[3];
        for (size_t i = 0; i < cl.size(); ++i) {
          vw[i] = std::max<idx_t>(1, (idx_t)std::round(10 * (1 + pred[cl[i]])));
          for (int j = 0; j < nn; ++j) {
            const i64 o = adj.nb[nn * cl[i] + j];
            if (o < 0) continue;
            const auto it = loc.find(o);
            if (it == loc.end()) continue;
            adjncy.push_back(it->second);
            ElemFace(mesh.dim, &mesh.T[nn * cl[i]], j, f);
            aw.push_back((idx_t)std::round(FaceWeightB3(mesh, f, ro.a, ro.b)));
          }
          xadj[i + 1] = adjncy.size();
        }
        const auto part = MetisPart(cl.size(), 1, xadj, adjncy, vw, aw, k);
        vector<Item> sub(k);
        for (i64 v : seedsCl) {
          /*--- the part holding most of the seed's star elements in the cluster ---*/
          std::map<idx_t, int> c;
          for (i64 s = adj.v2eStart[v]; s < adj.v2eStart[v + 1]; ++s) {
            const auto it = loc.find(adj.v2e[s]);
            if (it != loc.end()) ++c[part[it->second]];
          }
          idx_t best = 0;
          int bc = -1;
          for (auto& kv : c)
            if (kv.second > bc) {
              bc = kv.second;
              best = kv.first;
            }
          sub[best].seedGids.push_back(mesh.gid[v]);
        }
        for (auto& s : sub)
          if (!s.seedGids.empty()) queue.push_back(s);
      }
    });
    if (CollectiveDecision(S.empty())) continue;
    const int budget = phase == 0 ? ro.batchBudgetC : ro.batchBudgetD;
    long prevD = -1;
    int batch = 0;
    for (; CollectiveDecision(batch < budget); ++batch) {
      StepRecord rec;
      vector<PieceIn> pieces;
      bool stop = false;
      CollectivePhase("batch step planning", [&] {
        const auto Cur = phase == 0 ? CoverageSet() : DefectSet();
        if (phase == 1 && prevD >= 0 && !Cur.empty() && (double)Cur.size() >= 0.9 * prevD) {
          stopReason = "quality stagnation (S-batch)";
          stop = true;
          return;
        }
        prevD = Cur.size();
        std::unordered_map<i64, i64> g2v;
        for (i64 v = 0; v < mesh.nv(); ++v) g2v[mesh.gid[v]] = v;
        std::set<i64> curSet(Cur.begin(), Cur.end());
        /*--- quality phase: new DEFECT seeds join Q_D as their own items ---*/
        if (phase == 1) {
          std::set<i64> queued;
          for (auto& it : queue)
            for (i64 g : it.seedGids) queued.insert(g);
          Item extra;
          for (i64 v : Cur)
            if (!queued.count(mesh.gid[v])) extra.seedGids.push_back(mesh.gid[v]);
          if (!extra.seedGids.empty()) queue.push_back(extra);
        }
        vector<vector<i64>> seedsOf(queue.size());
        vector<Item> keep;
        for (size_t q = 0; q < queue.size(); ++q) {
          Item it;
          for (i64 g : queue[q].seedGids) {
            const auto f = g2v.find(g);
            if (f != g2v.end() && curSet.count(f->second)) it.seedGids.push_back(g);
          }
          if (!it.seedGids.empty()) keep.push_back(it);
        }
        queue.swap(keep);
        if (queue.empty()) {
          stop = true;
          return;
        }
        /*--- re-extract closures, conflict graph, greedy maximal independent set in label (queue) order ---*/
        vector<vector<i64>> closure(queue.size());
        for (size_t q = 0; q < queue.size(); ++q) {
          vector<i64> sv;
          for (i64 g : queue[q].seedGids) sv.push_back(g2v.at(g));
          closure[q] = WorkSet(SurfaceExpand(sv), L);
        }
        std::unordered_map<i64, vector<int>> vOwner;
        for (size_t q = 0; q < closure.size(); ++q) {
          std::set<i64> vs;
          for (i64 e : closure[q])
            for (int a = 0; a < nn; ++a) vs.insert(mesh.T[nn * e + a]);
          for (i64 v : vs) vOwner[v].push_back(q);
        }
        vector<std::set<int>> conflict(queue.size());
        for (auto& kv : vOwner)
          for (int x : kv.second)
            for (int y : kv.second)
              if (x != y) conflict[x].insert(y);
        vector<char> chosen(queue.size(), 0), blocked(queue.size(), 0);
        for (size_t q = 0; q < queue.size(); ++q) {
          if (blocked[q]) continue;
          chosen[q] = 1;
          for (int y : conflict[q]) blocked[y] = 1;
        }
        /*--- Predictions belong to the current committed mesh, not the phase-entry mesh. ---*/
        const auto batchPred = PredOut();
        /*--- LPT assignment of the batch to the virtual ranks ---*/
        vector<std::pair<double, size_t>> order;
        for (size_t q = 0; q < queue.size(); ++q)
          if (chosen[q]) order.push_back({Work(closure[q], batchPred), q});
        std::stable_sort(order.begin(), order.end(), [](const auto& x, const auto& y) { return x.first > y.first; });
        vector<double> load(P, 0);
        rec.kind = phase == 0 ? "batchC" : "batchD";
        rec.L = L;
        rec.C = CoverageSet().size();
        rec.D = DefectSet().size();
        rec.capLoad = capLoad;
        vector<i64> excluded;
        vector<PieceIn> pieces0;
        for (auto& o : order) {
          const int v = std::min_element(load.begin(), load.end()) - load.begin();
          load[v] += o.first;
          PieceIn pi;
          pi.elems = closure[o.second];
          pi.vrank = v;
          pieces0.push_back(std::move(pi));
        }
        RepairStep(pieces0, pieces, excluded);
        rec.nExcluded = excluded.size();
        rec.note = Fmt("queue %zu, batch %zu", queue.size(), order.size());
      });
      if (CollectiveDecision(stop)) break;
      vector<char> acc;
      const bool committed = Transaction(pieces, rec, acc);
      CollectivePhase("batch step bookkeeping", [&] {
        if (!committed) {
          status = phase == 0 ? "INCOMPLETE_COVERAGE" : "INCOMPLETE_QUALITY";
          stopReason = "batch rolled back (acceptance)";
          steps.push_back(rec);
          return;
        }
        steps.push_back(rec);
        if (rec.nAccepted == 0) {
          stopReason = "no patch committed in a batch";
          stop = true;
        }
      });
      if (!committed) return;
      if (CollectiveDecision(stop)) break;
    }
    CollectivePhase("batch phase bookkeeping", [&] {
      if (phase == 0 && !CoverageSet().empty()) {
        status = "INCOMPLETE_COVERAGE";
        if (stopReason.empty()) stopReason = "coverage budget spent";
      }
    });
  }
  CollectivePhase("batch final status", [&] {
    if (status != "INCOMPLETE_COVERAGE") status = ClassifyStatus(CoverageSet().size(), DefectSet().size());
  });
}

void Driver::SB4() {
  const int dim = mesh.dim;
  const int budgetC = dim == 2 ? 4 : 5, budgetD = 2;
  double capMem = 0;
  CollectivePhase("SB4 initialization", [&] {
    const auto pred0 = PredOut();
    double totalWork = 0;
    for (i64 k = 0; k < mesh.ne(); ++k) totalWork += 1 + pred0[k];
    capMem = ro.capMemFactor * totalWork / ro.P;
  });
  int stepC = 0, stepD = 0;
  vector<i64> prevCg;
  long prevD = -1;
  while (true) {
    vector<i64> C, D;
    StepRecord rec;
    vector<PieceIn> pieces;
    bool stop = false;
    CollectivePhase("SB4 step planning", [&] {
      C = CoverageSet();
      D = DefectSet();
      if (C.empty() && D.empty()) {
        status = "COMPLETE";
        stop = true;
        return;
      }
      vector<i64> Cg;
      for (i64 v : C) Cg.push_back(mesh.gid[v]);
      std::sort(Cg.begin(), Cg.end());
      if (!C.empty()) {
        if (stepC >= budgetC) {
          status = "INCOMPLETE_COVERAGE";
          stopReason = "S-B4 coverage budget";
          stop = true;
          return;
        }
        if (!prevCg.empty() && Cg == prevCg) {
          status = "INCOMPLETE_COVERAGE";
          stopReason = "S-B4 coverage stagnation";
          stop = true;
          return;
        }
      } else {
        if (stepD >= budgetD) {
          status = "INCOMPLETE_QUALITY";
          stopReason = "S-B4 quality budget";
          stop = true;
          return;
        }
        if (prevD >= 0 && (double)D.size() >= 0.9 * prevD) {
          status = "INCOMPLETE_QUALITY";
          stopReason = "S-B4 quality stagnation";
          stop = true;
          return;
        }
      }
      prevCg = Cg;
      rec.L = ro.L;
      rec.C = C.size();
      rec.D = D.size();
      vector<i64> seeds;
      vector<i64> exclude;  // vertices near junctions (step 1 only)
      std::unordered_map<i64, i64> g2v;
      for (i64 v = 0; v < mesh.nv(); ++v) g2v[mesh.gid[v]] = v;
      vector<char> nearJ(mesh.nv(), 0);
      if (!C.empty() && stepC == 0) {
        /*--- step 1: sheets, kept apart near the junctions ---*/
        rec.kind = "sb4-sheets";
        vector<i64> J;
        for (i64 g : level0Junction) {
          const auto it = g2v.find(g);
          if (it != g2v.end()) J.push_back(it->second);
        }
        for (i64 k : WorkSet(J, ro.L + 1))
          for (int a = 0; a < mesh.nn(); ++a) nearJ[mesh.T[mesh.nn() * k + a]] = 1;
        for (i64 v : C)
          if (!nearJ[v]) seeds.push_back(v);
      } else if (!C.empty() && stepC == 1) {
        rec.kind = "sb4-curves";
        seeds = C;
      } else {
        rec.kind = C.empty() ? "sb4-quality" : "sb4-whole";
        seeds = C;
        seeds.insert(seeds.end(), D.begin(), D.end());
      }
      if (C.empty()) {
        prevD = D.size();
        ++stepD;
      } else {
        ++stepC;
      }
      std::sort(seeds.begin(), seeds.end());
      seeds.erase(std::unique(seeds.begin(), seeds.end()), seeds.end());
      auto work = WorkSet(SurfaceExpand(seeds), ro.L);
      if (rec.kind == "sb4-sheets") {
        vector<i64> w2;
        for (i64 k : work) {
          bool nj = false;
          for (int a = 0; a < mesh.nn() && !nj; ++a)
            nj = nearJ[mesh.T[mesh.nn() * k + a]] &&
                 !std::binary_search(seeds.begin(), seeds.end(), mesh.T[mesh.nn() * k + a]);
          if (!nj) w2.push_back(k);
        }
        work.swap(w2);
      }
      auto comps = Components(work);
      vector<char> seedVert(mesh.nv(), 0);
      for (i64 v : C) seedVert[v] = 1;
      rec.capMem = capMem;
      auto pieces0 = AssignWhole(comps, capMem, rec, &seedVert, 0);
      vector<i64> excluded;
      RepairStep(pieces0, pieces, excluded);
      rec.nExcluded = excluded.size();
    });
    if (CollectiveDecision(stop)) break;
    vector<char> acc;
    const bool committed = Transaction(pieces, rec, acc);
    CollectivePhase("SB4 step bookkeeping", [&] {
      if (!committed) {
        status = C.empty() ? "INCOMPLETE_QUALITY" : "INCOMPLETE_COVERAGE";
        stopReason = "step rolled back (acceptance)";
        steps.push_back(rec);
        return;
      }
      steps.push_back(rec);
    });
    if (!committed) return;
  }
}

}  // namespace b0
