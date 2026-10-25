/*!
 * \file CNativeCurvedReference.hpp
 * \brief Curved wall reference shared by native 2D and 3D adaptation (DC-18): one curved-authority interface, a 3D
 *        curved PN-triangle evaluator (Vlachos 2001) with per-side normals across creases, a 2D curved polyline
 *        (cubic Hermite/Bezier per edge, per-side tangents at feature vertices), discrete principal curvatures
 *        (Rusinkiewicz 2004 in 3D, cubic + circumcircle in 2D) and the curvature size rule of the wall metric.
 * \details Design notes: integration_evidence/native_3d_bl_v1/dc18_prep/design.txt.
 *  - Every quantity is computed once at construction from the immutable original wall facets (quadric fits use
 *    up to three face rings; distributed component identities require separate coordination)
 *    and every query is a const, thread-safe function of (element, parameters) or (point, hint): no global state,
 *    no caches, so each rank/thread can evaluate any replicated or halo-complete wall.
 *  - Edge curves are owned by the edge (canonical orientation), so two elements sharing an edge evaluate the same
 *    curve: the surface is G0 (watertight) across every edge, bitwise on the edge itself.
 *  - Creases (turn > feature angle, marker junctions, open borders) keep one normal per side: the crease curve is
 *    built from crease tangents only, both sides share it, and the two sides keep their own tangent planes.
 *  - Planar markers (symmetry planes, any orientation) evaluate on their fitted plane: they stay exact planes.
 *  - Projections are restricted to a component (body) and, with a hint, to a local window: they never jump to another
 *    body across a narrow gap. A non-converged projection stays on its curve and is flagged; it never invents a closer flat foot.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace SU2NativeCurved {

/*--- Small fixed-size vector algebra (dimension template). ---*/
template <int D>
using Vec = std::array<double, D>;
template <int D>
inline Vec<D> Add(const Vec<D>& a, const Vec<D>& b) {
  Vec<D> r;
  for (int i = 0; i < D; ++i) r[i] = a[i] + b[i];
  return r;
}
template <int D>
inline Vec<D> Sub(const Vec<D>& a, const Vec<D>& b) {
  Vec<D> r;
  for (int i = 0; i < D; ++i) r[i] = a[i] - b[i];
  return r;
}
template <int D>
inline Vec<D> Scale(const Vec<D>& a, double s) {
  Vec<D> r;
  for (int i = 0; i < D; ++i) r[i] = s * a[i];
  return r;
}
template <int D>
inline Vec<D> Axpy(const Vec<D>& y, double s, const Vec<D>& x) {  // y + s x
  Vec<D> r;
  for (int i = 0; i < D; ++i) r[i] = y[i] + s * x[i];
  return r;
}
template <int D>
inline double Dot(const Vec<D>& a, const Vec<D>& b) {
  double s = 0;
  for (int i = 0; i < D; ++i) s += a[i] * b[i];
  return s;
}
template <int D>
inline double Norm(const Vec<D>& a) {
  return std::sqrt(Dot<D>(a, a));
}
template <int D>
inline Vec<D> Unit(const Vec<D>& a) {
  const double n = Norm<D>(a);
  Vec<D> r{};
  if (n > 0 && std::isfinite(n)) r = Scale<D>(a, 1 / n);
  return r;
}
inline Vec<3> Cross(const Vec<3>& a, const Vec<3>& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
inline double Cross2(const Vec<2>& a, const Vec<2>& b) { return a[0] * b[1] - a[1] * b[0]; }

enum class HandleRule {
  PN,     /*!< \brief Vlachos: handle = |chord projected on the tangent| / 3 = L cos(a) / 3. */
  Circle  /*!< \brief Circle-consistent handle 2 L / (3 (1 + cos a)): reproduces circular arcs to O(a^6). */
};
enum class NormalWeight { Area, Angle, Max /*!< \brief Max 1999: cross / (|e1|^2 |e2|^2), exact on spheres. */ };

/*!
 * \brief Cubic Bezier curve of one reference edge (shared by 2D curves and the boundary curves of 3D PN patches).
 */
template <int D>
struct EdgeCubic {
  std::array<Vec<D>, 4> b{};
  bool straight = true;
  Vec<D> Eval(double t) const {
    const double s = 1 - t;
    Vec<D> r;
    for (int i = 0; i < D; ++i) r[i] = s * s * s * b[0][i] + 3 * s * s * t * b[1][i] + 3 * s * t * t * b[2][i] + t * t * t * b[3][i];
    return r;
  }
  Vec<D> D1(double t) const {
    const double s = 1 - t;
    Vec<D> r;
    for (int i = 0; i < D; ++i)
      r[i] = 3 * (s * s * (b[1][i] - b[0][i]) + 2 * s * t * (b[2][i] - b[1][i]) + t * t * (b[3][i] - b[2][i]));
    return r;
  }
  Vec<D> D2(double t) const {
    Vec<D> r;
    for (int i = 0; i < D; ++i)
      r[i] = 6 * ((1 - t) * (b[2][i] - 2 * b[1][i] + b[0][i]) + t * (b[3][i] - 2 * b[2][i] + b[1][i]));
    return r;
  }
  /*! Convex-hull bound of |curve(t) - chord(t)| (degree-elevated chord control points). */
  double Bulge() const {
    double m = 0;
    for (int k = 1; k <= 2; ++k) m = std::max(m, Norm<D>(Sub<D>(b[k], Axpy<D>(b[0], k / 3.0, Sub<D>(b[3], b[0])))));
    return m;
  }
};

/*! Unit tangent at a for the chord a->b and the normal n at a: the chord projected on the tangent plane. */
template <int D>
inline Vec<D> TangentFromNormal(const Vec<D>& chord, const Vec<D>& n) {
  return Unit<D>(Axpy<D>(chord, -Dot<D>(chord, n), n));
}

/*!
 * \brief Edge cubic from unit tangents ta at a and tb at b, both oriented from a towards b. A missing, reversed or
 *        (nearly) orthogonal tangent gives the straight edge (exact chord).
 */
template <int D>
inline EdgeCubic<D> MakeEdgeCubic(const Vec<D>& a, const Vec<D>& b, const Vec<D>& ta, const Vec<D>& tb, HandleRule rule,
                                  double minimumCosine = 0.05) {
  EdgeCubic<D> c;
  const auto chord = Sub<D>(b, a);
  const double L = Norm<D>(chord);
  c.b = {a, Axpy<D>(a, 1 / 3.0, chord), Axpy<D>(a, 2 / 3.0, chord), b};
  if (!(L > 0)) return c;
  const double ca = Dot<D>(ta, chord) / L, cb = Dot<D>(tb, chord) / L;
  if (!(ca > minimumCosine && cb > minimumCosine && std::abs(Norm<D>(ta) - 1) < 1e-6 && std::abs(Norm<D>(tb) - 1) < 1e-6))
    return c;
  auto handle = [&](double cosine) { return rule == HandleRule::PN ? L * cosine / 3 : 2 * L / (3 * (1 + cosine)); };
  c.b[1] = Axpy<D>(a, handle(ca), ta);
  c.b[2] = Axpy<D>(b, -handle(cb), tb);
  c.straight = false;
  return c;
}

/*! Tangent at the middle point a of the circle through c, a, b, oriented towards b (exact on circles, second order on
 *  smooth curves): t ~ |c-a|^2 (b-a) - |b-a|^2 (c-a). Collinear points give the chord direction. */
template <int D>
inline Vec<D> CircleTangent(const Vec<D>& c, const Vec<D>& a, const Vec<D>& b) {
  const auto u = Sub<D>(c, a), w = Sub<D>(b, a);
  auto t = Unit<D>(Sub<D>(Scale<D>(w, Dot<D>(u, u)), Scale<D>(u, Dot<D>(w, w))));
  if (!(Norm<D>(t) > 0)) t = Unit<D>(w);
  return t;
}
/*! Curvature of the circle through three points (0 when collinear). */
template <int D>
inline double CircleCurvature(const Vec<D>& c, const Vec<D>& a, const Vec<D>& b) {
  const auto u = Sub<D>(c, a), w = Sub<D>(b, a), x = Sub<D>(b, c);
  const double lu = Norm<D>(u), lw = Norm<D>(w), lx = Norm<D>(x);
  if (!(lu > 0 && lw > 0 && lx > 0)) return 0;
  const double cosine = std::clamp(Dot<D>(u, w) / (lu * lw), -1.0, 1.0);
  return 2 * std::sqrt(std::max(0.0, 1 - cosine * cosine)) / lx;  // 2 sin(angle at a) / |b - c|
}

/*!
 * \brief Curvature size rule of the wall (shared 2D/3D): L(kappa) = clamp(min(c sqrt(2 R h0), 2 sqrt(eps (2R - eps)),
 *        hmax), hmin, hmax), R = 1 / |kappa|. The first term is the TWO_PASS rule (CReferenceWall SizeRule, chord
 *        height L^2 / (8 R) = c^2 h0 / 4 of the first height); the second the chord-height rule L <= sqrt(8 eps R).
 */
struct CurvatureSizeRule {
  double h0 = 0;              /*!< \brief First height (0: term off). */
  double factor = 0.7;        /*!< \brief c (TWO_PASS default). */
  double chordTolerance = 0;  /*!< \brief Native policy: ADAP_HAUSD with h0=0 (independent of first height). */
  double hmin = 0;            /*!< \brief Smallest size (e.g. max(ADAP_HMIN, 2 h0)). */
  double hmax = std::numeric_limits<double>::infinity();
  double Size(double curvature, bool* conflict = nullptr) const {
    if (!(std::isfinite(curvature) && std::isfinite(h0) && h0 >= 0 && std::isfinite(factor) && factor > 0 &&
          std::isfinite(chordTolerance) && chordTolerance >= 0 && std::isfinite(hmin) && hmin >= 0 && hmax > 0 && hmax >= hmin))
      throw std::invalid_argument("Invalid curvature size rule.");
    const double k = std::abs(curvature);
    double L = hmax;
    if (k > 0 && std::isfinite(k)) {
      const double R = 1 / k;
      if (h0 > 0) L = std::min(L, factor * std::sqrt(2 * R * h0));
      if (chordTolerance > 0) {
        const double e = std::min(chordTolerance, R);
        L = std::min(L, 2 * std::sqrt(e * (2 * R - e)));
      }
    }
    if (conflict) *conflict = L < hmin;
    return std::max(L, hmin);
  }
  /*! Chord height of a chord of length L on radius R (exact). */
  static double ChordHeight(double L, double R) {
    if (!(std::isfinite(L) && L >= 0 && std::isfinite(R) && R > 0))
      throw std::invalid_argument("Invalid chord length or radius.");
    const double q = .5 * (L / R);
    return q >= 1 ? R : (.5 * L) * q / (1 + std::sqrt((1 - q) * (1 + q)));
  }
};

/*! Principal curvatures at a wall point: |k1| >= |k2| (2D: k2 = 0, d1 = tangent), plus the discrete turn guard. */
struct Principal {
  double k1 = 0, k2 = 0;
  Vec<3> d1{}, d2{};
  double guard = 0;  /*!< \brief Largest turn curvature 2 sin(theta/2)/L of the smooth element edges (coarse-wall guard). */
};
/*! Tangential sizes of the curvature rule along d1, d2 (guard optional: a coarse wall cannot hide a small radius). */
inline std::array<double, 2> CurvatureSizes(const Principal& k, const CurvatureSizeRule& rule, bool useGuard = true) {
  return {rule.Size(useGuard ? std::max(std::abs(k.k1), k.guard) : k.k1), rule.Size(k.k2)};
}
/*! Curvature metric (xx, xy, xz, yy, yz, zz): d1 d1^T / h1^2 + d2 d2^T / h2^2 + n n^T / hmax^2 (no normal demand). */
inline std::array<double, 6> CurvatureMetric(const Principal& k, const Vec<3>& normal, const CurvatureSizeRule& rule,
                                             bool useGuard = true) {
  const auto h = CurvatureSizes(k, rule, useGuard);
  const double ln = std::isfinite(rule.hmax) ? 1 / (rule.hmax * rule.hmax) : 0;
  const double l1 = std::isfinite(h[0]) ? 1 / (h[0] * h[0]) : 0, l2 = std::isfinite(h[1]) ? 1 / (h[1] * h[1]) : 0;
  std::array<double, 6> m{};
  const int I[6] = {0, 0, 0, 1, 1, 2}, J[6] = {0, 1, 2, 1, 2, 2};
  for (int q = 0; q < 6; ++q)
    m[q] = l1 * k.d1[I[q]] * k.d1[J[q]] + l2 * k.d2[I[q]] * k.d2[J[q]] + ln * normal[I[q]] * normal[J[q]];
  return m;
}

/*! Closest point on the curved reference. */
template <int D>
struct Foot {
  Vec<D> point{}, normal{};
  double distance = std::numeric_limits<double>::infinity();
  int component = -1;
  long element = -1;  /*!< \brief 2D: edge; 3D: triangle. */
  double u = 0, v = 0;  /*!< \brief 2D: t in [0,1]; 3D: barycentric (u, v) of vertices 1, 2. */
  int feature = 0;     /*!< \brief 0 smooth, 1 on a crease (3D) / feature vertex (2D), 2 corner. */
  bool fallback = false; /*!< \brief Flattened patch or unconverged curved foot; callers must check before admission. */
};
/*! Search restriction of a projection. component < 0: any; element >= 0: a window of `rings` element rings around it
 *  (3D) or of `rings` edges on both sides (2D) inside its component. */
struct SearchHint {
  int component = -1;
  long element = -1;
  int rings = 2;
};

/*!
 * \brief The curved-authority interface shared by 2D and 3D.
 */
template <int D>
class CurvedReference {
 public:
  virtual ~CurvedReference() = default;
  virtual size_t Elements() const = 0;
  virtual int Component(long element) const = 0;
  virtual int Marker(long element) const = 0;
  /*! Point of the element at parameters (2D: t = u; 3D: barycentric u, v of the second and third vertex). */
  virtual Vec<D> Evaluate(long element, double u, double v = 0) const = 0;
  /*! Unit normal of the element's own side (per-side at creases), oriented like the input facet normal. */
  virtual Vec<D> Normal(long element, double u, double v = 0) const = 0;
  virtual Principal Curvature(long element, double u, double v = 0) const = 0;
  /*! 0 smooth vertex, 1 crease/feature vertex, 2 corner. */
  virtual int VertexFeature(long vertex) const = 0;
  virtual Foot<D> Project(const Vec<D>& p, const SearchHint& hint = {}) const = 0;
};

namespace detail {
struct UnionFind {
  std::vector<long> p;
  explicit UnionFind(size_t n) : p(n) { std::iota(p.begin(), p.end(), 0); }
  long Find(long x) {
    while (p[x] != x) x = p[x] = p[p[x]];
    return x;
  }
  void Join(long a, long b) {
    a = Find(a);
    b = Find(b);
    if (a != b) p[std::max(a, b)] = std::min(a, b);
  }
};
using Sym3 = std::array<double, 6>;  // xx xy xz yy yz zz
inline Sym3 SymOuter(const Vec<3>& a, const Vec<3>& b) {  // (a b^T + b a^T) / 2
  return {a[0] * b[0], .5 * (a[0] * b[1] + a[1] * b[0]), .5 * (a[0] * b[2] + a[2] * b[0]), a[1] * b[1],
          .5 * (a[1] * b[2] + a[2] * b[1]), a[2] * b[2]};
}
inline Vec<3> SymMul(const Sym3& m, const Vec<3>& x) {
  return {m[0] * x[0] + m[1] * x[1] + m[2] * x[2], m[1] * x[0] + m[3] * x[1] + m[4] * x[2],
          m[2] * x[0] + m[4] * x[1] + m[5] * x[2]};
}
/*! Shortest rotation taking unit a to unit b, applied to x. */
inline Vec<3> Rotate(const Vec<3>& a, const Vec<3>& b, const Vec<3>& x) {
  const auto v = Cross(a, b);
  const double c = Dot<3>(a, b);
  if (c < -1 + 1e-12) return Scale<3>(x, -1);  // antiparallel: not expected for sector normals
  const auto vx = Cross(v, x);
  return Axpy<3>(Axpy<3>(x, 1, vx), 1 / (1 + c), Cross(v, vx));
}
/*! Tangent frame of a unit normal. */
inline void Frame(const Vec<3>& n, Vec<3>& e1, Vec<3>& e2) {
  const int axis = std::abs(n[0]) <= std::abs(n[1]) ? (std::abs(n[0]) <= std::abs(n[2]) ? 0 : 2) : (std::abs(n[1]) <= std::abs(n[2]) ? 1 : 2);
  Vec<3> a{};
  a[axis] = 1;
  e1 = Unit<3>(Cross(n, a));
  e2 = Cross(n, e1);
}
/*! Principal values/directions of the tangential part of a symmetric tensor at the unit normal n. */
inline void TangentEigen(const Sym3& K, const Vec<3>& n, Principal& out) {
  Vec<3> e1, e2;
  Frame(n, e1, e2);
  const double a = Dot<3>(e1, SymMul(K, e1)), b = Dot<3>(e1, SymMul(K, e2)), c = Dot<3>(e2, SymMul(K, e2));
  const double m = .5 * (a + c), r = std::hypot(.5 * (a - c), b);
  const double angle = .5 * std::atan2(2 * b, a - c);
  const auto dp = Axpy<3>(Scale<3>(e1, std::cos(angle)), std::sin(angle), e2);
  const auto dm = Cross(n, dp);
  const double lp = m + r, lm = m - r;
  if (std::abs(lp) >= std::abs(lm)) { out.k1 = lp; out.d1 = dp; out.k2 = lm; out.d2 = dm; }
  else { out.k1 = lm; out.d1 = dm; out.k2 = lp; out.d2 = dp; }
}
/*! Closest point of the triangle (a, b, c) to p (Ericson), barycentric weights of b and c. */
inline Vec<3> ClosestOnTriangle(const Vec<3>& p, const Vec<3>& a, const Vec<3>& b, const Vec<3>& c, double& u, double& v) {
  const auto ab = Sub<3>(b, a), ac = Sub<3>(c, a), ap = Sub<3>(p, a);
  const double d1 = Dot<3>(ab, ap), d2 = Dot<3>(ac, ap);
  if (d1 <= 0 && d2 <= 0) { u = v = 0; return a; }
  const auto bp = Sub<3>(p, b);
  const double d3 = Dot<3>(ab, bp), d4 = Dot<3>(ac, bp);
  if (d3 >= 0 && d4 <= d3) { u = 1; v = 0; return b; }
  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) { const double t = d1 / (d1 - d3); u = t; v = 0; return Axpy<3>(a, t, ab); }
  const auto cp = Sub<3>(p, c);
  const double d5 = Dot<3>(ab, cp), d6 = Dot<3>(ac, cp);
  if (d6 >= 0 && d5 <= d6) { u = 0; v = 1; return c; }
  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) { const double t = d2 / (d2 - d6); u = 0; v = t; return Axpy<3>(a, t, ac); }
  const double va = d3 * d6 - d5 * d4;
  if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
    const double t = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    u = 1 - t; v = t;
    return Axpy<3>(b, t, Sub<3>(c, b));
  }
  const double den = 1 / (va + vb + vc);
  u = vb * den; v = vc * den;
  return Axpy<3>(Axpy<3>(a, u, ab), v, ac);
}
inline void ClampSimplex(double& u, double& v) {
  u = std::max(0.0, u); v = std::max(0.0, v);
  if (u + v > 1) {
    const double e = .5 * (u + v - 1);
    u = std::clamp(u - e, 0.0, 1.0);
    v = 1 - u;  // exactly on the edge: 1 - u - v evaluates to 0
  }
}
}  // namespace detail

/*!
 * \brief 3D curved reference: one cubic PN triangle per original wall facet (Vlachos 2001), boundary curves owned by
 *        the edges, vertex normals per smooth sector (creases keep both sides), crease curves from crease tangents,
 *        fitted exact planes for planar markers, Rusinkiewicz curvature tensors per vertex sector.
 */
class PNTriangleSurface : public CurvedReference<3> {
 public:
  struct Triangle {
    std::array<long, 3> v{};
    int marker = 0;
  };
  struct Options {
    double featureAngle = 45;       /*!< \brief ADAP_ANGLE (degrees): smooth-edge turn above it is a crease. */
    NormalWeight weight = NormalWeight::Max;
    HandleRule handle = HandleRule::PN;
    std::set<int> fixedMarkers;     /*!< \brief Keep unselected boundaries faceted, including their shared edge curves. */
    std::set<int> planarMarkers;    /*!< \brief Markers that are exact planes (symmetry kinds): fitted, kept flat. */
    double planarTolerance = 1e-9;  /*!< \brief Relative planarity contract of a planar marker component. */
    bool reflectivePlanes = true;   /*!< \brief Planar markers are mirror planes (MARKER_SYM): a wall sector meeting one
                                         (nearly) orthogonally gets n' = n - (n.s)s, the symmetry column rule. */
    bool projectPlanar = false;     /*!< \brief Unsupported: fit shared coordinates before constructing this authority. */
    bool featureQuadric = true;     /*!< \brief One-sided quadric normals/curvature at crease and border sectors. */
    bool markerJunctionIsCrease = true;
    double foldCosine = 0.2;        /*!< \brief Patch normal . facet normal below it at a sample: the patch is unsafe. */
    double bulgeFraction = 0.5;     /*!< \brief Control-net displacement above it x the longest edge: unsafe. */
  };
  struct Stats {
    size_t creaseEdges = 0, borderEdges = 0, junctionEdges = 0, cornerVertices = 0, creaseVertices = 0;
    size_t fallbackFaces = 0, straightEdges = 0, planarComponents = 0, rejectedPlanar = 0, components = 0;
    size_t inconsistentEdges = 0;  /*!< \brief Interior edges used twice in the same direction (orientation). */
    size_t quadricSectors = 0;     /*!< \brief Crease/border sectors with the one-sided quadric normal and curvature. */
    size_t mirroredSectors = 0;    /*!< \brief Sectors whose normal was put in a reflective plane. */
  };

  PNTriangleSurface(const std::vector<Vec<3>>& points, const std::vector<Triangle>& triangles, const Options& options)
      : x(points), tri(triangles), opt(options) {
    Build();
  }

  size_t Elements() const override { return tri.size(); }
  int Component(long f) const override { return component[f]; }
  int Marker(long f) const override { return tri[f].marker; }
  const Stats& Statistics() const { return stats; }
  const Options& Settings() const { return opt; }
  bool FallbackFace(long f) const { return fallbackFace[f]; }
  bool PlanarFace(long f) const { return planeOf[f] >= 0; }
  Vec<3> FacetNormal(long f) const { return facetNormal[f]; }
  /*! Rigorous bound of |patch(u,v) - facet(u,v)| (control-net convex hull). */
  double Bulge(long f) const { return bulge[f]; }
  const std::vector<Vec<3>>& Points() const { return x; }
  const std::vector<Triangle>& Triangles() const { return tri; }
  const std::vector<std::array<Vec<3>,10>>& ControlNets() const { return net; }

  int VertexFeature(long vertex) const override { return vertexFeature[vertex]; }

  Vec<3> Evaluate(long f, double u, double v) const override {
    const double w = 1 - u - v;
    const auto& t = tri[f].v;
    // On an edge or vertex: the canonical edge curve (bitwise identical from both sides).
    if (u == 0 && v == 0) return x[t[0]];
    if (w == 0 && v == 0) return x[t[1]];
    if (w == 0 && u == 0) return x[t[2]];
    if (v == 0) return EdgeEval(f, 0, u);
    if (w == 0) return EdgeEval(f, 1, v);
    if (u == 0) return EdgeEval(f, 2, 1 - v);
    return Patch(f, w, u, v);
  }
  /*! Partial derivatives of the patch in (u, v). */
  void Derivatives(long f, double u, double v, Vec<3>& Su, Vec<3>& Sv) const {
    const double w = 1 - u - v;
    const auto& b = net[f];
    Vec<3> dw{}, du{}, dv{};
    for (int i = 0; i < 3; ++i) {
      dw[i] = 3 * w * w * b[0][i] + 6 * w * u * b[3][i] + 3 * u * u * b[4][i] + 3 * v * v * b[7][i] + 6 * w * v * b[8][i] + 6 * u * v * b[9][i];
      du[i] = 3 * u * u * b[1][i] + 3 * w * w * b[3][i] + 6 * w * u * b[4][i] + 6 * u * v * b[5][i] + 3 * v * v * b[6][i] + 6 * w * v * b[9][i];
      dv[i] = 3 * v * v * b[2][i] + 3 * u * u * b[5][i] + 6 * u * v * b[6][i] + 6 * w * v * b[7][i] + 3 * w * w * b[8][i] + 6 * w * u * b[9][i];
    }
    Su = Sub<3>(du, dw);
    Sv = Sub<3>(dv, dw);
  }
  Vec<3> Normal(long f, double u, double v) const override {
    if (opt.fixedMarkers.count(tri[f].marker)) return facetNormal[f];
    if (planeOf[f] >= 0) return planes[planeOf[f]].second;
    Vec<3> Su, Sv;
    Derivatives(f, u, v, Su, Sv);
    auto n = Unit<3>(Cross(Su, Sv));
    if (!(Norm<3>(n) > 0)) return facetNormal[f];
    return Dot<3>(n, facetNormal[f]) < 0 ? Scale<3>(n, -1) : n;
  }
  /*! Normal of the smooth sector of face f at its local corner k (per-side at creases). */
  Vec<3> CornerNormal(long f, int k) const { return sectorNormal[sector[3 * f + k]]; }

  Principal Curvature(long f, double u, double v) const override {
    Principal out;
    if (opt.fixedMarkers.count(tri[f].marker)) { detail::TangentEigen(detail::Sym3{}, facetNormal[f], out); return out; }
    const double w[3] = {1 - u - v, u, v};
    const auto n = Normal(f, u, v);
    detail::Sym3 K{};
    for (int k = 0; k < 3; ++k) {
      const long s = sector[3 * f + k];
      // tensors live in their vertex tangent plane: rotate to this normal before averaging
      const auto& T = sectorK[s];
      const auto& ns = sectorNormal[s];
      Vec<3> e1, e2;
      detail::Frame(ns, e1, e2);
      const double a = Dot<3>(e1, detail::SymMul(T, e1)), b = Dot<3>(e1, detail::SymMul(T, e2)), c = Dot<3>(e2, detail::SymMul(T, e2));
      const auto r1 = detail::Rotate(ns, n, e1), r2 = detail::Rotate(ns, n, e2);
      const auto A = detail::SymOuter(r1, r1), B = detail::SymOuter(r1, r2), C = detail::SymOuter(r2, r2);
      for (int q = 0; q < 6; ++q) K[q] += w[k] * (a * A[q] + 2 * b * B[q] + c * C[q]);
    }
    detail::TangentEigen(K, n, out);
    out.guard = guard[f];
    return out;
  }
  /*! Curvature tensor (3D, tangential) of a vertex sector and its principal values. */
  Principal SectorCurvature(long f, int k) const {
    Principal out;
    if (opt.fixedMarkers.count(tri[f].marker)) { detail::TangentEigen(detail::Sym3{}, facetNormal[f], out); return out; }
    const long s = sector[3 * f + k];
    detail::TangentEigen(sectorK[s], sectorNormal[s], out);
    out.guard = guard[f];
    return out;
  }

  /*!
   * \brief Closest point. Candidates: the faces of the hint window (same component) or of the component (or all);
   *        each face is skipped when its facet distance minus its bulge bound cannot beat the best foot.
   */
  Foot<3> Project(const Vec<3>& p, const SearchHint& hint = {}) const override {
    if (!std::all_of(p.begin(), p.end(), [](double q) { return std::isfinite(q); }) ||
        hint.element < -1 || hint.element >= long(tri.size()) || hint.component < -1 ||
        hint.component >= int(stats.components) || hint.rings < 0 ||
        (hint.element >= 0 && hint.component >= 0 && component[hint.element] != hint.component))
      throw std::invalid_argument("Invalid curved surface projection point or hint.");
    std::vector<long> candidates;
    if (hint.element >= 0) candidates = Window(hint.element, hint.rings);
    else
      for (long f = 0; f < long(tri.size()); ++f)
        if (hint.component < 0 || component[f] == hint.component) candidates.push_back(f);
    std::vector<std::pair<double, long>> order;
    order.reserve(candidates.size());
    for (long f : candidates) {
      double u, v;
      const auto q = detail::ClosestOnTriangle(p, x[tri[f].v[0]], x[tri[f].v[1]], x[tri[f].v[2]], u, v);
      order.emplace_back(std::max(0.0, Norm<3>(Sub<3>(p, q)) - bulge[f]), f);
    }
    std::sort(order.begin(), order.end());
    Foot<3> best;
    for (const auto& c : order) {
      if (c.first > best.distance) break;
      const auto foot = ProjectOnFace(p, c.second);
      if (foot.distance < best.distance) best = foot;
    }
    return best;
  }
  /*! Projection on one patch: Gauss-Newton in (u, v) clamped to the simplex, from the facet foot. */
  Foot<3> ProjectOnFace(const Vec<3>& p, long f) const {
    Foot<3> foot;
    foot.element = f;
    foot.component = component[f];
    double u, v;
    const auto flat = detail::ClosestOnTriangle(p, x[tri[f].v[0]], x[tri[f].v[1]], x[tri[f].v[2]], u, v);
    bool converged = fallbackFace[f];
    if (!fallbackFace[f]) converged = Minimize(p, f, u, v);
    if (fallbackFace[f]) {
      foot.fallback = true;
      foot.point = flat;
      foot.normal = facetNormal[f];
      double uu, vv;
      detail::ClosestOnTriangle(p, x[tri[f].v[0]], x[tri[f].v[1]], x[tri[f].v[2]], uu, vv);
      foot.u = uu; foot.v = vv;
    } else {
      foot.fallback = !converged;
      // snap rounding-level parameters onto the boundary so feature feet use the canonical edge curve
      const double tiny = 1e-13;
      if (u < tiny) u = 0;
      if (v < tiny) v = 0;
      if (1 - u - v < tiny) { const double s = u + v; u /= s; v /= s; v = 1 - u; }
      foot.u = u; foot.v = v;
      foot.point = Evaluate(f, u, v);
      foot.normal = Normal(f, u, v);
    }
    foot.distance = Norm<3>(Sub<3>(p, foot.point));
    // feature classification of the foot
    const double w = 1 - foot.u - foot.v;
    const auto& t = tri[f].v;
    int zeros = (w <= 0) + (foot.u <= 0) + (foot.v <= 0);
    if (zeros >= 2) {
      const long vtx = (foot.u >= 1) ? t[1] : (foot.v >= 1) ? t[2] : t[0];
      foot.feature = vertexFeature[vtx];
    } else if (zeros == 1) {
      const int k = (foot.v <= 0) ? 0 : (w <= 0) ? 1 : 2;
      foot.feature = edgeInfo[faceEdge[3 * f + k]].crease ? 1 : 0;
    }
    return foot;
  }
  /*!
   * \brief Closest point of patch f to p: Gauss-Newton with backtracking in the simplex, an exact 1D Newton along an
   *        active edge (the edge curve), and a KKT check that re-enters the interior when that decreases the distance.
   */
  bool Minimize(const Vec<3>& p, long f, double& u, double& v) const {
    auto F = [&](double a, double b) { const auto r = Sub<3>(Evaluate(f, a, b), p); return Dot<3>(r, r); };
    auto stationary = [&] {
      Vec<3> Su, Sv; Derivatives(f, u, v, Su, Sv);
      const auto r = Sub<3>(Evaluate(f, u, v), p);
      const double gu = Dot<3>(Su, r), gv = Dot<3>(Sv, r), norm2 = Dot<3>(Su, Su) + Dot<3>(Sv, Sv);
      const double tolerance = 1e-7 * std::sqrt(Dot<3>(r, r) * norm2) + 64 * std::numeric_limits<double>::epsilon() * norm2;
      // KKT against all three feasible vertex directions, including active edges and corners.
      const double base = -u * gu - v * gv;
      return std::isfinite(tolerance) && std::min({base, base + gu, base + gv}) >= -tolerance;
    };
    for (int outer = 0; outer < 6; ++outer) {
      for (int it = 0; it < 100; ++it) {
        Vec<3> Su, Sv;
        Derivatives(f, u, v, Su, Sv);
        const auto r = Sub<3>(Evaluate(f, u, v), p);
        const double a = Dot<3>(Su, Su), b = Dot<3>(Su, Sv), c = Dot<3>(Sv, Sv), gu = Dot<3>(Su, r), gv = Dot<3>(Sv, r);
        const double det = a * c - b * b;
        if (!(det > 0)) return false;
        const double du = -(c * gu - b * gv) / det, dv = -(a * gv - b * gu) / det, f0 = Dot<3>(r, r);
        double lambda = 1, nu = u, nv = v;
        bool descent = false;
        for (int ls = 0; ls < 40; ++ls, lambda *= .5) {
          nu = u + lambda * du; nv = v + lambda * dv;
          detail::ClampSimplex(nu, nv);
          if (F(nu, nv) <= f0 * (1 + 1e-13)) { descent = true; break; }  // F resolves the foot only to sqrt(eps)
        }
        if (!descent) break;
        const double step = std::abs(nu - u) + std::abs(nv - v);
        u = nu; v = nv;
        if (step < 1e-15) break;
      }
      // near-boundary iterates (backtracking stalls against a constraint) are snapped onto it for the edge phase
      const double snap = 1e-10;
      if (u <= snap) u = 0;
      if (v <= snap) v = 0;
      if (1 - u - v <= snap) detail::ClampSimplex(u = std::max(u, 0.0), v = 1 - u + 1e-300);
      const double w = 1 - u - v;
      if (u > 0 && v > 0 && w > 0) return stationary();
      // edge phase on every active edge: parameter s, point (u, v) = A + s (B - A)
      double bu = u, bv = v, best = F(u, v);
      auto tryEdge = [&](double ua, double va, double ub, double vb, double s) {
        for (int it = 0; it < 100; ++it) {
          const double cu = ua + s * (ub - ua), cv = (ua + ub == 1 && va + vb == 1) ? 1 - cu : va + s * (vb - va);
          Vec<3> Su, Sv;
          Derivatives(f, cu, cv, Su, Sv);
          const auto d = Axpy<3>(Scale<3>(Su, ub - ua), vb - va, Sv);
          const auto r = Sub<3>(Evaluate(f, cu, cv), p);
          const double dd = Dot<3>(d, d), f0 = Dot<3>(r, r);
          if (!(dd > 0)) break;
          double lambda = 1, ns = s;
          bool descent = false;
          for (int ls = 0; ls < 40; ++ls, lambda *= .5) {
            ns = std::clamp(s - lambda * Dot<3>(d, r) / dd, 0.0, 1.0);
            const double tu = ua + ns * (ub - ua), tv = (ua + ub == 1 && va + vb == 1) ? 1 - tu : va + ns * (vb - va);
            if (F(tu, tv) <= f0 * (1 + 1e-13)) { descent = true; break; }
          }
          if (!descent) break;
          const double step = std::abs(ns - s);
          s = ns;
          if (step < 1e-15) break;
        }
        const double su = ua + s * (ub - ua), sv = (ua + ub == 1 && va + vb == 1) ? 1 - su : va + s * (vb - va), val = F(su, sv);
        if (val <= best) { best = val; bu = su; bv = sv; }
      };
      if (u <= 0) tryEdge(0, 0, 0, 1, std::clamp(v, 0.0, 1.0));  // corner 0 -> corner 2
      if (v <= 0) tryEdge(0, 0, 1, 0, std::clamp(u, 0.0, 1.0));  // corner 0 -> corner 1
      if (w <= 0) tryEdge(1, 0, 0, 1, std::clamp(v, 0.0, 1.0));  // corner 1 -> corner 2
      u = std::max(0.0, bu); v = std::max(0.0, bv);
      if (u + v > 1) detail::ClampSimplex(u, v);
      // KKT: does a step into the interior decrease the distance?
      Vec<3> Su, Sv;
      Derivatives(f, u, v, Su, Sv);
      const auto r = Sub<3>(Evaluate(f, u, v), p);
      const double gu = Dot<3>(Su, r), gv = Dot<3>(Sv, r);
      const double scale = 1e-12 * std::sqrt(Dot<3>(r, r) * (Dot<3>(Su, Su) + Dot<3>(Sv, Sv))) + 1e-300;
      const double ww = 1 - u - v;
      // a vertex: both incident edges were minimized, so no feasible descent remains
      if ((u <= 0) + (v <= 0) + (ww <= 0) >= 2) return stationary();
      const bool inward = (u <= 0 && gu < -scale) || (v <= 0 && gv < -scale) || (ww <= 0 && gu + gv > scale);
      if (!inward) return stationary();
      u = std::max(u, 1e-9); v = std::max(v, 1e-9);
      if (u + v > 1 - 1e-9) { const double t = (1 - 1e-9) / (u + v); u *= t; v *= t; }
    }
    return stationary();
  }
  /*! Faces within `rings` vertex rings of face f, same component. */
  std::vector<long> Window(long f, int rings) const {
    std::set<long> faces{f}, frontier{f};
    for (int r = 0; r < rings; ++r) {
      std::set<long> next;
      for (long g : frontier)
        for (long vtx : tri[g].v)
          for (long h : vertexFaces[vtx])
            if (component[h] == component[f] && faces.insert(h).second) next.insert(h);
      frontier.swap(next);
    }
    return {faces.begin(), faces.end()};
  }
  /*!
   * \brief Certified deviation bound of the flat child triangle with vertices at the parameters r0, r1, r2 of face f
   *        (barycentric (w,u,v)) from the patch over that child: control net of the sub-patch by blossoming, then the
   *        convex hull. Returns {bound of |S - T| at corresponding parameters, bound of the normal deviation}.
   */
  std::array<double, 2> ChildDeviation(long f, const Vec<3>& r0, const Vec<3>& r1, const Vec<3>& r2) const {
    std::array<Vec<3>, 10> d;
    const int ijk[10][3] = {{3, 0, 0}, {0, 3, 0}, {0, 0, 3}, {2, 1, 0}, {1, 2, 0}, {0, 2, 1}, {0, 1, 2}, {1, 0, 2}, {2, 0, 1}, {1, 1, 1}};
    for (int q = 0; q < 10; ++q) {
      std::array<Vec<3>, 3> args;
      int m = 0;
      for (int c = 0; c < 3; ++c)
        for (int r = 0; r < ijk[q][c]; ++r) args[m++] = c == 0 ? r0 : c == 1 ? r1 : r2;
      d[q] = Blossom(f, args);
    }
    // Planarity does not remove in-plane displacement or the residual of a noisy fitted plane.
    const auto n = Unit<3>(Cross(Sub<3>(d[1], d[0]), Sub<3>(d[2], d[0])));
    double full = 0, normal = 0;
    for (int q = 0; q < 10; ++q) {
      Vec<3> lin{};
      for (int i = 0; i < 3; ++i) lin[i] = (ijk[q][0] * d[0][i] + ijk[q][1] * d[1][i] + ijk[q][2] * d[2][i]) / 3;
      full = std::max(full, Norm<3>(Sub<3>(d[q], lin)));
      normal = std::max(normal, std::abs(Dot<3>(n, Sub<3>(d[q], d[0]))));
    }
    return {full, normal};
  }
  /*! Largest gap between the two sides of every interior edge at `samples` parameters (watertightness audit). */
  double MaximumEdgeGap(int samples = 9) const {
    double gap = 0;
    for (const auto& e : edgeInfo) {
      if (e.faces.size() != 2) continue;
      for (int s = 1; s < samples; ++s) {
        const double t = double(s) / samples;
        Vec<3> p[2];
        for (int side = 0; side < 2; ++side) {
          const long f = e.faces[side];
          int k = 0;
          while (faceEdge[3 * f + k] != e.id) ++k;
          // local parameter along the face's own edge k (from corner k to corner k+1)
          const bool forward = tri[f].v[k] == e.a;
          const double tl = forward ? t : 1 - t;
          const double bw[3] = {k == 0 ? 1 - tl : k == 2 ? tl : 0, k == 0 ? tl : k == 1 ? 1 - tl : 0, k == 1 ? tl : k == 2 ? 1 - tl : 0};
          p[side] = Patch(f, bw[0], bw[1], bw[2]);  // raw patch (not the canonical edge path): audits the nets
        }
        gap = std::max(gap, Norm<3>(Sub<3>(p[0], p[1])));
      }
    }
    return gap;
  }

 private:
  struct EdgeData {
    long id = 0, a = 0, b = 0;  // a < b canonical
    std::vector<long> faces;
    bool crease = false, straight = false;
    EdgeCubic<3> cubic;
  };
  std::vector<Vec<3>> x;
  std::vector<Triangle> tri;
  Options opt;
  Stats stats;
  std::vector<Vec<3>> facetNormal;
  std::vector<double> facetArea, bulge, guard;
  std::vector<EdgeData> edgeInfo;
  std::vector<long> faceEdge;   // 3 per face: edge k joins corners k, k+1
  std::vector<long> sector;     // 3 per face: sector of corner k
  std::vector<Vec<3>> sectorNormal;
  std::vector<detail::Sym3> sectorK;
  std::vector<long> sectorVertex;
  std::vector<std::vector<long>> vertexFaces, vertexCreases;
  std::vector<int> vertexFeature, component;
  std::vector<long> planeOf;
  std::vector<char> mirrored;
  std::vector<std::pair<Vec<3>, Vec<3>>> planes;  // origin, unit normal
  std::vector<std::array<Vec<3>, 10>> net;
  std::vector<char> fallbackFace;
  std::vector<char> sectorQuadric;
  std::vector<detail::Sym3> sectorQK;

  Vec<3> Patch(long f, double w, double u, double v) const {
    const auto& b = net[f];
    const double c[10] = {w * w * w, u * u * u, v * v * v, 3 * w * w * u, 3 * w * u * u, 3 * u * u * v, 3 * u * v * v, 3 * w * v * v, 3 * w * w * v, 6 * w * u * v};
    Vec<3> p{};
    for (int q = 0; q < 10; ++q) p = Axpy<3>(p, c[q], b[q]);
    return p;
  }
  /*! Point of local edge k of face f at local parameter t (corner k -> corner k+1) by the canonical edge curve. */
  Vec<3> EdgeEval(long f, int k, double t) const {
    const auto& e = edgeInfo[faceEdge[3 * f + k]];
    return e.cubic.Eval(tri[f].v[k] == e.a ? t : 1 - t);
  }
  Vec<3> Blossom(long f, const std::array<Vec<3>, 3>& args) const {
    // de Casteljau with three (possibly different) barycentric arguments
    std::vector<Vec<3>> level(net[f].begin(), net[f].end());
    auto at3 = [&](int i, int j, int k) {
      if (i == 3) return 0; if (j == 3) return 1; if (k == 3) return 2;
      if (i == 2 && j == 1) return 3; if (i == 1 && j == 2) return 4; if (j == 2 && k == 1) return 5;
      if (j == 1 && k == 2) return 6; if (i == 1 && k == 2) return 7; if (i == 2 && k == 1) return 8; return 9;
    };
    // level 2: c_{ijk}, i+j+k=2
    std::map<std::array<int, 3>, Vec<3>> l2, l1;
    for (int i = 0; i <= 2; ++i)
      for (int j = 0; i + j <= 2; ++j) {
        const int k = 2 - i - j;
        const auto& r = args[0];
        Vec<3> p = Scale<3>(level[at3(i + 1, j, k)], r[0]);
        p = Axpy<3>(p, r[1], level[at3(i, j + 1, k)]);
        p = Axpy<3>(p, r[2], level[at3(i, j, k + 1)]);
        l2[{i, j, k}] = p;
      }
    for (int i = 0; i <= 1; ++i)
      for (int j = 0; i + j <= 1; ++j) {
        const int k = 1 - i - j;
        const auto& r = args[1];
        Vec<3> p = Scale<3>(l2[{i + 1, j, k}], r[0]);
        p = Axpy<3>(p, r[1], l2[{i, j + 1, k}]);
        p = Axpy<3>(p, r[2], l2[{i, j, k + 1}]);
        l1[{i, j, k}] = p;
      }
    const auto& r = args[2];
    return Axpy<3>(Axpy<3>(Scale<3>(l1[{1, 0, 0}], r[0]), r[1], l1[{0, 1, 0}]), r[2], l1[{0, 0, 1}]);
  }

  void Build() {
    const size_t nf = tri.size(), nv = x.size();
    if (nf == 0 || !(std::isfinite(opt.featureAngle) && opt.featureAngle > 0 && opt.featureAngle <= 180) ||
        !(std::isfinite(opt.planarTolerance) && opt.planarTolerance >= 0) ||
        !(std::isfinite(opt.foldCosine) && opt.foldCosine > 0 && opt.foldCosine <= 1) ||
        !(std::isfinite(opt.bulgeFraction) && opt.bulgeFraction > 0))
      throw std::invalid_argument("Invalid curved surface options or empty surface.");
    if (opt.projectPlanar)
      throw std::invalid_argument("Curved surface: fit shared plane coordinates before construction; face-only projection opens seams.");
    for (const auto& p : x) for (double q : p)
      if (!std::isfinite(q)) throw std::invalid_argument("Curved reference: nonfinite point.");
    const double cosFeature = std::cos(opt.featureAngle * std::acos(-1.0) / 180);
    facetNormal.resize(nf);
    facetArea.resize(nf);
    vertexFaces.assign(nv, {});
    for (size_t f = 0; f < nf; ++f) {
      const auto& t = tri[f].v;
      for (long q : t)
        if (q < 0 || size_t(q) >= nv) throw std::invalid_argument("Curved reference: vertex index out of range.");
      const auto c = Cross(Sub<3>(x[t[1]], x[t[0]]), Sub<3>(x[t[2]], x[t[0]]));
      facetArea[f] = .5 * Norm<3>(c);
      if (!(facetArea[f] > 0 && std::isfinite(facetArea[f]))) throw std::invalid_argument("Curved reference: degenerate facet.");
      facetNormal[f] = Unit<3>(c);
      for (long q : t) vertexFaces[q].push_back(long(f));
    }
    // Reflective carrier planes must not connect separate wall bodies through their shared boundary.
    {
      detail::UnionFind uf(nf);
      for (const auto& faces : vertexFaces) {
        long first[3] = {-1, -1, -1};
        for (long f : faces) {
          const int planar = opt.fixedMarkers.count(tri[f].marker) ? 2 : opt.planarMarkers.count(tri[f].marker) != 0;
          if (first[planar] < 0) first[planar] = f;
          else uf.Join(first[planar], f);
        }
      }
      std::map<long, int> id;
      component.resize(nf);
      for (size_t f = 0; f < nf; ++f) {
        const auto r = uf.Find(long(f));
        auto it = id.find(r);
        if (it == id.end()) it = id.emplace(r, int(id.size())).first;
        component[f] = it->second;
      }
      stats.components = id.size();
    }
    // edges
    std::map<std::pair<long, long>, long> edgeId;
    faceEdge.assign(3 * nf, -1);
    std::map<std::pair<long, long>, int> directed;
    for (size_t f = 0; f < nf; ++f)
      for (int k = 0; k < 3; ++k) {
        const long a = tri[f].v[k], b = tri[f].v[(k + 1) % 3];
        ++directed[{a, b}];
        const auto key = std::make_pair(std::min(a, b), std::max(a, b));
        auto it = edgeId.find(key);
        if (it == edgeId.end()) {
          EdgeData e;
          e.id = long(edgeInfo.size());
          e.a = key.first;
          e.b = key.second;
          edgeInfo.push_back(e);
          it = edgeId.emplace(key, e.id).first;
        }
        edgeInfo[it->second].faces.push_back(long(f));
        faceEdge[3 * f + k] = it->second;
      }
    for (const auto& d : directed)
      if (d.second > 1) ++stats.inconsistentEdges;
    vertexCreases.assign(nv, {});
    for (auto& e : edgeInfo) {
      if (e.faces.size() != 2) { e.crease = true; ++stats.borderEdges; }
      else {
        const long f1 = e.faces[0], f2 = e.faces[1];
        if (tri[f1].marker != tri[f2].marker && opt.markerJunctionIsCrease) { e.crease = true; ++stats.junctionEdges; }
        else if (Dot<3>(facetNormal[f1], facetNormal[f2]) < cosFeature) e.crease = true;
      }
      if (e.crease) { ++stats.creaseEdges; vertexCreases[e.a].push_back(e.id); vertexCreases[e.b].push_back(e.id); }
    }
    // planar marker components: fitted planes (area-weighted normal, vertex centroid), within the contract
    planeOf.assign(nf, -1);
    {
      double scale = 1;
      for (const auto& p : x) scale = std::max({scale, std::abs(p[0]), std::abs(p[1]), std::abs(p[2])});
      std::map<std::pair<int, int>, std::vector<long>> groups;
      for (size_t f = 0; f < nf; ++f)
        if (opt.planarMarkers.count(tri[f].marker)) groups[{tri[f].marker, component[f]}].push_back(long(f));
      // planar markers form their own components only through marker-restricted connectivity
      for (auto& g : groups) {
        detail::UnionFind uf(g.second.size());
        std::map<long, std::vector<size_t>> at;
        for (size_t i = 0; i < g.second.size(); ++i)
          for (long q : tri[g.second[i]].v) at[q].push_back(i);
        for (const auto& row : at)
          for (size_t i = 1; i < row.second.size(); ++i) uf.Join(long(row.second[0]), long(row.second[i]));
        std::map<long, std::vector<long>> parts;
        for (size_t i = 0; i < g.second.size(); ++i) parts[uf.Find(long(i))].push_back(g.second[i]);
        for (const auto& part : parts) {
          Vec<3> n{}, c{};
          std::set<long> verts;
          for (long f : part.second) { n = Axpy<3>(n, facetArea[f], facetNormal[f]); for (long q : tri[f].v) verts.insert(q); }
          n = Unit<3>(n);
          for (long q : verts) c = Axpy<3>(c, 1.0 / verts.size(), x[q]);
          double worst = 0;
          for (long q : verts) worst = std::max(worst, std::abs(Dot<3>(n, Sub<3>(x[q], c))));
          if (worst > opt.planarTolerance * scale) { ++stats.rejectedPlanar; continue; }
          planes.emplace_back(c, n);
          ++stats.planarComponents;
          for (long f : part.second) planeOf[f] = long(planes.size() - 1);
        }
      }
    }
    // smooth sectors per vertex and their normals
    sector.assign(3 * nf, -1);
    for (size_t vtx = 0; vtx < nv; ++vtx) {
      const auto& faces = vertexFaces[vtx];
      if (faces.empty()) continue;
      detail::UnionFind uf(faces.size());
      for (size_t i = 0; i < faces.size(); ++i)
        for (size_t j = i + 1; j < faces.size(); ++j)
          for (int k = 0; k < 3; ++k) {
            const long e = faceEdge[3 * faces[i] + k];
            if (edgeInfo[e].crease || (edgeInfo[e].a != long(vtx) && edgeInfo[e].b != long(vtx))) continue;
            const auto& ef = edgeInfo[e].faces;
            if (std::find(ef.begin(), ef.end(), faces[j]) != ef.end()) uf.Join(long(i), long(j));
          }
      std::map<long, long> sid;
      for (size_t i = 0; i < faces.size(); ++i) {
        const long r = uf.Find(long(i));
        auto it = sid.find(r);
        if (it == sid.end()) {
          it = sid.emplace(r, long(sectorNormal.size())).first;
          sectorNormal.push_back({});
          sectorVertex.push_back(long(vtx));
        }
        const long f = faces[i];
        int k = 0;
        while (tri[f].v[k] != long(vtx)) ++k;
        sector[3 * f + k] = it->second;
        const auto e1 = Sub<3>(x[tri[f].v[(k + 1) % 3]], x[vtx]), e2 = Sub<3>(x[tri[f].v[(k + 2) % 3]], x[vtx]);
        const double l1 = Norm<3>(e1), l2 = Norm<3>(e2);
        double weight = 1;
        if (opt.weight == NormalWeight::Area) weight = facetArea[f];
        if (opt.weight == NormalWeight::Angle) weight = std::acos(std::clamp(Dot<3>(e1, e2) / (l1 * l2), -1.0, 1.0));
        if (opt.weight == NormalWeight::Max) weight = 2 * facetArea[f] / (l1 * l1 * l2 * l2);
        auto& n = sectorNormal[it->second];
        n = Axpy<3>(n, weight, facetNormal[f]);
      }
    }
    for (size_t s = 0; s < sectorNormal.size(); ++s) sectorNormal[s] = Unit<3>(sectorNormal[s]);
    // planar sectors take their plane normal
    for (size_t f = 0; f < nf; ++f)
      if (planeOf[f] >= 0)
        for (int k = 0; k < 3; ++k) {
          const auto& n = planes[planeOf[f]].second;
          sectorNormal[sector[3 * f + k]] = Dot<3>(n, facetNormal[f]) < 0 ? Scale<3>(n, -1) : n;
        }
    // mirror rule at reflective planes: one-sided sector normals of a wall meeting the plane orthogonally lie in it
    if (opt.reflectivePlanes && !planes.empty()) {
      std::vector<std::set<long>> vertexPlanes(nv);
      for (size_t f = 0; f < nf; ++f)
        if (planeOf[f] >= 0)
          for (long q : tri[f].v) vertexPlanes[q].insert(planeOf[f]);
      const double limit = std::sin(.5 * opt.featureAngle * std::acos(-1.0) / 180);
      std::set<long> done;
      mirrored.assign(sectorNormal.size(), 0);
      for (size_t f = 0; f < nf; ++f) {
        if (planeOf[f] >= 0) continue;
        for (int k = 0; k < 3; ++k) {
          const long sid = sector[3 * f + k];
          if (!done.insert(sid).second) continue;
          for (long pl : vertexPlanes[tri[f].v[k]]) {
            auto& n = sectorNormal[sid];
            const auto& sn = planes[pl].second;
            const double c = Dot<3>(n, sn);
            if (std::abs(c) < limit) { n = Unit<3>(Axpy<3>(n, -c, sn)); mirrored[sid] = 1; ++stats.mirroredSectors; }
          }
        }
      }
    }
    // vertex feature class: 0 smooth, 1 regular crease vertex (two crease edges, smooth crease turn), 2 corner
    vertexFeature.assign(nv, 0);
    for (size_t vtx = 0; vtx < nv; ++vtx) {
      const auto& c = vertexCreases[vtx];
      if (c.empty()) continue;
      int cls = 2;
      if (c.size() == 2) {
        const long p = Other(c[0], long(vtx)), q = Other(c[1], long(vtx));
        const auto u = Unit<3>(Sub<3>(x[vtx], x[p])), w = Unit<3>(Sub<3>(x[q], x[vtx]));
        if (Dot<3>(u, w) >= cosFeature) cls = 1;
      }
      vertexFeature[vtx] = cls;
      if (cls == 2) ++stats.cornerVertices; else ++stats.creaseVertices;
    }
    if (mirrored.size() != sectorNormal.size()) mirrored.assign(sectorNormal.size(), 0);
    if (opt.featureQuadric) FitFeatureSectors(mirrored);
    else { sectorQuadric.assign(sectorNormal.size(), 0); sectorQK.assign(sectorNormal.size(), detail::Sym3{}); }
    // edge curves, then patches; unsafe patches straighten their edges (and neighbours rebuild) until stable
    fallbackFace.assign(nf, 0);
    for (auto& e : edgeInfo) {
      for (long f : e.faces) e.straight = e.straight || opt.fixedMarkers.count(tri[f].marker);
      BuildEdge(e);
    }
    net.resize(nf);
    bulge.assign(nf, 0);
    for (size_t f = 0; f < nf; ++f) BuildNet(long(f));
    for (int round = 0; round < 16; ++round) {
      std::vector<long> bad;
      for (size_t f = 0; f < nf; ++f)
        if (!fallbackFace[f] && !SafePatch(long(f))) bad.push_back(long(f));
      if (bad.empty()) break;
      std::set<long> touched;
      for (long f : bad) {
        fallbackFace[f] = 1;
        for (int k = 0; k < 3; ++k) {
          auto& e = edgeInfo[faceEdge[3 * f + k]];
          if (!e.straight) { e.straight = true; BuildEdge(e); }
          for (long g : e.faces) touched.insert(g);
        }
      }
      for (long g : touched) BuildNet(g);
    }
    for (size_t f = 0; f < nf; ++f) stats.fallbackFaces += fallbackFace[f];
    for (const auto& e : edgeInfo) stats.straightEdges += e.cubic.straight;
    BuildCurvature();
  }
  /*! One-sided normals and curvature at crease/border vertex sectors: a quadric height fit z = d x + e y + a x^2 +
   *  b x y + c y^2 over two smooth face rings of the sector (never across a crease), refitted in the corrected frame.
   *  A sector-average normal there is biased by O(h), which turns into an O(1) curvature error (DC-18 tests). */
  void FitFeatureSectors(const std::vector<char>& skip) {
    sectorQuadric.assign(sectorNormal.size(), 0);
    sectorQK.assign(sectorNormal.size(), detail::Sym3{});
    std::vector<std::vector<long>> sectorFaces(sectorNormal.size());
    for (size_t f = 0; f < tri.size(); ++f)
      for (int k = 0; k < 3; ++k) sectorFaces[sector[3 * f + k]].push_back(long(f));
    for (size_t sid = 0; sid < sectorNormal.size(); ++sid) {
      const long v = sectorVertex[sid];
      if (vertexFeature[v] == 0 || skip[sid]) continue;
      bool planar = false;
      for (long f : sectorFaces[sid]) planar = planar || planeOf[f] >= 0;
      if (planar) continue;
      std::set<long> faces(sectorFaces[sid].begin(), sectorFaces[sid].end()), frontier = faces;
      for (int depth = 0; depth < (vertexFeature[v] == 2 ? 3 : 2); ++depth) {  // corners: one more ring (one-quadrant data)
        std::set<long> next;
        for (long f : frontier)
          for (int k = 0; k < 3; ++k) {
            const auto& e = edgeInfo[faceEdge[3 * f + k]];
            if (e.crease) continue;
            for (long g : e.faces) if (faces.insert(g).second) next.insert(g);
          }
        frontier.swap(next);
      }
      std::set<long> verts;
      for (long f : faces) for (long q : tri[f].v) if (q != v) verts.insert(q);
      if (verts.size() < 7) continue;
      Vec<3> n = sectorNormal[sid];
      double A[5] = {0}, ok = true;
      for (int pass = 0; pass < 3 && ok; ++pass) {
        Vec<3> e1, e2;
        detail::Frame(n, e1, e2);
        double M[5][6] = {{0}};
        for (long q : verts) {
          const auto d = Sub<3>(x[q], x[v]);
          const double px = Dot<3>(d, e1), py = Dot<3>(d, e2), pz = Dot<3>(d, n);
          const double r2 = px * px + py * py;
          if (!(r2 > 0)) continue;
          const double w = 1 / r2;  // nearer points dominate
          const double row[5] = {px, py, px * px, px * py, py * py};
          for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) M[i][j] += w * row[i] * row[j];
            M[i][5] += w * row[i] * pz;
          }
        }
        // Gaussian elimination with partial pivoting; a near-singular system keeps the original sector data
        double scale = 0;
        for (int i = 0; i < 5; ++i) scale = std::max(scale, std::abs(M[i][i]));
        for (int c = 0; c < 5 && ok; ++c) {
          int piv = c;
          for (int r = c + 1; r < 5; ++r) if (std::abs(M[r][c]) > std::abs(M[piv][c])) piv = r;
          if (!(std::abs(M[piv][c]) > 1e-12 * scale)) { ok = false; break; }
          for (int j = 0; j < 6; ++j) std::swap(M[c][j], M[piv][j]);
          for (int r = 0; r < 5; ++r) {
            if (r == c) continue;
            const double m = M[r][c] / M[c][c];
            for (int j = c; j < 6; ++j) M[r][j] -= m * M[c][j];
          }
        }
        if (!ok) break;
        for (int i = 0; i < 5; ++i) A[i] = M[i][5] / M[i][i];
        if (pass < 2) n = Unit<3>(Axpy<3>(Axpy<3>(n, -A[0], e1), -A[1], e2));
        else {
          // curvature tensor (dn = K dx convention): K = -[[2a, b], [b, 2c]] in the final frame
          const auto P = detail::SymOuter(e1, e1), Q = detail::SymOuter(e1, e2), R = detail::SymOuter(e2, e2);
          for (int q = 0; q < 6; ++q) sectorQK[sid][q] = -(2 * A[2] * P[q] + 2 * A[3] * Q[q] + 2 * A[4] * R[q]);
        }
      }
      if (!ok || Dot<3>(n, sectorNormal[sid]) < std::cos(std::acos(-1.0) / 6)) continue;  // > 30 deg: keep the original
      sectorNormal[sid] = n;
      sectorQuadric[sid] = 1;
      ++stats.quadricSectors;
    }
  }
  long Other(long edge, long vtx) const { return edgeInfo[edge].a == vtx ? edgeInfo[edge].b : edgeInfo[edge].a; }
  /*! Crease tangent at vertex vtx of crease edge e, oriented away from vtx along e. */
  Vec<3> CreaseTangent(const EdgeData& e, long vtx) const {
    const long far = e.a == vtx ? e.b : e.a;
    const auto chord = Sub<3>(x[far], x[vtx]);
    Vec<3> t = Unit<3>(chord);
    if (vertexFeature[vtx] == 1) {
      const auto& c = vertexCreases[vtx];
      const long other = c[0] == e.id ? c[1] : c[0];
      t = CircleTangent<3>(x[Other(other, vtx)], x[vtx], x[far]);
    }
    // planar sides: the crease curve stays in their planes
    for (long f : e.faces)
      if (planeOf[f] >= 0) t = Unit<3>(Axpy<3>(t, -Dot<3>(t, planes[planeOf[f]].second), planes[planeOf[f]].second));
    return t;
  }
  void BuildEdge(EdgeData& e) {
    const auto& a = x[e.a];
    const auto& b = x[e.b];
    if (e.straight) { e.cubic = MakeEdgeCubic<3>(a, b, {}, {}, opt.handle); return; }
    Vec<3> ta, tb;
    if (e.crease) {
      ta = CreaseTangent(e, e.a);
      tb = Scale<3>(CreaseTangent(e, e.b), -1);
    } else {
      const long f = e.faces[0];
      int ka = 0, kb = 0;
      while (tri[f].v[ka] != e.a) ++ka;
      while (tri[f].v[kb] != e.b) ++kb;
      const auto chord = Sub<3>(b, a);
      ta = TangentFromNormal<3>(chord, sectorNormal[sector[3 * f + ka]]);
      tb = TangentFromNormal<3>(chord, sectorNormal[sector[3 * f + kb]]);
    }
    e.cubic = MakeEdgeCubic<3>(a, b, ta, tb, opt.handle);
  }
  void BuildNet(long f) {
    auto& b = net[f];
    const auto& t = tri[f].v;
    b[0] = x[t[0]]; b[1] = x[t[1]]; b[2] = x[t[2]];
    // edge k joins corners k -> k+1; control points near corner k and near corner k+1
    auto inner = [&](int k, bool nearStart) {
      const auto& e = edgeInfo[faceEdge[3 * f + k]];
      const bool forward = t[k] == e.a;
      return e.cubic.b[(forward == nearStart) ? 1 : 2];
    };
    b[3] = inner(0, true);   // b210
    b[4] = inner(0, false);  // b120
    b[5] = inner(1, true);   // b021
    b[6] = inner(1, false);  // b012
    b[7] = inner(2, true);   // b102
    b[8] = inner(2, false);  // b201
    Vec<3> E{}, V{};
    for (int q = 3; q < 9; ++q) E = Axpy<3>(E, 1 / 6.0, b[q]);
    for (int q = 0; q < 3; ++q) V = Axpy<3>(V, 1 / 3.0, b[q]);
    b[9] = opt.fixedMarkers.count(tri[f].marker) ? V : Axpy<3>(E, .5, Sub<3>(E, V));
    const int ijk[10][3] = {{3, 0, 0}, {0, 3, 0}, {0, 0, 3}, {2, 1, 0}, {1, 2, 0}, {0, 2, 1}, {0, 1, 2}, {1, 0, 2}, {2, 0, 1}, {1, 1, 1}};
    double m = 0;
    for (int q = 0; q < 10; ++q) {
      Vec<3> lin{};
      for (int i = 0; i < 3; ++i) lin[i] = (ijk[q][0] * b[0][i] + ijk[q][1] * b[1][i] + ijk[q][2] * b[2][i]) / 3;
      m = std::max(m, Norm<3>(Sub<3>(b[q], lin)));
    }
    bulge[f] = m * (1 + 1e-12) + 1e-300;
  }
  bool SafePatch(long f) const {
    const auto& t = tri[f].v;
    double L = 0;
    for (int k = 0; k < 3; ++k) L = std::max(L, Norm<3>(Sub<3>(x[t[(k + 1) % 3]], x[t[k]])));
    if (bulge[f] > opt.bulgeFraction * L) return false;
    const int n = 6;
    for (int i = 0; i <= n; ++i)
      for (int j = 0; i + j <= n; ++j) {
        const double u = double(i) / n, v = double(j) / n;
        Vec<3> Su, Sv;
        Derivatives(f, u, v, Su, Sv);
        const auto c = Cross(Su, Sv);
        const double len = Norm<3>(c);
        if (!(len > 0)) {
          // degenerate derivative only at a vertex of a crease fan is acceptable when the facet normal is consistent
          if ((i == 0 && j == 0) || i == n || j == n) continue;
          return false;
        }
        if (Dot<3>(c, facetNormal[f]) / len < opt.foldCosine) return false;
      }
    return true;
  }
  void BuildCurvature() {
    const size_t nf = tri.size();
    sectorK.assign(sectorNormal.size(), detail::Sym3{});
    std::vector<double> weight(sectorNormal.size(), 0);
    guard.assign(nf, 0);
    for (size_t f = 0; f < nf; ++f) {
      const auto& t = tri[f].v;
      Vec<3> p[3], n[3];
      for (int k = 0; k < 3; ++k) { p[k] = x[t[k]]; n[k] = sectorNormal[sector[3 * f + k]]; }
      // turn guard over the smooth edges
      for (int k = 0; k < 3; ++k) {
        const auto& e = edgeInfo[faceEdge[3 * f + k]];
        if (e.crease) continue;
        const double L = Norm<3>(Sub<3>(p[(k + 1) % 3], p[k]));
        const double c = std::clamp(Dot<3>(n[k], n[(k + 1) % 3]), -1.0, 1.0);
        guard[f] = std::max(guard[f], 2 * std::sin(.5 * std::acos(c)) / L);
      }
      if (planeOf[f] >= 0) continue;
      // Rusinkiewicz: II from the normal differences along the three edges, in the face frame
      const Vec<3> e[3] = {Sub<3>(p[2], p[1]), Sub<3>(p[0], p[2]), Sub<3>(p[1], p[0])};
      const Vec<3> dn[3] = {Sub<3>(n[2], n[1]), Sub<3>(n[0], n[2]), Sub<3>(n[1], n[0])};
      const auto nf3 = facetNormal[f];
      const auto tu = Unit<3>(e[0]);
      const auto tv = Cross(nf3, tu);
      double A[3][3] = {{0}}, r[3] = {0};
      for (int k = 0; k < 3; ++k) {
        const double eu = Dot<3>(e[k], tu), ev = Dot<3>(e[k], tv);
        const double nu = Dot<3>(dn[k], tu), nv = Dot<3>(dn[k], tv);
        // rows [eu ev 0] -> nu and [0 eu ev] -> nv
        const double rows[2][3] = {{eu, ev, 0}, {0, eu, ev}};
        const double rhs[2] = {nu, nv};
        for (int q = 0; q < 2; ++q)
          for (int i = 0; i < 3; ++i) {
            r[i] += rows[q][i] * rhs[q];
            for (int j = 0; j < 3; ++j) A[i][j] += rows[q][i] * rows[q][j];
          }
      }
      double s[3];
      if (!Solve3(A, r, s)) continue;
      // face tensor in 3D: e tu tu + f (tu tv + tv tu) + g tv tv
      const auto Tu = detail::SymOuter(tu, tu), Tm = detail::SymOuter(tu, tv), Tv = detail::SymOuter(tv, tv);
      detail::Sym3 K;
      for (int q = 0; q < 6; ++q) K[q] = s[0] * Tu[q] + 2 * s[1] * Tm[q] + s[2] * Tv[q];
      // mixed Voronoi weights (Meyer)
      double wv[3];
      VoronoiWeights(p, wv);
      for (int k = 0; k < 3; ++k) {
        const long sid = sector[3 * f + k];
        const auto& ns = sectorNormal[sid];
        // rotate the face tensor into the vertex tangent plane
        Vec<3> e1, e2;
        detail::Frame(nf3, e1, e2);
        const double a = Dot<3>(e1, detail::SymMul(K, e1)), b = Dot<3>(e1, detail::SymMul(K, e2)), c = Dot<3>(e2, detail::SymMul(K, e2));
        const auto r1 = detail::Rotate(nf3, ns, e1), r2 = detail::Rotate(nf3, ns, e2);
        const auto R11 = detail::SymOuter(r1, r1), R12 = detail::SymOuter(r1, r2), R22 = detail::SymOuter(r2, r2);
        for (int q = 0; q < 6; ++q) sectorK[sid][q] += wv[k] * (a * R11[q] + 2 * b * R12[q] + c * R22[q]);
        weight[sid] += wv[k];
      }
    }
    for (size_t s = 0; s < sectorK.size(); ++s)
      if (weight[s] > 0)
        for (auto& q : sectorK[s]) q /= weight[s];
    for (size_t s = 0; s < sectorK.size(); ++s)
      if (sectorQuadric[s]) sectorK[s] = sectorQK[s];
  }
  static bool Solve3(double A[3][3], const double r[3], double s[3]) {
    const double det = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                       A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    if (!(std::abs(det) > 0)) return false;
    for (int c = 0; c < 3; ++c) {
      double M[3][3];
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) M[i][j] = j == c ? r[i] : A[i][j];
      s[c] = (M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
              M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0])) / det;
    }
    return true;
  }
  static void VoronoiWeights(const Vec<3> p[3], double w[3]) {
    const double area = .5 * Norm<3>(Cross(Sub<3>(p[1], p[0]), Sub<3>(p[2], p[0])));
    double ang[3], l2[3];
    for (int k = 0; k < 3; ++k) {
      const auto a = Sub<3>(p[(k + 1) % 3], p[k]), b = Sub<3>(p[(k + 2) % 3], p[k]);
      ang[k] = std::acos(std::clamp(Dot<3>(a, b) / (Norm<3>(a) * Norm<3>(b)), -1.0, 1.0));
      l2[k] = Dot<3>(Sub<3>(p[(k + 2) % 3], p[(k + 1) % 3]), Sub<3>(p[(k + 2) % 3], p[(k + 1) % 3]));  // opposite edge
    }
    const double half = std::acos(0.0);
    const bool obtuse = ang[0] > half || ang[1] > half || ang[2] > half;
    for (int k = 0; k < 3; ++k) {
      if (obtuse) { w[k] = ang[k] > half ? area / 2 : area / 4; continue; }
      const int i = (k + 1) % 3, j = (k + 2) % 3;
      // |p_k p_j|^2 cot(angle at i) + |p_k p_i|^2 cot(angle at j), /8
      w[k] = (l2[i] / std::tan(ang[i]) + l2[j] / std::tan(ang[j])) / 8;
    }
  }
};

/*!
 * \brief 2D curved reference: chains of wall edges (one per closed body or open chain, independent of markers), split
 *        logically at feature vertices (open ends, marker junctions, turn > feature angle, e.g. cusps and sharp trailing
 *        edges); one cubic per edge from per-side vertex tangents (circumcircle tangent at smooth vertices, one-sided
 *        circle tangent at features). Arc parameter = cumulative chord length (drop-in for PolylineReference::At).
 */
class CurvedPolyline : public CurvedReference<2> {
 public:
  struct Segment {
    long a = 0, b = 0;
    int marker = 0;
  };
  struct Options {
    double featureAngle = 45;
    HandleRule handle = HandleRule::Circle;
    bool markerJunctionIsFeature = true;
    std::set<long> forcedFeatures;  /*!< \brief Vertices that are features whatever their turn (e.g. inherited ones). */
  };
  struct Chain {
    bool closed = false;
    std::vector<long> nodes;      // closed: no repeat
    std::vector<long> edges;      // global edge ids in order
    std::vector<double> arc;      // cumulative chord length at nodes (closed: size nodes+1)
  };

  CurvedPolyline(const std::vector<Vec<2>>& points, const std::vector<Segment>& segments, const Options& options)
      : x(points), seg(segments), opt(options) {
    Build();
  }
  size_t Elements() const override { return edges.size(); }
  int Component(long e) const override { return edges[e].chain; }
  int Marker(long e) const override { return edges[e].marker; }
  const std::vector<Chain>& Chains() const { return chains; }
  int VertexFeature(long v) const override { return feature.count(v) ? 1 : 0; }
  /*! Turn angle (degrees) at a vertex (0 for open ends). */
  double TurnDegrees(long v) const { const auto it = turn.find(v); return it == turn.end() ? 0 : it->second; }
  const EdgeCubic<2>& Curve(long e) const { return edges[e].cubic; }
  long EdgeStart(long e) const { return edges[e].a; }
  long EdgeEnd(long e) const { return edges[e].b; }

  Vec<2> Evaluate(long e, double t, double = 0) const override {
    if (t == 0) return x[edges[e].a];
    if (t == 1) return x[edges[e].b];
    return edges[e].cubic.Eval(t);
  }
  Vec<2> Tangent(long e, double t) const { return Unit<2>(edges[e].cubic.D1(t)); }
  /*! Left normal of the chain direction (the fluid side for faces oriented with the fluid on the left). */
  Vec<2> Normal(long e, double t, double = 0) const override {
    const auto d = Tangent(e, t);
    return {-d[1], d[0]};
  }
  Principal Curvature(long e, double t, double = 0) const override {
    Principal k;
    const auto d1 = edges[e].cubic.D1(t), d2 = edges[e].cubic.D2(t);
    const double s = Norm<2>(d1);
    k.k1 = s > 0 ? Cross2(d1, d2) / (s * s * s) : 0;
    const auto u = Unit<2>(d1);
    k.d1 = {u[0], u[1], 0};
    k.d2 = {0, 0, 1};
    k.guard = std::max(edges[e].guardA, edges[e].guardB);
    return k;
  }
  Foot<2> Project(const Vec<2>& p, const SearchHint& hint = {}) const override {
    if (!std::all_of(p.begin(), p.end(), [](double q) { return std::isfinite(q); }) ||
        hint.element < -1 || hint.element >= long(edges.size()) || hint.component < -1 ||
        hint.component >= int(chains.size()) || hint.rings < 0 ||
        (hint.element >= 0 && hint.component >= 0 && edges[hint.element].chain != hint.component))
      throw std::invalid_argument("Invalid curved polyline projection point or hint.");
    std::vector<long> candidates;
    if (hint.element >= 0) {
      const auto& c = chains[edges[hint.element].chain];
      const long n = long(c.edges.size()), i = edges[hint.element].index;
      for (long d = -hint.rings; d <= hint.rings; ++d) {
        long j = i + d;
        if (c.closed) j = ((j % n) + n) % n;
        if (j >= 0 && j < n) candidates.push_back(c.edges[j]);
      }
      std::sort(candidates.begin(), candidates.end());
      candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    } else
      for (long e = 0; e < long(edges.size()); ++e)
        if (hint.component < 0 || edges[e].chain == hint.component) candidates.push_back(e);
    return ProjectOn(p, candidates);
  }
  /*! Projection on the edges [first, last] of a chain (cyclic for closed chains): never leaves that interval. */
  Foot<2> ProjectInterval(const Vec<2>& p, int chain, long first, long last) const {
    if (chain < 0 || chain >= int(chains.size()) ||
        !std::all_of(p.begin(), p.end(), [](double q) { return std::isfinite(q); }))
      throw std::invalid_argument("Invalid curved polyline projection interval.");
    const auto& c = chains[chain];
    const long n = long(c.edges.size());
    if (!c.closed && (first < 0 || last >= n || first > last))
      throw std::invalid_argument("Reversed or out-of-range open projection interval.");
    if (!c.closed) { first = std::max(first, 0L); last = std::min(last, n - 1); }
    std::vector<long> candidates;
    for (long j = first;; ++j) {
      const long k = c.closed ? ((j % n) + n) % n : j;
      if (k < 0 || k >= n) break;
      candidates.push_back(c.edges[k]);
      if (k == (c.closed ? ((last % n) + n) % n : last) || long(candidates.size()) > n) break;
    }
    return ProjectOn(p, candidates);
  }
  /*! Point at the arc parameter of a chain (PolylineReference::At on the curved edges). */
  Vec<2> At(int chain, double arc) const {
    const auto at = Locate(chain, arc);
    return Evaluate(at.first, at.second);
  }
  /*! Edge and local parameter of an arc parameter (closed chains wrap). */
  std::pair<long, double> Locate(int chain, double arc) const {
    if (chain < 0 || chain >= int(chains.size()) || !std::isfinite(arc))
      throw std::invalid_argument("Invalid curved polyline parameter.");
    const auto& c = chains[chain];
    const double total = c.arc.back();
    if (c.closed) { arc = std::fmod(arc, total); if (arc < 0) arc += total; }
    else arc = std::clamp(arc, 0.0, total);
    const size_t i = std::min(c.edges.size() - 1, size_t(std::upper_bound(c.arc.begin(), c.arc.end(), arc) - c.arc.begin() - 1));
    const double t = (arc - c.arc[i]) / (c.arc[i + 1] - c.arc[i]);
    return {c.edges[i], std::clamp(t, 0.0, 1.0)};
  }
  /*! Arc parameter of the closest point of a chain (PolylineReference::Parameter on the curved edges). */
  double Parameter(int chain, const Vec<2>& p) const {
    const auto foot = Project(p, {chain, -1, 0});
    if (foot.element < 0) throw std::invalid_argument("Empty curved polyline projection.");
    const auto& c = chains[chain];
    const long i = edges[foot.element].index;
    return c.arc[i] + foot.u * (c.arc[i + 1] - c.arc[i]);
  }
  /*! Oriented arc interval of a wall face (closed chains: the end may exceed the chain length). */
  std::pair<double, double> Interval(int chain, const Vec<2>& a, const Vec<2>& b) const {
    const double first = Parameter(chain, a);
    double last = Parameter(chain, b);
    if (chains[chain].closed && last <= first) last += chains[chain].arc.back();
    if (!(last > first)) throw std::invalid_argument("Curved reference interval reverses its chain orientation.");
    return {first, last};
  }
  /*! Exact deviation bound of the face [a, b] from the curve between their parameters (PolylineReference::Deviation). */
  double Deviation(int chain, const Vec<2>& a, const Vec<2>& b) const {
    const auto range = Interval(chain, a, b);
    const auto s = Locate(chain, range.first);
    auto e = Locate(chain, range.second);
    if (e.second == 0 && e.first != s.first) {  // an end exactly at a node: the end of the previous edge
      const auto& c = chains[chain];
      const long n = long(c.edges.size()), i = edges[e.first].index;
      e = {c.edges[size_t(c.closed ? (i - 1 + n) % n : std::max(0L, i - 1))], 1.0};
    }
    return ArcChordDistance(s.first, s.second, e.first, e.second, a, b);
  }
  /*! Curvature size L* at an arc parameter (the rule's maximum admissible edge length). */
  double TangentialSize(int chain, double arc, const CurvatureSizeRule& rule, bool* conflict = nullptr) const {
    const auto at = Locate(chain, arc);
    const auto k = Curvature(at.first, at.second);
    return rule.Size(std::max(std::abs(k.k1), k.guard), conflict);
  }
  /*!
   * \brief Exact bound of the distance between the curve from (e0, t0) to (e1, t1) (consecutive edges of one chain)
   *        and the finite chord [A, B]: per cubic piece the signed line distance and the chord parameter are cubics,
   *        their extremes come from the real roots of their derivatives (as CReferenceWall::ArcChordDistance).
   */
  double ArcChordDistance(long e0, double t0, long e1, double t1, const Vec<2>& A, const Vec<2>& B) const {
    if (e0 < 0 || e1 < 0 || e0 >= long(edges.size()) || e1 >= long(edges.size()) ||
        edges[e0].chain != edges[e1].chain || !(t0 >= 0 && t0 <= 1 && t1 >= 0 && t1 <= 1))
      throw std::invalid_argument("Invalid curved polyline deviation interval.");
    const auto& chain = chains[edges[e0].chain];
    if (!chain.closed && (edges[e1].index < edges[e0].index || (e0 == e1 && t1 < t0)))
      throw std::invalid_argument("Reversed open deviation interval.");
    const auto chord = Sub<2>(B, A);
    const double L = Norm<2>(chord);
    if (!(L > 0)) return std::numeric_limits<double>::infinity();
    const auto c = Scale<2>(chord, 1 / L);
    const Vec<2> n{-c[1], c[0]};
    const auto& ch = chains[edges[e0].chain];
    const long count = long(ch.edges.size());
    double worst2 = 0;
    long i = edges[e0].index;
    for (long guardCount = 0; guardCount <= count; ++guardCount) {
      const long e = ch.edges[i];
      const bool lastPiece = e == e1 && !(guardCount == 0 && e0 == e1 && t1 < t0);
      const double lo = guardCount == 0 ? t0 : 0, hi = lastPiece ? t1 : 1;
      if (hi > lo) {
        const auto& b = edges[e].cubic.b;
        double dmax2 = 0, omax = 0;
        auto cubicExtremes = [&](const Vec<2>& dir, double shift, auto use) {
          double q[4];
          for (int k = 0; k < 4; ++k) q[k] = Dot<2>(dir, Sub<2>(b[k], A)) - shift;
          // power basis of the Bernstein cubic
          const double a0 = q[0], a1 = 3 * (q[1] - q[0]), a2 = 3 * (q[2] - 2 * q[1] + q[0]), a3 = q[3] - 3 * q[2] + 3 * q[1] - q[0];
          auto f = [&](double t) { return ((a3 * t + a2) * t + a1) * t + a0; };
          use(f(lo)); use(f(hi));
          const double A2 = 3 * a3, A1 = 2 * a2, A0 = a1;  // derivative
          if (std::abs(A2) > 1e-300) {
            const double disc = A1 * A1 - 4 * A2 * A0;
            if (disc >= 0) for (double s : {(-A1 + std::sqrt(disc)) / (2 * A2), (-A1 - std::sqrt(disc)) / (2 * A2)}) if (s > lo && s < hi) use(f(s));
          } else if (std::abs(A1) > 1e-300) { const double s = -A0 / A1; if (s > lo && s < hi) use(f(s)); }
        };
        cubicExtremes(n, 0, [&](double d) { dmax2 = std::max(dmax2, d * d); });
        cubicExtremes(c, 0, [&](double u) { omax = std::max({omax, -u, u - L}); });
        double scale = 0;
        for (const auto& p : b) scale = std::max({scale, std::abs(p[0] - A[0]), std::abs(p[1] - A[1])});
        const double margin = 64 * std::numeric_limits<double>::epsilon() * (scale + L);
        worst2 = std::max(worst2, std::pow(std::sqrt(dmax2) + margin, 2) + std::pow(std::max(0.0, omax) + margin, 2));
      }
      if (lastPiece) break;
      i = ch.closed ? (i + 1) % count : i + 1;
      if (i >= count) break;
    }
    return std::sqrt(worst2);
  }

 private:
  struct EdgeData {
    long a = 0, b = 0;
    int marker = 0, chain = 0;
    long index = 0;  // position in its chain
    EdgeCubic<2> cubic;
    double guardA = 0, guardB = 0;
    double bulge = 0;
  };
  std::vector<Vec<2>> x;
  std::vector<Segment> seg;
  Options opt;
  std::vector<EdgeData> edges;
  std::vector<Chain> chains;
  std::set<long> feature;
  std::map<long, double> turn;

  Foot<2> ProjectOn(const Vec<2>& p, const std::vector<long>& candidates) const {
    std::vector<std::pair<double, long>> order;
    for (long e : candidates) {
      const auto& a = x[edges[e].a];
      const auto d = Sub<2>(x[edges[e].b], a);
      const double t = std::clamp(Dot<2>(Sub<2>(p, a), d) / Dot<2>(d, d), 0.0, 1.0);
      order.emplace_back(std::max(0.0, Norm<2>(Sub<2>(p, Axpy<2>(a, t, d))) - edges[e].bulge), e);
    }
    std::sort(order.begin(), order.end());
    Foot<2> best;
    for (const auto& c : order) {
      if (c.first > best.distance) break;
      const long e = c.second;
      const auto& cub = edges[e].cubic;
      // start from the best of nine samples (a cubic edge can hold two local minima), then Newton with
      // backtracking on the squared distance (accepted within rounding), clamped to [0, 1]
      auto F = [&](double t) { const auto r = Sub<2>(cub.Eval(t), p); return Dot<2>(r, r); };
      double t = 0, ft = F(0);
      for (int k = 1; k <= 8; ++k) { const double f = F(k / 8.0); if (f < ft) { ft = f; t = k / 8.0; } }
      bool ok = true;
      for (int it = 0; it < 100; ++it) {
        const auto r = Sub<2>(cub.Eval(t), p);
        const auto d1 = cub.D1(t), d2 = cub.D2(t);
        const double g = Dot<2>(r, d1), gn = Dot<2>(d1, d1);
        double h = gn + Dot<2>(r, d2);
        if (!(h > .25 * gn)) h = gn;
        if (!(h > 0)) break;
        const double f0 = Dot<2>(r, r);
        double lambda = 1, nt = t;
        bool descent = false;
        for (int ls = 0; ls < 40; ++ls, lambda *= .5) {
          nt = std::clamp(t - lambda * g / h, 0.0, 1.0);
          if (F(nt) <= f0 * (1 + 1e-13)) { descent = true; break; }
        }
        if (!descent) break;
        const double step = std::abs(nt - t);
        t = nt;
        if (step < 1e-15) break;
      }
      const auto residual = Sub<2>(cub.Eval(t), p), tangent = cub.D1(t);
      const double gradient = Dot<2>(residual, tangent), norm2 = Dot<2>(tangent, tangent);
      const double tolerance = 1e-7 * std::sqrt(Dot<2>(residual, residual) * norm2) +
                               64 * std::numeric_limits<double>::epsilon() * norm2;
      ok = std::isfinite(t) && std::isfinite(tolerance) &&
           std::min(-t * gradient, (1 - t) * gradient) >= -tolerance;
      Foot<2> foot;
      foot.element = e;
      foot.component = edges[e].chain;
      if (!std::isfinite(t)) throw std::runtime_error("Unrepresentable curved polyline projection.");
      foot.fallback = !ok;
      if (t < 1e-14) t = 0;
      if (t > 1 - 1e-14) t = 1;
      foot.point = Evaluate(e, t);
      foot.normal = Normal(e, t);
      foot.u = t;
      foot.distance = Norm<2>(Sub<2>(p, foot.point));
      if ((t == 0 && feature.count(edges[e].a)) || (t == 1 && feature.count(edges[e].b))) foot.feature = 1;
      if (foot.distance < best.distance) best = foot;
    }
    return best;
  }

  void Build() {
    if (seg.empty() || !(std::isfinite(opt.featureAngle) && opt.featureAngle > 0 && opt.featureAngle <= 180))
      throw std::invalid_argument("Invalid curved polyline options or empty curve.");
    for (const auto& p : x) for (double q : p)
      if (!std::isfinite(q)) throw std::invalid_argument("Curved polyline: nonfinite point.");
    for (long v : opt.forcedFeatures) if (v < 0 || v >= long(x.size()))
      throw std::invalid_argument("Curved polyline: feature index out of range.");
    std::set<std::pair<long,long>> unique;
    // undirected chains over all markers (bodies), broken only where the graph branches or ends
    std::map<long, std::vector<std::pair<long, long>>> adj;  // node -> (node, segment)
    for (size_t s = 0; s < seg.size(); ++s) {
      const long a = seg[s].a, b = seg[s].b;
      if (a < 0 || b < 0 || a >= long(x.size()) || b >= long(x.size()))
        throw std::invalid_argument("Curved polyline: vertex index out of range.");
      const double length = Norm<2>(Sub<2>(x[b], x[a]));
      if (!(length > 0 && std::isfinite(length))) throw std::invalid_argument("Curved polyline: degenerate segment.");
      if (!unique.emplace(std::min(a,b), std::max(a,b)).second)
        throw std::invalid_argument("Curved polyline: duplicate segment.");
      adj[seg[s].a].push_back({seg[s].b, long(s)});
      adj[seg[s].b].push_back({seg[s].a, long(s)});
    }
    for (const auto& row : adj)
      if (row.second.size() > 2) throw std::invalid_argument("Curved polyline: branched wall graph.");
    std::vector<char> used(seg.size(), 0);
    auto walk = [&](long start, long firstSeg) {
      Chain c;
      c.nodes.push_back(start);
      std::vector<long> segs;
      long cur = start, s = firstSeg;
      while (true) {
        used[s] = 1;
        segs.push_back(s);
        const long next = seg[s].a == cur ? seg[s].b : seg[s].a;
        if (next == start) { c.closed = true; break; }
        c.nodes.push_back(next);
        cur = next;
        long ns = -1;
        for (const auto& nb : adj[cur]) if (!used[nb.second]) { ns = nb.second; break; }
        if (ns < 0) break;
        s = ns;
      }
      return std::make_pair(c, segs);
    };
    std::vector<std::pair<Chain, std::vector<long>>> raw;
    for (const auto& row : adj)
      if (row.second.size() == 1 && !used[row.second[0].second]) raw.push_back(walk(row.first, row.second[0].second));
    for (const auto& row : adj)
      for (const auto& nb : row.second)
        if (!used[nb.second]) raw.push_back(walk(row.first, nb.second));
    const double cosFeature = std::cos(opt.featureAngle * std::acos(-1.0) / 180);
    for (auto& r : raw) {
      Chain& c = r.first;
      // keep the input orientation (fluid on the left) when most segments are traversed forwards
      {
        size_t forward = 0;
        for (size_t i = 0; i < r.second.size(); ++i) forward += seg[r.second[i]].a == c.nodes[i];
        if (forward != 0 && forward != r.second.size())
          throw std::invalid_argument("Curved polyline: inconsistent segment orientation.");
        if (2 * forward < r.second.size()) {
          if (c.closed) std::reverse(c.nodes.begin() + 1, c.nodes.end());
          else std::reverse(c.nodes.begin(), c.nodes.end());
          std::reverse(r.second.begin(), r.second.end());
        }
      }
      const long n = long(c.nodes.size()), ne = long(r.second.size());
      const int id = int(chains.size());
      c.arc.assign(1, 0);
      for (long i = 0; i < ne; ++i) {
        EdgeData e;
        e.a = c.nodes[i];
        e.b = c.nodes[(i + 1) % n];
        e.marker = seg[r.second[i]].marker;
        e.chain = id;
        e.index = i;
        c.edges.push_back(long(edges.size()));
        const double next = c.arc.back() + Norm<2>(Sub<2>(x[e.b], x[e.a]));
        if (!(std::isfinite(next) && next > c.arc.back()))
          throw std::invalid_argument("Curved polyline: unrepresentable arc length.");
        c.arc.push_back(next);
        edges.push_back(e);
      }
      // features
      for (long i = 0; i < n; ++i) {
        const long v = c.nodes[i];
        if (!c.closed && (i == 0 || i == n - 1)) { feature.insert(v); continue; }
        const long ein = c.edges[(i - 1 + ne) % ne], eout = c.edges[i % ne];
        const auto u = Unit<2>(Sub<2>(x[v], x[c.nodes[(i - 1 + n) % n]])), w = Unit<2>(Sub<2>(x[c.nodes[(i + 1) % n]], x[v]));
        turn[v] = std::acos(std::clamp(Dot<2>(u, w), -1.0, 1.0)) * 180 / std::acos(-1.0);
        if (Dot<2>(u, w) < cosFeature) feature.insert(v);
        if (opt.markerJunctionIsFeature && edges[ein].marker != edges[eout].marker) feature.insert(v);
        if (opt.forcedFeatures.count(v)) feature.insert(v);
      }
      chains.push_back(c);
    }
    // per-side tangents and edge cubics
    for (const auto& c : chains) {
      const long n = long(c.nodes.size()), ne = long(c.edges.size());
      auto node = [&](long i) { return c.closed ? c.nodes[((i % n) + n) % n] : c.nodes[i]; };
      auto valid = [&](long i) { return c.closed || (i >= 0 && i < n); };
      auto isFeature = [&](long i) { return feature.count(node(i)) > 0; };
      for (long i = 0; i < ne; ++i) {
        auto& e = edges[c.edges[i]];
        const long ia = i, ib = i + 1;
        const auto& A = x[node(ia)];
        const auto& B = x[node(ib)];
        const auto chord = Unit<2>(Sub<2>(B, A));
        Vec<2> ta = chord, tb = chord;
        // out-tangent at A
        if (!isFeature(ia)) ta = CircleTangent<2>(x[node(ia - 1)], A, B);
        else if (!isFeature(ib) && valid(ib + 1)) {
          const auto tB = CircleTangent<2>(A, B, x[node(ib + 1)]);
          ta = Sub<2>(Scale<2>(chord, 2 * Dot<2>(tB, chord)), tB);  // reflection about the chord
        }
        // in-tangent at B
        if (!isFeature(ib)) tb = CircleTangent<2>(A, B, x[node(ib + 1)]);
        else if (!isFeature(ia) && valid(ia - 1)) {
          const auto tA = CircleTangent<2>(x[node(ia - 1)], A, B);
          tb = Sub<2>(Scale<2>(chord, 2 * Dot<2>(tA, chord)), tA);
        }
        e.cubic = MakeEdgeCubic<2>(A, B, Unit<2>(ta), Unit<2>(tb), opt.handle);
        // monotone along the chord, else straight (fold guard)
        for (int s = 0; s <= 8 && !e.cubic.straight; ++s)
          if (!(Dot<2>(e.cubic.D1(s / 8.0), chord) > 0)) e.cubic = MakeEdgeCubic<2>(A, B, {}, {}, opt.handle);
        e.bulge = e.cubic.Bulge() * (1 + 1e-12);
        if (!isFeature(ia) && valid(ia - 1)) e.guardA = CircleCurvature<2>(x[node(ia - 1)], A, B);
        if (!isFeature(ib) && valid(ib + 1)) e.guardB = CircleCurvature<2>(A, B, x[node(ib + 1)]);
      }
    }
  }
};

}  // namespace SU2NativeCurved
