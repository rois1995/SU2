/*!
 * \file CConservativeTransfer_tests.cpp
 * \brief Unit tests for the conservative P1 solution transfer (supermesh, mesh adaptation).
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/ConvexClipping.hpp"
#include "TransferTestCase.hpp"

using namespace transfer_test;

namespace {

using convex_clip::Polygon;
using convex_clip::Polyhedron;

passivedouble TriangleArea(const passivedouble* a, const passivedouble* b, const passivedouble* c) {
  return 0.5 * fabs((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]));
}

passivedouble TetVolume(const passivedouble* a, const passivedouble* b, const passivedouble* c,
                        const passivedouble* d) {
  passivedouble u[3], v[3], w[3];
  for (int i = 0; i < 3; ++i) {
    u[i] = b[i] - a[i];
    v[i] = c[i] - a[i];
    w[i] = d[i] - a[i];
  }
  return fabs(u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) +
              u[2] * (v[0] * w[1] - v[1] * w[0])) /
         6.0;
}

/*--- Half-planes / half-spaces lambda_k >= 0 of a triangle / tetrahedron (inward normal, offset). ---*/
void TrianglePlanes(const passivedouble (*p)[2], passivedouble (*plane)[3]) {
  const passivedouble sign =
      ((p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) - (p[1][1] - p[0][1]) * (p[2][0] - p[0][0])) > 0 ? 1 : -1;
  for (int k = 0; k < 3; ++k) {
    const auto* a = p[(k + 1) % 3];
    const auto* b = p[(k + 2) % 3];
    /*--- Left of a -> b (counter-clockwise) is inside. ---*/
    plane[k][0] = -(b[1] - a[1]) * sign;
    plane[k][1] = (b[0] - a[0]) * sign;
    plane[k][2] = -(plane[k][0] * a[0] + plane[k][1] * a[1]);
  }
}

void TetPlanes(const passivedouble (*p)[3], passivedouble (*plane)[4]) {
  for (int k = 0; k < 4; ++k) {
    const auto* a = p[(k + 1) % 4];
    const auto* b = p[(k + 2) % 4];
    const auto* c = p[(k + 3) % 4];
    passivedouble u[3], v[3], n[3];
    for (int i = 0; i < 3; ++i) {
      u[i] = b[i] - a[i];
      v[i] = c[i] - a[i];
    }
    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
    passivedouble d = -(n[0] * a[0] + n[1] * a[1] + n[2] * a[2]);
    if (n[0] * p[k][0] + n[1] * p[k][1] + n[2] * p[k][2] + d < 0) {
      for (int i = 0; i < 3; ++i) n[i] = -n[i];
      d = -d;
    }
    for (int i = 0; i < 3; ++i) plane[k][i] = n[i];
    plane[k][3] = d;
  }
}

/*--- Projection with default options on a donor/target pair of meshes, the fields given per donor point. ---*/
struct ProjectionCase {
  std::unique_ptr<CConfig> config;
  std::unique_ptr<MeshSolution> donor, target;
};

std::vector<passivedouble> DonorValues(const CGeometry& geometry, unsigned short nField,
                                       const std::function<void(const su2double*, passivedouble*)>& field) {
  std::vector<passivedouble> values(geometry.GetnPoint() * nField);
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
    field(geometry.nodes->GetCoord(iPoint), &values[iPoint * nField]);
  return values;
}

passivedouble Total(const CGeometry& geometry, const std::vector<passivedouble>& values, unsigned short nField,
                    unsigned short f) {
  passivedouble total = 0.0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
    total += values[iPoint * nField + f] * SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint));
  return total;
}

passivedouble Scale(const CGeometry& geometry, const std::vector<passivedouble>& values, unsigned short nField,
                    unsigned short f) {
  passivedouble total = 0.0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint)
    total += fabs(values[iPoint * nField + f]) * SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint));
  return total;
}

}  // namespace

TEST_CASE("Convex clipping: polygons", "[Adaptation]") {
  const passivedouble p0[] = {0.0, 0.0}, p1[] = {1.0, 0.0}, p2[] = {0.0, 1.0};
  passivedouble area = 0.0, c[2] = {};

  /*--- Either orientation, the area is positive. ---*/
  Polygon poly;
  poly.InitTriangle(p0, p2, p1);
  poly.Moments(area, c);
  CHECK(area == Approx(0.5).epsilon(1e-15));
  CHECK(c[0] == Approx(1.0 / 3.0).epsilon(1e-15));

  /*--- x >= 0.25: a triangle of legs 0.75. ---*/
  poly.InitTriangle(p0, p1, p2);
  poly.Clip(1.0, 0.0, -0.25);
  poly.Moments(area, c);
  CHECK(area == Approx(0.28125).epsilon(1e-15));
  CHECK(c[0] == Approx(0.5).epsilon(1e-14));
  CHECK(c[1] == Approx(0.25).epsilon(1e-14));

  /*--- Degenerate planes: through a vertex (keeps all), along an edge with the triangle inside (keeps all), along an
   *    edge with the triangle outside (empty), through two vertices cutting nothing. ---*/
  poly.InitTriangle(p0, p1, p2);
  poly.Clip(-1.0, -1.0, 1.0);  // x + y <= 1: edge p1-p2, inside
  poly.Clip(1.0, 0.0, 0.0);    // x >= 0: edge p0-p2
  poly.Clip(1.0, 1.0, 0.0);    // x + y >= 0: through p0 only
  poly.Moments(area, c);
  CHECK(area == Approx(0.5).epsilon(1e-15));
  poly.Clip(-1.0, 0.0, 0.0);  // x <= 0: only the edge p0-p2 is left
  poly.Moments(area, c);
  CHECK(fabs(area) < 1e-16);

  /*--- Random pairs: |A n B| clipping A by B equals clipping B by A; triangles shifted by a large offset (local
   *    coordinates matter: the caller removes it). ---*/
  std::mt19937 gen(7);
  std::uniform_real_distribution<passivedouble> dist(-1.0, 1.0);
  for (int i = 0; i < 2000; ++i) {
    passivedouble A[3][2], B[3][2], planeA[3][3], planeB[3][3];
    for (int k = 0; k < 3; ++k)
      for (int d = 0; d < 2; ++d) {
        A[k][d] = dist(gen);
        B[k][d] = (i % 3 == 0) ? A[k][d] : 0.5 * dist(gen);  // every third pair: identical triangles
      }
    if (i % 3 == 1) {  // shared edge
      for (int d = 0; d < 2; ++d) {
        B[0][d] = A[0][d];
        B[1][d] = A[1][d];
      }
    }
    TrianglePlanes(A, planeA);
    TrianglePlanes(B, planeB);
    Polygon ab, ba;
    ab.InitTriangle(A[0], A[1], A[2]);
    ba.InitTriangle(B[0], B[1], B[2]);
    for (int k = 0; k < 3; ++k) {
      ab.Clip(planeB[k][0], planeB[k][1], planeB[k][2]);
      ba.Clip(planeA[k][0], planeA[k][1], planeA[k][2]);
    }
    passivedouble areaAB = 0.0, areaBA = 0.0, cAB[2], cBA[2];
    ab.Moments(areaAB, cAB);
    ba.Moments(areaBA, cBA);
    CHECK(areaAB >= -1e-15);
    CHECK(fabs(areaAB - areaBA) < 1e-14);
    if (i % 3 == 0) CHECK(fabs(areaAB - TriangleArea(A[0], A[1], A[2])) < 1e-14);
  }
}

TEST_CASE("Convex clipping: polyhedra", "[Adaptation]") {
  const passivedouble q0[] = {0, 0, 0}, q1[] = {1, 0, 0}, q2[] = {0, 1, 0}, q3[] = {0, 0, 1};
  const passivedouble* unit[4] = {q0, q1, q2, q3};
  const passivedouble* flipped[4] = {q0, q2, q1, q3};
  passivedouble volume = 0.0, c[3] = {};

  Polyhedron poly;
  poly.InitTetrahedron(flipped);
  poly.Moments(volume, c);
  CHECK(volume == Approx(1.0 / 6.0).epsilon(1e-15));
  CHECK(c[2] == Approx(0.25).epsilon(1e-14));

  /*--- x >= 0.5: a tetrahedron of edges 0.5. ---*/
  poly.InitTetrahedron(unit);
  poly.Clip(1, 0, 0, -0.5);
  poly.Moments(volume, c);
  CHECK(volume == Approx(1.0 / 48.0).epsilon(1e-14));
  CHECK(c[0] == Approx(0.625).epsilon(1e-14));

  /*--- Degenerate planes: a face plane with the tetrahedron inside, through an edge, through a vertex: no change;
   *    the face plane with the tetrahedron outside: empty. ---*/
  poly.InitTetrahedron(unit);
  poly.Clip(0, 0, 1, 0);     // z >= 0 (face)
  poly.Clip(-1, -1, -1, 1);  // x + y + z <= 1 (face)
  poly.Clip(1, 1, 0, 0);     // x + y >= 0 (edge q0-q3)
  poly.Clip(1, 1, 1, 0);     // x + y + z >= 0 (vertex q0)
  poly.Moments(volume, c);
  CHECK(volume == Approx(1.0 / 6.0).epsilon(1e-14));
  poly.Clip(0, 0, -1, 0);  // z <= 0: only the face is left
  poly.Moments(volume, c);
  CHECK(fabs(volume) < 1e-16);

  /*--- Random pairs: clipping A by B equals clipping B by A; identical tetrahedra; tetrahedra sharing a face. ---*/
  std::mt19937 gen(11);
  std::uniform_real_distribution<passivedouble> dist(-1.0, 1.0);
  for (int i = 0; i < 2000; ++i) {
    passivedouble A[4][3], B[4][3], planeA[4][4], planeB[4][4];
    for (int k = 0; k < 4; ++k)
      for (int d = 0; d < 3; ++d) {
        A[k][d] = dist(gen);
        B[k][d] = (i % 3 == 0) ? A[k][d] : 0.6 * dist(gen);
      }
    if (i % 3 == 1)
      for (int k = 0; k < 3; ++k)
        for (int d = 0; d < 3; ++d) B[k][d] = A[k][d];
    TetPlanes(A, planeA);
    TetPlanes(B, planeB);
    const passivedouble* pa[4] = {A[0], A[1], A[2], A[3]};
    const passivedouble* pb[4] = {B[0], B[1], B[2], B[3]};
    Polyhedron ab, ba;
    ab.InitTetrahedron(pa);
    ba.InitTetrahedron(pb);
    for (int k = 0; k < 4; ++k) {
      REQUIRE(ab.Clip(planeB[k][0], planeB[k][1], planeB[k][2], planeB[k][3]) >= 0);
      REQUIRE(ba.Clip(planeA[k][0], planeA[k][1], planeA[k][2], planeA[k][3]) >= 0);
    }
    passivedouble vAB = 0.0, vBA = 0.0, cAB[3], cBA[3];
    ab.Moments(vAB, cAB);
    ba.Moments(vBA, cBA);
    CHECK(vAB >= -1e-15);
    CHECK(fabs(vAB - vBA) < 1e-14);
    if (i % 3 == 0) CHECK(fabs(vAB - TetVolume(A[0], A[1], A[2], A[3])) < 1e-14);
  }

  /*--- Partition: a tetrahedron inside the unit cube against the Kuhn tetrahedra of a 3x3x3 grid of the cube: the
   *    pieces add up to its volume and first moment. ---*/
  const passivedouble T[4][3] = {{0.1, 0.15, 0.2}, {0.9, 0.3, 0.25}, {0.35, 0.85, 0.1}, {0.4, 0.45, 0.95}};
  passivedouble planeT[4][4];
  TetPlanes(T, planeT);
  const int n = 3;
  const int kuhn[6][4] = {{0, 1, 3, 7}, {0, 1, 5, 7}, {0, 2, 3, 7}, {0, 2, 6, 7}, {0, 4, 5, 7}, {0, 4, 6, 7}};
  passivedouble sum = 0.0, moment[3] = {};
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      for (int k = 0; k < n; ++k)
        for (const auto& tet : kuhn) {
          passivedouble corner[4][3];
          for (int v = 0; v < 4; ++v) {
            corner[v][0] = (i + (tet[v] & 1)) / passivedouble(n);
            corner[v][1] = (j + ((tet[v] >> 1) & 1)) / passivedouble(n);
            corner[v][2] = (k + ((tet[v] >> 2) & 1)) / passivedouble(n);
          }
          const passivedouble* pc[4] = {corner[0], corner[1], corner[2], corner[3]};
          Polyhedron piece;
          piece.InitTetrahedron(pc);
          for (int p = 0; p < 4; ++p) piece.Clip(planeT[p][0], planeT[p][1], planeT[p][2], planeT[p][3]);
          passivedouble v = 0.0, cp[3];
          piece.Moments(v, cp);
          sum += v;
          for (int d = 0; d < 3; ++d) moment[d] += v * cp[d];
        }
  const passivedouble vT = TetVolume(T[0], T[1], T[2], T[3]);
  CHECK(fabs(sum - vT) < 1e-15);
  for (int d = 0; d < 3; ++d) CHECK(fabs(moment[d] / sum - 0.25 * (T[0][d] + T[1][d] + T[2][d] + T[3][d])) < 1e-14);
}

TEST_CASE("Conservative projection: supermesh and mass matrix", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 0);
      MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 0);
      CConservativeProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, {});
      const auto values = DonorValues(donor.Fine(), 1, [](const su2double* x, passivedouble* v) { v[0] = 1.0; });
      std::vector<passivedouble> result;
      projection.Project(1, values, result);
      const auto& s = projection.GetSummary();

      /*--- Supermesh measure = domain measure (both meshes), nothing uncovered. ---*/
      const passivedouble domain = (nDim == 2) ? 2.0 : 1.0;
      CHECK(fabs(s.overlapVolume - domain) < 1e-14 * domain);
      CHECK(fabs(s.targetVolume - domain) < 1e-14 * domain);
      CHECK(fabs(s.donorVolume - domain) < 1e-14 * domain);
      CHECK(fabs(s.targetCV - domain) < 1e-14 * domain);
      CHECK(s.nFillPieces == 0);
      CHECK(s.nSliverElems == 0);
      CHECK(s.nElemOutside == 0);
      CHECK(s.maxUncovered < 1e-12);
      CHECK(fabs(s.supermeshDefect[0]) < 1e-14);
      CHECK(s.nPairs > target.Fine().GetnElem());

      /*--- Mass matrix: symmetric, rows (= columns) sum to the SU2 control volumes, strictly diagonally dominant. ---*/
      std::vector<unsigned long> rowPtr, col;
      std::vector<passivedouble> value;
      projection.GetMassMatrix(rowPtr, col, value);
      passivedouble maxRowError = 0.0, maxAsym = 0.0, minDominance = 1e300;
      for (auto i = 0ul; i + 1 < rowPtr.size(); ++i) {
        passivedouble sum = 0.0, diag = 0.0, off = 0.0;
        for (auto p = rowPtr[i]; p < rowPtr[i + 1]; ++p) {
          sum += value[p];
          if (col[p] == i) {
            diag = value[p];
          } else {
            off += value[p];
            const auto j = col[p];
            const auto q = std::lower_bound(col.begin() + rowPtr[j], col.begin() + rowPtr[j + 1], i) - col.begin();
            maxAsym = std::max(maxAsym, fabs(value[q] - value[p]) / value[p]);
          }
        }
        const passivedouble cv = SU2_TYPE::GetValue(target.Fine().nodes->GetVolume(i));
        maxRowError = std::max(maxRowError, fabs(sum - cv) / cv);
        minDominance = std::min(minDominance, diag / off);
      }
      CHECK(maxRowError < 1e-13);
      CHECK(maxAsym < 1e-14);
      CHECK(minDominance > 1.0);

      /*--- Constant field: exact. ---*/
      passivedouble maxError = 0.0;
      for (const auto v : result) maxError = std::max(maxError, fabs(v - 1.0));
      CHECK(maxError < 1e-13);
    }
  }
}

TEST_CASE("Conservative projection: random fields keep their totals", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    for (const passivedouble tolerance : {2e-3, 0.0}) {
      SECTION("nDim " + std::to_string(nDim) + ", limiter tolerance " + std::to_string(tolerance)) {
        auto config = MakeConfig(nDim, "SOLVER= EULER\n");
        MeshSolution donor(config.get(), BoxMesh(nDim, 5, true), 0);
        MeshSolution target(config.get(), BoxMesh(nDim, 3, true), 0);
        CConservativeProjection::Options options;
        options.limiterTolerance = tolerance;
        CConservativeProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, options);

        const unsigned short nField = 3;
        std::mt19937 gen(3);
        std::uniform_real_distribution<passivedouble> dist(-1.0, 1.0);
        auto values = DonorValues(donor.Fine(), nField, [&](const su2double*, passivedouble* v) {
          v[0] = dist(gen);        // random, sign changes
          v[1] = 5.0 + dist(gen);  // random, positive
          v[2] = 1e3 * dist(gen) * dist(gen);
        });
        std::vector<passivedouble> result;
        projection.Project(nField, values, result);
        const auto& s = projection.GetSummary();
        for (unsigned short f = 0; f < nField; ++f) {
          const passivedouble donorTotal = Total(donor.Fine(), values, nField, f);
          const passivedouble newTotal = Total(target.Fine(), result, nField, f);
          const passivedouble scale = Scale(donor.Fine(), values, nField, f);
          CHECK(fabs(newTotal - donorTotal) < 1e-14 * scale);
          CHECK(fabs(s.supermeshDefect[f]) < 1e-14);
          CHECK_FALSE(s.infeasible[f]);
          /*--- Global bounds of the donor, widened by the tolerance. ---*/
          passivedouble lo = 1e300, hi = -1e300;
          for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
            lo = std::min(lo, values[iPoint * nField + f]);
            hi = std::max(hi, values[iPoint * nField + f]);
          }
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            CHECK(result[iPoint * nField + f] >= lo - tolerance * (hi - lo) - 1e-12 * (hi - lo));
            CHECK(result[iPoint * nField + f] <= hi + tolerance * (hi - lo) + 1e-12 * (hi - lo));
          }
        }
      }
    }
  }
}

TEST_CASE("Conservative projection: identity and limiter bounds", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      MeshSolution donor(config.get(), BoxMesh(nDim, 5, true), 0);
      std::mt19937 gen(5);
      std::uniform_real_distribution<passivedouble> dist(-1.0, 1.0);
      const auto random = DonorValues(donor.Fine(), 1, [&](const su2double*, passivedouble* v) { v[0] = dist(gen); });

      /*--- Same mesh: the identity (the limiter changes nothing). ---*/
      {
        CConservativeProjection same(donor.Fine(), donor.markerTags, donor.Fine(), donor.markerTags, {});
        std::vector<passivedouble> result;
        same.Project(1, random, result);
        passivedouble maxError = 0.0;
        for (auto i = 0ul; i < result.size(); ++i) maxError = std::max(maxError, fabs(result[i] - random[i]));
        CHECK(maxError < 1e-12);
        CHECK(same.GetSummary().nLimited[0] == 0);
      }

      /*--- A step (values 0 and 1, the jump across x = 0.53): the unlimited projection over- and undershoots, the
       *    limited one (tolerance 0) stays in [0, 1] and within the values of the donor points near each new point
       *    (a superset of its donor elements), with the same total. ---*/
      MeshSolution target(config.get(), BoxMesh(nDim, 7, true), 0);
      const auto step = DonorValues(donor.Fine(), 1, [](const su2double* x, passivedouble* v) {
        v[0] = SU2_TYPE::GetValue(x[0]) < 0.53 ? 1.0 : 0.0;
      });
      CConservativeProjection::Options noLimiter, hard;
      noLimiter.limiter = false;
      hard.limiterTolerance = 0.0;
      std::vector<passivedouble> raw, limited;
      CConservativeProjection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, noLimiter)
          .Project(1, step, raw);
      CConservativeProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, hard);
      projection.Project(1, step, limited);
      CHECK(projection.GetSummary().nLimited[0] > 0);

      const passivedouble total = Total(donor.Fine(), step, 1, 0);
      CHECK(fabs(Total(target.Fine(), raw, 1, 0) - total) < 1e-14 * total);
      CHECK(fabs(Total(target.Fine(), limited, 1, 0) - total) < 1e-14 * total);
      const passivedouble rawMin = *std::min_element(raw.begin(), raw.end());
      const passivedouble rawMax = *std::max_element(raw.begin(), raw.end());
      CHECK((rawMin < -1e-3 || rawMax > 1.0 + 1e-3));
      const passivedouble radius = 1.5 * (1.0 / 5 + 1.0 / 7) * sqrt(nDim);
      for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
        const auto* x = target.Fine().nodes->GetCoord(iPoint);
        passivedouble lo = 1e300, hi = -1e300;
        for (auto jPoint = 0ul; jPoint < donor.Fine().GetnPoint(); ++jPoint) {
          const auto* y = donor.Fine().nodes->GetCoord(jPoint);
          passivedouble d2 = 0.0;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) d2 += SU2_TYPE::GetValue(pow(x[iDim] - y[iDim], 2));
          if (sqrt(d2) > radius) continue;
          lo = std::min(lo, step[jPoint]);
          hi = std::max(hi, step[jPoint]);
        }
        CHECK(limited[iPoint] >= lo - 1e-14);
        CHECK(limited[iPoint] <= hi + 1e-14);
      }
    }
  }
}

TEST_CASE("Conservative transfer: affine and constant flow fields", "[Adaptation]") {
  /*--- Where the two domains coincide, an affine field is reproduced exactly (u_h^A = u, so S_j = integral of u over
   *    C_j, which the nodal values of u satisfy: M u = S); the error is that of the linear solver. ---*/
  for (const unsigned short nDim : {2, 3}) {
    for (const bool constant : {false, true}) {
      SECTION("nDim " + std::to_string(nDim) + (constant ? ", constant" : ", affine")) {
        auto config = MakeConfig(nDim, "SOLVER= EULER\n");
        MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
        const Field field = constant ? Field([nDim](const su2double*, su2double* U) {
          const su2double x0[3] = {0.3, 0.4, 0.5};
          AffineFlow(nDim)(x0, U);
        })
                                     : AffineFlow(nDim);
        donor.SetField(FLOW_SOL, field);

        MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
        CConservativeTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        const auto& summary = transfer.GetSummary();
        CHECK(summary.nFlowFixed == 0);
        CHECK(summary.projection.nFillPieces == 0);
        for (unsigned short f = 0; f < nDim + 2; ++f) {
          CHECK(fabs(summary.relativeDefect[f]) < 1e-13);
          CHECK(summary.projection.nLimited[f] == 0);
        }
        CHECK(transfer.GetReport().conservationDefect < 1e-13);

        const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
        passivedouble maxDiff = 0.0, maxPressureDiff = 0.0;
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
          su2double exact[MAXVAR] = {}, U[MAXVAR] = {};
          field(target.Fine().nodes->GetCoord(iPoint), exact);
          for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
            U[iVar] = nodes->GetSolution(iPoint, iVar);
            maxDiff = max(maxDiff, RelDiff(U[iVar], exact[iVar]));
          }
          maxPressureDiff =
              max(maxPressureDiff, RelDiff(nodes->GetPressure(iPoint), IdealGasPressure(nDim, config->GetGamma(), U)));
        }
        CHECK(maxDiff < (constant ? 1e-13 : 1e-11));
        CHECK(maxPressureDiff < 1e-12);
        CheckCoarseLevels(target, FLOW_SOL);
      }
    }
  }
}

TEST_CASE("Conservative transfer: smooth field converges at second order", "[Adaptation]") {
  /*--- Smooth (not affine) field from a mesh to a finer one, two refinements: the maximum nodal error drops by about 4
   *    per halving of the sizes, the totals are exact. ---*/
  const unsigned short nDim = 2;
  auto config = MakeConfig(nDim, "SOLVER= EULER\n");
  std::vector<passivedouble> errors;
  for (const unsigned long n : {8ul, 16ul}) {
    MeshSolution donor(config.get(), BoxMesh(nDim, n, true), 0);
    MeshSolution target(config.get(), BoxMesh(nDim, n + n / 2, true), 0);
    donor.SetField(FLOW_SOL, [](const su2double* x, su2double* U) { SmoothFlow(2, x, U); });
    CConservativeTransfer transfer;
    {
      Mute mute;
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
    }
    for (unsigned short f = 0; f < nDim + 2; ++f) CHECK(fabs(transfer.GetSummary().relativeDefect[f]) < 1e-13);
    const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
    passivedouble maxError = 0.0;
    for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
      su2double exact[MAXVAR] = {};
      SmoothFlow(2, target.Fine().nodes->GetCoord(iPoint), exact);
      maxError = std::max(maxError, RelDiff(nodes->GetSolution(iPoint, 0), exact[0]));
    }
    errors.push_back(maxError);
  }
  CHECK(errors[0] < 2e-3);
  CHECK(errors[0] / errors[1] > 3.0);
}

TEST_CASE("Conservative transfer: curved boundary outside the donor", "[Adaptation]") {
  /*--- Donor disk/ball with its boundary inscribed in the circle/sphere, new mesh finer with its boundary points on
   *    it: new fluid outside the donor (S_n) and donor fluid outside the new domain (S_d). BOUNDARY and GLOBAL: the
   *    totals are the donor's; a constant field becomes the constant times the volume ratio with GLOBAL. NONE: the
   *    totals change by exactly the S_n minus the S_d content, a constant field stays the same constant. The
   *    supermesh defect is round-off in every case. ---*/
  for (const unsigned short nDim : {2, 3}) {
    using Rule = CConservativeProjection::SliverRule;
    const std::vector<std::pair<Rule, bool>> rules = {{Rule::BOUNDARY, false},
                                                      {Rule::GLOBAL, false},
                                                      {Rule::NONE, false},
                                                      {Rule::CLOSED, false},
                                                      {Rule::CLOSED, true}};
    for (const auto& ruleCase : rules) {
      const auto rule = ruleCase.first;
      const bool openBoundary = ruleCase.second;
      /*--- CLOSED: like GLOBAL at walls, like NONE at open boundaries. ---*/
      const bool global = rule == Rule::GLOBAL || (rule == Rule::CLOSED && !openBoundary);
      const bool none = rule == Rule::NONE || (rule == Rule::CLOSED && openBoundary);
      const string name =
          rule == Rule::BOUNDARY
              ? "boundary"
              : rule == Rule::GLOBAL
                    ? "global"
                    : rule == Rule::NONE ? "none" : openBoundary ? "closed, open markers" : "closed, walls";
      SECTION("nDim " + std::to_string(nDim) + ", " + name) {
        auto config = MakeRoundConfig("SOLVER= EULER\n", "MARKER_EULER= (round_a, round_b)\n");
        MeshSolution donor(config.get(), simplex_test::MakeRoundMesh(nDim, 4, 1.0), 0);
        MeshSolution target(config.get(), simplex_test::MakeRoundMesh(nDim, 6, 1.0), 0);
        CConservativeProjection::Options options;
        options.sliverRule = rule;
        options.absoluteLimit = 0.02;
        if (openBoundary) options.openMarkers = {"round_a", "round_b"};
        CConservativeProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, options);

        const unsigned short nField = nDim + 3;
        auto values = DonorValues(donor.Fine(), nField, [nDim](const su2double* x, passivedouble* v) {
          su2double U[MAXVAR];
          SmoothFlow(nDim, x, U);
          for (unsigned short f = 0; f < nDim + 2; ++f) v[f] = SU2_TYPE::GetValue(U[f]);
          v[nDim + 2] = 2.0;  // constant
        });
        std::vector<passivedouble> result;
        projection.Project(nField, values, result);
        const auto& s = projection.GetSummary();
        CHECK(s.nFillPieces > 0);
        CHECK(s.nSliverElems > 0);
        CHECK(s.fillVolume > 0.0);
        CHECK(s.sliverVolume > 0.0);
        CHECK(s.nInteriorFill == 0);
        /*--- Measures: covered + S_n = new domain, covered + S_d = donor domain. ---*/
        CHECK(fabs(s.overlapVolume + s.fillVolume - s.targetVolume) < 1e-12 * s.targetVolume);
        CHECK(fabs(s.overlapVolume + s.sliverVolume - s.donorVolume) < 1e-12 * s.donorVolume);
        for (unsigned short f = 0; f < nField; ++f) {
          const passivedouble scale = Scale(donor.Fine(), values, nField, f);
          const passivedouble expected =
              Total(donor.Fine(), values, nField, f) + (none ? s.fill[f] - s.sliver[f] : 0.0);
          CHECK(fabs(Total(target.Fine(), result, nField, f) - expected) < 1e-13 * scale);
          if (none) CHECK(fabs(s.fill[f] - s.sliver[f]) > 1e-6 * scale);
          CHECK(fabs(s.supermeshDefect[f]) < 1e-13);
        }
        /*--- Constant field. ---*/
        const passivedouble ratio = s.donorCV / s.targetCV;
        passivedouble maxDev = 0.0;
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
          const auto v = result[iPoint * nField + nDim + 2];
          maxDev = std::max(maxDev, global ? fabs(v - 2.0 * ratio) : fabs(v - 2.0) / 2.0);
        }
        if (global || none) {
          CHECK(maxDev < 1e-12);
        } else {
          CHECK(maxDev < 0.5);
        }
        /*--- Smooth fields: close to the exact values (the donor is coarse, h about 0.5). ---*/
        passivedouble maxError = 0.0;
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
          su2double exact[MAXVAR];
          SmoothFlow(nDim, target.Fine().nodes->GetCoord(iPoint), exact);
          maxError = std::max(maxError, RelDiff(result[iPoint * nField], exact[0]));
        }
        CHECK(maxError < 0.1);
      }
    }
  }
}

TEST_CASE("Conservative transfer: time history and turbulence", "[Adaptation]") {
  /*--- U^n (solution and Solution_time_n) and U^(n-1) affine and different; SA: nu_tilde affine (transferred itself,
   *    exact); SST: k and omega affine, rho k and rho omega transferred (quadratic, not exact) with their totals. ---*/
  using Getter = su2activematrix& (*)(CVariable*);
  const Getter getters[] = {[](CVariable* n) -> su2activematrix& { return n->GetSolution(); },
                            [](CVariable* n) -> su2activematrix& { return n->GetSolution_time_n(); },
                            [](CVariable* n) -> su2activematrix& { return n->GetSolution_time_n1(); }};
  /*--- With RANS the lower side is a no-slip wall: momentum zero there (affine in y), as the solver imposes it. ---*/
  auto flowField = [](unsigned short nDim, int level, bool wall) {
    return Field([nDim, level, wall](const su2double* x, su2double* U) {
      AffineFlow(nDim)(x, U);
      U[0] *= 1.0 + 0.05 * level;
      U[1] += 7.0 * level * x[1];
      U[nDim + 1] += 1e3 * level * x[0];
      if (wall) {
        U[1] = (300.0 + 7.0 * level) * x[1];
        U[2] = (-50.0 - 3.0 * level) * x[1];
      }
    });
  };
  auto turbField = [](bool sst, int level) {
    return Field([sst, level](const su2double* x, su2double* v) {
      if (sst) {
        v[0] = 1e-3 + 2e-4 * x[0] + 1e-4 * level * x[1];
        v[1] = 1e3 + 100.0 * x[1] - 50.0 * level * x[0];
      } else {
        v[0] = 1e-4 + 2e-5 * x[0] - (1e-5 + 1e-6 * level) * x[1];
      }
    });
  };
  const std::vector<string> cases = {
      "SOLVER= EULER\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n",
      "SOLVER= EULER\nTIME_MARCHING= DUAL_TIME_STEPPING-1ST_ORDER\n",
      "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n",
      "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"};
  for (const auto& options : cases) {
    SECTION(options) {
      const unsigned short nDim = 2;
      auto config = MakeConfig(nDim, options + "TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\n");
      const bool rans = config->GetKind_Solver() == MAIN_SOLVER::RANS;
      const bool sst = rans && config->GetKind_Turb_Model() == TURB_MODEL::SST;
      const bool secondOrder = config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND;
      std::vector<unsigned short> solvers = {FLOW_SOL};
      if (rans) solvers.push_back(TURB_SOL);
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
      for (int level = 0; level < 3; ++level) {
        for (const auto iSol : solvers) {
          auto& array = getters[level](donor.solver[MESH_0][iSol]->GetNodes());
          const int fieldLevel = level == 0 ? 1 : level;
          const auto field = (iSol == FLOW_SOL) ? flowField(nDim, fieldLevel, rans) : turbField(sst, fieldLevel);
          su2double values[MAXVAR] = {};
          for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
            field(donor.Fine().nodes->GetCoord(iPoint), values);
            for (unsigned short iVar = 0; iVar < array.cols(); ++iVar) array(iPoint, iVar) = values[iVar];
          }
        }
      }
      MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
      CConservativeTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      const auto& summary = transfer.GetSummary();
      CHECK(summary.nTimeLevels == 2);
      CHECK(summary.interpolateTimeN1 == secondOrder);
      CHECK(summary.nFlowFixed == 0);
      CHECK(summary.nHistoryFixed == 0);
      CHECK(summary.nTurbLimited == 0);
      for (const auto defect : summary.relativeDefect) CHECK(fabs(defect) < 1e-12);
      CHECK(summary.relativeDefect.size() == (secondOrder ? 2u : 1u) * (nDim + 2 + (rans ? (sst ? 2 : 1) : 0)));

      for (int level = 0; level < 3; ++level) {
        const int expected = (level == 2 && secondOrder) ? 2 : 1;
        for (const auto iSol : solvers) {
          INFO("level " << level << ", solver " << iSol);
          const auto field = (iSol == FLOW_SOL) ? flowField(nDim, expected, rans) : turbField(sst, expected);
          const auto& array = getters[level](target.solver[MESH_0][iSol]->GetNodes());
          passivedouble maxDiff = 0.0;
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            su2double exact[MAXVAR] = {};
            field(target.Fine().nodes->GetCoord(iPoint), exact);
            for (unsigned short iVar = 0; iVar < array.cols(); ++iVar)
              maxDiff = max(maxDiff, RelDiff(array(iPoint, iVar), exact[iVar], 1e-6));
          }
          /*--- rho k is quadratic: second-order error, the rest is exact. ---*/
          CHECK(maxDiff < ((iSol == TURB_SOL && sst) ? 1e-2 : 1e-11));
          if (level == 0) continue;
          for (unsigned short iMesh = 1; iMesh <= target.nMGLevels; ++iMesh) {
            const auto& fine = getters[level](target.solver[iMesh - 1][iSol]->GetNodes());
            const auto& coarse = getters[level](target.solver[iMesh][iSol]->GetNodes());
            su2activematrix restricted = coarse;
            CSolver::MultigridRestriction(*target.geometry[iMesh - 1], fine, *target.geometry[iMesh], restricted);
            passivedouble maxCoarse = 0.0;
            for (auto iPoint = 0ul; iPoint < target.geometry[iMesh]->GetnPointDomain(); ++iPoint)
              for (unsigned short iVar = 0; iVar < coarse.cols(); ++iVar)
                maxCoarse = max(maxCoarse, RelDiff(coarse(iPoint, iVar), restricted(iPoint, iVar), 1e-6));
            CHECK(maxCoarse < 1e-14);
          }
        }
      }
    }
  }
}

TEST_CASE("Conservative transfer: inadmissible projected state", "[Adaptation]") {
  /*--- Donor with zero total energy (negative pressure) at one point: the projected states near it are not admissible;
   *    they are fixed (control-volume mean or a donor state), every state is admissible afterwards and the totals are
   *    kept by the redistribution. ---*/
  const unsigned short nDim = 2;
  auto config = MakeConfig(nDim, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(nDim, 4, false), 0);
  donor.SetField(FLOW_SOL, AffineFlow(nDim));
  auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
    const auto* x = donor.Fine().nodes->GetCoord(iPoint);
    if (fabs(x[0] - 1.0) < 1e-12 && fabs(x[1] - 0.5) < 1e-12) donorNodes->SetSolution(iPoint, nDim + 1, 0.0);
  }
  MeshSolution target(config.get(), BoxMesh(nDim, 8, false), 0);
  CConservativeTransfer transfer;
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  const auto& summary = transfer.GetSummary();
  CHECK(summary.nFlowFixed >= 1);
  auto* fluidModel = target.solver[MESH_0][FLOW_SOL]->GetFluidModel();
  const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
    su2double U[MAXVAR] = {};
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) U[iVar] = nodes->GetSolution(iPoint, iVar);
    CHECK(CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, U));
  }
  for (unsigned short f = 0; f < nDim + 2; ++f) CHECK(fabs(summary.relativeDefect[f]) < 1e-12);
}

TEST_CASE("Conservative transfer: no-slip wall points", "[Adaptation]") {
  /*--- Heat-flux walls on a disk/ball, donor momentum zero on the wall: the projected wall momentum is not zero (the
   *    control volume of a wall point reaches into the moving fluid); the transfer sets it to zero, as the solver
   *    imposes it, and redistributes it, so the momentum totals stay exact. ---*/
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeRoundConfig("SOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\n",
                                    "MARKER_HEATFLUX= (round_a, 0.0, round_b, 0.0)\n");
      MeshSolution donor(config.get(), simplex_test::MakeRoundMesh(nDim, 6, 1.0), 0);
      MeshSolution target(config.get(), simplex_test::MakeRoundMesh(nDim, 5, 1.0), 0);
      donor.SetField(FLOW_SOL, [nDim](const su2double* x, su2double* U) { SmoothFlow(nDim, x, U); });
      auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
      for (unsigned short iMarker = 0; iMarker < donor.Fine().GetnMarker(); ++iMarker)
        for (auto iVertex = 0ul; iVertex < donor.Fine().GetnVertex(iMarker); ++iVertex)
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            donorNodes->SetSolution(donor.Fine().vertex[iMarker][iVertex]->GetNode(), iDim + 1, 0.0);

      CConservativeTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      const auto& summary = transfer.GetSummary();
      CHECK(summary.nWallPoints > 0);
      CHECK(summary.maxWallMomentum > 1.0);
      for (unsigned short f = 0; f < nDim + 2; ++f) CHECK(fabs(summary.relativeDefect[f]) < 1e-13);
      const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
      passivedouble maxMomentum = 0.0;
      for (unsigned short iMarker = 0; iMarker < target.Fine().GetnMarker(); ++iMarker)
        for (auto iVertex = 0ul; iVertex < target.Fine().GetnVertex(iMarker); ++iVertex)
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            maxMomentum = std::max(maxMomentum, SU2_TYPE::GetValue(fabs(nodes->GetSolution(
                                                    target.Fine().vertex[iMarker][iVertex]->GetNode(), iDim + 1))));
      CHECK(maxMomentum == 0.0);
    }
  }
}
