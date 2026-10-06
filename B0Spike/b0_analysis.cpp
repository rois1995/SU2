/*!
 * \file b0_analysis.cpp
 * \brief B0 spike: gate statistics, matched regions, degradation tails, macro-orientation, classifier.
 */
#include "b0_analysis.hpp"

#include <Eigen/Dense>
#include <cstring>
#include <queue>

namespace b0 {

int DistBin(double d) {
  if (d < 1) return 0;
  if (d < 2) return 1;
  if (d < 4) return 2;
  if (d < 8) return 3;
  if (d < 16) return 4;
  return 5;
}
int ARClass(double logAR) { return logAR < 1 ? 0 : logAR < 2 ? 1 : logAR < 3 ? 2 : 3; }

static double LogAR(int dim, const Sym& M) {
  double val[3], vec[3][3];
  Eig(dim, M, val, vec);
  return 0.5 * std::log10(std::max(val[dim - 1], 1e-300) / std::max(val[0], 1e-300));
}

/*----------------------------------------------------------------------------------------------------------------*/

void FormerFaces::AddCoords(int d, const double* c, i64 nf) {
  dim = d;
  fx.insert(fx.end(), c, c + nf * dim * dim);
  gids.insert(gids.end(), nf * dim, -1);
}

void FormerFaces::Add(const Mesh& m, const vector<i64>& faceGids) {
  dim = m.dim;
  std::unordered_map<i64, i64> g2v;
  g2v.reserve(m.nv() * 2);
  for (i64 v = 0; v < m.nv(); ++v) g2v[m.gid[v]] = v;
  for (size_t q = 0; q + dim <= faceGids.size(); q += dim) {
    gids.insert(gids.end(), faceGids.begin() + q, faceGids.begin() + q + dim);
    for (int a = 0; a < dim; ++a) {
      const auto it = g2v.find(faceGids[q + a]);
      if (it == g2v.end()) throw std::runtime_error("FormerFaces: vertex not in mesh");
      for (int d = 0; d < dim; ++d) fx.push_back(m.X[dim * it->second + d]);
    }
  }
}

void FormerFaces::Index() {
  pts.clear();
  owner.clear();
  const i64 nf = nFaces();
  for (i64 f = 0; f < nf; ++f) {
    const double* p = &fx[dim * dim * f];
    double c[3] = {0, 0, 0};
    for (int a = 0; a < dim; ++a)
      for (int d = 0; d < dim; ++d) c[d] += p[dim * a + d] / dim;
    pts.insert(pts.end(), c, c + dim);
    owner.push_back(f);
    for (int a = 0; a < dim; ++a) {
      pts.insert(pts.end(), p + dim * a, p + dim * a + dim);
      owner.push_back(f);
      for (int b = a + 1; b < dim; ++b) {
        for (int d = 0; d < dim; ++d) pts.push_back(0.5 * (p[dim * a + d] + p[dim * b + d]));
        owner.push_back(f);
      }
    }
  }
  kd.Build(dim, pts.data(), owner.size());
  order.resize(nf);
  std::iota(order.begin(), order.end(), 0);
  boxes.clear();
  if (nf) BuildBoxes(0, nf);
}

double FormerFaces::DistApprox(const double* x, const Sym& M, i64* face) const {
  if (owner.empty()) {
    if (face) *face = -1;
    return 1e300;
  }
  thread_local vector<std::pair<double, i64>> cand;
  kd.KNN(x, 16, cand);
  double best = 1e300;
  i64 bestF = -1;
  for (auto& c : cand) {
    const i64 s = c.second;
    double v[3] = {0, 0, 0};
    for (int d = 0; d < dim; ++d) v[d] = pts[dim * s + d] - x[d];
    const double dm = std::sqrt(std::max(0.0, Quad(dim, M, v)));
    if (dm < best) {
      best = dm;
      bestF = owner[s];
    }
  }
  if (face) *face = bestF;
  return best;
}

/*----------------------------------------------------------------------------------------------------------------*/

static double PointSegDist2(const double* p, const double* a, const double* b) {
  double ab[2] = {b[0] - a[0], b[1] - a[1]}, ap[2] = {p[0] - a[0], p[1] - a[1]};
  const double l2 = ab[0] * ab[0] + ab[1] * ab[1];
  double t = l2 > 0 ? (ap[0] * ab[0] + ap[1] * ab[1]) / l2 : 0;
  t = std::max(0.0, std::min(1.0, t));
  const double d[2] = {ap[0] - t * ab[0], ap[1] - t * ab[1]};
  return d[0] * d[0] + d[1] * d[1];
}
static double PointTriDist2(const double* p_, const double* a_, const double* b_, const double* c_) {
  using V = Eigen::Vector3d;
  const V p(p_[0], p_[1], p_[2]), a(a_[0], a_[1], a_[2]), b(b_[0], b_[1], b_[2]), c(c_[0], c_[1], c_[2]);
  const V ab = b - a, ac = c - a, ap = p - a;
  const double d1 = ab.dot(ap), d2 = ac.dot(ap);
  if (d1 <= 0 && d2 <= 0) return (p - a).squaredNorm();
  const V bp = p - b;
  const double d3 = ab.dot(bp), d4 = ac.dot(bp);
  if (d3 >= 0 && d4 <= d3) return (p - b).squaredNorm();
  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) return (p - (a + d1 / (d1 - d3) * ab)).squaredNorm();
  const V cp = p - c;
  const double d5 = ab.dot(cp), d6 = ac.dot(cp);
  if (d6 >= 0 && d5 <= d6) return (p - c).squaredNorm();
  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) return (p - (a + d2 / (d2 - d6) * ac)).squaredNorm();
  const double va = d3 * d6 - d5 * d4;
  if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0)
    return (p - (b + (d4 - d3) / ((d4 - d3) + (d5 - d6)) * (c - b))).squaredNorm();
  const double den = 1.0 / (va + vb + vc);
  const double v = vb * den, w = vc * den;
  return (p - (a + ab * v + ac * w)).squaredNorm();
}

/*--- Exact metric projection onto a segment/triangle, with an eigenvalue/AABB lower bound. ---*/
int FormerFaces::BuildBoxes(i64 begin, i64 end) {
  Box b;
  b.begin = begin; b.end = end;
  b.lo.fill(1e300); b.hi.fill(-1e300);
  for (i64 q = begin; q < end; ++q) for (int a = 0; a < dim; ++a) for (int d = 0; d < dim; ++d) {
    const double v = fx[dim * dim * order[q] + dim * a + d];
    b.lo[d] = std::min(b.lo[d], v); b.hi[d] = std::max(b.hi[d], v);
  }
  const int id = boxes.size();
  boxes.push_back(b);
  if (end - begin > 8) {
    int axis = 0;
    for (int d = 1; d < dim; ++d) if (b.hi[d] - b.lo[d] > b.hi[axis] - b.lo[axis]) axis = d;
    const i64 mid = (begin + end) / 2;
    auto center = [&](i64 f) { double c = 0; for (int a = 0; a < dim; ++a) c += fx[dim * dim * f + dim * a + axis]; return c; };
    std::nth_element(order.begin() + begin, order.begin() + mid, order.begin() + end,
                     [&](i64 a, i64 c) { return center(a) != center(c) ? center(a) < center(c) : a < c; });
    const int left = BuildBoxes(begin, mid), right = BuildBoxes(mid, end);
    boxes[id].left = left; boxes[id].right = right;
  }
  return id;
}

double FormerFaces::FaceDist(i64 face, const double* x, const Sym& M) const {
  const double* p = &fx[dim * dim * face];
  auto dot = [&](const double* a, const double* b) {
    double v = 0;
    for (int i = 0; i < dim; ++i) for (int j = 0; j < dim; ++j) v += a[i] * M.a[i][j] * b[j];
    return v;
  };
  double best = 1e300;
  for (int a = 0; a < dim; ++a) for (int b = a + 1; b < dim; ++b) {
    double u[3]{}, v[3]{}, w[3]{};
    for (int d = 0; d < dim; ++d) { u[d] = p[dim * b + d] - p[dim * a + d]; v[d] = x[d] - p[dim * a + d]; }
    const double den = dot(u, u);
    const double t = den > 0 ? std::clamp(dot(u, v) / den, 0.0, 1.0) : 0;
    for (int d = 0; d < dim; ++d) w[d] = v[d] - t * u[d];
    best = std::min(best, dot(w, w));
  }
  if (dim == 3) {
    double u[3], v[3], w[3];
    for (int d = 0; d < 3; ++d) { u[d] = p[3 + d] - p[d]; v[d] = p[6 + d] - p[d]; w[d] = x[d] - p[d]; }
    const double uu = dot(u, u), vv = dot(v, v), uv = dot(u, v), uw = dot(u, w), vw = dot(v, w);
    const double den = uu * vv - uv * uv;
    if (den > 0) {
      const double a = (uw * vv - vw * uv) / den, b = (vw * uu - uw * uv) / den;
      if (a >= 0 && b >= 0 && a + b <= 1) {
        for (int d = 0; d < 3; ++d) w[d] -= a * u[d] + b * v[d];
        best = std::min(best, dot(w, w));
      }
    }
  }
  return std::sqrt(std::max(0.0, best));
}

double FormerFaces::DistBrute(const double* x, const Sym& M, i64* face) const {
  double best = 1e300;
  i64 hit = -1;
  for (i64 f = 0; f < nFaces(); ++f) {
    const double d = FaceDist(f, x, M);
    if (d < best) { best = d; hit = f; }
  }
  if (face) *face = hit;
  return best;
}

double FormerFaces::Dist(const double* x, const Sym& M, i64* face) const {
  if (boxes.empty() || nFaces() < 32) return DistBrute(x, M, face);
  double val[3], vec[3][3];
  Eig(dim, M, val, vec);
  /*--- A slightly reduced lower eigenvalue makes pruning conservative under roundoff. ---*/
  const double lambda = std::max(0.0, val[0]) * (1 - 1e-10);
  double best = 1e300;
  i64 hit = -1;
  std::function<void(int)> visit = [&](int id) {
    const auto& b = boxes[id];
    double lower = 0;
    for (int d = 0; d < dim; ++d) {
      const double v = std::max({b.lo[d] - x[d], x[d] - b.hi[d], 0.0});
      lower += v * v;
    }
    if (lambda * lower > best * best * (1 + 1e-10)) return;
    if (b.left >= 0) { visit(b.left); visit(b.right); return; }
    for (i64 q = b.begin; q < b.end; ++q) {
      const i64 f = order[q];
      const double d = FaceDist(f, x, M);
      if (d < best || (d == best && f < hit)) { best = d; hit = f; }
    }
  };
  visit(0);
  if (face) *face = hit;
  return best;
}

static CellStat MeasureCell(const Mesh& m, i64 k, const FormerFaces* ff) {
  CellStat c;
  const Sym Mk = ElemMetric(m, k);
  const i64* e = &m.T[m.nn() * k];
  const double volume = SignedVolume(m.dim, m.X.data(), e);
  if (!std::isfinite(volume) || volume <= 0)
    throw std::runtime_error("cannot score cell with non-finite or non-positive volume");
  c.quality = ElemQuality(m.dim, m.X.data(), e, Mk);
  if (!std::isfinite(c.quality) || c.quality <= 0)
    throw std::runtime_error("cannot score cell with non-finite or non-positive quality");
  c.ar = ARClass(LogAR(m.dim, Mk));
  for (int a = 0; a < m.nn(); ++a) for (int b = a + 1; b < m.nn(); ++b)
    c.edges.push_back(EdgeLen(m.dim, &m.X[m.dim * e[a]], &m.X[m.dim * e[b]], &m.M[m.nm() * e[a]], &m.M[m.nm() * e[b]]));
  if (ff) { const auto x = ElemCentroid(m, k); c.distance = ff->Dist(x.data(), Mk, &c.face); }
  c.db = DistBin(c.distance);
  return c;
}

Mesh TargetMetricMesh(const Mesh& m, const Background& target) {
  // MMG ridge tensors are carried data, never a fallback for the scoring metric.
  if (!target.mesh || target.mesh->dim != m.dim || target.loc.ne == 0)
    throw std::runtime_error("cannot score mesh without immutable target metric");
  Mesh mt = m;
  mt.M.resize(m.nv() * m.nm());
  for (double x : m.X)
    if (!std::isfinite(x)) throw std::runtime_error("cannot score mesh with non-finite coordinates");
  for (i64 v = 0; v < m.nv(); ++v)
    if (!InterpMetric(target.loc, target.mesh->M.data(), m.dim, &m.X[m.dim * v], &mt.M[m.nm() * v]))
      throw std::runtime_error("cannot score mesh: immutable target metric interpolation failed (not SPD or not located)");
  return mt;
}

vector<CellStat> CellStats(const Mesh& m, const Background& target, const FormerFaces* ff) {
  const Mesh mt = TargetMetricMesh(m, target);
  Adjacency ad;
  ad.Build(m);
  vector<CellStat> cells(m.ne());
  for (i64 k = 0; k < m.ne(); ++k) {
    cells[k] = MeasureCell(mt, k, ff);
    bool touch = false;
    for (int j = 0; j < m.nn(); ++j) touch |= ad.fphys[m.nn() * k + j] >= 0;
    cells[k].cls = cells[k].ar + 4 * touch;
  }
  return cells;
}

/*--- Cells intersecting the former face with positive length/area, even after it disappears. ---*/
vector<double> FrozenFaceFinalQuality(const Mesh& m, const FormerFaces& ff, const vector<CellStat>& cells) {
  using V = Eigen::Vector3d;
  FormerFaces bounds; bounds.dim = m.dim;
  vector<Eigen::Matrix3d> inverse(m.ne());
  for (i64 k = 0; k < m.ne(); ++k) {
    const i64* e = &m.T[m.nn() * k];
    double lo[3] = {1e300, 1e300, 1e300}, hi[3] = {-1e300, -1e300, -1e300};
    for (int a = 0; a < m.nn(); ++a) for (int d = 0; d < m.dim; ++d) {
      lo[d] = std::min(lo[d], m.X[m.dim * e[a] + d]); hi[d] = std::max(hi[d], m.X[m.dim * e[a] + d]);
    }
    bounds.fx.insert(bounds.fx.end(), lo, lo + m.dim); bounds.fx.insert(bounds.fx.end(), hi, hi + m.dim);
    if (m.dim == 3) bounds.fx.insert(bounds.fx.end(), lo, lo + m.dim);
    Eigen::Matrix3d A = Eigen::Matrix3d::Identity();
    for (int a = 0; a < m.dim; ++a) for (int d = 0; d < m.dim; ++d)
      A(d, a) = m.X[m.dim * e[a + 1] + d] - m.X[m.dim * e[0] + d];
    inverse[k] = A.inverse();
  }
  bounds.order.resize(m.ne()); std::iota(bounds.order.begin(), bounds.order.end(), 0);
  if (m.ne()) bounds.BuildBoxes(0, m.ne());
  vector<double> out(ff.nFaces(), -1);
  for (i64 face = 0; face < ff.nFaces(); ++face) {
    const double* x = &ff.fx[ff.dim * ff.dim * face];
    V points[3] = {V::Zero(), V::Zero(), V::Zero()};
    std::array<double, 3> lo{{1e300, 1e300, 1e300}}, hi{{-1e300, -1e300, -1e300}};
    for (int a = 0; a < m.dim; ++a) for (int d = 0; d < m.dim; ++d) {
      points[a][d] = x[m.dim * a + d]; lo[d] = std::min(lo[d], points[a][d]); hi[d] = std::max(hi[d], points[a][d]);
    }
    const double measure = m.dim == 2 ? (points[1] - points[0]).norm() : (points[1] - points[0]).cross(points[2] - points[0]).norm();
    auto intersects = [&](i64 k) {
      const i64* e = &m.T[m.nn() * k];
      V origin = V::Zero(); for (int d = 0; d < m.dim; ++d) origin[d] = m.X[m.dim * e[0] + d];
      auto lam = [&](const V& p) {
        std::array<double, 4> l{}; const V t = inverse[k] * (p - origin); l[0] = 1;
        for (int d = 0; d < m.dim; ++d) { l[d + 1] = t[d]; l[0] -= t[d]; } return l;
      };
      if (m.dim == 2) {
        const auto a = lam(points[0]), b = lam(points[1]); double left = 0, right = 1;
        for (int j = 0; j < m.nn(); ++j) {
          if (a[j] < 0 && b[j] < 0) return false;
          if (a[j] < 0) left = std::max(left, a[j] / (a[j] - b[j]));
          if (b[j] < 0) right = std::min(right, a[j] / (a[j] - b[j]));
        }
        return right - left > 1e-12;
      }
      vector<V> poly(points, points + 3);
      for (int j = 0; j < m.nn() && !poly.empty(); ++j) {
        vector<V> next;
        for (size_t a = 0; a < poly.size(); ++a) {
          const V& p = poly[a]; const V& q = poly[(a + 1) % poly.size()];
          const double lp = lam(p)[j], lq = lam(q)[j];
          if (lp >= -1e-12) next.push_back(p);
          if ((lp < -1e-12) != (lq < -1e-12)) next.push_back(p + lp / (lp - lq) * (q - p));
        }
        poly.swap(next);
      }
      double area = 0;
      for (size_t a = 1; a + 1 < poly.size(); ++a) area += (poly[a] - poly[0]).cross(poly[a + 1] - poly[0]).norm();
      return area > measure * 1e-12;
    };
    std::function<void(int)> visit = [&](int id) {
      const auto& b = bounds.boxes[id];
      for (int d = 0; d < m.dim; ++d) if (lo[d] > b.hi[d] + 1e-12 || hi[d] < b.lo[d] - 1e-12) return;
      if (b.left >= 0) { visit(b.left); visit(b.right); return; }
      for (i64 q = b.begin; q < b.end; ++q) {
        const i64 k = bounds.order[q];
        if (intersects(k)) out[face] = out[face] < 0 ? cells[k].quality : std::min(out[face], cells[k].quality);
      }
    };
    if (!bounds.boxes.empty()) visit(0);
  }
  return out;
}

std::map<FKey, double> FaceQualityMap(const Mesh& m, const Background& target) {
  const auto cells = CellStats(m, target, nullptr);
  std::map<FKey, double> out;
  for (i64 k = 0; k < m.ne(); ++k) for (int j = 0; j < m.nn(); ++j) {
    i64 f[3];
    ElemFace(m.dim, &m.T[m.nn() * k], j, f);
    for (int a = 0; a < m.dim; ++a) f[a] = m.gid[f[a]];
    const auto key = MakeKey(m.dim, f);
    const auto it = out.find(key);
    if (it == out.end()) out[key] = cells[k].quality;
    else it->second = std::min(it->second, cells[k].quality);
  }
  return out;
}

double FaceAdjacentQuality(const Mesh& m, const vector<i64>& gids, const Background& target) {
  const auto qs = FaceQualityMap(m, target);
  const auto it = qs.find(MakeKey(m.dim, gids.data()));
  return it == qs.end() ? -1 : it->second;
}

MeshStats ComputeStats(const Mesh& m, const Background& target, const StatsOpts& so) {
  MeshStats s;
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  s.nv = m.nv();
  s.ne = m.ne();
  /*--- target metric at the vertices ---*/
  const Mesh measured = TargetMetricMesh(m, target);
  const auto& Mt = measured.M;
  /*--- elements ---*/
  vector<double> q(s.ne), dist(s.ne, 1e300);
  vector<int> arc(s.ne), db(s.ne);
  vector<i64> nearFace(s.ne, -1);
  vector<char> bad(s.ne, 0);
  vector<vector<double>> regQ(NAR * NDB), classQ(NAR);
  vector<double> allQ;
  allQ.reserve(s.ne);
  for (i64 k = 0; k < s.ne; ++k) {
    const i64* e = &m.T[nn * k];
    Sym Mk;
    for (int a = 0; a < nn; ++a) {
      const Sym v = FromUpper(dim, &Mt[nm * e[a]]);
      for (int i = 0; i < dim; ++i)
        for (int j = 0; j < dim; ++j) Mk.a[i][j] += v.a[i][j] / nn;
    }
    const auto cell = MeasureCell(measured, k, so.ff);
    q[k] = cell.quality;
    if (dim == 3)
      bad[k] = MinDihedralDeg(m.X.data(), e, Mk) < 5.0;
    else
      bad[k] = q[k] < 0.1;
    arc[k] = cell.ar;
    dist[k] = cell.distance;
    nearFace[k] = cell.face;
    db[k] = DistBin(dist[k]);
    regQ[arc[k] * NDB + db[k]].push_back(q[k]);
    classQ[arc[k]].push_back(q[k]);
    allQ.push_back(q[k]);
    s.nBad += bad[k];
    auto& rg = s.reg[arc[k]][db[k]];
    ++rg.nCell;
    rg.bad += bad[k];
    double det = dim == 2 ? Mk.a[0][0] * Mk.a[1][1] - Mk.a[0][1] * Mk.a[1][0]
                          : Mk.a[0][0] * (Mk.a[1][1] * Mk.a[2][2] - Mk.a[1][2] * Mk.a[2][1]) -
                                Mk.a[0][1] * (Mk.a[1][0] * Mk.a[2][2] - Mk.a[1][2] * Mk.a[2][0]) +
                                Mk.a[0][2] * (Mk.a[1][0] * Mk.a[2][1] - Mk.a[1][1] * Mk.a[2][0]);
    s.complexityTarget += std::fabs(SignedVolume(dim, m.X.data(), e)) * std::sqrt(std::max(det, 0.0));
    s.complexityCarried += ElemComplexity(m, k);
  }
  auto pct = [](vector<double>& v, double p) {
    if (v.empty()) return 0.0;
    const size_t i = std::min(v.size() - 1, (size_t)std::floor(p * (v.size() - 1)));
    std::nth_element(v.begin(), v.begin() + i, v.end());
    return v[i];
  };
  s.badFrac = s.ne ? (double)s.nBad / s.ne : 0;
  s.q1global = pct(allQ, 0.01);
  s.q5global = pct(allQ, 0.05);
  s.qmin = allQ.empty() ? 0 : *std::min_element(allQ.begin(), allQ.end());
  for (int a = 0; a < NAR; ++a) {
    s.classQ1[a] = pct(classQ[a], 0.01);
    s.classQ5[a] = pct(classQ[a], 0.05);
    for (int d = 0; d < NDB; ++d) {
      auto& v = regQ[a * NDB + d];
      auto& rg = s.reg[a][d];
      if (rg.nCell) rg.bad /= rg.nCell;
      rg.q1 = pct(v, 0.01);
      rg.q5 = pct(v, 0.05);
      rg.qmin = v.empty() ? 0 : *std::min_element(v.begin(), v.end());
    }
  }
  s.regionQuality = regQ;
  /*--- edges ---*/
  vector<std::pair<i64, i64>> edges;
  edges.reserve(s.ne * (dim == 2 ? 3 : 6));
  for (i64 k = 0; k < s.ne; ++k)
    for (int a = 0; a < nn; ++a)
      for (int b = a + 1; b < nn; ++b) {
        i64 u = m.T[nn * k + a], v = m.T[nn * k + b];
        if (u > v) std::swap(u, v);
        edges.push_back({u, v});
      }
  std::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  for (const auto& e : edges) {
    const double l = EdgeLen(dim, &m.X[dim * e.first], &m.X[dim * e.second], &Mt[nm * e.first], &Mt[nm * e.second]);
    const bool in = (l >= 0.71 && l <= 1.41);
    Sym Me = FromUpper(dim, &Mt[nm * e.first]);
    const Sym Mb = FromUpper(dim, &Mt[nm * e.second]);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j) Me.a[i][j] = 0.5 * (Me.a[i][j] + Mb.a[i][j]);
    double mid[3] = {0, 0, 0};
    for (int d = 0; d < dim; ++d) mid[d] = 0.5 * (m.X[dim * e.first + d] + m.X[dim * e.second + d]);
    const double dd = so.ff ? so.ff->Dist(mid, Me) : 1e300;
    ++s.nEdge;
    s.edgeIn += in;
    if (dd < 2) {
      ++s.nEdgeNear;
      s.edgeInNear += in;
    }
    auto& rg = s.reg[ARClass(LogAR(dim, Me))][DistBin(dd)];
    ++rg.nEdge;
    rg.nEdgeIn += in;
  }
  s.edgeIn = s.nEdge ? s.edgeIn / s.nEdge : 0;
  s.edgeInNear = s.nEdgeNear ? s.edgeInNear / s.nEdgeNear : 0;
  /*--- H-2 degradation depth per former face ---*/
  if (so.ff && so.degradeQ) {
    s.faceDepth.assign(so.ff->nFaces(), 0.0);
    for (i64 k = 0; k < s.ne; ++k) {
      if (nearFace[k] < 0 || dist[k] > 8) continue;
      if (q[k] < so.degradeQ[arc[k]]) s.faceDepth[nearFace[k]] = std::max(s.faceDepth[nearFace[k]], dist[k]);
    }
    vector<double> dpt(s.faceDepth);
    for (double d : dpt) s.nFacesDegraded += d > 0;
    s.depthP95 = pct(dpt, 0.95);
    s.depthP99 = pct(dpt, 0.99);
    s.depthMax = dpt.empty() ? 0 : *std::max_element(dpt.begin(), dpt.end());
  }
  if (so.perElem) s.perElemFace = nearFace;
  /*--- surface deviation from the original surface, per marker ---*/
  if (so.surfRef) {
    const Mesh& R = *so.surfRef;
    s.devMean.assign(R.markerRef.size(), 0);
    s.devMax.assign(R.markerRef.size(), 0);
    for (size_t mk = 0; mk < R.markerRef.size(); ++mk) {
      const int ref = R.markerRef[mk];
      vector<i64> rf;
      for (i64 f = 0; f < R.nf(); ++f)
        if (R.Fref[f] == ref) rf.push_back(f);
      if (rf.empty()) continue;
      /*--- bins over face boxes ---*/
      double lo[3] = {1e300, 1e300, 1e300}, hi[3] = {-1e300, -1e300, -1e300};
      for (i64 f : rf)
        for (int a = 0; a < dim; ++a)
          for (int d = 0; d < dim; ++d) {
            lo[d] = std::min(lo[d], R.X[dim * R.F[dim * f + a] + d]);
            hi[d] = std::max(hi[d], R.X[dim * R.F[dim * f + a] + d]);
          }
      int nb[3] = {1, 1, 1};
      double hb[3] = {1, 1, 1};
      const double cnt = std::max<double>(1, rf.size() / 2.0);
      double ext = 0;
      for (int d = 0; d < dim; ++d) ext = std::max(ext, hi[d] - lo[d]);
      const double cell = std::max(ext / std::pow(cnt, 1.0 / (dim - 1)), 1e-12);
      for (int d = 0; d < dim; ++d) {
        nb[d] = std::max(1, std::min(500, (int)std::ceil((hi[d] - lo[d]) / cell)));
        hb[d] = std::max((hi[d] - lo[d]) / nb[d], 1e-12);
      }
      std::unordered_map<i64, vector<i64>> grid;
      auto cellOf = [&](const double* x, int* c) {
        for (int d = 0; d < 3; ++d) c[d] = d < dim ? std::max(0, std::min(nb[d] - 1, (int)std::floor((x[d] - lo[d]) / hb[d]))) : 0;
      };
      for (i64 f : rf) {
        int c0[3] = {nb[0], nb[1], nb[2]}, c1[3] = {-1, -1, -1}, c[3];
        for (int a = 0; a < dim; ++a) {
          cellOf(&R.X[dim * R.F[dim * f + a]], c);
          for (int d = 0; d < 3; ++d) {
            c0[d] = std::min(c0[d], c[d]);
            c1[d] = std::max(c1[d], c[d]);
          }
        }
        for (int i = c0[0]; i <= c1[0]; ++i)
          for (int j = c0[1]; j <= c1[1]; ++j)
            for (int k = c0[2]; k <= c1[2]; ++k) grid[((i64)k * nb[1] + j) * nb[0] + i].push_back(f);
      }
      vector<char> onM(m.nv(), 0);
      for (i64 f = 0; f < m.nf(); ++f)
        if (m.Fref[f] == ref)
          for (int a = 0; a < dim; ++a) onM[m.F[dim * f + a]] = 1;
      double sum = 0, mx = 0;
      i64 cntV = 0;
      for (i64 v = 0; v < m.nv(); ++v) {
        if (!onM[v]) continue;
        int c[3];
        cellOf(&m.X[dim * v], c);
        double best = 1e300;
        for (int r = 0; r < 64 && best == 1e300; ++r) {
          for (int i = c[0] - r; i <= c[0] + r; ++i)
            for (int j = c[1] - r; j <= c[1] + r; ++j)
              for (int k = (dim == 3 ? c[2] - r : 0); k <= (dim == 3 ? c[2] + r : 0); ++k) {
                if (i < 0 || j < 0 || k < 0 || i >= nb[0] || j >= nb[1] || k >= nb[2]) continue;
                const auto it = grid.find(((i64)k * nb[1] + j) * nb[0] + i);
                if (it == grid.end()) continue;
                for (i64 f : it->second) {
                  const double* a = &R.X[dim * R.F[dim * f]];
                  const double* b = &R.X[dim * R.F[dim * f + 1]];
                  const double d2 = dim == 2 ? PointSegDist2(&m.X[2 * v], a, b)
                                             : PointTriDist2(&m.X[3 * v], a, b, &R.X[3 * R.F[3 * f + 2]]);
                  best = std::min(best, d2);
                }
              }
          if (best < 1e300) {  // one more ring for safety
            ++r;
            for (int i = c[0] - r; i <= c[0] + r; ++i)
              for (int j = c[1] - r; j <= c[1] + r; ++j)
                for (int k = (dim == 3 ? c[2] - r : 0); k <= (dim == 3 ? c[2] + r : 0); ++k) {
                  if (i < 0 || j < 0 || k < 0 || i >= nb[0] || j >= nb[1] || k >= nb[2]) continue;
                  const auto it = grid.find(((i64)k * nb[1] + j) * nb[0] + i);
                  if (it == grid.end()) continue;
                  for (i64 f : it->second) {
                    const double* a = &R.X[dim * R.F[dim * f]];
                    const double* b = &R.X[dim * R.F[dim * f + 1]];
                    const double d2 = dim == 2 ? PointSegDist2(&m.X[2 * v], a, b)
                                               : PointTriDist2(&m.X[3 * v], a, b, &R.X[3 * R.F[3 * f + 2]]);
                    best = std::min(best, d2);
                  }
                }
            break;
          }
        }
        const double d = std::sqrt(best);
        sum += d;
        mx = std::max(mx, d);
        ++cntV;
      }
      s.devMean[mk] = cntV ? sum / cntV : 0;
      s.devMax[mk] = mx;
    }
  }
  return s;
}

/*----------------------------------------------------------------------------------------------------------------*/

vector<CutFace> CutFaces(const Mesh& m, const Adjacency& adj, const vector<int>& part) {
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  vector<CutFace> out;
  i64 f[3];
  for (i64 e = 0; e < m.ne(); ++e)
    for (int j = 0; j < nn; ++j) {
      const i64 o = adj.nb[nn * e + j];
      if (o < 0 || part[e] >= part[o]) continue;
      CutFace c;
      c.elemA = e;
      c.elemB = o;
      c.a = part[e];
      c.b = part[o];
      ElemFace(dim, &m.T[nn * e], j, f);
      const double* p0 = &m.X[dim * f[0]];
      const double* p1 = &m.X[dim * f[1]];
      if (dim == 2) {
        const double d[2] = {p1[0] - p0[0], p1[1] - p0[1]};
        c.area = std::hypot(d[0], d[1]);
        c.n[0] = d[1] / c.area;
        c.n[1] = -d[0] / c.area;
        c.n[2] = 0;
        c.c[0] = 0.5 * (p0[0] + p1[0]);
        c.c[1] = 0.5 * (p0[1] + p1[1]);
        c.c[2] = 0;
      } else {
        const double* p2 = &m.X[3 * f[2]];
        const Eigen::Vector3d u(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]), w(p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]);
        Eigen::Vector3d nrm = u.cross(w);
        const double l = nrm.norm();
        c.area = 0.5 * l;
        nrm /= l;
        for (int d = 0; d < 3; ++d) {
          c.n[d] = nrm(d);
          c.c[d] = (p0[d] + p1[d] + p2[d]) / 3.0;
        }
      }
      vector<const double*> mets;
      for (int a = 0; a < dim; ++a) mets.push_back(&m.M[nm * f[a]]);
      const Sym Mf = LogEuclidMean(dim, mets);
      double val[3], vec[3][3];
      Eig(dim, Mf, val, vec);
      c.logAR = 0.5 * std::log10(val[dim - 1] / val[0]);
      c.ratioMid = dim == 3 ? val[2] / val[1] : val[1] / val[0];
      for (int d = 0; d < 3; ++d) c.fine[d] = d < dim ? vec[dim - 1][d] : 0;
      c.ht = dim == 2 ? 1.0 / std::sqrt(val[0]) : 1.0 / std::sqrt(std::sqrt(val[0] * val[1]));
      double cf = 0;
      for (int a = 0; a < dim; ++a)
        for (int b = a + 1; b < dim; ++b) {
          const double l = EdgeLen(dim, &m.X[dim * f[a]], &m.X[dim * f[b]], &m.M[nm * f[a]], &m.M[nm * f[b]]);
          cf = std::max(cf, std::fabs(std::log2(std::max(l, 1e-300))));
        }
      c.cf = cf;
      for (int a = 0; a < 3; ++a) c.key[a] = a < dim ? m.gid[f[a]] : -1;
      std::sort(c.key, c.key + dim);
      out.push_back(c);
    }
  return out;
}

MacroReport MacroOrientation(const Mesh& m, vector<CutFace>& cf, double rhoFactor, double arP, double unresolved,
                             double cosMax) {
  const int dim = m.dim;
  MacroReport r;
  r.nCut = cf.size();
  /*--- sheet adjacency: same pair (a,b), sharing an edge (3D) or a vertex (2D) ---*/
  std::map<std::tuple<int, int, i64, i64>, vector<i64>> share;
  for (size_t i = 0; i < cf.size(); ++i) {
    const auto& c = cf[i];
    if (dim == 2) {
      for (int a = 0; a < 2; ++a) share[{c.a, c.b, c.key[a], -1}].push_back(i);
    } else {
      for (int a = 0; a < 3; ++a)
        for (int b = a + 1; b < 3; ++b) share[{c.a, c.b, c.key[a], c.key[b]}].push_back(i);
    }
  }
  vector<vector<i64>> nbr(cf.size());
  for (auto& kv : share)
    for (i64 x : kv.second)
      for (i64 y : kv.second)
        if (x != y) nbr[x].push_back(y);
  vector<double> cfs;
  for (size_t i = 0; i < cf.size(); ++i) {
    auto& c = cf[i];
    r.areaCut += c.area;
    cfs.push_back(c.cf);
    r.meanCf += c.cf / cf.size();
    r.meanLogAR += c.logAR / cf.size();
    c.aniso = std::pow(10.0, c.logAR) >= arP && c.ratioMid >= 4.0;
    if (!c.aniso) {
      c.macro = 0;
      continue;
    }
    ++r.nAniso;
    r.areaAniso += c.area;
    /*--- resultant over the sheet within rho_f (surface-graph walk, Euclidean centroid radius) ---*/
    const double rho = rhoFactor * c.ht;
    double R[3] = {0, 0, 0}, sumA = 0;
    std::vector<i64> stack = {(i64)i};
    std::set<i64> seen = {(i64)i};
    while (!stack.empty()) {
      const i64 g = stack.back();
      stack.pop_back();
      for (int d = 0; d < 3; ++d) R[d] += cf[g].area * cf[g].n[d];
      sumA += cf[g].area;
      for (i64 h : nbr[g]) {
        if (seen.count(h)) continue;
        double d2 = 0;
        for (int d = 0; d < 3; ++d) d2 += std::pow(cf[h].c[d] - c.c[d], 2);
        if (d2 > rho * rho) continue;
        seen.insert(h);
        stack.push_back(h);
        if (seen.size() > 20000) break;
      }
    }
    const double nR = std::sqrt(R[0] * R[0] + R[1] * R[1] + R[2] * R[2]);
    if (nR < unresolved * sumA) {
      c.macro = 3;
      ++r.nUnres;
      r.areaUnres += c.area;
      continue;
    }
    double dot = 0;
    for (int d = 0; d < 3; ++d) dot += R[d] / nR * c.fine[d];
    if (std::fabs(dot) > cosMax) {
      c.macro = 2;
      ++r.nViol;
      r.areaViol += c.area;
    } else {
      c.macro = 1;
    }
  }
  if (!cfs.empty()) {
    std::sort(cfs.begin(), cfs.end());
    r.p95Cf = cfs[std::min(cfs.size() - 1, (size_t)(0.95 * (cfs.size() - 1)))];
  }
  /*--- longest violating run: connected violating faces within a sheet ---*/
  vector<char> vis(cf.size(), 0);
  for (size_t i = 0; i < cf.size(); ++i) {
    if (vis[i] || cf[i].macro != 2) continue;
    i64 cnt = 0;
    vector<i64> st = {(i64)i};
    vis[i] = 1;
    while (!st.empty()) {
      const i64 g = st.back();
      st.pop_back();
      ++cnt;
      for (i64 h : nbr[g])
        if (!vis[h] && cf[h].macro == 2) {
          vis[h] = 1;
          st.push_back(h);
        }
    }
    r.longestViolRun = std::max(r.longestViolRun, cnt);
  }
  return r;
}

double FaceWeightB3(const Mesh& m, const i64* f, double a, double b) {
  const int dim = m.dim, nm = m.nm();
  vector<const double*> mets;
  for (int k = 0; k < dim; ++k) mets.push_back(&m.M[nm * f[k]]);
  const Sym Mf = LogEuclidMean(dim, mets);
  double val[3], vec[3][3];
  Eig(dim, Mf, val, vec);
  const double logAR = 0.5 * std::log10(val[dim - 1] / val[0]);
  double cf = 0;
  for (int x = 0; x < dim; ++x)
    for (int y = x + 1; y < dim; ++y) {
      const double l = EdgeLen(dim, &m.X[dim * f[x]], &m.X[dim * f[y]], &m.M[nm * f[x]], &m.M[nm * f[y]]);
      cf = std::max(cf, std::fabs(std::log2(std::max(l, 1e-300))));
    }
  return std::max(1.0, std::min(1e4, 1 + a * cf + b * logAR));
}

/*----------------------------------------------------------------------------------------------------------------*/

ClassifierOut Classify(const Mesh& carried, const Background& target, const Adjacency& adj, const FormerFaces* ff,
                       double R, const double* allow) {
  const Mesh m = TargetMetricMesh(carried, target);
  ClassifierOut o;
  const int dim = m.dim, nn = m.nn(), nm = m.nm();
  const double qHard = dim == 3 ? 0.05 : 0.1;
  vector<char> bad(m.ne(), 0), near(m.ne(), 0);
  vector<int> cls(m.ne());
  for (i64 k = 0; k < m.ne(); ++k) {
    const i64* e = &m.T[nn * k];
    const Sym Mk = ElemMetric(m, k);
    const auto cell = MeasureCell(m, k, ff);
    bool b = cell.quality < qHard;
    for (double l : cell.edges) if (l > 4 || l < 0.25) b = true;
    if (!b && dim == 3) b = MinDihedralDeg(m.X.data(), e, Mk) < 2.0;
    bad[k] = b;
    bool touch = false;
    for (int j = 0; j < nn; ++j) touch |= adj.fphys[nn * k + j] >= 0;
    cls[k] = cell.ar + 4 * touch;
    near[k] = ff && cell.distance < R;
    ++o.allCells[cls[k]];
    o.allDensity[cls[k]] += b;
    if (near[k]) {
      ++o.nearCells[cls[k]];
      o.nearDensity[cls[k]] += b;
      o.nBadNear += b;
    } else {
      ++o.farCells[cls[k]];
      o.farDensity[cls[k]] += b;
    }
  }
  for (int c = 0; c < 8; ++c) {
    if (o.allCells[c]) o.allDensity[c] /= o.allCells[c];
    if (o.nearCells[c]) o.nearDensity[c] /= o.nearCells[c];
    if (o.farCells[c]) o.farDensity[c] /= o.farCells[c];
  }
  if (!ff) return o;
  bool seedClass[8];
  for (int c = 0; c < 8; ++c) {
    const double a = allow ? allow[c] : 0.0;
    const double ref = o.farCells[c] >= 1000 ? o.farDensity[c] : a;
    seedClass[c] = o.nearCells[c] > 0 && o.nearDensity[c] > std::max(2 * ref, a);
  }
  vector<char> dv(m.nv(), 0);
  for (i64 k = 0; k < m.ne(); ++k)
    if (bad[k] && near[k] && seedClass[cls[k]]) {
      ++o.nSeedCells;
      for (int a = 0; a < nn; ++a) dv[m.T[nn * k + a]] = 1;
    }
  for (i64 v = 0; v < m.nv(); ++v)
    if (dv[v]) o.defectVerts.push_back(v);
  return o;
}

}  // namespace b0
