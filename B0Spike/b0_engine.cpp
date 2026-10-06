/*!
 * \file b0_engine.cpp
 * \brief B0 spike: one MMG call on a piece with the contract of plan 6.3 (artificial faces required, explicit flags,
 *        identity of frozen vertices, face states, metric carry), the serial reference, and the splice.
 */
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "b0.hpp"
#include "mmg/libmmg.h"

extern "C" void SCOTCH_randomReset();

namespace b0 {

namespace {

struct InjectedFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct MMGHandle {
  int dim;
  MMG5_pMesh mesh = nullptr;
  MMG5_pSol met = nullptr;
  explicit MMGHandle(int d) : dim(d) {
    if (dim == 2)
      MMG2D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    else
      MMG3D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
  }
  ~MMGHandle() {
    if (dim == 2)
      MMG2D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    else
      MMG3D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
  }
};

struct RunFlags {
  double hmin, hmax, hgrad, hausd, angle, hgradreq = 0;
  bool setHgradreq = false;
  int nosurf = 0, nosizreq = 0, noswap = 0, verbose = -1;
  double memMB = -1;
};

/*--- Load a local mesh into MMG. bnd: boundary faces (dim per face, 0-based local), bref; required: per face flag. ---*/
bool Load(MMGHandle& h, int dim, const vector<double>& X, const vector<int>& vref, const vector<int>& T,
          const vector<int>& Tref, const vector<int>& B, const vector<int>& Bref, const vector<char>& Breq,
          const vector<int>& reqVert, const vector<int>& corners, const vector<double>& Met, double memMB) {
  const int np = X.size() / dim, ne = T.size() / (dim + 1), nb = B.size() / dim;
  vector<int> T1(T.size()), B1(B.size());
  for (size_t i = 0; i < T.size(); ++i) T1[i] = T[i] + 1;
  for (size_t i = 0; i < B.size(); ++i) B1[i] = B[i] + 1;
  vector<double> Xc(X), Mc(Met);
  vector<int> vr(vref), tr(Tref), br(Bref);
  /*--- every call is checked before the next one: after a failed Set_meshSize (memory cap) MMG has no arrays ---*/
#define B0_CHK(x) \
  if (!(x)) return false;
  if (dim == 2) {
    B0_CHK(MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_verbose, -1));
    if (memMB > 0) B0_CHK(MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_mem, (int)std::ceil(memMB)));
    B0_CHK(MMG2D_Set_meshSize(h.mesh, np, ne, 0, nb));
    B0_CHK(MMG2D_Set_vertices(h.mesh, Xc.data(), vr.data()));
    B0_CHK(MMG2D_Set_triangles(h.mesh, T1.data(), tr.data()));
    if (nb) B0_CHK(MMG2D_Set_edges(h.mesh, B1.data(), br.data()));
    for (int k = 0; k < nb; ++k)
      if (Breq[k]) B0_CHK(MMG2D_Set_requiredEdge(h.mesh, k + 1));
    for (int v : corners) B0_CHK(MMG2D_Set_corner(h.mesh, v + 1));
    for (int v : reqVert) B0_CHK(MMG2D_Set_requiredVertex(h.mesh, v + 1));
    B0_CHK(MMG2D_Set_solSize(h.mesh, h.met, MMG5_Vertex, np, MMG5_Tensor));
    B0_CHK(MMG2D_Set_tensorSols(h.met, Mc.data()));
    B0_CHK(MMG2D_Chk_meshData(h.mesh, h.met));
  } else {
    B0_CHK(MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_verbose, -1));
    if (memMB > 0) B0_CHK(MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_mem, (int)std::ceil(memMB)));
    B0_CHK(MMG3D_Set_meshSize(h.mesh, np, ne, 0, nb, 0, 0));
    B0_CHK(MMG3D_Set_vertices(h.mesh, Xc.data(), vr.data()));
    B0_CHK(MMG3D_Set_tetrahedra(h.mesh, T1.data(), tr.data()));
    if (nb) B0_CHK(MMG3D_Set_triangles(h.mesh, B1.data(), br.data()));
    for (int k = 0; k < nb; ++k)
      if (Breq[k]) B0_CHK(MMG3D_Set_requiredTriangle(h.mesh, k + 1));
    for (int v : corners) B0_CHK(MMG3D_Set_corner(h.mesh, v + 1));
    for (int v : reqVert) B0_CHK(MMG3D_Set_requiredVertex(h.mesh, v + 1));
    B0_CHK(MMG3D_Set_solSize(h.mesh, h.met, MMG5_Vertex, np, MMG5_Tensor));
    B0_CHK(MMG3D_Set_tensorSols(h.met, Mc.data()));
    B0_CHK(MMG3D_Chk_meshData(h.mesh, h.met));
  }
#undef B0_CHK
  return true;
}

int Run(MMGHandle& h, const RunFlags& f, bool& paramOk) {
  int ok = 1;
  if (h.dim == 2) {
    ok &= MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_verbose, f.verbose);
    ok &= MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_angle, 1);
    ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_angleDetection, f.angle);
    ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_hmin, f.hmin);
    ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_hmax, f.hmax);
    ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_hgrad, f.hgrad);
    ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_hausd, f.hausd);
    ok &= MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_nosurf, f.nosurf);
    ok &= MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_nosizreq, f.nosizreq);
    if (f.setHgradreq) ok &= MMG2D_Set_dparameter(h.mesh, h.met, MMG2D_DPARAM_hgradreq, f.hgradreq);
    ok &= MMG2D_Set_iparameter(h.mesh, h.met, MMG2D_IPARAM_noswap, f.noswap);
    paramOk = ok;
    if (!ok) return MMG5_STRONGFAILURE;
    SCOTCH_randomReset();
    return MMG2D_mmg2dlib(h.mesh, h.met);
  }
  ok &= MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_verbose, f.verbose);
  ok &= MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_angle, 1);
  ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_angleDetection, f.angle);
  ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_hmin, f.hmin);
  ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_hmax, f.hmax);
  ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_hgrad, f.hgrad);
  ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_hausd, f.hausd);
  ok &= MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_nosurf, f.nosurf);
  ok &= MMG3D_Set_iparameter(h.mesh, h.met, MMG3D_IPARAM_nosizreq, f.nosizreq);
  if (f.setHgradreq) ok &= MMG3D_Set_dparameter(h.mesh, h.met, MMG3D_DPARAM_hgradreq, f.hgradreq);
  paramOk = ok;
  if (!ok) return MMG5_STRONGFAILURE;
  SCOTCH_randomReset();
  return MMG3D_mmg3dlib(h.mesh, h.met);
}

struct Output {
  int np = 0, ne = 0, nb = 0;
  vector<double> X, M;
  vector<int> vref, T, Tref, B, Bref;
};

bool Get(MMGHandle& h, Output& o) {
  const int dim = h.dim, nn = dim + 1, nm = dim * (dim + 1) / 2;
  int ok = 1, nprism = 0, nquad = 0, na = 0, se = 0, snp = 0, st = 0;
  if (dim == 2) {
    ok &= MMG2D_Get_meshSize(h.mesh, &o.np, &o.ne, &nquad, &o.nb);
    ok &= MMG2D_Get_solSize(h.mesh, h.met, &se, &snp, &st);
  } else {
    ok &= MMG3D_Get_meshSize(h.mesh, &o.np, &o.ne, &nprism, &o.nb, &nquad, &na);
    ok &= MMG3D_Get_solSize(h.mesh, h.met, &se, &snp, &st);
  }
  if (!ok || o.np <= 0 || o.ne <= 0 || nprism || nquad) return false;
  o.X.resize(o.np * dim);
  o.vref.resize(o.np);
  o.T.resize(o.ne * nn);
  o.Tref.resize(o.ne);
  o.B.resize(o.nb * dim);
  o.Bref.resize(o.nb);
  vector<int> corner(o.np), req(o.np), treq(o.ne), bridge(o.nb), breq(o.nb);
  const bool hasMet = (snp == o.np && st == MMG5_Tensor);
  if (hasMet) o.M.resize(o.np * nm);
  if (dim == 2) {
    ok &= MMG2D_Get_vertices(h.mesh, o.X.data(), o.vref.data(), corner.data(), req.data());
    ok &= MMG2D_Get_triangles(h.mesh, o.T.data(), o.Tref.data(), treq.data());
    if (o.nb) ok &= MMG2D_Get_edges(h.mesh, o.B.data(), o.Bref.data(), bridge.data(), breq.data());
    if (hasMet) ok &= MMG2D_Get_tensorSols(h.met, o.M.data());
  } else {
    ok &= MMG3D_Get_vertices(h.mesh, o.X.data(), o.vref.data(), corner.data(), req.data());
    ok &= MMG3D_Get_tetrahedra(h.mesh, o.T.data(), o.Tref.data(), treq.data());
    if (o.nb) ok &= MMG3D_Get_triangles(h.mesh, o.B.data(), o.Bref.data(), breq.data());
    if (hasMet) ok &= MMG3D_Get_tensorSols(h.met, o.M.data());
  }
  for (auto& v : o.T) --v;
  for (auto& v : o.B) --v;
  return ok;
}

}  // namespace

/*----------------------------------------------------------------------------------------------------------------*/

PieceOut Engine::AdaptPiece(const Mesh& m, const Adjacency& adj, const PieceIn& p, int step) const {
  PieceOut out;
  const double t0 = Now();
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  Stage cur = Stage::MIGRATION;
  auto inject = [&](Stage s) {
    cur = s;
    if (opt.injectStage == s && (opt.injectTarget && opt.injectLabel == p.label) &&
        (opt.injectStep < 0 || opt.injectStep == step))
      throw InjectedFailure(string("injected failure at stage ") + StageName(s));
  };
  try {
    /*======== extraction (migration in a distributed run) ========*/
    inject(Stage::MIGRATION);
    auto inPiece = [&](i64 e) { return std::binary_search(p.elems.begin(), p.elems.end(), e); };
    vector<i64> l2g;
    l2g.reserve(p.elems.size() * nn);
    for (i64 e : p.elems)
      for (int a = 0; a < nn; ++a) l2g.push_back(m.T[nn * e + a]);
    std::sort(l2g.begin(), l2g.end());
    l2g.erase(std::unique(l2g.begin(), l2g.end()), l2g.end());
    auto g2l = [&](i64 g) { return (int)(std::lower_bound(l2g.begin(), l2g.end(), g) - l2g.begin()); };
    const int np = l2g.size();
    vector<double> X(np * dim), Mc(np * nm);
    for (int l = 0; l < np; ++l) {
      for (int d = 0; d < dim; ++d) X[dim * l + d] = m.X[dim * l2g[l] + d];
      for (int c = 0; c < nm; ++c) Mc[nm * l + c] = m.M[nm * l2g[l] + c];
    }
    vector<int> T(p.elems.size() * nn), Tref(p.elems.size());
    for (size_t k = 0; k < p.elems.size(); ++k) {
      for (int a = 0; a < nn; ++a) T[nn * k + a] = g2l(m.T[nn * p.elems[k] + a]);
      Tref[k] = m.Tref[p.elems[k]];
    }
    /*--- boundary faces of the piece: physical or artificial ---*/
    vector<int> artF;      // local, dim per face (outward)
    vector<i64> physF;     // global physical face ids
    i64 fa[3];
    for (i64 e : p.elems)
      for (int j = 0; j < nn; ++j) {
        const i64 nbE = adj.nb[nn * e + j];
        if (nbE >= 0 && inPiece(nbE)) continue;
        const i64 pf = adj.fphys[nn * e + j];
        if (pf >= 0) {
          physF.push_back(pf);
        } else if (nbE >= 0) {
          ElemFace(dim, &m.T[nn * e], j, fa);
          for (int a = 0; a < dim; ++a) artF.push_back(g2l(fa[a]));
        } else {
          throw std::runtime_error("boundary face without a marker in the piece input");
        }
      }
    std::sort(physF.begin(), physF.end());
    const int nArt = artF.size() / dim;
    out.nArtFaces = nArt;
    vector<char> isArtV(np, 0);
    for (int v : artF) isArtV[v] = 1;
    /*--- physical faces by state (6.9.6): freeness = complete surface star in the piece, no star vertex on an
     *    artificial face. ---*/
    const i64 nPhys = physF.size();
    vector<char> physReq(nPhys, 1);
    if (prm.surface) {
      for (i64 q = 0; q < nPhys; ++q) {
        const i64 f = physF[q];
        if (m.Fstate[f] != F_UNTOUCHED && m.Fstate[f] != F_PENDING) continue;
        bool free_ = true;
        for (int a = 0; a < dim && free_; ++a) {
          const i64 v = m.F[dim * f + a];
          for (i64 s = adj.v2fStart[v]; s < adj.v2fStart[v + 1] && free_; ++s) {
            const i64 g = adj.v2f[s];
            if (!std::binary_search(physF.begin(), physF.end(), g)) { free_ = false; break; }
            for (int b = 0; b < dim; ++b)
              if (isArtV[g2l(m.F[dim * g + b])]) { free_ = false; break; }
          }
        }
        physReq[q] = !free_;
      }
    }
    vector<int> B, Bref;
    vector<char> Breq;
    for (int q = 0; q < nArt; ++q) {
      for (int a = 0; a < dim; ++a) B.push_back(artF[dim * q + a]);
      Bref.push_back(refArt);
      Breq.push_back(1);
    }
    for (i64 q = 0; q < nPhys; ++q) {
      for (int a = 0; a < dim; ++a) B.push_back(g2l(m.F[dim * physF[q] + a]));
      Bref.push_back(m.Fref[physF[q]]);
      Breq.push_back(prm.surface ? physReq[q] : 1);
    }
    /*--- point refs, required vertices, corners ---*/
    vector<int> vref(np, 0), reqV, corners;
    vector<char> mustRetain(np, 0);
    for (int q = 0; q < (int)Bref.size(); ++q)
      if (Breq[q])
        for (int a = 0; a < dim; ++a) mustRetain[B[dim * q + a]] = 1;
    for (int l = 0; l < np; ++l) {
      if (m.vflag[l2g[l]] & V_CORNER) {
        corners.push_back(l);
        mustRetain[l] = 1;
      }
      if (isArtV[l]) reqV.push_back(l);
      if (mustRetain[l]) vref[l] = l + 1;
    }
    for (int l : corners)
      if (!isArtV[l]) reqV.push_back(l);
    out.nFrozenVerts = std::count(mustRetain.begin(), mustRetain.end(), 1);
    out.nFreeVerts = np - out.nFrozenVerts;
    /*--- M_mmg (6.2): carried metric, physical floor at fixed-surface vertices, artificial floor (FLOOR) ---*/
    vector<double> Mmg(Mc);
    if (!prm.surface) {
      for (int l = 0; l < np; ++l) {
        const auto it = physFloor.find(m.gid[l2g[l]]);
        if (it != physFloor.end()) std::copy(it->second.begin(), it->second.begin() + nm, &Mmg[nm * l]);
      }
    }
    if (opt.floorArt && nArt) {
      vector<vector<std::array<double, 3>>> edges(np);
      for (int q = 0; q < nArt; ++q)
        for (int a = 0; a < dim; ++a)
          for (int b = 0; b < dim; ++b) {
            if (a == b) continue;
            std::array<double, 3> e = {0, 0, 0};
            const int va = artF[dim * q + a], vb = artF[dim * q + b];
            for (int d = 0; d < dim; ++d) e[d] = X[dim * vb + d] - X[dim * va + d];
            edges[va].push_back(e);
          }
      for (int l = 0; l < np; ++l)
        if (!edges[l].empty()) FloorWithEdges(dim, &Mmg[nm * l], edges[l]);
    }
    out.bytesIn = X.size() * 8 + Mc.size() * 16 + T.size() * 8 + B.size() * 8;
    out.nInElem = p.elems.size();

    /*======== MMG ========*/
    inject(Stage::MMG);
    MMGHandle h(dim);
    double memMB = opt.mmgMemMB;
    if (opt.mmgMemFactor > 0)
      memMB = std::max(opt.mmgMemMin, opt.mmgMemFactor * opt.mmgBytesPerElem * (p.elems.size() + p.predOut) / 1048576.0);
    out.capMB = memMB;
    if (!Load(h, dim, X, vref, T, Tref, B, Bref, Breq, reqV, corners, Mmg, memMB))
      throw std::runtime_error("could not load the piece into MMG");
    RunFlags f;
    f.hmin = prm.hmin;
    f.hmax = prm.hmax;
    f.hgrad = prm.hgrad;
    f.hausd = prm.hausd;
    f.angle = prm.angle;
    f.verbose = opt.verbosity;
    f.nosurf = prm.surface ? 0 : 1;
    f.noswap = (prm.bl && dim == 2) ? 1 : 0;
    if (nArt) {
      f.nosizreq = 1;
      f.setHgradreq = true;
      f.hgradreq = -1;
    } else {
      f.nosizreq = prm.surface ? 0 : 1;
      f.setHgradreq = !prm.surface;
      f.hgradreq = -1;
    }
    bool paramOk = true;
    /*--- debug: B0_DUMP_STEP=<step>:<min elements> saves the MMG input and output of large pieces ---*/
    string dumpName;
    if (const char* ds = std::getenv("B0_DUMP_STEP")) {
      int dstep = -1;
      long dmin = 0;
      sscanf(ds, "%d:%ld", &dstep, &dmin);
      if (dstep == step && (long)p.elems.size() >= dmin) {
        dumpName = Fmt("piece_s%d_v%d_n%zu", step, p.vrank, p.elems.size());
        if (dim == 2) {
          MMG2D_saveMesh(h.mesh, (dumpName + "_in.mesh").c_str());
          MMG2D_saveSol(h.mesh, h.met, (dumpName + "_in.sol").c_str());
        } else {
          MMG3D_saveMesh(h.mesh, (dumpName + "_in.mesh").c_str());
          MMG3D_saveSol(h.mesh, h.met, (dumpName + "_in.sol").c_str());
        }
      }
    }
    const double tm0 = Now();
    const int ier = Run(h, f, paramOk);
    if (!dumpName.empty()) {
      if (dim == 2) {
        MMG2D_saveMesh(h.mesh, (dumpName + "_out.mesh").c_str());
        MMG2D_saveSol(h.mesh, h.met, (dumpName + "_out.sol").c_str());
      } else {
        MMG3D_saveMesh(h.mesh, (dumpName + "_out.mesh").c_str());
        MMG3D_saveSol(h.mesh, h.met, (dumpName + "_out.sol").c_str());
      }
    }
    out.tMMG = Now() - tm0;
    out.mmgStatus = ier;
    if (!paramOk) throw std::runtime_error("MMG parameter failure");
    if (ier != MMG5_SUCCESS && ier != MMG5_LOWFAILURE)
      throw std::runtime_error(Fmt("MMG STRONGFAILURE (status %d)", ier));
    Output o;
    if (!Get(h, o)) throw std::runtime_error("could not read the MMG output");

    /*======== identity of frozen vertices (validation) ========*/
    inject(Stage::VALIDATION);
    const double tol2 = std::pow(1e-10 * domainSize, 2);
    vector<int> outToIn(o.np, -1), retainedCount(np, 0);
    for (int v = 0; v < o.np; ++v) {
      const int r = o.vref[v];
      if (r < 1 || r > np || !mustRetain[r - 1]) continue;
      double d2 = 0;
      for (int d = 0; d < dim; ++d) d2 += std::pow(o.X[dim * v + d] - X[dim * (r - 1) + d], 2);
      if (d2 > tol2) continue;
      outToIn[v] = r - 1;
      ++retainedCount[r - 1];
    }
    for (int l = 0; l < np; ++l) {
      if (mustRetain[l] && retainedCount[l] != 1)
        throw std::runtime_error(Fmt("MMG changed a frozen vertex (gid %ld retained %d times)", (long)m.gid[l2g[l]],
                                     retainedCount[l]));
    }
    /*--- output boundary faces ---*/
    std::set<FKey> inArt, inReqPhys;
    std::map<FKey, i64> reqPhysFace;
    for (int q = 0; q < nArt; ++q) {
      i64 g[3];
      for (int a = 0; a < dim; ++a) g[a] = l2g[artF[dim * q + a]];
      inArt.insert(MakeKey(dim, g));
    }
    for (i64 q = 0; q < nPhys; ++q)
      if (!prm.surface || physReq[q]) reqPhysFace[MakeKey(dim, &m.F[dim * physF[q]])] = physF[q];
    std::set<FKey> outArt, outReq;
    // coded vertex: retained -> global vertex index; new -> -(1+k)
    vector<i64> code(o.np);
    int nNew = 0;
    for (int v = 0; v < o.np; ++v) code[v] = outToIn[v] >= 0 ? l2g[outToIn[v]] : -(1 + nNew++);
    out.newX.resize(nNew * dim);
    out.newM.resize(nNew * nm);
    for (int v = 0; v < o.np; ++v)
      if (code[v] < 0)
        for (int d = 0; d < dim; ++d) out.newX[dim * (-code[v] - 1) + d] = o.X[dim * v + d];
    for (int q = 0; q < o.nb; ++q) {
      i64 g[3] = {-1, -1, -1};
      bool allRet = true;
      for (int a = 0; a < dim; ++a) {
        g[a] = code[o.B[dim * q + a]];
        if (g[a] < 0) allRet = false;
      }
      if (o.Bref[q] == refArt) {
        if (!allRet) throw std::runtime_error("an artificial face came back with a new vertex");
        const FKey k = MakeKey(dim, g);
        if (!inArt.count(k)) throw std::runtime_error("MMG returned an unknown artificial face");
        if (!outArt.insert(k).second) throw std::runtime_error("artificial face returned twice");
        continue;
      }
      uint8_t state = prm.surface ? F_DONE : F_FIXED;
      if (allRet) {
        const FKey k = MakeKey(dim, g);
        const auto it = reqPhysFace.find(k);
        if (it != reqPhysFace.end()) {
          outReq.insert(k);
          const uint8_t s0 = m.Fstate[it->second];
          state = (s0 == F_UNTOUCHED) ? F_PENDING : s0;
          if (m.Fref[it->second] != o.Bref[q]) throw std::runtime_error("a protected face changed its marker");
        }
      }
      for (int a = 0; a < dim; ++a) out.F.push_back(g[a]);
      out.Fref.push_back(o.Bref[q]);
      out.Fstate.push_back(state);
    }
    if (outArt.size() != inArt.size())
      throw std::runtime_error(Fmt("%zu of %zu artificial faces returned", outArt.size(), inArt.size()));
    if (outReq.size() != reqPhysFace.size())
      throw std::runtime_error(Fmt("%zu of %zu protected physical faces returned", outReq.size(), reqPhysFace.size()));

    /*======== metric of new vertices (interpolation) ========*/
    inject(Stage::INTERPOLATION);
    {
      Locator loc;
      const Locator* L = nullptr;
      const double* Mv = nullptr;
      if (opt.background && bg) {
        L = &bg->loc;
        Mv = bg->mesh->M.data();
      } else {
        vector<i64> T64(T.begin(), T.end());
        loc.Build(dim, X.data(), T64.data(), T.size() / nn);
        // keep T64 alive during the queries
        for (int k = 0; k < nNew; ++k) {
          if (!InterpMetric(loc, Mc.data(), dim, &out.newX[dim * k], &out.newM[nm * k]))
            throw std::runtime_error("metric interpolation failed (not SPD or not located)");
        }
        L = nullptr;
      }
      if (L) {
        for (int k = 0; k < nNew; ++k)
          if (!InterpMetric(*L, Mv, dim, &out.newX[dim * k], &out.newM[nm * k]))
            throw std::runtime_error("background metric interpolation failed");
      }
    }

    /*======== local validation of the output ========*/
    inject(Stage::VALIDATION);
    {
      // positive volumes in final coordinates
      auto X_of = [&](int v, double* x) {
        if (code[v] >= 0)
          for (int d = 0; d < dim; ++d) x[d] = m.X[dim * code[v] + d];
        else
          for (int d = 0; d < dim; ++d) x[d] = out.newX[dim * (-code[v] - 1) + d];
      };
      vector<double> Xo(o.np * dim);
      for (int v = 0; v < o.np; ++v) X_of(v, &Xo[dim * v]);
      vector<char> used(o.np, 0);
      i64 nNeg = 0;
      vector<i64> e64(nn);
      for (int k = 0; k < o.ne; ++k) {
        for (int a = 0; a < nn; ++a) {
          e64[a] = o.T[nn * k + a];
          used[o.T[nn * k + a]] = 1;
        }
        if (!(SignedVolume(dim, Xo.data(), e64.data()) > 0)) ++nNeg;
      }
      if (nNeg) throw std::runtime_error(Fmt("%ld output elements with non-positive volume", (long)nNeg));
      for (int v = 0; v < o.np; ++v)
        if (!used[v]) throw std::runtime_error("unused output vertex");
      // face counts: count-1 faces == output boundary faces
      std::map<FKey, int> cnt;
      i64 fa2[3];
      for (int k = 0; k < o.ne; ++k)
        for (int j = 0; j < nn; ++j) {
          for (int a = 0; a < nn; ++a) e64[a] = o.T[nn * k + a];
          ElemFace(dim, e64.data(), j, fa2);
          ++cnt[MakeKey(dim, fa2)];
        }
      std::set<FKey> bset;
      for (int q = 0; q < o.nb; ++q) {
        i64 g[3];
        for (int a = 0; a < dim; ++a) g[a] = o.B[dim * q + a];
        bset.insert(MakeKey(dim, g));
      }
      i64 nBad = 0;
      for (auto& kv : cnt) {
        if (kv.second > 2) ++nBad;
        if (kv.second == 1 && !bset.count(kv.first)) ++nBad;
      }
      if (nBad) throw std::runtime_error(Fmt("%ld output faces with a wrong count/classification", (long)nBad));
      if ((i64)bset.size() != o.nb) throw std::runtime_error("duplicate output boundary faces");
    }

    /*======== splice preparation ========*/
    inject(Stage::SPLICE);
    out.T.resize(o.ne * nn);
    for (int k = 0; k < o.ne * nn; ++k) out.T[k] = code[o.T[k]];
    out.Tref = o.Tref;
    out.removedFaces = physF;
    for (const auto& k : inArt)
      for (int a = 0; a < dim; ++a) out.artFaceKeysGid.push_back(m.gid[k.v[a]]);
    for (int l = 0; l < np; ++l) (isArtV[l] ? out.frozenVerts : out.freedVerts).push_back(l2g[l]);
    out.nOutElem = o.ne;
    out.nOutVert = o.np;
    out.bytesOut = out.T.size() * 8 + out.newX.size() * 8 + out.newM.size() * 8 + out.F.size() * 8;
  } catch (const InjectedFailure& e) {
    out.st = {1, cur, e.what()};
  } catch (const std::bad_alloc&) {
    out.st = {1, cur, string("out of memory in stage ") + StageName(cur)};
  } catch (const std::exception& e) {
    out.st = {1, cur, e.what()};
  }
  out.tTotal = Now() - t0;
  return out;
}

/*----------------------------------------------------------------------------------------------------------------*/

Mesh Engine::SerialRemesh(const Mesh& m, int& status, double& tMMG, double& peakMB) const {
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  const int np = m.nv(), ne = m.ne(), nb = m.nf();
  TrimHeap();
  const double rss0 = RSSMB();
  ResetHWM();
  vector<double> X(m.X), Mmg(m.M);
  vector<int> vref(np, 0), T(m.T.begin(), m.T.end()), Tref(m.Tref), B(m.F.begin(), m.F.end()), Bref(m.Fref);
  vector<char> Breq(nb, 0);
  vector<int> corners, reqV;
  for (int v = 0; v < np; ++v)
    if (m.vflag[v] & V_CORNER) {
      corners.push_back(v);
      reqV.push_back(v);
    }
  if (!prm.surface) {
    for (int q = 0; q < nb; ++q)
      for (int a = 0; a < dim; ++a) vref[B[dim * q + a]] = B[dim * q + a] + 1;
    for (int v = 0; v < np; ++v) {
      const auto it = physFloor.find(m.gid[v]);
      if (it != physFloor.end()) std::copy(it->second.begin(), it->second.begin() + nm, &Mmg[nm * v]);
    }
  }
  MMGHandle h(dim);
  if (!Load(h, dim, X, vref, T, Tref, B, Bref, Breq, reqV, corners, Mmg, opt.mmgMemMB))
    throw std::runtime_error("serial load failed");
  RunFlags f;
  f.hmin = prm.hmin;
  f.hmax = prm.hmax;
  f.hgrad = prm.hgrad;
  f.hausd = prm.hausd;
  f.angle = prm.angle;
  f.verbose = opt.verbosity;
  f.nosurf = prm.surface ? 0 : 1;
  f.nosizreq = prm.surface ? 0 : 1;
  f.setHgradreq = !prm.surface;
  f.hgradreq = -1;
  f.noswap = (prm.bl && dim == 2) ? 1 : 0;
  bool paramOk;
  const double t0 = Now();
  status = Run(h, f, paramOk);
  tMMG = Now() - t0;
  if (status != MMG5_SUCCESS && status != MMG5_LOWFAILURE) throw std::runtime_error("serial MMG failed");
  Output o;
  if (!Get(h, o)) throw std::runtime_error("serial get failed");
  peakMB = HWMMB() - rss0;
  Mesh r;
  r.dim = dim;
  r.X = o.X;
  if (!prm.surface) {
    const double tol2 = std::pow(1e-10 * domainSize, 2);
    for (int v = 0; v < o.np; ++v) {
      const int ref = o.vref[v];
      if (ref < 1 || ref > np) continue;
      double d2 = 0;
      for (int d = 0; d < dim; ++d) d2 += std::pow(o.X[dim * v + d] - m.X[dim * (ref - 1) + d], 2);
      if (d2 <= tol2)
        for (int d = 0; d < dim; ++d) r.X[dim * v + d] = m.X[dim * (ref - 1) + d];
    }
  }
  r.M = o.M;
  r.T.assign(o.T.begin(), o.T.end());
  r.Tref = o.Tref;
  r.F.assign(o.B.begin(), o.B.end());
  r.Fref = o.Bref;
  r.Fstate.assign(o.nb, prm.surface ? F_DONE : F_FIXED);
  r.markerName = m.markerName;
  r.markerRef = m.markerRef;
  r.gid.resize(o.np);
  std::iota(r.gid.begin(), r.gid.end(), 0);
  r.gidEnd = o.np;
  r.vflag.assign(o.np, 0);
  (void)nn;
  (void)ne;
  return r;
}

/*----------------------------------------------------------------------------------------------------------------*/

Mesh Splice(const Mesh& m, const Adjacency& adj, const vector<PieceIn>& pieces, const vector<PieceOut>& outs,
            const vector<char>& accepted, vector<i64>* newGidRange) {
  (void)adj;
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  vector<char> delE(m.ne(), 0), delF(m.nf(), 0);
  for (size_t i = 0; i < pieces.size(); ++i) {
    if (!accepted[i]) continue;
    for (i64 e : pieces[i].elems) delE[e] = 1;
    for (i64 f : outs[i].removedFaces) delF[f] = 1;
  }
  /*--- old vertices kept: used by a remaining element or retained by an output ---*/
  vector<char> keepV(m.nv(), 0);
  for (i64 k = 0; k < m.ne(); ++k)
    if (!delE[k])
      for (int a = 0; a < nn; ++a) keepV[m.T[nn * k + a]] = 1;
  for (size_t i = 0; i < pieces.size(); ++i) {
    if (!accepted[i]) continue;
    for (i64 c : outs[i].T)
      if (c >= 0) keepV[c] = 1;
  }
  Mesh r;
  r.dim = dim;
  r.markerName = m.markerName;
  r.markerRef = m.markerRef;
  vector<i64> newIdx(m.nv(), -1);
  for (i64 v = 0; v < m.nv(); ++v) {
    if (!keepV[v]) continue;
    newIdx[v] = r.nv();
    for (int d = 0; d < dim; ++d) r.X.push_back(m.X[dim * v + d]);
    for (int c = 0; c < nm; ++c) r.M.push_back(m.M[nm * v + c]);
    r.gid.push_back(m.gid[v]);
    r.vflag.push_back(m.vflag[v]);
  }
  /*--- CONSTRAINED cleared for vertices that were free in an accepted call ---*/
  for (size_t i = 0; i < pieces.size(); ++i) {
    if (!accepted[i]) continue;
    for (i64 v : outs[i].freedVerts)
      if (newIdx[v] >= 0) r.vflag[newIdx[v]] &= ~V_CONSTRAINED;
  }
  for (i64 k = 0; k < m.ne(); ++k) {
    if (delE[k]) continue;
    for (int a = 0; a < nn; ++a) r.T.push_back(newIdx[m.T[nn * k + a]]);
    r.Tref.push_back(m.Tref[k]);
  }
  for (i64 f = 0; f < m.nf(); ++f) {
    if (delF[f]) continue;
    for (int a = 0; a < dim; ++a) r.F.push_back(newIdx[m.F[dim * f + a]]);
    r.Fref.push_back(m.Fref[f]);
    r.Fstate.push_back(m.Fstate[f]);
  }
  /*--- new vertices of each accepted output, gids in piece order (the scheduler orders pieces by label) ---*/
  i64 gidNext = m.gidEnd;
  for (size_t i = 0; i < pieces.size(); ++i) {
    if (!accepted[i]) continue;
    const auto& o = outs[i];
    const i64 base = r.nv();
    const i64 nNew = o.newX.size() / dim;
    if (newGidRange) newGidRange->push_back(gidNext);
    for (i64 k = 0; k < nNew; ++k) {
      for (int d = 0; d < dim; ++d) r.X.push_back(o.newX[dim * k + d]);
      for (int c = 0; c < nm; ++c) r.M.push_back(o.newM[nm * k + c]);
      r.gid.push_back(gidNext++);
      r.vflag.push_back(o.mmgStatus == 1 ? V_DEFECT : 0);  // LOWFAILURE: its new vertices become DEFECT seeds
    }
    auto map = [&](i64 c) { return c >= 0 ? newIdx[c] : base + (-c - 1); };
    for (size_t q = 0; q < o.T.size(); ++q) r.T.push_back(map(o.T[q]));
    r.Tref.insert(r.Tref.end(), o.Tref.begin(), o.Tref.end());
    for (size_t q = 0; q < o.F.size(); ++q) r.F.push_back(map(o.F[q]));
    r.Fref.insert(r.Fref.end(), o.Fref.begin(), o.Fref.end());
    r.Fstate.insert(r.Fstate.end(), o.Fstate.begin(), o.Fstate.end());
  }
  r.gidEnd = gidNext;
  return r;
}

}  // namespace b0
