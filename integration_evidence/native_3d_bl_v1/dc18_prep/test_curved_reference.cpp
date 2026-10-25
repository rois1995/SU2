// DC-18 prep: standalone tests of Common/include/adaptation/CNativeCurvedReference.hpp (no SU2 build, no MPI).
// Usage: test_curved_reference <test> [args]; tests: sphere, cylinder, crease, m6 <Coarse.su2>,
//        airfoil <mesh.su2> <h0> <marker...>  (2D, several bodies allowed), gap <mesh.su2> <marker...>
#include "adaptation/CNativeCurvedReference.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

using namespace SU2NativeCurved;
using V3 = Vec<3>;
using V2 = Vec<2>;
static int failures = 0;
#define CHECK(cond, ...)                                  \
  do {                                                    \
    if (!(cond)) { ++failures; std::printf("FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
    else { std::printf("pass: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
  } while (0)

static double Order(double e1, double e2) { return std::log(e1 / e2) / std::log(2.0); }

// ---------------------------------------------------------------- 3D fixtures
struct TriMesh {
  std::vector<V3> p;
  std::vector<PNTriangleSurface::Triangle> t;
};
static TriMesh Icosphere(int level, double R, double jitter, unsigned seed) {
  const double g = (1 + std::sqrt(5.0)) / 2;
  TriMesh m;
  m.p = {{-1, g, 0}, {1, g, 0}, {-1, -g, 0}, {1, -g, 0}, {0, -1, g}, {0, 1, g}, {0, -1, -g}, {0, 1, -g}, {g, 0, -1}, {g, 0, 1}, {-g, 0, -1}, {-g, 0, 1}};
  std::vector<std::array<long, 3>> f = {{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
  for (auto& q : m.p) q = Scale<3>(Unit<3>(q), R);
  for (int l = 0; l < level; ++l) {
    std::map<std::pair<long, long>, long> mid;
    auto M = [&](long a, long b) {
      const auto k = std::make_pair(std::min(a, b), std::max(a, b));
      auto it = mid.find(k);
      if (it != mid.end()) return it->second;
      m.p.push_back(Scale<3>(Unit<3>(Add<3>(m.p[a], m.p[b])), R));
      return mid[k] = long(m.p.size() - 1);
    };
    std::vector<std::array<long, 3>> nf;
    for (auto& t : f) {
      const long a = M(t[0], t[1]), b = M(t[1], t[2]), c = M(t[2], t[0]);
      nf.push_back({t[0], a, c}); nf.push_back({t[1], b, a}); nf.push_back({t[2], c, b}); nf.push_back({a, b, c});
    }
    f.swap(nf);
  }
  if (jitter > 0) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> U(-1, 1);
    const double h = R * 1.1 / std::pow(2.0, level);
    for (auto& q : m.p) { V3 d{U(rng), U(rng), U(rng)}; q = Scale<3>(Unit<3>(Axpy<3>(q, jitter * h, d)), R); }
  }
  for (auto& t : f) m.t.push_back({t, 1});
  return m;
}
static TriMesh Cylinder(int n, int mz, double R, double H) {
  TriMesh m;
  for (int j = 0; j <= mz; ++j)
    for (int i = 0; i < n; ++i) {
      const double a = 2 * std::acos(-1.0) * (i + .5 * (j % 2)) / n;
      m.p.push_back({R * std::cos(a), R * std::sin(a), H * j / mz});
    }
  auto id = [&](int i, int j) { return long(j * n + ((i % n) + n) % n); };
  for (int j = 0; j < mz; ++j)
    for (int i = 0; i < n; ++i) {
      if (j % 2 == 0) { m.t.push_back({{id(i, j), id(i + 1, j), id(i, j + 1)}, 1}); m.t.push_back({{id(i + 1, j), id(i + 1, j + 1), id(i, j + 1)}, 1}); }
      else { m.t.push_back({{id(i, j), id(i + 1, j), id(i + 1, j + 1)}, 1}); m.t.push_back({{id(i, j), id(i + 1, j + 1), id(i, j + 1)}, 1}); }
    }
  return m;
}
// Closed axis-aligned box [-1,1]^3, each face n x n quads split into triangles; marker = face index.
static TriMesh Box(int n) {
  TriMesh m;
  std::map<std::array<long long, 3>, long> index;
  auto P = [&](V3 x) {
    std::array<long long, 3> k{std::llround(x[0] * 1e9), std::llround(x[1] * 1e9), std::llround(x[2] * 1e9)};
    auto it = index.find(k);
    if (it != index.end()) return it->second;
    m.p.push_back(x);
    return index[k] = long(m.p.size() - 1);
  };
  for (int axis = 0; axis < 3; ++axis)
    for (int side = -1; side <= 1; side += 2)
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
          auto X = [&](int a, int b) { V3 x{}; x[axis] = side; x[(axis + 1) % 3] = -1 + 2.0 * a / n; x[(axis + 2) % 3] = -1 + 2.0 * b / n; return P(x); };
          long q[4] = {X(i, j), X(i + 1, j), X(i + 1, j + 1), X(i, j + 1)};
          if (side > 0) { m.t.push_back({{q[0], q[1], q[2]}, axis * 2 + 1}); m.t.push_back({{q[0], q[2], q[3]}, axis * 2 + 1}); }
          else { m.t.push_back({{q[0], q[2], q[1]}, axis * 2}); m.t.push_back({{q[0], q[3], q[2]}, axis * 2}); }
        }
  return m;
}
// Closed half cylinder (x >= 0, radius 1, z in [0, H]): curved side (marker 1), flat face x = 0 (marker 2), caps
// (marker 3): 90 degree straight creases along z and 90 degree circular creases at the caps.
static TriMesh HalfCylinder(int n, int mz, int nr, double H) {
  TriMesh m;
  std::map<std::array<long long, 3>, long> index;
  auto P = [&](V3 x) {
    std::array<long long, 3> k{std::llround(x[0] * 1e12), std::llround(x[1] * 1e12), std::llround(x[2] * 1e12)};
    auto it = index.find(k);
    if (it != index.end()) return it->second;
    m.p.push_back(x);
    return index[k] = long(m.p.size() - 1);
  };
  const double pi = std::acos(-1.0);
  auto side = [&](int i, int j) { const double a = -pi / 2 + pi * i / n; return P({std::cos(a), std::sin(a), H * j / mz}); };
  for (int j = 0; j < mz; ++j)
    for (int i = 0; i < n; ++i) {
      m.t.push_back({{side(i, j), side(i + 1, j), side(i + 1, j + 1)}, 1});
      m.t.push_back({{side(i, j), side(i + 1, j + 1), side(i, j + 1)}, 1});
    }
  // flat face x = 0: y in [-1, 1] (n points like the side ends: use 2*nr intervals), z grid
  const int ny = 2 * nr;
  auto flat = [&](int i, int j) { return P({0, 1 - 2.0 * i / ny, H * j / mz}); };
  for (int j = 0; j < mz; ++j)
    for (int i = 0; i < ny; ++i) {
      m.t.push_back({{flat(i, j), flat(i + 1, j), flat(i + 1, j + 1)}, 2});
      m.t.push_back({{flat(i, j), flat(i + 1, j + 1), flat(i, j + 1)}, 2});
    }
  // caps: fan-free structured triangulation of the half disk: rings r_k = k/nr with arc points; simple approach:
  // polar grid with n arc intervals on every ring, centre strip handled by the flat-face points on y axis.
  for (int c = 0; c < 2; ++c) {
    const double z = c ? H : 0;
    auto ring = [&](int k, int i) {  // k = 0..nr (radius k/nr), i = 0..n
      const double r = double(k) / nr, a = -pi / 2 + pi * i / n;
      return P({r * std::cos(a), r * std::sin(a), z});
    };
    // ring k has n intervals; at k = 0 all points coincide (centre) -> triangles
    for (int k = 0; k < nr; ++k)
      for (int i = 0; i < n; ++i) {
        const long a = ring(k, i), b = ring(k, i + 1), cc = ring(k + 1, i + 1), d = ring(k + 1, i);
        auto add = [&](long p, long q, long r) {
          if (p == q || q == r || r == p) return;
          if (c) m.t.push_back({{p, q, r}, 3}); else m.t.push_back({{p, r, q}, 3});
        };
        add(a, b, cc); add(a, cc, d);
      }
    // the half-disk diameter (x = 0) is covered by ring points at i = 0 and i = n: radius k/nr at angles -pi/2, pi/2,
    // the same y = -/+ k/nr as the flat face points (ny = 2 nr).
  }
  return m;
}

static double SphereDeviation(const PNTriangleSurface& s, double R, double& facet) {
  double worst = 0;
  facet = 0;
  const int n = 6;
  for (size_t f = 0; f < s.Elements(); ++f) {
    const auto& t = s.Triangles()[f].v;
    for (int i = 0; i <= n; ++i)
      for (int j = 0; i + j <= n; ++j) {
        const double u = double(i) / n, v = double(j) / n;
        worst = std::max(worst, std::abs(Norm<3>(s.Evaluate(long(f), u, v)) - R));
        const auto flat = Axpy<3>(Axpy<3>(Scale<3>(s.Points()[t[0]], 1 - u - v), u, s.Points()[t[1]]), v, s.Points()[t[2]]);
        facet = std::max(facet, std::abs(Norm<3>(flat) - R));
      }
  }
  return worst;
}
static double MeanEdge(const TriMesh& m) {
  double s = 0;
  for (const auto& t : m.t)
    for (int k = 0; k < 3; ++k) s += Norm<3>(Sub<3>(m.p[t.v[(k + 1) % 3]], m.p[t.v[k]]));
  return s / (3 * m.t.size());
}

static void TestSphere() {
  std::printf("\n== (a) sphere R = 1: PN deviation from the sphere, projection, curvature ==\n");
  const double R = 1;
  for (double jitter : {0.0, 0.25}) {
    for (auto weight : {NormalWeight::Area, NormalWeight::Angle, NormalWeight::Max})
      for (auto handle : {HandleRule::PN, HandleRule::Circle}) {
        std::printf("jitter %.2f weight %s handle %s\n", jitter, weight == NormalWeight::Area ? "area" : weight == NormalWeight::Angle ? "angle" : "max",
                    handle == HandleRule::PN ? "PN" : "circle");
        double prevPN = 0, prevFacet = 0, prevK = 0;
        for (int level = 1; level <= 5; ++level) {
          const auto m = Icosphere(level, R, jitter, 7 + level);
          PNTriangleSurface::Options o;
          o.featureAngle = 60; o.weight = weight; o.handle = handle;
          PNTriangleSurface s(m.p, m.t, o);
          double facet;
          const double pn = SphereDeviation(s, R, facet);
          double kerr = 0;
          for (size_t f = 0; f < s.Elements(); ++f)
            for (int k = 0; k < 3; ++k) {
              const auto c = s.SectorCurvature(long(f), k);
              kerr = std::max({kerr, std::abs(c.k1 * R - 1), std::abs(c.k2 * R - 1)});
            }
          std::printf("  level %d faces %6zu h %.4f facet %.3e PN %.3e (ratio %.4f)  k max rel err %.3e  fallback %zu",
                      level, s.Elements(), MeanEdge(m), facet, pn, pn / facet, kerr, s.Statistics().fallbackFaces);
          if (level > 1) std::printf("  order facet %.2f PN %.2f k %.2f", Order(prevFacet, facet), Order(prevPN, pn), Order(prevK, kerr));
          std::printf("\n");
          if (weight == NormalWeight::Max && handle == HandleRule::PN && level == 5) {
            CHECK(Order(prevPN, pn) > 3.5, "Max-normal PN sphere convergence remains above order 3.5");
            CHECK(Order(prevFacet, facet) > 1.7 && Order(prevFacet, facet) < 2.3, "flat sphere convergence is second order");
          }
          prevPN = pn; prevFacet = facet; prevK = kerr;
        }
      }
  }
  // projection on the level-4 sphere (Max weight, PN handle): random points at 0.8 R .. 1.2 R
  const auto m = Icosphere(4, R, 0.25, 11);
  PNTriangleSurface::Options o;
  o.featureAngle = 60;
  PNTriangleSurface s(m.p, m.t, o);
  double facet;
  const double pn = SphereDeviation(s, R, facet);
  std::mt19937 rng(3);
  std::normal_distribution<double> N(0, 1);
  std::uniform_real_distribution<double> U(.8, 1.2);
  double radial = 0, angular = 0, optimality = 0;
  size_t fallback = 0, hinted = 0;
  for (int i = 0; i < 2000; ++i) {
    const auto d = Unit<3>(V3{N(rng), N(rng), N(rng)});
    const auto p = Scale<3>(d, R * U(rng));
    const auto foot = s.Project(p);
    fallback += foot.fallback;
    radial = std::max(radial, std::abs(Norm<3>(foot.point) - R));
    angular = std::max(angular, Norm<3>(Sub<3>(Unit<3>(foot.point), d)));
    // optimality: the residual is along the patch normal
    const auto r = Unit<3>(Sub<3>(p, foot.point));
    optimality = std::max(optimality, Norm<3>(Cross(r, foot.normal)));
    // hint window from the facet foot must agree with the global result
    SearchHint h; h.element = foot.element; h.rings = 1;
    const auto local = s.Project(p, h);
    hinted += Norm<3>(Sub<3>(local.point, foot.point)) > 1e-12;
  }
  std::printf("projection (level 4 jittered, 2000 points at 0.8-1.2 R): |foot|-R max %.3e (PN deviation %.3e), direction err %.3e, "
              "|r x n| max %.3e, fallbacks %zu, hint/global mismatches %zu\n", radial, pn, angular, optimality, fallback, hinted);
  CHECK(radial <= 1.01 * pn + 1e-12, "sphere projection feet lie on the PN surface within its deviation");
  CHECK(optimality < 1e-6 && fallback == 0, "sphere projection is a converged orthogonal foot");
  CHECK(hinted == 0, "local hint window and global search agree");
  CHECK(s.MaximumEdgeGap() < 1e-14, "sphere PN surface watertight (raw patch gap %.2e)", s.MaximumEdgeGap());
}

// Two spheres R = 1 with a narrow gap: hinted projections never jump to the other body; certified child bound.
static void TestMultiBody() {
  std::printf("\n== (a') two bodies with a narrow gap (spheres R = 1, gap 0.05) and the certified child deviation bound ==\n");
  auto a = Icosphere(3, 1, 0.25, 21), b = Icosphere(3, 1, 0.25, 22);
  const double gap = 0.05;
  TriMesh m = a;
  const long off = long(m.p.size());
  for (auto q : b.p) m.p.push_back({q[0] + 2 + gap, q[1], q[2]});
  for (auto t : b.t) m.t.push_back({{t.v[0] + off, t.v[1] + off, t.v[2] + off}, 2});
  PNTriangleSurface::Options o;
  o.featureAngle = 60;
  PNTriangleSurface s(m.p, m.t, o);
  size_t probes = 0, hintJumps = 0, globalJumps = 0;
  for (size_t f = 0; f < s.Elements(); ++f) {
    const auto c = s.Evaluate(long(f), 1 / 3.0, 1 / 3.0);
    const int own = s.Component(long(f));
    const auto other = s.Project(c, SearchHint{1 - own, -1, 0});
    for (double frac : {0.25, 0.49}) {
      const auto q = Axpy<3>(c, frac, Sub<3>(other.point, c));
      SearchHint h; h.element = long(f); h.rings = 2;
      ++probes;
      hintJumps += s.Project(q, h).component != own;
      globalJumps += s.Project(q).component != own;
    }
  }
  std::printf(" components %zu; %zu probes towards the other body at 0.25/0.49 of the distance: hinted jumps %zu, global jumps %zu\n",
              s.Statistics().components, probes, hintJumps, globalJumps);
  CHECK(s.Statistics().components == 2 && hintJumps == 0, "two bodies: hinted projections stay on their body");
  // certified bound of a flat child triangle on its patch vs dense sampling
  std::mt19937 rng(9);
  std::uniform_real_distribution<double> U(0, 1);
  double worstRatio = 0, minRatio = 1e300;
  size_t violations = 0;
  for (int it = 0; it < 2000; ++it) {
    const long f = long(U(rng) * s.Elements()) % long(s.Elements());
    Vec<3> r[3];
    for (auto& x : r) { double u = U(rng), v = U(rng); if (u + v > 1) { u = 1 - u; v = 1 - v; } x = {1 - u - v, u, v}; }
    const auto bound = s.ChildDeviation(f, r[0], r[1], r[2]);
    const V3 P[3] = {s.Evaluate(f, r[0][1], r[0][2]), s.Evaluate(f, r[1][1], r[1][2]), s.Evaluate(f, r[2][1], r[2][2])};
    double sampled = 0;
    const int n = 12;
    for (int i = 0; i <= n; ++i)
      for (int j = 0; i + j <= n; ++j) {
        const double a1 = double(i) / n, a2 = double(j) / n, a0 = 1 - a1 - a2;
        const double u = a0 * r[0][1] + a1 * r[1][1] + a2 * r[2][1], v = a0 * r[0][2] + a1 * r[1][2] + a2 * r[2][2];
        const auto T = Axpy<3>(Axpy<3>(Scale<3>(P[0], a0), a1, P[1]), a2, P[2]);
        sampled = std::max(sampled, Norm<3>(Sub<3>(s.Evaluate(f, u, v), T)));
      }
    if (sampled > bound[0] * (1 + 1e-9) + 1e-15) ++violations;
    if (sampled > 1e-12) { worstRatio = std::max(worstRatio, bound[0] / sampled); minRatio = std::min(minRatio, bound[0] / sampled); }
  }
  std::printf(" child bound: 2000 random children, sampled > bound in %zu, bound/sampled in [%.3f, %.3f]\n", violations, minRatio, worstRatio);
  CHECK(violations == 0, "certified child deviation bound never below the sampled deviation");
}

static void TestCylinder() {
  std::printf("\n== (a) open cylinder R = 1, H = 2: PN deviation, curvature k1 = 1, k2 = 0, rim curves ==\n");
  for (auto handle : {HandleRule::PN, HandleRule::Circle}) {
    std::printf("handle %s\n", handle == HandleRule::PN ? "PN" : "circle");
    double prev = 0, prevF = 0, prevK = 0;
    for (int level = 0; level < 5; ++level) {
      const int n = 12 << level, mz = 4 << level;
      const auto m = Cylinder(n, mz, 1, 2);
      PNTriangleSurface::Options o;
      o.featureAngle = 60; o.handle = handle;
      PNTriangleSurface s(m.p, m.t, o);
      double worst = 0, facet = 0, kerr = 0, k2 = 0, dir = 0, rim = 0, kerrB = 0, k2B = 0;
      const int ns = 6;
      for (size_t f = 0; f < s.Elements(); ++f) {
        const auto& t = s.Triangles()[f].v;
        for (int i = 0; i <= ns; ++i)
          for (int j = 0; i + j <= ns; ++j) {
            const double u = double(i) / ns, v = double(j) / ns;
            const auto q = s.Evaluate(long(f), u, v);
            worst = std::max(worst, std::abs(std::hypot(q[0], q[1]) - 1));
            const auto fl = Axpy<3>(Axpy<3>(Scale<3>(m.p[t[0]], 1 - u - v), u, m.p[t[1]]), v, m.p[t[2]]);
            facet = std::max(facet, std::abs(std::hypot(fl[0], fl[1]) - 1));
            if (q[2] == 0 || q[2] == 2) rim = std::max(rim, std::abs(std::hypot(q[0], q[1]) - 1));
          }
        const auto c = s.Curvature(long(f), 1 / 3.0, 1 / 3.0);
        bool border = false;
        for (long q : t) border = border || m.p[q][2] == 0 || m.p[q][2] == 2;
        (border ? kerrB : kerr) = std::max(border ? kerrB : kerr, std::abs(std::abs(c.k1) - 1));
        (border ? k2B : k2) = std::max(border ? k2B : k2, std::abs(c.k2));
        dir = std::max(dir, std::abs(c.d1[2]));  // max curvature direction must be horizontal
      }
      std::printf("  n %4d faces %6zu facet %.3e PN %.3e  interior |k1|-1 %.3e |k2| %.3e; open-border faces |k1|-1 %.3e |k2| %.3e; |d1.z| %.3e rim %.3e borders %zu",
                  n, s.Elements(), facet, worst, kerr, k2, kerrB, k2B, dir, rim, s.Statistics().borderEdges);
      if (level) std::printf("  order facet %.2f PN %.2f k1 %.2f", Order(prevF, facet), Order(prev, worst), Order(prevK, kerr));
      std::printf("\n");
      prev = worst; prevF = facet; prevK = kerr;
      if (level == 4 && handle == HandleRule::PN) {
        CHECK(kerr < 0.05 && k2 < 0.05, "cylinder curvature within 5%% at n = %d", n);
        CHECK(dir < 0.05, "cylinder max-curvature direction horizontal");
      }
    }
  }
}

static void TestCrease() {
  std::printf("\n== (b) sharp creases: box (12 creases, 8 corners) and closed half cylinder (90 deg wedges) ==\n");
  {
    const auto m = Box(4);
    PNTriangleSurface::Options o;
    o.featureAngle = 45;
    o.markerJunctionIsCrease = false;  // creases from the angle only
    PNTriangleSurface s(m.p, m.t, o);
    double planar = 0;
    for (size_t f = 0; f < s.Elements(); ++f) {
      const auto n = s.FacetNormal(long(f));
      const auto& a = m.p[s.Triangles()[f].v[0]];
      for (int i = 0; i <= 8; ++i)
        for (int j = 0; i + j <= 8; ++j) planar = std::max(planar, std::abs(Dot<3>(n, Sub<3>(s.Evaluate(long(f), i / 8.0, j / 8.0), a))));
    }
    // projection near an edge and a corner: foot on the box surface, flagged
    std::mt19937 rng(5);
    std::uniform_real_distribution<double> U(-1.2, 1.2);
    double off = 0;
    size_t features = 0, edges = 0;
    for (int i = 0; i < 4000; ++i) {
      V3 p{U(rng), U(rng), U(rng)};
      const auto foot = s.Project(p);
      const auto& q = foot.point;
      const double inf = std::max({std::abs(q[0]), std::abs(q[1]), std::abs(q[2])});
      off = std::max(off, std::abs(inf - 1));
      // closest point on the exact box surface
      V3 c = p;
      int outside = 0;
      for (int k = 0; k < 3; ++k) { if (std::abs(c[k]) > 1) ++outside; c[k] = std::clamp(c[k], -1.0, 1.0); }
      if (outside == 0) { int k = 0; for (int j = 1; j < 3; ++j) if (1 - std::abs(p[j]) < 1 - std::abs(p[k])) k = j; c[k] = p[k] > 0 ? 1 : -1; }
      edges += outside >= 2;
      features += outside >= 2 && foot.feature >= 1;
      off = std::max(off, Norm<3>(Sub<3>(c, q)));
    }
    std::printf("box n=4: creases %zu corners %zu crease vertices %zu fallback %zu; max off-plane %.2e; projection vs exact box %.2e; "
                "edge/corner-region feet flagged %zu/%zu; gap %.2e\n", s.Statistics().creaseEdges, s.Statistics().cornerVertices,
                s.Statistics().creaseVertices, s.Statistics().fallbackFaces, planar, off, features, edges, s.MaximumEdgeGap());
    CHECK(planar < 1e-14 && off < 1e-13, "box faces stay exact planes and creases stay sharp");
    CHECK(features == edges, "projections into the edge/corner regions land on flagged features");
  }
  for (int mode = 0; mode < 3; ++mode) {
    const bool declarePlanar = mode > 0;
    std::printf("half cylinder (planar markers: %s)\n", mode == 0 ? "not declared" : mode == 1 ? "declared, reflective" : "declared, reflective, projected");
    double prev = 0;
    for (int level = 0; level < 4; ++level) {
      const int n = 8 << level, mz = 4 << level, nr = 4 << level;
      const auto m = HalfCylinder(n, mz, nr, 1);
      PNTriangleSurface::Options o;
      o.featureAngle = 60;
      if (declarePlanar) o.planarMarkers = {2, 3};
      o.projectPlanar = mode == 2;
      if (o.projectPlanar) {
        bool refused = false;
        try { PNTriangleSurface unsupported(m.p, m.t, o); } catch (const std::invalid_argument&) { refused = true; }
        CHECK(refused, "face-only projection refuses: shared affine preprocessing is required");
        continue;
      }
      PNTriangleSurface s(m.p, m.t, o);
      double curved = 0, flatOff = 0, rim = 0, straight = 0, kerr = 0, k2 = 0;
      for (size_t f = 0; f < s.Elements(); ++f) {
        const int mk = s.Marker(long(f));
        if (mk == 1) {
          const auto c = s.Curvature(long(f), 1 / 3.0, 1 / 3.0);
          kerr = std::max(kerr, std::abs(std::abs(c.k1) - 1));
          k2 = std::max(k2, std::abs(c.k2));
        }
        for (int i = 0; i <= 8; ++i)
          for (int j = 0; i + j <= 8; ++j) {
            const auto q = s.Evaluate(long(f), i / 8.0, j / 8.0);
            if (mk == 1) curved = std::max(curved, std::abs(std::hypot(q[0], q[1]) - 1));
            if (mk == 2) flatOff = std::max(flatOff, std::abs(q[0]));
            if (mk == 3) flatOff = std::max(flatOff, std::min(std::abs(q[2]), std::abs(q[2] - 1)));
            if (mk == 1 && (q[2] == 0 || q[2] == 1)) rim = std::max(rim, std::abs(std::hypot(q[0], q[1]) - 1));
            if (mk == 1 && std::abs(q[0]) < 1e-15) straight = std::max(straight, std::abs(std::abs(q[1]) - 1));
          }
      }
      std::printf("  n %3d faces %6zu creases %zu corners %zu fallback %zu: curved side dev %.3e%s, flat faces off-plane %.2e, "
                  "rim arcs off-circle %.3e, wedge lines off-line %.2e, gap %.2e; side |k1|-1 %.3e |k2| %.3e\n", n, s.Elements(), s.Statistics().creaseEdges,
                  s.Statistics().cornerVertices, s.Statistics().fallbackFaces, curved,
                  level ? (" (order " + std::to_string(Order(prev, curved)).substr(0, 4) + ")").c_str() : "", flatOff, rim, straight,
                  s.MaximumEdgeGap(), kerr, k2);
      prev = curved;
      if (level == 3) {
        CHECK(flatOff < (mode == 2 ? 1e-13 : 1e-15), "half cylinder: flat faces exact planes (mode %d)", mode);
        CHECK(straight < 1e-14, "half cylinder: 90 deg wedge creases stay straight lines");
        CHECK(s.MaximumEdgeGap() < 1e-14, "half cylinder: watertight across creases");
      }
    }
  }
}

// ---------------------------------------------------------------- SU2 mesh reading
struct Su2 {
  int dim = 0;
  std::vector<std::array<double, 3>> p;
  std::map<std::string, std::vector<std::vector<long>>> markers;
};
static Su2 ReadSu2(const std::string& file) {
  std::ifstream in(file);
  if (!in) throw std::runtime_error("cannot open " + file);
  Su2 m;
  std::string line;
  auto value = [](const std::string& l) { return l.substr(l.find('=') + 1); };
  while (std::getline(in, line)) {
    if (line.rfind("NDIME=", 0) == 0) m.dim = std::stoi(value(line));
    else if (line.rfind("NPOIN=", 0) == 0) {
      const long n = std::stol(value(line));
      m.p.resize(n);
      for (long i = 0; i < n; ++i) {
        std::getline(in, line);
        std::istringstream s(line);
        double a = 0, b = 0, c = 0;
        s >> a >> b;
        if (m.dim == 3) s >> c;
        m.p[i] = {a, b, c};
      }
    } else if (line.rfind("MARKER_TAG=", 0) == 0) {
      std::string name = value(line);
      name.erase(0, name.find_first_not_of(" \t"));
      name.erase(name.find_last_not_of(" \t\r") + 1);
      std::getline(in, line);
      const long n = std::stol(value(line));
      auto& el = m.markers[name];
      for (long i = 0; i < n; ++i) {
        std::getline(in, line);
        std::istringstream s(line);
        int type;
        s >> type;
        std::vector<long> v(type == 3 ? 2 : type == 5 ? 3 : 4);
        for (auto& q : v) s >> q;
        el.push_back(v);
      }
    }
  }
  return m;
}

// ---------------------------------------------------------------- (c) ONERA M6
static void TestM6(const std::string& file) {
  std::printf("\n== (c) ONERA M6 wall %s ==\n", file.c_str());
  const auto mesh = ReadSu2(file);
  TriMesh m;
  std::map<long, long> local;
  auto id = [&](long g) { auto it = local.find(g); if (it != local.end()) return it->second; m.p.push_back({mesh.p[g][0], mesh.p[g][1], mesh.p[g][2]}); return local[g] = long(m.p.size() - 1); };
  for (const auto& e : mesh.markers.at("WING3D")) m.t.push_back({{id(e[0]), id(e[1]), id(e[2])}, 1});
  const size_t wingFaces = m.t.size();
  for (const auto& e : mesh.markers.at("SYMMETRY")) m.t.push_back({{id(e[0]), id(e[1]), id(e[2])}, 0});
  const double h0 = 2e-6;
  for (auto weight : {NormalWeight::Max, NormalWeight::Angle}) {
    PNTriangleSurface::Options o;
    o.featureAngle = 60;
    o.weight = weight;
    o.planarMarkers = {0};
    PNTriangleSurface s(m.p, m.t, o);
    const auto& st = s.Statistics();
    std::printf("weight %s: faces %zu (wing %zu) components %zu creases %zu (junction %zu border %zu) crease vertices %zu corners %zu "
                "planar components %zu (rejected %zu) fallback faces %zu straight edges %zu inconsistent edges %zu; gap %.2e\n",
                weight == NormalWeight::Max ? "max" : "angle", s.Elements(), wingFaces, st.components, st.creaseEdges, st.junctionEdges,
                st.borderEdges, st.creaseVertices, st.cornerVertices, st.planarComponents, st.rejectedPlanar, st.fallbackFaces,
                st.straightEdges, st.inconsistentEdges, s.MaximumEdgeGap());
    // symmetry plane exactness
    double sym = 0;
    for (size_t f = wingFaces; f < s.Elements(); ++f)
      for (int i = 0; i <= 4; ++i)
        for (int j = 0; i + j <= 4; ++j) sym = std::max(sym, std::abs(s.Evaluate(long(f), i / 4.0, j / 4.0)[1]));
    double symInput = 0;
    for (size_t f = wingFaces; f < s.Elements(); ++f) for (long q : m.t[f].v) symInput = std::max(symInput, std::abs(m.p[q][1]));
    std::printf("  symmetry faces: max |y| of the curved reference %.2e (input symmetry vertices %.2e)\n", sym, symInput);
    // per face: planform region (test-only labels, the DC-15 bins), deviation flat vs curved, curvature, sizes
    struct Bin { std::vector<double> dev, R, guardR, L2p, L2p32, Leps, Lan; size_t faces = 0; double bound = 0; };
    std::map<std::string, Bin> bins;
    auto region = [&](const V3& g, std::string& tag) {
      const double y = g[1], xle = 0.57735 * y, xte = 0.8059 + 0.28297 * y, frac = (g[0] - xle) / (xte - xle), eta = y / 1.1963;
      const char* part = frac < .05 ? "LE<5%" : frac < .2 ? "5-20%" : frac < .95 ? "mid" : "TE";
      std::string span = eta < .15 ? "" : eta < .25 ? "eta.15-.25" : eta < .45 ? "" : eta < .55 ? "eta.45-.55" : eta < .85 ? "" : eta < .95 ? "eta.85-.95" : "";
      tag = part;
      return span;
    };
    CurvatureSizeRule twoPass, twoPass32, chord, analytic;
    twoPass.h0 = h0; twoPass32.h0 = 32 * h0; chord.chordTolerance = 1e-4; analytic.chordTolerance = h0 / 2;
    for (size_t f = 0; f < wingFaces; ++f) {
      const auto& t = m.t[f].v;
      const V3 g = Scale<3>(Add<3>(Add<3>(m.p[t[0]], m.p[t[1]]), m.p[t[2]]), 1 / 3.0);
      std::string part;
      const auto span = region(g, part);
      double dev = 0;
      for (int i = 0; i <= 6; ++i)
        for (int j = 0; i + j <= 6; ++j) {
          const double u = i / 6.0, v = j / 6.0;
          const auto fl = Axpy<3>(Axpy<3>(Scale<3>(m.p[t[0]], 1 - u - v), u, m.p[t[1]]), v, m.p[t[2]]);
          dev = std::max(dev, Norm<3>(Sub<3>(s.Evaluate(long(f), u, v), fl)));
        }
      const auto k = s.Curvature(long(f), 1 / 3.0, 1 / 3.0);
      for (const std::string& key : {span.empty() ? std::string() : span + " " + part, std::string("all ") + part}) {
        if (key.empty()) continue;
        auto& b = bins[key];
        ++b.faces;
        b.dev.push_back(dev);
        b.bound = std::max(b.bound, s.Bulge(long(f)));
        b.R.push_back(1 / std::max(std::abs(k.k1), 1e-30));
        b.guardR.push_back(1 / std::max(k.guard, 1e-30));
        b.L2p.push_back(twoPass.Size(k.k1));
        b.L2p32.push_back(twoPass32.Size(k.k1));
        b.Leps.push_back(chord.Size(k.k1));
        b.Lan.push_back(analytic.Size(k.k1));
      }
    }
    auto med = [](std::vector<double> v) { std::sort(v.begin(), v.end()); return v.empty() ? 0 : v[v.size() / 2]; };
    auto mn = [](const std::vector<double>& v) { return v.empty() ? 0 : *std::min_element(v.begin(), v.end()); };
    auto mx = [](const std::vector<double>& v) { return v.empty() ? 0 : *std::max_element(v.begin(), v.end()); };
    for (const auto& b : bins)
      std::printf("  %-20s faces %5zu  facet dev med/max %7.1f/%7.1f um (bound %7.1f) = %6.1f h0  R(k1) med/min %8.2f/%7.2f mm  R(turn) med %8.2f mm"
                  "  L*: 0.7sqrt(2Rh0) %.3f mm, x32 %.3f mm, chord(1e-4) %.2f mm, analytic hook (h0/2) %.3f mm (medians)\n",
                  b.first.c_str(), b.second.faces, med(b.second.dev) * 1e6, mx(b.second.dev) * 1e6, b.second.bound * 1e6, mx(b.second.dev) / h0,
                  med(b.second.R) * 1e3, mn(b.second.R) * 1e3, med(b.second.guardR) * 1e3, med(b.second.L2p) * 1e3, med(b.second.L2p32) * 1e3,
                  med(b.second.Leps) * 1e3, med(b.second.Lan) * 1e3);
    if (weight != NormalWeight::Max) continue;
    // W demand: every wall edge against each rule (max admissible edge L* along d1/d2), bisection model
    std::map<std::pair<long, long>, std::pair<long, int>> edges;  // edge -> face, local index
    for (size_t f = 0; f < wingFaces; ++f)
      for (int k = 0; k < 3; ++k) {
        const long a = m.t[f].v[k], b = m.t[f].v[(k + 1) % 3];
        edges.emplace(std::make_pair(std::min(a, b), std::max(a, b)), std::make_pair(long(f), k));
      }
    struct Demand { size_t edges = 0, longEdges = 0, splits = 0; double vertexFactor = 0, current = 0; };
    const char* names[4] = {"0.7 sqrt(2 R h0), h0 = 2e-6", "0.7 sqrt(2 R h0), h0 x32", "chord height eps = 1e-4", "analytic hook eps = h0/2"};
    const CurvatureSizeRule* rules[4] = {&twoPass, &twoPass32, &chord, &analytic};
    for (int guardMode = 0; guardMode < 2; ++guardMode)
      for (int r = 0; r < 4; ++r) {
        Demand le, all;
        for (const auto& e : edges) {
          const long f = e.second.first;
          const int k = e.second.second;
          const auto& a = m.p[e.first.first];
          const auto& b = m.p[e.first.second];
          const V3 mid = Scale<3>(Add<3>(a, b), .5);
          std::string part;
          region(mid, part);
          const double bw[3] = {k == 0 ? .5 : k == 2 ? .5 : 0, k == 0 ? .5 : k == 1 ? .5 : 0, k == 1 ? .5 : k == 2 ? .5 : 0};
          const auto c = s.Curvature(f, bw[1], bw[2]);
          const auto L = CurvatureSizes(c, *rules[r], guardMode == 1);
          const auto d = Sub<3>(b, a);
          const double ratio = std::sqrt(std::pow(Dot<3>(d, c.d1) / L[0], 2) + std::pow(Dot<3>(d, c.d2) / L[1], 2));
          size_t splits = 0;
          if (ratio > 1) splits = (size_t(1) << int(std::ceil(std::log2(ratio)))) - 1;
          for (Demand* D : {&all, part == "LE<5%" ? &le : nullptr}) {
            if (!D) continue;
            ++D->edges; D->longEdges += ratio > 1; D->splits += splits;
          }
        }
        // metric-area vertex estimate: target density 1/(h1' h2') with h1' = min(L1, current size along d1)
        for (size_t f = 0; f < wingFaces; ++f) {
          const auto& t = m.t[f].v;
          const V3 g = Scale<3>(Add<3>(Add<3>(m.p[t[0]], m.p[t[1]]), m.p[t[2]]), 1 / 3.0);
          std::string part;
          region(g, part);
          const auto c = s.Curvature(long(f), 1 / 3.0, 1 / 3.0);
          const auto L = CurvatureSizes(c, *rules[r], guardMode == 1);
          double S[3][3] = {{0}};
          for (int q = 0; q < 3; ++q) {
            const auto e = Sub<3>(m.p[t[(q + 1) % 3]], m.p[t[q]]);
            for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) S[i][j] += 2.0 / 3 * e[i] * e[j];
          }
          auto along = [&](const V3& d) { double s2 = 0; for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) s2 += d[i] * S[i][j] * d[j]; return std::sqrt(s2); };
          const double s1 = along(c.d1), s2 = along(c.d2);
          const double area = .5 * Norm<3>(Cross(Sub<3>(m.p[t[1]], m.p[t[0]]), Sub<3>(m.p[t[2]], m.p[t[0]])));
          const double now = area / (s1 * s2), target = area / (std::min(L[0], s1) * std::min(L[1], s2));
          for (Demand* D : {&all, part == "LE<5%" ? &le : nullptr}) { if (!D) continue; D->current += now; D->vertexFactor += target; }
        }
        std::printf("  W demand [%s, guard %s]: LE<5%%: %zu/%zu wall edges above L*, %zu bisection points, metric-area vertex factor x%.1f; "
                    "whole wing: %zu/%zu edges, %zu points, factor x%.2f\n", names[r], guardMode ? "on" : "off", le.longEdges, le.edges, le.splits,
                    le.vertexFactor / le.current, all.longEdges, all.edges, all.splits, all.vertexFactor / all.current);
      }
    // projection checks: facet-centroid points lifted by h0 and 100 h0 along the facet normal, hint (face, 2 rings) vs global
    size_t mismatch = 0, fallback = 0, n = 0;
    double worstNormal = 0;
    for (size_t f = 0; f < wingFaces; f += 7) {
      const auto& t = m.t[f].v;
      const V3 g = Scale<3>(Add<3>(Add<3>(m.p[t[0]], m.p[t[1]]), m.p[t[2]]), 1 / 3.0);
      for (double lift : {h0, 100 * h0}) {
        const auto q = Axpy<3>(g, lift, s.FacetNormal(long(f)));
        SearchHint h; h.element = long(f); h.rings = 2;
        const auto a = s.Project(q, h);
        const auto b = s.Project(q, SearchHint{s.Component(long(f)), -1, 0});
        ++n; mismatch += Norm<3>(Sub<3>(a.point, b.point)) > 1e-12; fallback += a.fallback;
        worstNormal = std::max(worstNormal, Norm<3>(Cross(Unit<3>(Sub<3>(q, a.point)), a.normal)));
      }
    }
    std::printf("  projection: %zu points (facet centroids + h0 / 100 h0): hint-vs-component mismatches %zu, fallbacks %zu, |r x n| max %.2e\n",
                n, mismatch, fallback, worstNormal);
    CHECK(sym <= symInput + 1e-15, "M6 symmetry plane as exact as its input (|y| %.1e, input %.1e)", sym, symInput);
    CHECK(st.fallbackFaces == 0 || st.fallbackFaces < wingFaces / 100, "M6 fallback faces %zu", st.fallbackFaces);
    CHECK(mismatch == 0, "M6 hinted projection equals the component search");
  }
}

// ---------------------------------------------------------------- 2D: CReferenceWall algorithm (port for comparison)
// Natural / periodic interpolating cubic spline in the chord-length parameter per feature segment, curvature
// max(|spline|, 2 sin(turn/2)/L at the nearest knots), as in SU2_CFD/src/adaptation/CReferenceWall.cpp (Fit, Evaluate,
// Curvature). Ported, not linked (the SU2 class needs CSimplexMesh and MPI).
struct SplineSeg {
  bool closed = false;
  std::vector<double> s;
  std::vector<V2> x, m;
  std::vector<double> turn;
};
static void Thomas(std::vector<double> a, std::vector<double> b, std::vector<double> c, std::vector<double>& d) {
  const size_t n = b.size();
  for (size_t i = 1; i < n; ++i) { const double w = a[i] / b[i - 1]; b[i] -= w * c[i - 1]; d[i] -= w * d[i - 1]; }
  d[n - 1] /= b[n - 1];
  for (size_t i = n - 1; i > 0; --i) d[i - 1] = (d[i - 1] - c[i - 1] * d[i]) / b[i - 1];
}
static void CyclicThomas(const std::vector<double>& a, const std::vector<double>& b, const std::vector<double>& c, std::vector<double>& d) {
  const size_t n = b.size();
  const double alpha = c[n - 1], beta = a[0], gamma = -b[0];
  std::vector<double> bb(b), u(n, 0.0), aa(a), cc(c);
  bb[0] = b[0] - gamma; bb[n - 1] = b[n - 1] - alpha * beta / gamma;
  u[0] = gamma; u[n - 1] = alpha; aa[0] = 0; cc[n - 1] = 0;
  Thomas(aa, bb, cc, d); Thomas(aa, bb, cc, u);
  const double fact = (d[0] + beta * d[n - 1] / gamma) / (1.0 + u[0] + beta * u[n - 1] / gamma);
  for (size_t i = 0; i < n; ++i) d[i] -= fact * u[i];
}
static double Turn2(const V2& a, const V2& b, const V2& c) {
  const auto u = Sub<2>(b, a), v = Sub<2>(c, b);
  return std::acos(std::clamp(Dot<2>(u, v) / (Norm<2>(u) * Norm<2>(v)), -1.0, 1.0));
}
static void FitSpline(SplineSeg& g) {
  const size_t nk = g.x.size(), ni = nk - 1;
  g.s.assign(nk, 0);
  for (size_t i = 1; i < nk; ++i) g.s[i] = g.s[i - 1] + Norm<2>(Sub<2>(g.x[i], g.x[i - 1]));
  g.m.assign(nk, V2{0, 0});
  std::vector<double> h(ni);
  for (size_t i = 0; i < ni; ++i) h[i] = g.s[i + 1] - g.s[i];
  if (g.closed && ni >= 3) {
    for (int d = 0; d < 2; ++d) {
      std::vector<double> a(ni), b(ni), c(ni), r(ni);
      for (size_t i = 0; i < ni; ++i) {
        const double hm = h[(i + ni - 1) % ni], hp = h[i];
        a[i] = hm; b[i] = 2 * (hm + hp); c[i] = hp;
        r[i] = 6 * ((g.x[i + 1][d] - g.x[i][d]) / hp - (g.x[i][d] - g.x[(i + ni - 1) % ni][d]) / hm);
      }
      CyclicThomas(a, b, c, r);
      for (size_t i = 0; i < ni; ++i) g.m[i][d] = r[i];
      g.m[ni][d] = r[0];
    }
  } else if (ni >= 2) {
    const size_t n = ni - 1;
    for (int d = 0; d < 2; ++d) {
      std::vector<double> a(n), b(n), c(n), r(n);
      for (size_t k = 0; k < n; ++k) {
        const size_t i = k + 1;
        a[k] = h[i - 1]; b[k] = 2 * (h[i - 1] + h[i]); c[k] = h[i];
        r[k] = 6 * ((g.x[i + 1][d] - g.x[i][d]) / h[i] - (g.x[i][d] - g.x[i - 1][d]) / h[i - 1]);
      }
      Thomas(a, b, c, r);
      for (size_t k = 0; k < n; ++k) g.m[k + 1][d] = r[k];
    }
  }
  g.turn.assign(nk, 0);
  for (size_t i = 0; i < nk; ++i) {
    long ip, in;
    if (g.closed) { ip = i == 0 ? long(ni) - 1 : long(i) - 1; in = i == nk - 1 ? 1 : long(i) + 1; }
    else { if (i == 0 || i == nk - 1) continue; ip = long(i) - 1; in = long(i) + 1; }
    const double len = .5 * (Norm<2>(Sub<2>(g.x[i], g.x[ip])) + Norm<2>(Sub<2>(g.x[in], g.x[i])));
    if (len > 0) g.turn[i] = 2 * std::sin(.5 * Turn2(g.x[ip], g.x[i], g.x[in])) / len;
  }
  if (!g.closed && nk > 2) { g.turn[0] = g.turn[1]; g.turn[nk - 1] = g.turn[nk - 2]; }
}
static void EvalSpline(const SplineSeg& g, double s, V2& x, V2* dx = nullptr, V2* ddx = nullptr) {
  const size_t ni = g.s.size() - 1;
  if (g.closed) { const double L = g.s.back(); s = std::fmod(s, L); if (s < 0) s += L; }
  s = std::clamp(s, g.s.front(), g.s.back());
  size_t i = std::upper_bound(g.s.begin(), g.s.end(), s) - g.s.begin();
  i = i == 0 ? 0 : i - 1;
  if (i >= ni) i = ni - 1;
  const double h = g.s[i + 1] - g.s[i], A = g.s[i + 1] - s, B = s - g.s[i];
  for (int d = 0; d < 2; ++d) {
    const double m0 = g.m[i][d], m1 = g.m[i + 1][d], x0 = g.x[i][d], x1 = g.x[i + 1][d];
    x[d] = m0 * A * A * A / (6 * h) + m1 * B * B * B / (6 * h) + (x0 / h - m0 * h / 6) * A + (x1 / h - m1 * h / 6) * B;
    if (dx) (*dx)[d] = -m0 * A * A / (2 * h) + m1 * B * B / (2 * h) - (x0 / h - m0 * h / 6) + (x1 / h - m1 * h / 6);
    if (ddx) (*ddx)[d] = m0 * A / h + m1 * B / h;
  }
}
static double SplineCurvature(const SplineSeg& g, double s) {
  V2 x, dx, ddx;
  EvalSpline(g, s, x, &dx, &ddx);
  const double sp = Norm<2>(dx);
  const double spline = sp > 0 ? std::abs(Cross2(dx, ddx)) / (sp * sp * sp) : 0;
  const size_t ni = g.s.size() - 1;
  size_t i = std::upper_bound(g.s.begin(), g.s.end(), s) - g.s.begin();
  i = i == 0 ? 0 : i - 1;
  if (i >= ni) i = ni - 1;
  return std::max(spline, std::max(g.turn[i], g.turn[i + 1]));
}
// spline segments of one chain node list (closed or open) split at feature nodes
static std::vector<SplineSeg> SplineChain(const std::vector<V2>& pts, bool closed, const std::vector<char>& sharp) {
  std::vector<SplineSeg> out;
  const size_t n = pts.size();
  long first = -1;
  for (size_t i = 0; i < n; ++i) if (sharp[i]) { first = long(i); break; }
  if (closed && first < 0) {
    SplineSeg g; g.closed = true; g.x = pts; g.x.push_back(pts[0]); FitSpline(g); out.push_back(g); return out;
  }
  std::vector<size_t> order;
  if (closed) { for (size_t k = 0; k <= n; ++k) order.push_back((first + k) % n); }
  else for (size_t k = 0; k < n; ++k) order.push_back(k);
  SplineSeg g;
  g.x.push_back(pts[order[0]]);
  for (size_t k = 1; k < order.size(); ++k) {
    g.x.push_back(pts[order[k]]);
    if (sharp[order[k]] || k + 1 == order.size()) { FitSpline(g); out.push_back(g); g = SplineSeg(); g.x.push_back(pts[order[k]]); }
  }
  return out;
}
static double SplineDistance(const std::vector<SplineSeg>& segs, const V2& p) {
  double best = 1e300;
  for (const auto& g : segs)
    for (size_t i = 0; i + 1 < g.x.size(); ++i) {
      double s = .5 * (g.s[i] + g.s[i + 1]);
      for (int it = 0; it < 40; ++it) {
        V2 x, dx, ddx;
        EvalSpline(g, s, x, &dx, &ddx);
        const auto r = Sub<2>(x, p);
        double fp = Dot<2>(dx, dx) + Dot<2>(r, ddx);
        if (fp <= 0) fp = Dot<2>(dx, dx);
        s = std::clamp(s - Dot<2>(r, dx) / fp, g.s[i], g.s[i + 1]);
      }
      V2 x;
      EvalSpline(g, s, x);
      best = std::min(best, Norm<2>(Sub<2>(x, p)));
    }
  return best;
}

// ---------------------------------------------------------------- 2D tests
struct Wall2D {
  std::vector<V2> p;
  std::vector<CurvedPolyline::Segment> s;
  std::vector<std::string> names;
};
static Wall2D ReadWall2D(const std::string& file, const std::vector<std::string>& markers) {
  const auto mesh = ReadSu2(file);
  Wall2D w;
  std::map<long, long> local;
  auto id = [&](long g) { auto it = local.find(g); if (it != local.end()) return it->second; w.p.push_back({mesh.p[g][0], mesh.p[g][1]}); return local[g] = long(w.p.size() - 1); };
  for (size_t k = 0; k < markers.size(); ++k) {
    for (const auto& e : mesh.markers.at(markers[k])) w.s.push_back({id(e[0]), id(e[1]), int(k)});
    w.names.push_back(markers[k]);
  }
  return w;
}

static void TestAirfoil(const std::string& file, double h0, const std::vector<std::string>& markers, double angle) {
  std::printf("\n== 2D curved polyline: %s (h0 %g, feature angle %g) ==\n", file.c_str(), h0, angle);
  const auto w = ReadWall2D(file, markers);
  CurvedPolyline::Options o;
  o.featureAngle = angle;
  CurvedPolyline c(w.p, w.s, o);
  for (size_t ch = 0; ch < c.Chains().size(); ++ch) {
    const auto& chain = c.Chains()[ch];
    double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
    for (long v : chain.nodes) { xmin = std::min(xmin, w.p[v][0]); xmax = std::max(xmax, w.p[v][0]); ymin = std::min(ymin, w.p[v][1]); ymax = std::max(ymax, w.p[v][1]); }
    std::printf(" chain %zu (marker %s): %s, %zu nodes, x [%.4f, %.4f] y [%.4f, %.4f], length %.4f; features:", ch,
                w.names[c.Marker(chain.edges[0])].c_str(), chain.closed ? "closed" : "open", chain.nodes.size(), xmin, xmax, ymin, ymax,
                chain.arc.back());
    for (long v : chain.nodes) if (c.VertexFeature(v)) std::printf(" (%.4f,%.4f) turn %.1f;", w.p[v][0], w.p[v][1], c.TurnDegrees(v));
    std::printf("\n");
  }
  // (1) leave-every-other-node-out: references built from the even nodes (features kept), distance of the dropped nodes
  for (size_t ch = 0; ch < c.Chains().size(); ++ch) {
    const auto& chain = c.Chains()[ch];
    const size_t n = chain.nodes.size();
    std::vector<char> keep(n, 0);
    for (size_t i = 0; i < n; ++i) keep[i] = (i % 2 == 0) || c.VertexFeature(chain.nodes[i]);
    if (!chain.closed) keep[n - 1] = 1;
    std::vector<V2> pts;
    std::vector<long> map;
    for (size_t i = 0; i < n; ++i) if (keep[i]) { map.push_back(long(pts.size())); pts.push_back(w.p[chain.nodes[i]]); } else map.push_back(-1);
    std::vector<CurvedPolyline::Segment> segs;
    for (size_t i = 0; i + 1 < pts.size(); ++i) segs.push_back({long(i), long(i + 1), 0});
    if (chain.closed) segs.push_back({long(pts.size() - 1), 0, 0});
    std::vector<char> sharp(pts.size(), 0);
    {
      size_t k = 0;
      for (size_t i = 0; i < n; ++i) if (keep[i]) sharp[k++] = c.VertexFeature(chain.nodes[i]) ? 1 : 0;
    }
    // make the coarse curve's features the fine curve's features: a turn limit above every coarse smooth turn
    CurvedPolyline::Options oc = o;
    oc.featureAngle = 179.9;
    for (size_t i = 0; i < pts.size(); ++i) if (sharp[i]) oc.forcedFeatures.insert(long(i));
    CurvedPolyline circle(pts, segs, oc);
    oc.handle = HandleRule::PN;
    CurvedPolyline pn(pts, segs, oc);
    // features must match the fine reference: rebuild with explicit sharp flags through the feature angle is not
    // possible, so the comparison only uses chains whose coarse smooth turns stay below the fine features (checked)
    const auto spline = SplineChain(pts, chain.closed, sharp);
    std::vector<double> eFlat, eSpline, eCircle, ePN;
    size_t coarseFeatureMismatch = 0;
    for (size_t i = 0; i < pts.size(); ++i) coarseFeatureMismatch += (circle.VertexFeature(long(i)) != 0) != (sharp[i] != 0) && !(!chain.closed && (i == 0 || i + 1 == pts.size()));
    for (size_t i = 0; i < n; ++i) {
      if (keep[i]) continue;
      const auto& q = w.p[chain.nodes[i]];
      // the dropped node lies between coarse nodes map[i-1] and map[i+1]: restrict every projection to that interval
      const long a = map[(i + n - 1) % n];
      const V2& A = pts[a];
      const V2& B = pts[(a + 1) % pts.size()];
      const auto d = Sub<2>(B, A);
      const double t = std::clamp(Dot<2>(Sub<2>(q, A), d) / Dot<2>(d, d), 0.0, 1.0);
      eFlat.push_back(Norm<2>(Sub<2>(q, Axpy<2>(A, t, d))));
      eCircle.push_back(circle.ProjectInterval(q, 0, a - 1, a + 1).distance);
      ePN.push_back(pn.ProjectInterval(q, 0, a - 1, a + 1).distance);
      eSpline.push_back(SplineDistance(spline, q));
    }
    auto stat = [](std::vector<double> v) { std::sort(v.begin(), v.end()); return std::make_pair(v[v.size() / 2], v[v.size() * 9 / 10]); };
    const auto f = stat(eFlat), sp = stat(eSpline), ci = stat(eCircle), p = stat(ePN);
    std::printf(" chain %zu leave-one-out (%zu dropped nodes, coarse feature mismatches %zu): distance med/p90  flat %.2e/%.2e  "
                "CReferenceWall spline %.2e/%.2e  curved(circle) %.2e/%.2e  curved(PN) %.2e/%.2e\n", ch, eFlat.size(), coarseFeatureMismatch,
                f.first, f.second, sp.first, sp.second, ci.first, ci.second, p.first, p.second);
    if (f.first > 1e-12) CHECK(ci.first < 0.2 * f.first, "chain %zu curved polyline (circle handle) leave-one-out median error < 0.2 x flat", ch);
    else std::printf("note: chain %zu dropped nodes lie on the flat chords (an adapted wall built on a flat reference): accuracy not measurable\n", ch);
  }
  // (2) curvature: curved polyline (cubic, at the node, with the guard) vs CReferenceWall curvature at the knots,
  //     (3) projection exactness, (4) size rule per chain
  for (size_t ch = 0; ch < c.Chains().size(); ++ch) {
    const auto& chain = c.Chains()[ch];
    const size_t n = chain.nodes.size();
    std::vector<V2> pts;
    std::vector<char> sharp;
    for (long v : chain.nodes) { pts.push_back(w.p[v]); sharp.push_back(c.VertexFeature(v) ? 1 : 0); }
    const auto spline = SplineChain(pts, chain.closed, sharp);
    std::vector<double> rel;
    double minR = 1e300, minRSpline = 1e300;
    V2 at{};
    // CReferenceWall curvature at every knot (keyed by the knot coordinates; a feature knot ends two segments)
    std::map<std::pair<double, double>, double> splineK;
    for (const auto& g : spline)
      for (size_t k = 0; k < g.x.size(); ++k) {
        auto& slot = splineK[{g.x[k][0], g.x[k][1]}];
        slot = std::max(slot, SplineCurvature(g, g.s[k]));
      }
    for (size_t i = 0; i < n; ++i) {
      if (sharp[i]) continue;
      // curved polyline at the node: mean of the two adjacent cubic end curvatures, with the circumcircle guard
      const size_t ne = chain.edges.size();
      const long eout = chain.edges[i % ne], ein = chain.edges[(i + ne - 1) % ne];
      const double kc = .5 * (std::abs(c.Curvature(eout, 0).k1) + std::abs(c.Curvature(ein, 1).k1));
      const double k = std::max(kc, CircleCurvature<2>(pts[(i + n - 1) % n], pts[i], pts[(i + 1) % n]));
      const double ks = splineK.at({pts[i][0], pts[i][1]});
      if (k > 0 && ks > 0) rel.push_back(std::abs(k / ks - 1));
      if (1 / k < minR) { minR = 1 / k; at = pts[i]; }
      minRSpline = std::min(minRSpline, 1 / ks);
    }
    std::sort(rel.begin(), rel.end());
    // projection exactness against dense sampling (points offset along the normal by +-1e-3 L and random)
    std::mt19937 rng(17 + unsigned(ch));
    std::uniform_real_distribution<double> U(0, 1), O(-1, 1);
    double projErr = 0;
    size_t wrong = 0;
    for (int it = 0; it < 300; ++it) {
      const long e = chain.edges[size_t(U(rng) * chain.edges.size()) % chain.edges.size()];
      const double t = U(rng);
      const double L = Norm<2>(Sub<2>(w.p[c.EdgeEnd(e)], w.p[c.EdgeStart(e)]));
      const auto q = Axpy<2>(c.Evaluate(e, t), O(rng) * 0.3 * L, c.Normal(e, t));
      SearchHint h; h.element = e; h.rings = 3;
      const auto foot = c.Project(q, h);
      wrong += foot.component != int(ch);
      double best = 1e300;
      for (long d = -3; d <= 3; ++d) {
        long j = c.Chains()[ch].closed ? ((long(c.Chains()[ch].edges.size()) + (long(std::find(chain.edges.begin(), chain.edges.end(), e) - chain.edges.begin()) + d)) % long(chain.edges.size()))
                                       : long(std::find(chain.edges.begin(), chain.edges.end(), e) - chain.edges.begin()) + d;
        if (j < 0 || j >= long(chain.edges.size())) continue;
        for (int s = 0; s <= 4000; ++s) best = std::min(best, Norm<2>(Sub<2>(c.Evaluate(chain.edges[j], s / 4000.0), q)));
      }
      projErr = std::max(projErr, foot.distance - best);
    }
    // size rule along the chain: L* = 0.7 sqrt(2 R h0) (TWO_PASS) with the curved curvature and with the spline's
    CurvatureSizeRule rule;
    rule.h0 = h0;
    rule.hmin = 2 * h0;
    double need = 0, needSpline = 0, minL = 1e300;
    for (size_t i = 0; i < chain.edges.size(); ++i) {
      const long e = chain.edges[i];
      for (int s = 0; s < 8; ++s) {
        const double t = (s + .5) / 8;
        const auto d = Norm<2>(c.Curve(e).D1(t)) / 8;
        const auto k = c.Curvature(e, t);
        const double L = CurvatureSizes(k, rule, true)[0];
        need += d / L;
        minL = std::min(minL, L);
      }
    }
    for (const auto& g : spline)
      for (size_t i = 0; i + 1 < g.s.size(); ++i)
        for (int s = 0; s < 8; ++s) {
          const double sv = g.s[i] + (s + .5) / 8 * (g.s[i + 1] - g.s[i]);
          needSpline += (g.s[i + 1] - g.s[i]) / 8 / rule.Size(SplineCurvature(g, sv));
        }
    // native 2D drop-in: deviation of every current wall face from the curved reference (the SPLIT demand of
    // CNativeEngine2D.hpp:206 is deviation / ADAP_HAUSD) and the points a chord-height eps would add (dev ~ L^2)
    double devMax = 0, roundTrip = 0;
    size_t above = 0, added = 0;
    const double eps = 1e-6;  // ADAP_HAUSD of the RAE configurations
    for (size_t i = 0; i < chain.edges.size(); ++i) {
      const long e = chain.edges[i];
      const auto& A = w.p[c.EdgeStart(e)];
      const auto& B = w.p[c.EdgeEnd(e)];
      const double dev = c.Deviation(int(ch), A, B);
      devMax = std::max(devMax, dev);
      if (dev > eps) { ++above; added += size_t(std::ceil(std::sqrt(dev / eps))) - 1; }
      const double arc = c.Parameter(int(ch), A);
      roundTrip = std::max(roundTrip, Norm<2>(Sub<2>(c.At(int(ch), arc), A)));
    }
    std::printf(" chain %zu drop-in: max face deviation from the curve %.2e, faces above eps %.0e: %zu (+%zu points by the chord rule), "
                "At(Parameter(node)) round trip %.1e\n", ch, devMax, eps, above, added, roundTrip);
    CHECK(roundTrip <= 64 * std::numeric_limits<double>::epsilon() * 25, "chain %zu wall nodes are exact reference points", ch);
    std::printf(" chain %zu: min radius curved %.5f at (%.4f, %.4f), spline %.5f; curvature rel. diff to CReferenceWall med %.3f p90 %.3f; "
                "projection excess over dense sampling %.1e, wrong-body feet %zu; size rule 0.7 sqrt(2 R h0): min L* %.2e, wall points needed "
                "%.0f (spline curvature %.0f) vs %zu now\n", ch, minR, at[0], at[1], minRSpline, rel[rel.size() / 2], rel[rel.size() * 9 / 10],
                projErr, wrong, minL, need, needSpline, n);
    CHECK(projErr < 1e-10 && wrong == 0, "chain %zu projection is the exact closest curve point and stays on its body", ch);
  }
}

// narrow gaps between bodies: a projection with a hint never jumps to another body
static void TestGap(const std::string& file, const std::vector<std::string>& markers, double angle) {
  std::printf("\n== 2D multi-body gaps: %s ==\n", file.c_str());
  const auto w = ReadWall2D(file, markers);
  CurvedPolyline::Options o;
  o.featureAngle = angle;
  CurvedPolyline c(w.p, w.s, o);
  const size_t nc = c.Chains().size();
  // closest other-body point of every edge midpoint (on the curve)
  double minGap = 1e300;
  size_t tests = 0, hintJumps = 0, globalJumps = 0, globalJumpsBelowHalf = 0;
  double worstOffset = 0;
  for (size_t ch = 0; ch < nc; ++ch) {
    const auto& chain = c.Chains()[ch];
    for (size_t i = 0; i < chain.edges.size(); ++i) {
      const long e = chain.edges[i];
      const auto mid = c.Evaluate(e, .5);
      Foot<2> other;
      for (size_t oc = 0; oc < nc; ++oc) {
        if (oc == ch) continue;
        const auto f = c.Project(mid, SearchHint{int(oc), -1, 0});
        if (f.distance < other.distance) other = f;
      }
      if (other.component < 0) continue;
      minGap = std::min(minGap, other.distance);
      const auto dir = Unit<2>(Sub<2>(other.point, mid));
      for (double frac : {0.25, 0.45, 0.49, 0.6}) {
        const auto q = Axpy<2>(mid, frac * other.distance, dir);
        SearchHint h; h.element = e; h.rings = 2;
        const auto local = c.Project(q, h);
        const auto global = c.Project(q);
        ++tests;
        hintJumps += local.component != int(ch);
        if (global.component != int(ch)) { ++globalJumps; globalJumpsBelowHalf += frac < .5; }
        worstOffset = std::max(worstOffset, local.distance / (frac * other.distance));
      }
    }
  }
  std::printf(" %zu bodies, min gap between bodies %.3e; %zu probes at 0.25/0.45/0.49/0.6 of the local gap towards the nearest other "
              "body: hinted feet on another body %zu, global nearest on another body %zu (%zu of them below half the gap); "
              "hinted foot distance / probe offset max %.3f\n", nc, minGap, tests, hintJumps, globalJumps, globalJumpsBelowHalf, worstOffset);
  CHECK(hintJumps == 0, "hinted projections never jump to another body across a gap");
}

// Contract regressions from the integration review: these are acceptance predicates, not shape demos.
static void TestContracts() {
  auto rejects = [](auto action) {
    try { action(); } catch (const std::invalid_argument&) { return true; }
    return false;
  };
  std::vector<V2> points;
  std::vector<CurvedPolyline::Segment> segments;
  for (int body = 0; body < 2; ++body)
    for (int i = 0; i < 16; ++i) {
      const double a = 2 * std::acos(-1.0) * i / 16;
      points.push_back({3.0 * body + std::cos(a), std::sin(a)});
      segments.push_back({16 * body + i, 16 * body + (i + 1) % 16, body});
    }
  CurvedPolyline curve(points, segments, {});
  CHECK(rejects([&] { curve.Project({1, 0}, {1, 0, 2}); }), "2D conflicting body/element hints reject");
  const auto a = curve.Evaluate(0, .8), b = curve.Evaluate(0, .2);
  const double bound = curve.Deviation(0, a, b);
  CHECK(bound >= Norm<2>(Sub<2>(curve.Evaluate(8, .5), a)) - Norm<2>(Sub<2>(a, b)),
        "wrapped interval on the same edge includes the rest of the body (bound %.6g)", bound);
  const double shortBound = curve.Deviation(0, b, a);
  CHECK(shortBound > 0 && shortBound < .03, "non-wrapped subedge retains a useful bound");

  std::vector<V3> planePoints{{0, 0, 0}};
  std::vector<PNTriangleSurface::Triangle> planeFaces;
  for (int i = 0; i < 16; ++i) {
    planePoints.push_back({points[i][0], points[i][1], 0});
    planeFaces.push_back({{0, i + 1, (i + 1) % 16 + 1}, 1});
  }
  PNTriangleSurface::Options options; options.planarMarkers = {1};
  PNTriangleSurface plane(planePoints, planeFaces, options);
  const auto error = plane.ChildDeviation(0, {1,0,0}, {0,1,0}, {0,0,1});
  const auto curved = plane.Evaluate(0, .5, .5);
  const auto flat = Scale<3>(Add<3>(planePoints[1], planePoints[2]), .5);
  CHECK(error[0] >= Norm<3>(Sub<3>(curved, flat)), "planar patch bounds in-plane parameter displacement");
  CHECK(error[1] < 1e-12, "exact plane has negligible normal deviation");

  auto sphere = Icosphere(1, 1, 0, 1);
  const size_t originalVertices = sphere.p.size(), originalFaces = sphere.t.size();
  for (size_t i = 0; i < originalVertices; ++i) sphere.p.push_back(Add<3>(sphere.p[i], {3,0,0}));
  for (size_t i = 0; i < originalFaces; ++i) {
    auto t = sphere.t[i]; for (auto& v : t.v) v += originalVertices; sphere.t.push_back(t);
  }
  PNTriangleSurface surface(sphere.p, sphere.t, {});
  CHECK(rejects([&] { surface.Project({1,0,0}, {1,0,2}); }), "3D conflicting body/element hints reject");
  const std::vector<V3> junctionPoints{{0,0,1}, {0,0,0}, {0,1,0}, {3,0,0}, {3,0,1}, {3,1,0}};
  const std::vector<PNTriangleSurface::Triangle> junctionFaces{{{0,1,2},0}, {{3,4,5},0}, {{1,3,5},1}, {{1,5,2},1}};
  PNTriangleSurface junction(junctionPoints, junctionFaces, options);
  CHECK(junction.Component(0) != junction.Component(1), "a shared symmetry plane cannot join separate wall bodies");
  CHECK(junction.Component(0) != junction.Component(2), "wall projections cannot escape onto a carrier plane");
  CHECK(rejects([&] { CurvedPolyline bad(points, {{0,1,0}, {2,1,0}}, {}); }), "inconsistent segment orientations reject");
  CHECK(rejects([&] { CurvedPolyline bad({{0,0}, {0,0}}, {{0,1,0}}, {}); }), "zero-length geometry rejects");
  CHECK(rejects([&] { CurvedPolyline bad(points, {{-1,0,0}}, {}); }), "invalid 2D indices reject before access");
  CHECK(rejects([&] { CurvedPolyline bad(points, {{0,1,0}, {1,0,0}}, {}); }), "duplicate 2D edges reject");
  CHECK(rejects([&] { curve.At(0, std::numeric_limits<double>::quiet_NaN()); }), "nonfinite arc parameters reject");
  CHECK(rejects([&] { surface.Project({1,0,0}, {-1,999999,2}); }), "invalid 3D hint rejects before access");
  options.projectPlanar = true;
  CHECK(rejects([&] { PNTriangleSurface bad(planePoints, planeFaces, options); }), "face-only plane projection cannot silently open seams");
  CurvatureSizeRule geometric; geometric.chordTolerance = 1e-4;
  const double length = geometric.Size(1/.024);
  CHECK(std::abs(CurvatureSizeRule::ChordHeight(length, .024) - 1e-4) < 1e-16,
        "geometric size reproduces the configured sagitta at the M6 leading-edge radius");
  geometric.hmin = 1; bool conflict = false;
  geometric.Size(1/.024, &conflict);
  CHECK(conflict, "a size floor conflicting with geometry is reported");
  CHECK(rejects([&] { geometric.Size(std::numeric_limits<double>::quiet_NaN()); }), "nonfinite curvature rejects");
  CHECK(CurvatureSizeRule::ChordHeight(1e-8, 1) > 0,
        "small sagitta does not vanish through cancellation");
}

int main(int argc, char** argv) {
  if (argc < 2) { std::printf("usage: %s sphere|cylinder|crease|m6 file|airfoil file h0 angle markers...|gap file angle markers...\n", argv[0]); return 2; }
  const std::string t = argv[1];
  try {
    if (t == "contracts") TestContracts();
    else if (t == "sphere") TestSphere();
    else if (t == "cylinder") TestCylinder();
    else if (t == "multibody") TestMultiBody();
    else if (t == "crease") TestCrease();
    else if (t == "m6") TestM6(argv[2]);
    else if (t == "airfoil") TestAirfoil(argv[2], std::atof(argv[3]), std::vector<std::string>(argv + 5, argv + argc), std::atof(argv[4]));
    else if (t == "gap") TestGap(argv[2], std::vector<std::string>(argv + 4, argv + argc), std::atof(argv[3]));
    else { std::printf("unknown test %s\n", t.c_str()); return 2; }
  } catch (const std::exception& x) {
    std::printf("EXCEPTION: %s\n", x.what());
    return 3;
  }
  std::printf("%s: %d failure(s)\n", t.c_str(), failures);
  return failures ? 1 : 0;
}
