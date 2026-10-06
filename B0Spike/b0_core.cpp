/*!
 * \file b0_core.cpp
 * \brief B0 spike: mesh utilities, metric math, locator, I/O, acceptance checks, memory probes.
 */
#include <malloc.h>
#include <sys/time.h>

#include <Eigen/Dense>
#include <cstdarg>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "b0.hpp"

namespace b0 {

string Fmt(const char* f, ...) {
  char buf[4096];
  va_list ap;
  va_start(ap, f);
  vsnprintf(buf, sizeof(buf), f, ap);
  va_end(ap);
  return buf;
}

double Now() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + 1e-6 * tv.tv_usec;
}

static double ReadStatusMB(const char* key) {
  std::ifstream in("/proc/self/status");
  string line;
  const size_t n = strlen(key);
  while (std::getline(in, line)) {
    if (line.compare(0, n, key) == 0) {
      std::istringstream ss(line.substr(n + 1));
      double kb = 0;
      ss >> kb;
      return kb / 1024.0;
    }
  }
  return -1;
}
double RSSMB() { return ReadStatusMB("VmRSS"); }
double HWMMB() { return ReadStatusMB("VmHWM"); }
void ResetHWM() {
  std::ofstream out("/proc/self/clear_refs");
  out << "5";
}
void TrimHeap() { malloc_trim(0); }

const char* StageName(Stage s) {
  switch (s) {
    case Stage::NONE: return "none";
    case Stage::PLANNING: return "planning";
    case Stage::ALLOCATION: return "allocation";
    case Stage::MIGRATION: return "migration";
    case Stage::MMG: return "mmg";
    case Stage::INTERPOLATION: return "interpolation";
    case Stage::VALIDATION: return "validation";
    case Stage::SPLICE: return "splice";
    case Stage::COMMIT: return "commit";
  }
  return "?";
}

/*----------------------------------------------------------------------------------------------------------------*/
/* Metric math                                                                                                    */
/*----------------------------------------------------------------------------------------------------------------*/

Sym FromUpper(int dim, const double* m) {
  Sym s;
  for (int i = 0, k = 0; i < dim; ++i)
    for (int j = i; j < dim; ++j, ++k) s.a[i][j] = s.a[j][i] = m[k];
  return s;
}
void ToUpper(int dim, const Sym& s, double* m) {
  for (int i = 0, k = 0; i < dim; ++i)
    for (int j = i; j < dim; ++j, ++k) m[k] = 0.5 * (s.a[i][j] + s.a[j][i]);
}
void Eig(int dim, const Sym& s, double* val, double vec[3][3]) {
  if (dim == 2) {
    Eigen::Matrix2d A;
    A << s.a[0][0], s.a[0][1], s.a[1][0], s.a[1][1];
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> es(A);
    for (int i = 0; i < 2; ++i) {
      val[i] = es.eigenvalues()(i);
      for (int d = 0; d < 2; ++d) vec[i][d] = es.eigenvectors()(d, i);
      vec[i][2] = 0;
    }
  } else {
    Eigen::Matrix3d A;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) A(i, j) = s.a[i][j];
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(A);
    for (int i = 0; i < 3; ++i) {
      val[i] = es.eigenvalues()(i);
      for (int d = 0; d < 3; ++d) vec[i][d] = es.eigenvectors()(d, i);
    }
  }
}
Sym Recompose(int dim, const double* val, const double vec[3][3]) {
  Sym s;
  for (int i = 0; i < dim; ++i)
    for (int j = 0; j < dim; ++j) {
      double v = 0;
      for (int k = 0; k < dim; ++k) v += vec[k][i] * val[k] * vec[k][j];
      s.a[i][j] = v;
    }
  return s;
}
Sym FuncSym(int dim, const Sym& s, double (*f)(double)) {
  double val[3], vec[3][3];
  Eig(dim, s, val, vec);
  for (int i = 0; i < dim; ++i) val[i] = f(val[i]);
  return Recompose(dim, val, vec);
}
double Quad(int dim, const Sym& s, const double* v) {
  double q = 0;
  for (int i = 0; i < dim; ++i)
    for (int j = 0; j < dim; ++j) q += v[i] * s.a[i][j] * v[j];
  return q;
}
Sym LogEuclidMean(int dim, const vector<const double*>& mets) {
  Sym acc;
  for (auto* m : mets) {
    const Sym l = FuncSym(dim, FromUpper(dim, m), [](double v) { return std::log(std::max(v, 1e-300)); });
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j) acc.a[i][j] += l.a[i][j] / mets.size();
  }
  return FuncSym(dim, acc, [](double v) { return std::exp(v); });
}
Sym InvTensorMean(int dim, const vector<const double*>& mets, const double* w) {
  Sym acc;
  for (size_t k = 0; k < mets.size(); ++k) {
    const Sym inv = FuncSym(dim, FromUpper(dim, mets[k]), [](double v) { return 1.0 / v; });
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j) acc.a[i][j] += w[k] * inv.a[i][j];
  }
  return FuncSym(dim, acc, [](double v) { return 1.0 / v; });
}
bool SPD(int dim, const double* m) {
  const int nm = dim * (dim + 1) / 2;
  for (int i = 0; i < nm; ++i)
    if (!std::isfinite(m[i])) return false;
  Sym s = FromUpper(dim, m);
  double sc[3];
  for (int i = 0; i < dim; ++i) {
    if (!(s.a[i][i] > 0)) return false;
    sc[i] = 1.0 / std::sqrt(s.a[i][i]);
  }
  for (int i = 0; i < dim; ++i)
    for (int j = 0; j < dim; ++j) s.a[i][j] *= sc[i] * sc[j];
  double val[3], vec[3][3];
  Eig(dim, s, val, vec);
  return val[0] > 1e-14 * val[dim - 1];
}
double EdgeLen(int dim, const double* xa, const double* xb, const double* ma, const double* mb) {
  double e[3] = {0, 0, 0};
  for (int d = 0; d < dim; ++d) e[d] = xb[d] - xa[d];
  const double la = std::sqrt(std::max(0.0, Quad(dim, FromUpper(dim, ma), e)));
  const double lb = std::sqrt(std::max(0.0, Quad(dim, FromUpper(dim, mb), e)));
  if (std::fabs(la - lb) < 1e-6 * std::max(la, lb)) return 0.5 * (la + lb);
  return (la - lb) / std::log(la / lb);
}
void FloorWithEdges(int dim, double* metric, const vector<std::array<double, 3>>& edges) {
  Sym M = FromUpper(dim, metric);
  bool changed = false;
  for (const auto& e : edges) {
    if (Quad(dim, M, e.data()) <= 1.0) continue;
    const Sym S = FuncSym(dim, M, [](double v) { return 1.0 / v; });
    const Sym half = FuncSym(dim, S, [](double v) { return std::sqrt(v); });
    const Sym invHalf = FuncSym(dim, S, [](double v) { return 1.0 / std::sqrt(v); });
    Sym E, tmp, T, Snew;
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j) E.a[i][j] = e[i] * e[j];
    auto prod = [dim](const Sym& A, const Sym& B) {
      Sym C;
      for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j) {
          double v = 0;
          for (int k = 0; k < dim; ++k) v += A.a[i][k] * B.a[k][j];
          C.a[i][j] = v;
        }
      return C;
    };
    T = prod(prod(invHalf, E), invHalf);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < i; ++j) T.a[i][j] = T.a[j][i] = 0.5 * (T.a[i][j] + T.a[j][i]);
    T = FuncSym(dim, T, [](double v) { return std::max(v, 1.0); });
    Snew = prod(prod(half, T), half);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < i; ++j) Snew.a[i][j] = Snew.a[j][i] = 0.5 * (Snew.a[i][j] + Snew.a[j][i]);
    M = FuncSym(dim, Snew, [](double v) { return 1.0 / v; });
    changed = true;
  }
  if (changed) ToUpper(dim, M, metric);
}

double SignedVolume(int dim, const double* X, const i64* e) {
  if (dim == 2) {
    const double* a = X + 2 * e[0];
    const double* b = X + 2 * e[1];
    const double* c = X + 2 * e[2];
    return 0.5 * ((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]));
  }
  const double* a = X + 3 * e[0];
  const double* b = X + 3 * e[1];
  const double* c = X + 3 * e[2];
  const double* d = X + 3 * e[3];
  const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
  const double v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
  const double w[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
  return (u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) + u[2] * (v[0] * w[1] - v[1] * w[0])) /
         6.0;
}

double ElemQuality(int dim, const double* X, const i64* e, const Sym& Mk) {
  double det;
  if (dim == 2)
    det = Mk.a[0][0] * Mk.a[1][1] - Mk.a[0][1] * Mk.a[1][0];
  else
    det = Mk.a[0][0] * (Mk.a[1][1] * Mk.a[2][2] - Mk.a[1][2] * Mk.a[2][1]) -
          Mk.a[0][1] * (Mk.a[1][0] * Mk.a[2][2] - Mk.a[1][2] * Mk.a[2][0]) +
          Mk.a[0][2] * (Mk.a[1][0] * Mk.a[2][1] - Mk.a[1][1] * Mk.a[2][0]);
  const double vol = SignedVolume(dim, X, e) * std::sqrt(std::max(det, 0.0));
  double sum = 0;
  const int nn = dim + 1;
  for (int a = 0; a < nn; ++a)
    for (int b = a + 1; b < nn; ++b) {
      double v[3] = {0, 0, 0};
      for (int d = 0; d < dim; ++d) v[d] = X[dim * e[b] + d] - X[dim * e[a] + d];
      sum += Quad(dim, Mk, v);
    }
  if (sum <= 0) return 0;
  if (dim == 2) return 4.0 * std::sqrt(3.0) * vol / sum;
  return vol <= 0 ? vol : 12.0 * std::pow(3.0 * vol, 2.0 / 3.0) / sum;
}

double MinDihedralDeg(const double* X, const i64* e, const Sym& Mk) {
  /*--- Map to the metric space with L^T, M = L L^T (Cholesky). ---*/
  Eigen::Matrix3d A;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) A(i, j) = Mk.a[i][j];
  Eigen::LLT<Eigen::Matrix3d> llt(A);
  const Eigen::Matrix3d L = llt.matrixL();
  Eigen::Vector3d p[4];
  for (int a = 0; a < 4; ++a) p[a] = L.transpose() * Eigen::Vector3d(X[3 * e[a]], X[3 * e[a] + 1], X[3 * e[a] + 2]);
  /*--- Face normals (outward), dihedral angle at edge (a,b) = pi - angle between the normals of the two faces. ---*/
  Eigen::Vector3d n[4];
  static const int L4[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
  for (int f = 0; f < 4; ++f) {
    n[f] = (p[L4[f][1]] - p[L4[f][0]]).cross(p[L4[f][2]] - p[L4[f][0]]);
    const double nn = n[f].norm();
    if (nn > 0) n[f] /= nn;
  }
  double minA = 180.0;
  for (int f = 0; f < 4; ++f)
    for (int g = f + 1; g < 4; ++g) {
      const double c = std::max(-1.0, std::min(1.0, -n[f].dot(n[g])));
      minA = std::min(minA, std::acos(c) * 180.0 / M_PI);
    }
  return minA;
}

Sym ElemMetric(const Mesh& m, i64 k) {
  Sym s;
  const int nn = m.nn(), nm = m.nm(), dim = m.dim;
  for (int a = 0; a < nn; ++a) {
    const Sym v = FromUpper(dim, &m.M[nm * m.T[nn * k + a]]);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j) s.a[i][j] += v.a[i][j] / nn;
  }
  return s;
}

double ElemComplexity(const Mesh& m, i64 k) {
  const Sym s = ElemMetric(m, k);
  const int dim = m.dim;
  double det;
  if (dim == 2)
    det = s.a[0][0] * s.a[1][1] - s.a[0][1] * s.a[1][0];
  else
    det = s.a[0][0] * (s.a[1][1] * s.a[2][2] - s.a[1][2] * s.a[2][1]) -
          s.a[0][1] * (s.a[1][0] * s.a[2][2] - s.a[1][2] * s.a[2][0]) +
          s.a[0][2] * (s.a[1][0] * s.a[2][1] - s.a[1][1] * s.a[2][0]);
  return std::fabs(SignedVolume(dim, m.X.data(), &m.T[m.nn() * k])) * std::sqrt(std::max(det, 0.0));
}

vector<double> ElemCentroid(const Mesh& m, i64 k) {
  vector<double> c(m.dim, 0.0);
  for (int a = 0; a < m.nn(); ++a)
    for (int d = 0; d < m.dim; ++d) c[d] += m.X[m.dim * m.T[m.nn() * k + a] + d] / m.nn();
  return c;
}

i64 CanonicalKeyHash(const Mesh& m, i64 k) {
  i64 g[4] = {0, 0, 0, 0};
  for (int a = 0; a < m.nn(); ++a) g[a] = m.gid[m.T[m.nn() * k + a]];
  std::sort(g, g + m.nn());
  uint64_t h = 1469598103934665603ull;
  for (int a = 0; a < m.nn(); ++a) {
    const uint64_t gid = g[a];
    for (int b = 0; b < 8; ++b) { h ^= (gid >> (8 * b)) & 255; h *= 1099511628211ull; }
  }
  return (i64)(h & 0x7fffffffffffffffull);
}

/*----------------------------------------------------------------------------------------------------------------*/
/* Adjacency                                                                                                      */
/*----------------------------------------------------------------------------------------------------------------*/

void Adjacency::Build(const Mesh& m) {
  const int dim = m.dim, nn = m.nn();
  const i64 ne = m.ne(), nv = m.nv();
  struct Item {
    FKey k;
    i64 id;  // elem*nn + j, or -(1+faceIndex) for physical faces
  };
  vector<Item> items;
  items.reserve(ne * nn + m.nf());
  i64 f[3];
  for (i64 e = 0; e < ne; ++e)
    for (int j = 0; j < nn; ++j) {
      ElemFace(dim, &m.T[nn * e], j, f);
      items.push_back({MakeKey(dim, f), e * nn + j});
    }
  for (i64 i = 0; i < m.nf(); ++i) items.push_back({MakeKey(dim, &m.F[dim * i]), -(1 + i)});
  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
    if (a.k != b.k) return a.k < b.k;
    return a.id < b.id;
  });
  nb.assign(ne * nn, -1);
  fphys.assign(ne * nn, -1);
  for (size_t a = 0; a < items.size();) {
    size_t b = a;
    while (b < items.size() && items[b].k == items[a].k) ++b;
    vector<i64> el, ph;
    for (size_t c = a; c < b; ++c) (items[c].id >= 0 ? el : ph).push_back(items[c].id);
    if (el.size() == 2) {
      nb[el[0]] = el[1] / nn;
      nb[el[1]] = el[0] / nn;
    }
    if (!ph.empty() && el.size() >= 1) fphys[el[0]] = -(ph[0] + 1);
    a = b;
  }
  v2fStart.assign(nv + 1, 0);
  for (i64 i = 0; i < m.nf(); ++i)
    for (int a = 0; a < dim; ++a) ++v2fStart[m.F[dim * i + a] + 1];
  for (i64 v = 0; v < nv; ++v) v2fStart[v + 1] += v2fStart[v];
  v2f.resize(v2fStart[nv]);
  {
    vector<i64> pos(v2fStart.begin(), v2fStart.end() - 1);
    for (i64 i = 0; i < m.nf(); ++i)
      for (int a = 0; a < dim; ++a) v2f[pos[m.F[dim * i + a]]++] = i;
  }
  v2eStart.assign(nv + 1, 0);
  for (i64 e = 0; e < ne; ++e)
    for (int j = 0; j < nn; ++j) ++v2eStart[m.T[nn * e + j] + 1];
  for (i64 v = 0; v < nv; ++v) v2eStart[v + 1] += v2eStart[v];
  v2e.resize(v2eStart[nv]);
  vector<i64> pos(v2eStart.begin(), v2eStart.end() - 1);
  for (i64 e = 0; e < ne; ++e)
    for (int j = 0; j < nn; ++j) v2e[pos[m.T[nn * e + j]]++] = e;
}

/*----------------------------------------------------------------------------------------------------------------*/
/* Locator                                                                                                        */
/*----------------------------------------------------------------------------------------------------------------*/

static void Bary(int dim, const double* X, const i64* e, const double* x, double* lam) {
  if (dim == 2) {
    const double* a = X + 2 * e[0];
    const double* b = X + 2 * e[1];
    const double* c = X + 2 * e[2];
    const double det = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    lam[1] = ((x[0] - a[0]) * (c[1] - a[1]) - (x[1] - a[1]) * (c[0] - a[0])) / det;
    lam[2] = ((b[0] - a[0]) * (x[1] - a[1]) - (b[1] - a[1]) * (x[0] - a[0])) / det;
    lam[0] = 1 - lam[1] - lam[2];
  } else {
    Eigen::Matrix3d A;
    for (int c = 0; c < 3; ++c)
      for (int d = 0; d < 3; ++d) A(d, c) = X[3 * e[c + 1] + d] - X[3 * e[0] + d];
    const Eigen::Vector3d r(x[0] - X[3 * e[0]], x[1] - X[3 * e[0] + 1], x[2] - X[3 * e[0] + 2]);
    const Eigen::Vector3d s = A.partialPivLu().solve(r);
    lam[1] = s(0);
    lam[2] = s(1);
    lam[3] = s(2);
    lam[0] = 1 - s(0) - s(1) - s(2);
  }
}

void KDTree::Build(int d, const double* pts, i64 n) {
  dim = d;
  P.assign(pts, pts + n * d);
  idx.resize(n);
  std::iota(idx.begin(), idx.end(), 0);
  axis.assign(std::max<i64>(n, 1), 0);
  if (n) BuildRec(0, n, 0);
}
void KDTree::BuildRec(i64 lo, i64 hi, int depth) {
  if (hi - lo <= 8) return;
  /*--- split along the widest extent ---*/
  double mn[3] = {1e300, 1e300, 1e300}, mx[3] = {-1e300, -1e300, -1e300};
  for (i64 i = lo; i < hi; ++i)
    for (int c = 0; c < dim; ++c) {
      mn[c] = std::min(mn[c], P[dim * idx[i] + c]);
      mx[c] = std::max(mx[c], P[dim * idx[i] + c]);
    }
  int ax = 0;
  for (int c = 1; c < dim; ++c)
    if (mx[c] - mn[c] > mx[ax] - mn[ax]) ax = c;
  const i64 mid = (lo + hi) / 2;
  std::nth_element(idx.begin() + lo, idx.begin() + mid, idx.begin() + hi,
                   [&](i64 a, i64 b) { return P[dim * a + ax] < P[dim * b + ax]; });
  axis[mid] = ax;
  BuildRec(lo, mid, depth + 1);
  BuildRec(mid + 1, hi, depth + 1);
}
void KDTree::Query(i64 lo, i64 hi, const double* x, int k, vector<std::pair<double, i64>>& heap) const {
  if (hi - lo <= 8) {
    for (i64 i = lo; i < hi; ++i) {
      double d2 = 0;
      for (int c = 0; c < dim; ++c) d2 += std::pow(P[dim * idx[i] + c] - x[c], 2);
      if ((int)heap.size() < k) {
        heap.push_back({d2, idx[i]});
        std::push_heap(heap.begin(), heap.end());
      } else if (d2 < heap.front().first) {
        std::pop_heap(heap.begin(), heap.end());
        heap.back() = {d2, idx[i]};
        std::push_heap(heap.begin(), heap.end());
      }
    }
    return;
  }
  const i64 mid = (lo + hi) / 2;
  const int ax = axis[mid];
  const double diff = x[ax] - P[dim * idx[mid] + ax];
  {
    double d2 = 0;
    for (int c = 0; c < dim; ++c) d2 += std::pow(P[dim * idx[mid] + c] - x[c], 2);
    if ((int)heap.size() < k) {
      heap.push_back({d2, idx[mid]});
      std::push_heap(heap.begin(), heap.end());
    } else if (d2 < heap.front().first) {
      std::pop_heap(heap.begin(), heap.end());
      heap.back() = {d2, idx[mid]};
      std::push_heap(heap.begin(), heap.end());
    }
  }
  if (diff < 0) {
    Query(lo, mid, x, k, heap);
    if ((int)heap.size() < k || diff * diff < heap.front().first) Query(mid + 1, hi, x, k, heap);
  } else {
    Query(mid + 1, hi, x, k, heap);
    if ((int)heap.size() < k || diff * diff < heap.front().first) Query(lo, mid, x, k, heap);
  }
}
void KDTree::KNN(const double* x, int k, vector<std::pair<double, i64>>& out) const {
  out.clear();
  if (idx.empty()) return;
  Query(0, idx.size(), x, k, out);
  std::sort_heap(out.begin(), out.end());
}

void Locator::Build(int dim_, const double* X_, const i64* T_, i64 ne_) {
  dim = dim_;
  X = X_;
  T = T_;
  ne = ne_;
  const int nn = dim + 1;
  vector<double> cen(ne * dim, 0.0);
  for (i64 e = 0; e < ne; ++e)
    for (int a = 0; a < nn; ++a)
      for (int d = 0; d < dim; ++d) cen[dim * e + d] += X[dim * T[nn * e + a] + d] / nn;
  kd.Build(dim, cen.data(), ne);
  /*--- neighbours across faces (face j opposite node j) ---*/
  struct It {
    FKey k;
    i64 id;
  };
  vector<It> it;
  it.reserve(ne * nn);
  i64 f[3];
  for (i64 e = 0; e < ne; ++e)
    for (int j = 0; j < nn; ++j) {
      int q = 0;
      for (int a = 0; a < nn; ++a)
        if (a != j) f[q++] = T[nn * e + a];
      it.push_back({MakeKey(dim, f), e * nn + j});
    }
  std::sort(it.begin(), it.end(), [](const It& a, const It& b) { return a.k != b.k ? a.k < b.k : a.id < b.id; });
  nbr.assign(ne * nn, -1);
  for (size_t a = 0; a + 1 < it.size(); ++a)
    if (it[a].k == it[a + 1].k) {
      nbr[it[a].id] = it[a + 1].id / nn;
      nbr[it[a + 1].id] = it[a].id / nn;
    }
}

i64 Locator::Find(const double* x, double* lam) const {
  if (ne == 0) return -1;
  const int nn = dim + 1;
  vector<std::pair<double, i64>> cand;
  kd.KNN(x, 8, cand);
  i64 best = -1;
  double bestMin = -1e300, l[4];
  auto test = [&](i64 e) {
    Bary(dim, X, T + nn * e, x, l);
    double mn = l[0];
    for (int a = 1; a <= dim; ++a) mn = std::min(mn, l[a]);
    if (mn > bestMin || (mn == bestMin && e < best)) {
      bestMin = mn;
      best = e;
      for (int a = 0; a <= dim; ++a) lam[a] = l[a];
    }
    return mn;
  };
  for (auto& c : cand) test(c.second);
  if (bestMin >= -1e-12) return best;
  /*--- walk from the best candidate towards x across the face of the most negative weight ---*/
  i64 e = best;
  for (int step = 0; step < 4000; ++step) {
    Bary(dim, X, T + nn * e, x, l);
    int jm = 0;
    for (int a = 1; a <= dim; ++a)
      if (l[a] < l[jm]) jm = a;
    if (l[jm] >= -1e-12) {
      test(e);
      break;
    }
    const i64 nx = nbr[nn * e + jm];
    test(e);
    if (nx < 0) break;  // boundary reached: x is outside (curved boundary) or the domain is not convex here
    e = nx;
  }
  if (bestMin >= -1e-12) return best;
  /*--- fallback: more candidates (non-convex boundaries, strongly stretched elements) ---*/
  kd.KNN(x, 64, cand);
  for (auto& c : cand) test(c.second);
  return best;
}

bool InterpMetric(const Locator& loc, const double* Mv, int dim, const double* x, double* out) {
  double lam[4];
  const i64 e = loc.Find(x, lam);
  if (e < 0) return false;
  const int nn = dim + 1, nm = dim * (dim + 1) / 2;
  /*--- Outside points (curved boundary): weights clamped to the element and renormalised. ---*/
  double s = 0;
  for (int a = 0; a < nn; ++a) {
    lam[a] = std::max(0.0, lam[a]);
    s += lam[a];
  }
  for (int a = 0; a < nn; ++a) lam[a] /= s;
  vector<const double*> mets(nn);
  for (int a = 0; a < nn; ++a) mets[a] = Mv + nm * loc.T[nn * e + a];
  const Sym r = InvTensorMean(dim, mets, lam);
  ToUpper(dim, r, out);
  return SPD(dim, out);
}

/*----------------------------------------------------------------------------------------------------------------*/
/* I/O                                                                                                            */
/*----------------------------------------------------------------------------------------------------------------*/

Mesh ReadB0In(const string& file, Params& prm) {
  std::ifstream in(file, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + file);
  auto get = [&in](void* p, size_t n) { in.read(static_cast<char*>(p), n); };
  char magic[8];
  get(magic, 8);
  if (string(magic, 7) != "B0MESH1") throw std::runtime_error("bad magic in " + file);
  uint64_t head[4];
  get(head, sizeof(head));
  double dprm[5];
  get(dprm, sizeof(dprm));
  int32_t iprm[2];
  get(iprm, sizeof(iprm));
  prm.hmin = dprm[0];
  prm.hmax = dprm[1];
  prm.hgrad = dprm[2];
  prm.hausd = dprm[3];
  prm.angle = dprm[4];
  prm.surface = iprm[0];
  prm.bl = iprm[1];
  Mesh m;
  m.dim = head[0];
  const i64 nv = head[1], ne = head[2], nmk = head[3];
  m.X.resize(nv * m.dim);
  get(m.X.data(), m.X.size() * 8);
  m.M.resize(nv * m.nm());
  get(m.M.data(), m.M.size() * 8);
  vector<uint64_t> e(ne * m.nn());
  get(e.data(), e.size() * 8);
  m.T.assign(e.begin(), e.end());
  vector<int32_t> er(ne);
  get(er.data(), er.size() * 4);
  m.Tref.assign(er.begin(), er.end());
  for (i64 k = 0; k < nmk; ++k) {
    uint64_t mh[3];
    get(mh, sizeof(mh));
    string name(mh[0], ' ');
    get(&name[0], mh[0]);
    vector<uint64_t> me(mh[2] * m.dim);
    get(me.data(), me.size() * 8);
    m.markerName.push_back(name);
    m.markerRef.push_back((int)mh[1]);
    for (auto v : me) m.F.push_back((i64)v);
    for (uint64_t f = 0; f < mh[2]; ++f) m.Fref.push_back((int)mh[1]);
  }
  if (!in) throw std::runtime_error("truncated " + file);
  m.gid.resize(nv);
  std::iota(m.gid.begin(), m.gid.end(), 0);
  m.gidEnd = nv;
  m.vflag.assign(nv, 0);
  m.Fstate.assign(m.nf(), prm.surface ? F_UNTOUCHED : F_FIXED);
  return m;
}

void WriteB0In(const string& file, const Mesh& m, const Params& prm) {
  std::ofstream out(file, std::ios::binary);
  auto put = [&out](const void* p, size_t n) { out.write(static_cast<const char*>(p), n); };
  const char magic[8] = {'B', '0', 'M', 'E', 'S', 'H', '1', 0};
  put(magic, 8);
  const uint64_t head[4] = {(uint64_t)m.dim, (uint64_t)m.nv(), (uint64_t)m.ne(), m.markerName.size()};
  put(head, sizeof(head));
  const double dprm[5] = {prm.hmin, prm.hmax, prm.hgrad, prm.hausd, prm.angle};
  put(dprm, sizeof(dprm));
  const int32_t iprm[2] = {prm.surface, prm.bl};
  put(iprm, sizeof(iprm));
  put(m.X.data(), m.X.size() * 8);
  put(m.M.data(), m.M.size() * 8);
  vector<uint64_t> e(m.T.begin(), m.T.end());
  put(e.data(), e.size() * 8);
  vector<int32_t> er(m.Tref.begin(), m.Tref.end());
  put(er.data(), er.size() * 4);
  for (size_t k = 0; k < m.markerName.size(); ++k) {
    vector<uint64_t> me;
    for (i64 f = 0; f < m.nf(); ++f)
      if (m.Fref[f] == m.markerRef[k])
        for (int a = 0; a < m.dim; ++a) me.push_back(m.F[m.dim * f + a]);
    const uint64_t mh[3] = {m.markerName[k].size(), (uint64_t)m.markerRef[k], me.size() / m.dim};
    put(mh, sizeof(mh));
    put(m.markerName[k].data(), m.markerName[k].size());
    put(me.data(), me.size() * 8);
  }
  out.close();
  if (!out) throw std::runtime_error("cannot write mesh " + file);
}

void WriteText(const string& file, const string& text) {
  std::ofstream out(file);
  out << text;
  out.close();
  if (!out) throw std::runtime_error("cannot write " + file);
}

void WriteVTU(const string& file, const Mesh& m, const vector<double>* cellData, const char* name) {
  std::ofstream o(file);
  o << "<?xml version=\"1.0\"?>\n<VTKFile type=\"UnstructuredGrid\" version=\"0.1\">\n<UnstructuredGrid>\n";
  o << "<Piece NumberOfPoints=\"" << m.nv() << "\" NumberOfCells=\"" << m.ne() << "\">\n";
  o << "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
  for (i64 v = 0; v < m.nv(); ++v)
    o << m.X[m.dim * v] << " " << m.X[m.dim * v + 1] << " " << (m.dim == 3 ? m.X[3 * v + 2] : 0.0) << "\n";
  o << "</DataArray></Points>\n<Cells><DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n";
  for (i64 k = 0; k < m.ne(); ++k) {
    for (int a = 0; a < m.nn(); ++a) o << m.T[m.nn() * k + a] << " ";
    o << "\n";
  }
  o << "</DataArray><DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n";
  for (i64 k = 0; k < m.ne(); ++k) o << (k + 1) * m.nn() << "\n";
  o << "</DataArray><DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
  for (i64 k = 0; k < m.ne(); ++k) o << (m.dim == 2 ? 5 : 10) << "\n";
  o << "</DataArray></Cells>\n";
  if (cellData) {
    o << "<CellData><DataArray type=\"Float64\" Name=\"" << (name ? name : "data") << "\" format=\"ascii\">\n";
    for (double v : *cellData) o << v << "\n";
    o << "</DataArray></CellData>\n";
  }
  o << "</Piece>\n</UnstructuredGrid>\n</VTKFile>\n";
  o.close();
  if (!o) throw std::runtime_error("cannot write VTU " + file);
}

uint64_t MeshHash(const Mesh& m) {
  uint64_t h = 1469598103934665603ull;
  auto mix = [&h](const void* p, size_t n) {
    const auto* c = static_cast<const unsigned char*>(p);
    for (size_t i = 0; i < n; ++i) { h ^= c[i]; h *= 1099511628211ull; }
  };
  auto scalar = [&](const auto& v) { mix(&v, sizeof(v)); };
  scalar(m.dim); scalar(m.gidEnd);
  const i64 nv = m.nv(), ne = m.ne(), nf = m.nf();
  scalar(nv); scalar(ne); scalar(nf);
  vector<i64> order(nv);
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](i64 a, i64 b) { return m.gid[a] < m.gid[b]; });
  for (i64 v : order) {
    scalar(m.gid[v]); scalar(m.vflag[v]);
    mix(&m.X[m.dim * v], m.dim * sizeof(double));
    mix(&m.M[m.nm() * v], m.nm() * sizeof(double));
  }
  vector<vector<i64>> records;
  for (i64 k = 0; k < ne; ++k) {
    vector<i64> r;
    for (int a = 0; a < m.nn(); ++a) r.push_back(m.gid[m.T[m.nn() * k + a]]);
    std::sort(r.begin(), r.end()); r.push_back(m.Tref[k]); records.push_back(r);
  }
  std::sort(records.begin(), records.end());
  for (const auto& r : records) mix(r.data(), r.size() * sizeof(i64));
  records.clear();
  for (i64 f = 0; f < nf; ++f) {
    vector<i64> r;
    for (int a = 0; a < m.dim; ++a) r.push_back(m.gid[m.F[m.dim * f + a]]);
    std::sort(r.begin(), r.end()); r.push_back(m.Fref[f]); r.push_back(m.Fstate[f]); records.push_back(r);
  }
  std::sort(records.begin(), records.end());
  for (const auto& r : records) mix(r.data(), r.size() * sizeof(i64));
  vector<std::pair<int, string>> markers;
  for (size_t i = 0; i < m.markerRef.size(); ++i) markers.push_back({m.markerRef[i], m.markerName[i]});
  std::sort(markers.begin(), markers.end());
  for (const auto& mk : markers) { scalar(mk.first); const uint64_t n = mk.second.size(); scalar(n); mix(mk.second.data(), n); }
  return h;
}

/*----------------------------------------------------------------------------------------------------------------*/
/* Acceptance checks (gathered equivalents of plan 3.7)                                                          */
/*----------------------------------------------------------------------------------------------------------------*/

CheckReport AcceptanceChecks(const Mesh& m, const Mesh* fixedRef, const vector<i64>& frozenFaceGids,
                             const Mesh& before) {
  CheckReport r;
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  const i64 nv = m.nv(), ne = m.ne();
  /*--- geometry: nodes in range, finite coordinates, positive volumes, every vertex used ---*/
  vector<char> used(nv, 0);
  i64 nBadVol = 0, nRange = 0;
  for (i64 k = 0; k < ne; ++k) {
    bool ok = true;
    for (int a = 0; a < nn; ++a) {
      const i64 v = m.T[nn * k + a];
      if (v < 0 || v >= nv) ok = false;
      else used[v] = 1;
    }
    if (!ok) { ++nRange; continue; }
    if (!(SignedVolume(dim, m.X.data(), &m.T[nn * k]) > 0)) ++nBadVol;
  }
  if (nRange) r.Fail(Fmt("geometry: %ld elements with nodes out of range", (long)nRange));
  if (nBadVol) r.Fail(Fmt("geometry: %ld elements with non-positive volume", (long)nBadVol));
  i64 nUnused = 0;
  for (i64 v = 0; v < nv; ++v) nUnused += !used[v];
  if (nUnused) r.Fail(Fmt("geometry: %ld unused vertices", (long)nUnused));
  for (double x : m.X)
    if (!std::isfinite(x)) { r.Fail("geometry: non-finite coordinate"); break; }
  /*--- metric SPD ---*/
  i64 nBadMet = 0;
  for (i64 v = 0; v < nv; ++v) nBadMet += !SPD(dim, &m.M[nm * v]);
  if (nBadMet) r.Fail(Fmt("metric: %ld vertices with a non-SPD metric", (long)nBadMet));
  /*--- ownership/numbering: gids unique ---*/
  {
    vector<i64> g(m.gid);
    std::sort(g.begin(), g.end());
    if (std::adjacent_find(g.begin(), g.end()) != g.end()) r.Fail("numbering: duplicate gid");
    if (!g.empty() && g.back() >= m.gidEnd) r.Fail("numbering: gid beyond gidEnd");
  }
  if (nRange) return r;
  /*--- element multiplicity, face orientation, boundary classification ---*/
  struct It { FKey k; int par; i64 id; };
  vector<It> it;
  it.reserve(ne * nn + m.nf());
  i64 f[3];
  for (i64 k = 0; k < ne; ++k)
    for (int j = 0; j < nn; ++j) {
      ElemFace(dim, &m.T[nn * k], j, f);
      it.push_back({MakeKey(dim, f), FaceParity(dim, f), k});
    }
  for (i64 i = 0; i < m.nf(); ++i) it.push_back({MakeKey(dim, &m.F[dim * i]), 2, -(1 + i)});
  std::sort(it.begin(), it.end(), [](const It& a, const It& b) { return a.k != b.k ? a.k < b.k : a.id < b.id; });
  i64 nMult = 0, nOrient = 0, nUncl = 0, nPhys2 = 0, nPhysNoElem = 0, nPhysMulti = 0;
  for (size_t a = 0; a < it.size();) {
    size_t b = a;
    int nE = 0, nP = 0, par[2] = {0, 0};
    while (b < it.size() && it[b].k == it[a].k) {
      if (it[b].id >= 0) { if (nE < 2) par[nE] = it[b].par; ++nE; } else ++nP;
      ++b;
    }
    if (nE > 2) ++nMult;
    if (nE == 2 && par[0] == par[1]) ++nOrient;
    if (nE == 1 && nP == 0) ++nUncl;
    if (nE == 2 && nP > 0) ++nPhys2;
    if (nE == 0 && nP > 0) ++nPhysNoElem;
    if (nP > 1) ++nPhysMulti;
    a = b;
  }
  if (nMult) r.Fail(Fmt("multiplicity: %ld faces with more than two elements", (long)nMult));
  if (nOrient) r.Fail(Fmt("orientation: %ld shared faces with equal orientation", (long)nOrient));
  if (nUncl) r.Fail(Fmt("classification: %ld boundary faces without a marker", (long)nUncl));
  if (nPhys2) r.Fail(Fmt("classification: %ld physical faces between two elements", (long)nPhys2));
  if (nPhysNoElem) r.Fail(Fmt("classification: %ld physical faces without an element", (long)nPhysNoElem));
  if (nPhysMulti) r.Fail(Fmt("classification: %ld faces in more than one marker", (long)nPhysMulti));
  /*--- markers: every marker with faces before still has faces ---*/
  for (int ref : before.markerRef) {
    const bool had = std::count(before.Fref.begin(), before.Fref.end(), ref) > 0;
    const bool has = std::count(m.Fref.begin(), m.Fref.end(), ref) > 0;
    if (had && !has) r.Fail(Fmt("markers: marker %d lost its faces", ref));
  }
  /*--- frozen-entity identity: faces of the step's artificial boundaries exist, vertices bitwise ---*/
  if (!frozenFaceGids.empty()) {
    std::unordered_map<i64, i64> g2v;
    for (i64 v = 0; v < nv; ++v) g2v[m.gid[v]] = v;
    std::unordered_map<i64, i64> g2vOld;
    for (i64 v = 0; v < before.nv(); ++v) g2vOld[before.gid[v]] = v;
    std::set<FKey> faces;
    for (size_t a = 0; a < it.size(); ++a)
      if (it[a].id >= 0) faces.insert(it[a].k);
    i64 nMiss = 0, nMoved = 0;
    for (size_t q = 0; q + dim <= frozenFaceGids.size(); q += dim) {
      i64 lv[3] = {-1, -1, -1};
      bool ok = true;
      for (int a = 0; a < dim; ++a) {
        auto itv = g2v.find(frozenFaceGids[q + a]);
        if (itv == g2v.end()) { ok = false; break; }
        lv[a] = itv->second;
        const auto ito = g2vOld.find(frozenFaceGids[q + a]);
        if (ito != g2vOld.end() &&
            memcmp(&m.X[dim * lv[a]], &before.X[dim * ito->second], dim * sizeof(double)) != 0)
          ++nMoved;
      }
      if (!ok || !faces.count(MakeKey(dim, lv))) ++nMiss;
    }
    if (nMiss) r.Fail(Fmt("frozen identity: %ld artificial faces missing after the splice", (long)nMiss));
    if (nMoved) r.Fail(Fmt("frozen identity: %ld frozen vertices moved", (long)nMoved));
  }
  /*--- fixed surface bitwise (ADAP_SURFACE= NO) ---*/
  if (fixedRef) {
    auto bset = [](const Mesh& q) {
      std::multiset<std::pair<int, vector<double>>> s;
      for (i64 i = 0; i < q.nf(); ++i) {
        vector<std::array<double, 3>> pts;
        for (int a = 0; a < q.dim; ++a) {
          std::array<double, 3> p = {0, 0, 0};
          for (int d = 0; d < q.dim; ++d) p[d] = q.X[q.dim * q.F[q.dim * i + a] + d];
          pts.push_back(p);
        }
        std::sort(pts.begin(), pts.end());
        vector<double> flat;
        for (auto& p : pts) flat.insert(flat.end(), p.begin(), p.end());
        s.insert({q.Fref[i], flat});
      }
      return s;
    };
    if (bset(m) != bset(*fixedRef)) r.Fail("fixed surface: boundary faces or coordinates changed");
  }
  return r;
}

}  // namespace b0
