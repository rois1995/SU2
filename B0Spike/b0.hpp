/*!
 * \file b0.hpp
 * \brief B0 spike of approach B (MPI_REMESH_PLAN.md, B_DESIGN_EXPLORATION.md rev. 3): developer program on gathered
 *        meshes. Pieces from a partition array, the repair engine in-process, serial MMG per piece.
 * \note Spike code, branch b0-spike only. Standalone (no SU2 library): MMG, METIS (SU2's 64-bit build), Eigen, MPI.
 */
#pragma once

#include <mpi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace b0 {

/*--- Explicit singleton execution for serial smoke tests where MPI sockets are blocked. ---*/
inline bool SerialExecution() { return std::getenv("B0_SERIAL_ONLY") != nullptr; }
inline void CommRank(MPI_Comm comm, int* rank) { if (SerialExecution()) *rank = 0; else MPI_Comm_rank(comm, rank); }
inline void CommSize(MPI_Comm comm, int* size) { if (SerialExecution()) *size = 1; else MPI_Comm_size(comm, size); }
inline void Reduce(const void* in, void* out, int n, MPI_Datatype type, MPI_Op op, MPI_Comm comm) {
  if (!SerialExecution()) { MPI_Allreduce(in, out, n, type, op, comm); return; }
  const size_t bytes = type == MPI_INT ? sizeof(int) : type == MPI_DOUBLE ? sizeof(double) : type == MPI_UINT64_T ? sizeof(uint64_t) : 0;
  if (!bytes) throw std::runtime_error("unsupported singleton reduction type");
  std::memcpy(out, in, n * bytes);
}

using i64 = int64_t;
using std::string;
using std::vector;

/*--- All ranks enter a phase boundary, including ranks with no local work.
 * Keep MPI calls outside CollectivePhase: a local exception must not skip one.
 * Broadcast a bounded diagnostic without allocating memory before the Bcast. ---*/
inline void CollectiveCheck(const char* context, const char* error) {
  int rank = 0;
  CommRank(MPI_COMM_WORLD, &rank);
  int local = error ? rank : INT_MAX, failed = INT_MAX;
  Reduce(&local, &failed, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
  if (failed == INT_MAX) return;
  char message[1024] = {};
  if (rank == failed) std::snprintf(message, sizeof(message), "%s", error);
  if (!SerialExecution()) MPI_Bcast(message, sizeof(message), MPI_CHAR, failed, MPI_COMM_WORLD);
  throw std::runtime_error(string(context) + " failed on rank " + std::to_string(failed) + ": " + message);
}

template <class F>
void CollectivePhase(const char* context, F&& work) {
  char error[1024] = {};
  bool failed = false;
  try {
    work();
  } catch (const std::exception& e) {
    failed = true;
    std::snprintf(error, sizeof(error), "%s", e.what());
  } catch (...) {
    failed = true;
    std::snprintf(error, sizeof(error), "unknown error");
  }
  CollectiveCheck(context, failed ? error : nullptr);
}

/*--- Reject divergent scheduler/command decisions before ranks branch. ---*/
inline int CollectiveDecision(int value) {
  int local = value, lo = 0, hi = 0;
  Reduce(&local, &lo, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
  Reduce(&local, &hi, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  CollectiveCheck("control flow", lo != hi ? "rank decisions differ" : nullptr);
  return value;
}

inline void CheckExchangeSize(size_t bytes) {
  CollectiveCheck("piece exchange", bytes > INT_MAX ? "piece exchange exceeds MPI count limit" : nullptr);
}

/*--- Vertex flags (artificial fields of the plan, 3.2). ---*/
enum : uint8_t { V_CONSTRAINED = 1, V_DEFECT = 2, V_CORNER = 4 };
/*--- Physical face states (plan 6.9.6). ---*/
enum : uint8_t { F_UNTOUCHED = 0, F_PENDING = 1, F_DONE = 2, F_FIXED = 3 };

struct Params {
  double hmin = 1e-8, hmax = 10.0, hgrad = 1.3, hausd = 0.01, angle = 45.0;
  bool surface = true, bl = false;
};

/*--- Gathered mesh with the distributed-mesh fields of the plan (3.2) that B0 needs. ---*/
struct Mesh {
  int dim = 0;
  vector<double> X;      // nv*dim
  vector<double> M;      // nv*nm, carried metric (upper triangle)
  vector<i64> gid;       // stable vertex id
  vector<uint8_t> vflag; // V_*
  vector<i64> T;         // ne*(dim+1)
  vector<int> Tref;
  vector<i64> F;         // nf*dim physical faces
  vector<int> Fref;      // marker ref
  vector<uint8_t> Fstate;
  vector<string> markerName;
  vector<int> markerRef;
  i64 gidEnd = 0;

  int nn() const { return dim + 1; }
  int nm() const { return dim * (dim + 1) / 2; }
  i64 nv() const { return dim ? (i64)X.size() / dim : 0; }
  i64 ne() const { return dim ? (i64)T.size() / (dim + 1) : 0; }
  i64 nf() const { return dim ? (i64)F.size() / dim : 0; }
};

/*--- Face key: sorted vertex ids (third = -1 in 2D). ---*/
struct FKey {
  i64 v[3];
  bool operator<(const FKey& o) const {
    return v[0] != o.v[0] ? v[0] < o.v[0] : v[1] != o.v[1] ? v[1] < o.v[1] : v[2] < o.v[2];
  }
  bool operator==(const FKey& o) const { return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2]; }
  bool operator!=(const FKey& o) const { return !(*this == o); }
};
struct FKeyHash {
  size_t operator()(const FKey& k) const {
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < 3; ++i) { h ^= (uint64_t)k.v[i] + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2); }
    return h;
  }
};
inline FKey MakeKey(int dim, const i64* n) {
  FKey k{{n[0], n[1], dim == 3 ? n[2] : -1}};
  std::sort(k.v, k.v + dim);
  return k;
}
/*--- Local outward face j of a simplex (opposite node j), positively oriented element. ---*/
inline void ElemFace(int dim, const i64* e, int j, i64* f) {
  if (dim == 2) {
    static const int L[3][2] = {{1, 2}, {2, 0}, {0, 1}};
    f[0] = e[L[j][0]]; f[1] = e[L[j][1]];
  } else {
    static const int L[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
    f[0] = e[L[j][0]]; f[1] = e[L[j][1]]; f[2] = e[L[j][2]];
  }
}
/*--- Parity of the permutation that sorts a face (orientation class). ---*/
inline int FaceParity(int dim, const i64* f) {
  int inv = 0;
  for (int a = 0; a < dim; ++a)
    for (int b = a + 1; b < dim; ++b)
      if (f[a] > f[b]) ++inv;
  return inv & 1;
}

/*--- Element-face adjacency of a mesh. ---*/
struct Adjacency {
  vector<i64> nb;   // ne*(dim+1): neighbour element across local face j, -1 = none
  vector<i64> fphys;// ne*(dim+1): physical face index on local face j, -1 = none
  vector<i64> v2eStart, v2e; // vertex -> elements (CSR)
  vector<i64> v2fStart, v2f; // vertex -> physical faces (CSR)
  void Build(const Mesh& m);
};

/*--- Small symmetric matrix helpers (dim 2 or 3), full 3x3 storage. ---*/
struct Sym {
  double a[3][3] = {{0}};
};
Sym FromUpper(int dim, const double* m);
void ToUpper(int dim, const Sym& s, double* m);
void Eig(int dim, const Sym& s, double* val, double vec[3][3]); // ascending, vec[:,i] columns -> vec[i] rows
Sym Recompose(int dim, const double* val, const double vec[3][3]);
Sym FuncSym(int dim, const Sym& s, double (*f)(double));
double Quad(int dim, const Sym& s, const double* v);
Sym LogEuclidMean(int dim, const vector<const double*>& mets);
Sym InvTensorMean(int dim, const vector<const double*>& mets, const double* w);
bool SPD(int dim, const double* m);
double EdgeLen(int dim, const double* xa, const double* xb, const double* ma, const double* mb);
/*--- Union of a metric with segments (the floor of CMMGInterface::FloorFixedBoundaryMetric). ---*/
void FloorWithEdges(int dim, double* metric, const vector<std::array<double, 3>>& edges);

double SignedVolume(int dim, const double* X, const i64* e);
double ElemQuality(int dim, const double* X, const i64* e, const Sym& Mk);      // mean ratio in Mk
double MinDihedralDeg(const double* X, const i64* e, const Sym& Mk);            // 3D, in Mk
Sym ElemMetric(const Mesh& m, i64 k);  // arithmetic mean of vertex tensors
double ElemComplexity(const Mesh& m, i64 k);  // |K| sqrt(det M_K)

/*--- Static k-d tree on points (k nearest neighbours). ---*/
struct KDTree {
  int dim = 0;
  vector<double> P;   // points
  vector<i64> idx;    // permutation
  vector<int> axis;   // per node (implicit balanced tree on idx ranges)
  void Build(int dim, const double* pts, i64 n);
  /*--- k nearest (Euclidean), sorted by distance; returns indices into the original point array ---*/
  void KNN(const double* x, int k, vector<std::pair<double, i64>>& out) const;
 private:
  void BuildRec(i64 lo, i64 hi, int depth);
  void Query(i64 lo, i64 hi, const double* x, int k, vector<std::pair<double, i64>>& heap) const;
};

/*--- Point locator on a simplex mesh: start at the element of the nearest centroid, then walk; inverse-tensor
 *    interpolation of the metric. ---*/
struct Locator {
  int dim = 0;
  const double* X = nullptr;
  const i64* T = nullptr;
  i64 ne = 0;
  KDTree kd;
  vector<i64> nbr;  // ne*(dim+1) neighbour across face j (opposite node j), -1 none
  void Build(int dim, const double* X, const i64* T, i64 ne);
  /*--- Element containing x (largest minimum barycentric weight among candidates), weights in lam. ---*/
  i64 Find(const double* x, double* lam) const;
};
bool InterpMetric(const Locator& loc, const double* Mv, int dim, const double* x, double* out);

/*--- I/O. ---*/
Mesh ReadB0In(const string& file, Params& prm);
void WriteB0In(const string& file, const Mesh& m, const Params& prm);
void WriteText(const string& file, const string& text);
void WriteVTU(const string& file, const Mesh& m, const vector<double>* cellData = nullptr, const char* name = nullptr);
uint64_t MeshHash(const Mesh& m);

/*--- Status of one stage of a call (plan 6.3/6.10). ---*/
enum class Stage { NONE = 0, PLANNING, ALLOCATION, MIGRATION, MMG, INTERPOLATION, VALIDATION, SPLICE, COMMIT };
const char* StageName(Stage s);
struct LStatus {
  int code = 0;  // 0 ok, 1 recoverable local failure, 2 fatal
  Stage stage = Stage::NONE;
  string msg;
};

/*--- One MMG call on a piece (plan 6.3). ---*/
struct PieceIn {
  vector<i64> elems;    // global element ids (sorted)
  i64 label = 0;        // smallest canonical element key hash (deterministic id)
  int vrank = 0;        // virtual rank
  int kind = 0;         // 0 coverage/level, 1 quality
  double predOut = 0;   // predicted output elements
};
struct PieceOut {
  LStatus st;
  int mmgStatus = -1;
  double tMMG = 0, tTotal = 0;
  i64 nInElem = 0, nOutElem = 0, nOutVert = 0, nArtFaces = 0;
  i64 nFreeVerts = -1, nFrozenVerts = -1;  // MMG input freedom, all required faces/corners
  // output in global terms: vertex id >= 0 retained global vertex index, < 0 = -(1 + new local index)
  vector<double> newX, newM;
  vector<i64> T;   // (dim+1) per element, coded vertex ids
  vector<int> Tref;
  vector<i64> F;   // physical faces, coded
  vector<int> Fref;
  vector<uint8_t> Fstate;
  vector<i64> removedFaces;  // global physical face ids replaced
  vector<i64> artFaceKeysGid;// artificial faces (gid keys, dim per face) for the record of former interfaces
  vector<i64> freedVerts;    // global vertices that were not artificially frozen in this call (CONSTRAINED cleared)
  vector<i64> frozenVerts;   // global vertices of artificial faces
  size_t bytesIn = 0, bytesOut = 0;
  double peakDeltaMB = 0;    // measured work peak of the call (process HWM delta)
  double capMB = -1;         // MMG allowance of the call
};

struct EngineOpts {
  bool floorArt = true;       // FLOOR (true) / NONE
  bool background = false;    // N5: metric of new vertices from the level-0 input (background)
  double mmgMemMB = -1;       // IPARAM_mem per call (-1: MMG default)
  double mmgMemFactor = 0;    // >0: per-call cap = factor * mmgBytesPerElem * (nIn + predOut), at least mmgMemMin
  double mmgBytesPerElem = 0, mmgMemMin = 0;
  int verbosity = -1;
  // failure injection (N1)
  Stage injectStage = Stage::NONE;
  int injectStep = -1;
  string injectPiece = "first";
  i64 injectLabel = 0;
  bool injectTarget = false;
};

struct Background {
  const Mesh* mesh = nullptr;
  Locator loc;
};

class Engine {
 public:
  Params prm;
  EngineOpts opt;
  int refArt = 0;
  double domainSize = 1.0;
  const Background* bg = nullptr;
  std::unordered_map<i64, std::array<double, 6>> physFloor;  // FIXED_SURFACE vertices (ADAP_SURFACE= NO), by gid
  /*--- Run one piece of the current mesh. step/vrank for failure injection. ---*/
  PieceOut AdaptPiece(const Mesh& m, const Adjacency& adj, const PieceIn& p, int step) const;
  /*--- Serial MMG on the whole mesh with the serial flags (reference). ---*/
  Mesh SerialRemesh(const Mesh& m, int& status, double& tMMG, double& peakMB) const;
};

/*--- Commit of a step: splice all accepted pieces. Returns the new mesh. ---*/
Mesh Splice(const Mesh& m, const Adjacency& adj, const vector<PieceIn>& pieces, const vector<PieceOut>& outs,
            const vector<char>& accepted, vector<i64>* newGidRange = nullptr);

/*--- Acceptance checks (gathered equivalents of plan 3.7). ---*/
struct CheckReport {
  bool ok = true;
  vector<string> errors;
  void Fail(const string& s) { ok = false; if (errors.size() < 20) errors.push_back(s); }
};
CheckReport AcceptanceChecks(const Mesh& m, const Mesh* fixedRef, const vector<i64>& frozenFaceGids,
                             const Mesh& before);

/*--- Memory probes. ---*/
double RSSMB();
double HWMMB();
void ResetHWM();
void TrimHeap();
double Now();

/*--- Misc. ---*/
vector<double> ElemCentroid(const Mesh& m, i64 k);
i64 CanonicalKeyHash(const Mesh& m, i64 k);
string Fmt(const char* f, ...);

}  // namespace b0
