/*!
 * \file CBoundaryLayerRemesher.cpp
 * \brief Two-pass remesh for boundary-layer walls (ADAP_BL_METHOD= TWO_PASS, 2D).
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

#include "../../include/adaptation/CBoundaryLayerRemesher.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/adaptation/CReaderSlices.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../include/adaptation/CBoundaryLayerMetric.hpp"

namespace {

using Mat = passivedouble[3][3];

void Unpack(unsigned short nDim, const passivedouble* metric, Mat& M) {
  for (unsigned short i = 0, k = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j, ++k) M[i][j] = M[j][i] = metric[k];
}

void Pack(unsigned short nDim, const Mat& M, passivedouble* metric) {
  for (unsigned short i = 0, k = 0; i < nDim; ++i)
    for (unsigned short j = i; j < nDim; ++j, ++k) metric[k] = 0.5 * (M[i][j] + M[j][i]);
}

/*--- Smallest eigenvalue of a metric (its largest size is its inverse square root). ---*/
passivedouble SmallestEigenvalue(unsigned short nDim, const passivedouble* metric) {
  Mat M = {{0.0}}, vec;
  passivedouble val[3], work[3];
  Unpack(nDim, metric, M);
  CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
  return *std::min_element(val, val + nDim);
}

/*--- Distance from p to the segment a-b. ---*/
passivedouble SegmentDistance(const passivedouble* p, const passivedouble* a, const passivedouble* b) {
  const passivedouble u[2] = {b[0] - a[0], b[1] - a[1]};
  const auto len2 = u[0] * u[0] + u[1] * u[1];
  passivedouble t = (len2 > 0.0) ? ((p[0] - a[0]) * u[0] + (p[1] - a[1]) * u[1]) / len2 : 0.0;
  t = std::max(0.0, std::min(1.0, t));
  return std::hypot(a[0] + t * u[0] - p[0], a[1] + t * u[1] - p[1]);
}

/*--- Signed area and smallest angle (radians) of a triangle. ---*/
passivedouble SignedArea(const passivedouble* a, const passivedouble* b, const passivedouble* c) {
  return 0.5 * ((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]));
}
passivedouble MinAngle(const passivedouble* a, const passivedouble* b, const passivedouble* c) {
  const passivedouble* p[3] = {a, b, c};
  passivedouble minAngle = M_PI;
  for (int k = 0; k < 3; ++k) {
    const auto* o = p[k];
    const auto* q = p[(k + 1) % 3];
    const auto* r = p[(k + 2) % 3];
    const passivedouble u[2] = {q[0] - o[0], q[1] - o[1]}, v[2] = {r[0] - o[0], r[1] - o[1]};
    const auto nu = std::hypot(u[0], u[1]), nv = std::hypot(v[0], v[1]);
    if (nu <= 0.0 || nv <= 0.0) return 0.0;
    minAngle = std::min(minAngle, std::acos(std::max(-1.0, std::min(1.0, (u[0] * v[0] + u[1] * v[1]) / (nu * nv)))));
  }
  return minAngle;
}

passivedouble DomainSize(const CSimplexMesh& mesh) {
  const auto nDim = mesh.nDim;
  passivedouble size2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    passivedouble lo = std::numeric_limits<passivedouble>::max(), hi = -lo;
    for (auto i = 0ul; i < mesh.GetnPoint(); ++i) {
      lo = std::min(lo, mesh.coord[i * nDim + iDim]);
      hi = std::max(hi, mesh.coord[i * nDim + iDim]);
    }
    size2 += (hi - lo) * (hi - lo);
  }
  return std::sqrt(size2);
}

/*--- Boundary-layer walls of the config present in the mesh. ---*/
struct WallInfo {
  std::string name;
  passivedouble h0, thickness;
};
std::vector<WallInfo> Walls(const CConfig& config, const CSimplexMesh& mesh) {
  std::vector<WallInfo> walls;
  for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL) {
    const auto& layer = config.GetAdap_BL(iBL);
    const auto* marker = mesh.FindMarker(layer.marker);
    if (marker == nullptr || marker->elem.empty()) continue;
    walls.push_back({layer.marker, SU2_TYPE::GetValue(layer.firstHeight), SU2_TYPE::GetValue(layer.thickness)});
  }
  return walls;
}

/*--- Points of a marker that must not move along it: sharp vertices and points shared with other markers. ---*/
std::set<unsigned long> FixedWallPoints(const CSimplexMesh& mesh, const std::string& name, passivedouble angle) {
  std::set<unsigned long> fixed;
  const auto* marker = mesh.FindMarker(name);
  if (marker == nullptr) return fixed;
  std::map<unsigned long, std::vector<unsigned long>> neighbours;
  for (auto iLine = 0ul; iLine < marker->GetnElem(2); ++iLine) {
    const auto a = marker->elem[2 * iLine], b = marker->elem[2 * iLine + 1];
    neighbours[a].push_back(b);
    neighbours[b].push_back(a);
  }
  std::set<unsigned long> other;
  for (const auto& m : mesh.markers)
    if (m.name != name) other.insert(m.elem.begin(), m.elem.end());
  for (const auto& entry : neighbours) {
    const auto p = entry.first;
    if (entry.second.size() != 2 || other.count(p)) {
      fixed.insert(p);
      continue;
    }
    const auto* x = &mesh.coord[2 * p];
    const auto* a = &mesh.coord[2 * entry.second[0]];
    const auto* b = &mesh.coord[2 * entry.second[1]];
    const passivedouble u[2] = {x[0] - a[0], x[1] - a[1]}, v[2] = {b[0] - x[0], b[1] - x[1]};
    const auto c = (u[0] * v[0] + u[1] * v[1]) / (std::hypot(u[0], u[1]) * std::hypot(v[0], v[1]));
    if (std::acos(std::max(-1.0, std::min(1.0, c))) > angle * M_PI / 180.0) fixed.insert(p);
  }
  return fixed;
}

}  // namespace

void CBoundaryLayerRemesher::LogEuclideanMean(unsigned short nDim, unsigned short n,
                                              const passivedouble* const* metrics, const passivedouble* weights,
                                              passivedouble* result) {
  Mat sum = {{0.0}};
  for (unsigned short k = 0; k < n; ++k) {
    Mat M = {{0.0}}, vec, L;
    passivedouble val[3], work[3];
    Unpack(nDim, metrics[k], M);
    CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
    for (unsigned short i = 0; i < nDim; ++i) val[i] = std::log(std::max(val[i], 1e-300));
    CBlasStructure::EigenRecomposition(L, vec, val, nDim);
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j) sum[i][j] += weights[k] * L[i][j];
  }
  Mat vec, E;
  passivedouble val[3], work[3];
  for (unsigned short i = 0; i < nDim; ++i)
    for (unsigned short j = 0; j < i; ++j) sum[i][j] = sum[j][i] = 0.5 * (sum[i][j] + sum[j][i]);
  CBlasStructure::EigenDecomposition(sum, vec, val, nDim, work);
  for (unsigned short i = 0; i < nDim; ++i) val[i] = std::exp(val[i]);
  CBlasStructure::EigenRecomposition(E, vec, val, nDim);
  Pack(nDim, E, result);
}

void CBoundaryLayerRemesher::BoundMetric(unsigned short nDim, passivedouble hmin, passivedouble hmax,
                                         passivedouble armax, passivedouble* metric) {
  Mat M = {{0.0}}, vec;
  passivedouble val[3], work[3];
  Unpack(nDim, metric, M);
  CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
  const auto eigMax = 1.0 / (hmin * hmin), eigMin = 1.0 / (hmax * hmax);
  for (unsigned short i = 0; i < nDim; ++i) val[i] = std::max(eigMin, std::min(eigMax, val[i]));
  const auto largest = *std::max_element(val, val + nDim);
  for (unsigned short i = 0; i < nDim; ++i) val[i] = std::max(val[i], largest / (armax * armax));
  CBlasStructure::EigenRecomposition(M, vec, val, nDim);
  Pack(nDim, M, metric);
}

CSimplexMesh CBoundaryLayerRemesher::BoundaryLayerPass(const CConfig& config, const CSimplexMesh& mesh,
                                                       Report& report) {
  const auto nDim = mesh.nDim;
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  const auto nPoint = mesh.GetnPoint();
  const auto walls = Walls(config, mesh);

  /*--- Boundary-layer metric of the walls of this mesh, intersected with its metric. ---*/
  std::vector<CBoundaryLayerMetric::Wall> blWalls;
  for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL) {
    const auto& layer = config.GetAdap_BL(iBL);
    const auto* marker = mesh.FindMarker(layer.marker);
    if (marker == nullptr || marker->elem.empty()) continue;
    CBoundaryLayerMetric::Wall wall;
    wall.layer = layer;
    std::map<unsigned long, unsigned long> local;
    for (const auto p : marker->elem) {
      if (local.count(p)) continue;
      const unsigned long index = local.size();
      local[p] = index;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) wall.coord.push_back(mesh.coord[p * nDim + iDim]);
    }
    for (auto iFace = 0ul; iFace < marker->GetnElem(nDim); ++iFace) {
      for (unsigned short k = 0; k < nDim; ++k) wall.conn.push_back(local[marker->elem[iFace * nDim + k]]);
      if (nDim == 3) wall.conn.push_back(local[marker->elem[iFace * nDim + 2]]);
    }
    blWalls.push_back(std::move(wall));
  }

  auto withBL = mesh;
  if (!blWalls.empty()) {
    std::vector<su2double> coord(mesh.coord.begin(), mesh.coord.end());
    std::vector<CBoundaryLayerMetric::Tensor> metric(nPoint);
    passivedouble smallest = std::numeric_limits<passivedouble>::max();
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      for (unsigned short i = 0, k = 0; i < nDim; ++i)
        for (unsigned short j = i; j < nDim; ++j, ++k)
          metric[iPoint].m[i][j] = metric[iPoint].m[j][i] = mesh.metric[iPoint * nMetric + k];
      smallest = std::min(smallest, SmallestEigenvalue(nDim, &mesh.metric[iPoint * nMetric]));
    }
    CBoundaryLayerMetric layers(nDim, blWalls, config.GetAdap_Angle());

    /*--- Sensor loss (SERIAL_BL_FIX_PLAN.md 2.1.2): band points where the sensor metric is finer than the
     *    boundary-layer normal size in the wall-normal direction (by more than the MetricCheck tolerance, 2.5%). ---*/
    report.nSensorLoss = 0;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto* m = &mesh.metric[iPoint * nMetric];
      for (unsigned short iWall = 0; iWall < layers.GetnWall(); ++iWall) {
        const auto sample = layers.Evaluate(iWall, &coord[iPoint * nDim], smallest);
        if (!(sample.weight > 0.0)) continue;
        passivedouble n[3] = {0.0, 0.0, 0.0}, nMn = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) n[iDim] = SU2_TYPE::GetValue(sample.normal[iDim]);
        for (unsigned short i = 0, k = 0; i < nDim; ++i)
          for (unsigned short j = i; j < nDim; ++j, ++k) nMn += (i == j ? 1.0 : 2.0) * m[k] * n[i] * n[j];
        if (nMn > 0.0 && 1.0 / std::sqrt(nMn) < 0.975 * SU2_TYPE::GetValue(sample.hn)) {
          report.nSensorLoss++;
          break;
        }
      }
    }
    layers.Apply(coord, metric, smallest);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short i = 0, k = 0; i < nDim; ++i)
        for (unsigned short j = i; j < nDim; ++j, ++k)
          withBL.metric[iPoint * nMetric + k] = SU2_TYPE::GetValue(metric[iPoint].m[i][j]);
  }

  /*--- Floor activity at the wall points: wall-normal size (vertex normal) raised by the fixed wall lines. ---*/
  report.nFloorRaised = 0;
  report.maxFloorRatio = 1.0;
  if (nDim == 2) {
    auto floored = withBL;
    CMMGInterface::FloorFixedBoundaryMetric(floored);
    for (const auto& wall : walls) {
      const auto* marker = withBL.FindMarker(wall.name);
      std::map<unsigned long, std::array<passivedouble, 2>> normal;
      for (auto iLine = 0ul; iLine < marker->GetnElem(2); ++iLine) {
        const auto a = marker->elem[2 * iLine], b = marker->elem[2 * iLine + 1];
        const passivedouble t[2] = {withBL.coord[2 * b] - withBL.coord[2 * a], withBL.coord[2 * b + 1] - withBL.coord[2 * a + 1]};
        const auto len = std::hypot(t[0], t[1]);
        for (const auto p : {a, b}) {
          auto& n = normal[p];  // lines may be oriented either way: align with the first one
          const passivedouble sign = (n[0] * (-t[1]) + n[1] * t[0] < 0.0) ? -1.0 : 1.0;
          n[0] += -sign * t[1] / len;
          n[1] += sign * t[0] / len;
        }
      }
      for (const auto& entry : normal) {
        const auto len = std::hypot(entry.second[0], entry.second[1]);
        if (len <= 0.0) continue;
        const passivedouble n[2] = {entry.second[0] / len, entry.second[1] / len};
        auto normalSize = [&](const passivedouble* m) {
          return 1.0 / std::sqrt(m[0] * n[0] * n[0] + 2.0 * m[1] * n[0] * n[1] + m[2] * n[1] * n[1]);
        };
        const auto p = entry.first;
        const auto ratio = normalSize(&floored.metric[p * nMetric]) / normalSize(&withBL.metric[p * nMetric]);
        report.maxFloorRatio = std::max(report.maxFloorRatio, ratio);
        if (ratio > 1.1) report.nFloorRaised++;
      }
    }
  }

  CMMGInterface mmg(config);
  auto& params = mmg.GetParameters();
  params.surface = false;
  params.boundaryLayer = true;
  params.swap = config.GetAdap_BL_Swap() ? 1 : 0;
  auto adapted = mmg.Adapt(withBL);
  const auto& check = mmg.GetMetricCheck();
  report.nMetricChecked = check.nChecked;
  report.nMetricViolations = check.nViolations;
  report.nMetricCornerViolations = check.nCornerViolations;
  report.metricWorstRatio = check.worstRatio;

  /*--- Wall feasibility of the final wall (what the fixed wall lines permit, KC diagnostic). ---*/
  report.maxExtentRatio = 0.0;
  report.nExtentAbove = 0;
  if (nDim == 2) {
    for (const auto& wall : walls) {
      for (const auto e : BLWallRule::NormalExtent(adapted, wall.name, SU2_TYPE::GetValue(config.GetAdap_Angle()))) {
        report.maxExtentRatio = std::max(report.maxExtentRatio, e / wall.h0);
        if (e > 1.1 * wall.h0) report.nExtentAbove++;
      }
    }
  }
  report.nPointB = adapted.GetnPoint();
  return adapted;
}

namespace {

passivedouble GeomTol(const CConfig& config) {
  if (config.GetAdap_BL_GeomTol() > 0.0) return SU2_TYPE::GetValue(config.GetAdap_BL_GeomTol());
  passivedouble minH0 = std::numeric_limits<passivedouble>::max();
  for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL)
    minH0 = std::min(minH0, SU2_TYPE::GetValue(config.GetAdap_BL(iBL).firstHeight));
  return 0.25 * minH0;
}

/*--- Boundary-layer markers of the config with lines in the mesh. ---*/
std::vector<std::string> WallNames(const CConfig& config, const CSimplexMesh& mesh) {
  std::vector<std::string> names;
  for (const auto& wall : Walls(config, mesh)) names.push_back(wall.name);
  return names;
}

/*--- Nearest and second-nearest candidate of a point (brute force). ---*/
void Nearest2(const CSimplexMesh& mesh, const passivedouble* x, const std::vector<unsigned long>& candidates,
              unsigned long& best, passivedouble& d1, passivedouble& d2) {
  d1 = d2 = std::numeric_limits<passivedouble>::max();
  best = 0;
  for (const auto p : candidates) {
    const auto d = std::hypot(mesh.coord[2 * p] - x[0], mesh.coord[2 * p + 1] - x[1]);
    if (d < d1) {
      d2 = d1;
      d1 = d;
      best = p;
    } else if (d < d2) {
      d2 = d;
    }
  }
}

std::vector<unsigned long> UniquePoints(const std::vector<unsigned long>& elem) {
  std::vector<unsigned long> points(elem.begin(), elem.end());
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
  return points;
}

std::string Number(passivedouble value) {
  std::ostringstream text;
  text << value;
  return text.str();
}

}  // namespace

std::vector<CBoundaryLayerRemesher::WallPiece> CBoundaryLayerRemesher::WallPieces(const CSimplexMesh& mesh,
                                                                                  const std::string& name,
                                                                                  const std::set<unsigned long>& breaks) {
  std::vector<WallPiece> pieces;
  const auto* marker = mesh.FindMarker(name);
  if (marker == nullptr) return pieces;
  const auto nLine = marker->GetnElem(2);
  std::map<unsigned long, std::vector<std::pair<unsigned long, unsigned long>>> adjacency;  // point -> (point, line)
  for (auto iLine = 0ul; iLine < nLine; ++iLine) {
    const auto a = marker->elem[2 * iLine], b = marker->elem[2 * iLine + 1];
    adjacency[a].push_back({b, iLine});
    adjacency[b].push_back({a, iLine});
  }
  auto isBreak = [&](unsigned long p) { return breaks.count(p) > 0 || adjacency[p].size() != 2; };
  std::vector<bool> used(nLine, false);
  auto walk = [&](unsigned long start, unsigned long line, unsigned long next) {
    WallPiece piece;
    piece.points = {start};
    used[line] = true;
    auto cur = next;
    while (true) {
      piece.points.push_back(cur);
      if (cur == start || isBreak(cur)) break;
      bool advanced = false;
      for (const auto& nb : adjacency[cur]) {
        if (used[nb.second]) continue;
        used[nb.second] = true;
        cur = nb.first;
        advanced = true;
        break;
      }
      if (!advanced) break;
    }
    return piece;
  };
  for (const auto& entry : adjacency) {
    if (!isBreak(entry.first)) continue;
    for (const auto& nb : entry.second)
      if (!used[nb.second]) pieces.push_back(walk(entry.first, nb.second, nb.first));
  }
  for (const auto& entry : adjacency) {
    for (const auto& nb : entry.second) {
      if (used[nb.second]) continue;
      auto piece = walk(entry.first, nb.second, nb.first);
      piece.closed = piece.points.size() > 2 && piece.points.back() == piece.points.front();
      pieces.push_back(piece);
    }
  }
  return pieces;
}

void CBoundaryLayerRemesher::AssignSegments(const CSimplexMesh& mesh, const CReferenceWall& reference,
                                            const std::string& marker, std::vector<WallPiece>& pieces) {
  for (auto& piece : pieces) {
    std::map<long, unsigned long> votes;
    for (auto k = 0ul; k + 1 < piece.points.size(); ++k) {
      const auto* a = &mesh.coord[2 * piece.points[k]];
      const auto* b = &mesh.coord[2 * piece.points[k + 1]];
      const passivedouble mid[2] = {0.5 * (a[0] + b[0]), 0.5 * (a[1] + b[1])};
      const auto proj = reference.Project(mid, marker);
      if (proj.segment >= 0) votes[proj.segment]++;
    }
    piece.segment = -1;
    unsigned long most = 0;
    for (const auto& vote : votes)
      if (vote.second > most) {
        most = vote.second;
        piece.segment = vote.first;
      }
  }
}

void CBoundaryLayerRemesher::CheckPieces(const CSimplexMesh& mesh, const CReferenceWall& reference,
                                         const std::string& marker, const std::vector<WallPiece>& pieces,
                                         passivedouble geomTol,
                                         const std::function<passivedouble(long, passivedouble)>& sizeAt,
                                         Report& report, std::vector<std::array<passivedouble, 2>>& failures) {
  /*--- Parameter interval of every piece on its segment (start, signed length), for the coverage check. ---*/
  struct Cover {
    passivedouble start, length;
  };
  std::map<long, std::vector<Cover>> covers;
  for (const auto& piece : pieces) {
    const auto n = piece.points.size();
    if (n < 2) continue;
    auto x = [&](unsigned long k) { return &mesh.coord[2 * piece.points[k]]; };
    auto midpoint = [&](unsigned long k) {
      return std::array<passivedouble, 2>{0.5 * (x(k)[0] + x(k + 1)[0]), 0.5 * (x(k)[1] + x(k + 1)[1])};
    };
    if (piece.segment < 0) {
      report.nG3 += n - 1;
      for (auto k = 0ul; k + 1 < n; ++k) failures.push_back(midpoint(k));
      continue;
    }
    const auto& seg = reference.GetSegments()[piece.segment];
    const auto length = seg.s.back();

    /*--- G3: points on their own segment. ---*/
    std::vector<CReferenceWall::Projection> proj(n);
    for (auto k = 0ul; k < n; ++k) {
      proj[k] = reference.Project(x(k), marker, piece.segment);
      if (piece.closed && k + 1 == n) continue;  // the first point again
      report.maxResidual = std::max(report.maxResidual, proj[k].distance);
      if (!(proj[k].distance <= geomTol)) {
        report.nG3++;
        failures.push_back({x(k)[0], x(k)[1]});
      }
    }
    /*--- An open segment that starts and ends at the same point (a loop with one sharp corner, e.g. an airfoil with a
     *    sharp trailing edge): the end points of the piece take the segment end next to their neighbour. ---*/
    if (!seg.closed && !piece.closed && n > 2) {
      const auto& first = seg.x.front();
      const auto& last = seg.x.back();
      for (const auto k : {0ul, n - 1}) {
        const auto* p = x(k);
        if (std::hypot(p[0] - first[0], p[1] - first[1]) > geomTol || std::hypot(p[0] - last[0], p[1] - last[1]) > geomTol)
          continue;
        const auto neighbour = proj[k == 0 ? 1 : n - 2].s;
        proj[k].s = (neighbour < 0.5 * length) ? seg.s.front() : length;
      }
    }

    /*--- G3: parameters in order along the piece (closed segments: steps wrapped, a closed piece turns once). ---*/
    std::vector<passivedouble> step(n - 1);
    passivedouble total = 0.0;
    for (auto k = 0ul; k + 1 < n; ++k) {
      auto d = proj[k + 1].s - proj[k].s;
      if (seg.closed) d -= length * std::round(d / length);
      step[k] = d;
      total += d;
    }
    const passivedouble direction = (total >= 0.0) ? 1.0 : -1.0;
    covers[piece.segment].push_back({total >= 0.0 ? proj[0].s : proj[0].s + total, std::fabs(total)});
    bool turnFailed = piece.closed && !(std::fabs(std::fabs(total) - length) <= 1e-6 * length);
    if (turnFailed) {
      report.nG3++;
      failures.push_back({x(0)[0], x(0)[1]});
    }
    for (auto k = 0ul; k + 1 < n; ++k) {
      const auto* a = x(k);
      const auto* b = x(k + 1);
      const auto mid = midpoint(k);
      const auto len = std::hypot(b[0] - a[0], b[1] - a[1]);
      /*--- Degenerate: shorter than the representation error of its coordinates (independent of a translation up to
       *    that error itself). ---*/
      const auto scale = std::max({std::fabs(a[0]), std::fabs(a[1]), std::fabs(b[0]), std::fabs(b[1])});
      const auto minLength = 64.0 * std::numeric_limits<passivedouble>::epsilon() * scale;
      /*--- G3: order, degenerate line, and the arc of the reference between the two parameters near the line. ---*/
      bool failed = !(step[k] * direction > 0.0) || !(len > minLength);
      if (!failed) {
        const auto deviation = reference.ArcChordDistance(seg, proj[k].s, proj[k].s + step[k], a, b);
        report.maxArcDeviation = std::max(report.maxArcDeviation, deviation);
        failed = !(deviation <= geomTol);
      }
      if (failed) {
        report.nG3++;
        failures.push_back(mid);
      }

      /*--- G5: lines away from the piece ends at most 2 t_w (very short lines reported). ---*/
      if (!sizeAt) continue;
      auto sMid = proj[k].s + 0.5 * step[k];
      if (seg.closed) sMid -= length * std::floor(sMid / length);
      const auto tw = sizeAt(piece.segment, sMid);
      if (!(tw > 0.0)) continue;
      if (!piece.closed) {
        const auto* first = x(0);
        const auto* last = x(n - 1);
        passivedouble nearEnd = std::numeric_limits<passivedouble>::max();
        for (const auto* p : {a, b})
          for (const auto* q : {first, last}) nearEnd = std::min(nearEnd, std::hypot(p[0] - q[0], p[1] - q[1]));
        if (nearEnd <= tw) continue;
      }
      const auto ratio = len / tw;
      report.maxSizeRatio = std::max(report.maxSizeRatio, ratio);
      if (ratio < 0.5) report.nUndersized++;
      if (!(ratio <= 2.0)) {
        report.nG5++;
        failures.push_back(mid);
      }
    }
  }

  /*--- G3 coverage: the pieces cover every segment of the marker exactly once (no segment lost, none covered twice,
   *    e.g. by both sides of a thin body projected onto one side). ---*/
  for (long iSeg = 0; iSeg < static_cast<long>(reference.GetSegments().size()); ++iSeg) {
    const auto& seg = reference.GetSegments()[iSeg];
    if (seg.marker != marker) continue;
    const auto length = seg.s.back();
    const auto tol = std::max(geomTol, 1e-9 * length);
    auto list = covers[iSeg];
    bool ok = !list.empty();
    if (ok && seg.closed) {
      for (auto& cover : list) cover.start -= length * std::floor(cover.start / length);
      std::sort(list.begin(), list.end(), [](const Cover& a, const Cover& b) { return a.start < b.start; });
      passivedouble sum = 0.0;
      for (auto k = 0ul; k < list.size(); ++k) {
        sum += list[k].length;
        const auto next = (k + 1 < list.size()) ? list[k + 1].start : list[0].start + length;
        ok = ok && std::fabs(list[k].start + list[k].length - next) <= tol;
      }
      ok = ok && std::fabs(sum - length) <= tol;
    } else if (ok) {
      std::sort(list.begin(), list.end(), [](const Cover& a, const Cover& b) { return a.start < b.start; });
      passivedouble reached = seg.s.front();
      for (const auto& cover : list) {
        ok = ok && std::fabs(cover.start - reached) <= tol;
        reached = cover.start + cover.length;
      }
      ok = ok && std::fabs(reached - length) <= tol;
    }
    if (!ok) {
      report.nG3++;
      passivedouble x[2];
      reference.Evaluate(seg, 0.5 * length, x);
      failures.push_back({x[0], x[1]});
    }
  }
}

unsigned long CBoundaryLayerRemesher::CountInverted(const CSimplexMesh& mesh,
                                                    std::vector<std::array<passivedouble, 2>>* failures) {
  unsigned long count = 0;
  for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem) {
    const auto* v = &mesh.elem[3 * iElem];
    const passivedouble* x[3] = {&mesh.coord[2 * v[0]], &mesh.coord[2 * v[1]], &mesh.coord[2 * v[2]]};
    if (BLWallRule::Orientation(x[0], x[1], x[2]) > 0) continue;
    ++count;
    if (failures != nullptr)
      failures->push_back({(x[0][0] + x[1][0] + x[2][0]) / 3.0, (x[0][1] + x[1][1] + x[2][1]) / 3.0});
  }
  return count;
}

std::string CBoundaryLayerRemesher::CheckOtherBoundaries(const CSimplexMesh& in, CSimplexMesh& out,
                                                         const std::vector<std::string>& blMarkers,
                                                         const std::vector<unsigned long>& required,
                                                         passivedouble tol, std::set<unsigned long>& breaks) {
  breaks.clear();
  const std::set<std::string> bl(blMarkers.begin(), blMarkers.end());
  std::map<unsigned long, unsigned long> outToIn;
  auto bind = [&](unsigned long q, unsigned long p) {
    const auto it = outToIn.find(q);
    if (it != outToIn.end() && it->second != p) return false;
    outToIn[q] = p;
    return true;
  };

  /*--- Markers other than the boundary-layer walls: bijective point map, the same edges. ---*/
  for (const auto& marker : in.markers) {
    if (bl.count(marker.name) || marker.elem.empty()) continue;
    const auto* other = out.FindMarker(marker.name);
    if (other == nullptr || other->GetnElem(2) != marker.GetnElem(2))
      return "marker " + marker.name + " lost or changed its number of lines";
    const auto inPoints = UniquePoints(marker.elem), outPoints = UniquePoints(other->elem);
    if (inPoints.size() != outPoints.size()) return "marker " + marker.name + " changed its number of points";
    std::map<unsigned long, unsigned long> local;
    std::set<unsigned long> usedIn;
    for (const auto q : outPoints) {
      unsigned long p;
      passivedouble d1, d2;
      Nearest2(in, &out.coord[2 * q], inPoints, p, d1, d2);
      if (!(d1 <= tol)) return "a point of marker " + marker.name + " moved by " + Number(d1);
      if (!(3.0 * d1 < d2)) return "a point of marker " + marker.name + " is not clearly closest to one input point";
      if (!usedIn.insert(p).second) return "two points of marker " + marker.name + " map onto one input point";
      if (!bind(q, p)) return "a point of marker " + marker.name + " maps onto two input points";
      local[q] = p;
    }
    std::vector<std::pair<unsigned long, unsigned long>> inEdges, outEdges;
    for (auto iLine = 0ul; iLine < marker.GetnElem(2); ++iLine)
      inEdges.push_back(std::minmax(marker.elem[2 * iLine], marker.elem[2 * iLine + 1]));
    for (auto iLine = 0ul; iLine < other->GetnElem(2); ++iLine)
      outEdges.push_back(std::minmax(local.at(other->elem[2 * iLine]), local.at(other->elem[2 * iLine + 1])));
    std::sort(inEdges.begin(), inEdges.end());
    std::sort(outEdges.begin(), outEdges.end());
    if (inEdges != outEdges) return "the lines of marker " + marker.name + " changed";
  }

  /*--- Required corners: each a boundary point of the output. ---*/
  std::vector<unsigned long> outBoundary;
  for (const auto& marker : out.markers) outBoundary.insert(outBoundary.end(), marker.elem.begin(), marker.elem.end());
  outBoundary = UniquePoints(outBoundary);
  for (const auto p : required) {
    unsigned long q;
    passivedouble d1, d2;
    Nearest2(out, &in.coord[2 * p], outBoundary, q, d1, d2);
    if (!(d1 <= tol))
      return "the required corner (" + Number(in.coord[2 * p]) + ", " + Number(in.coord[2 * p + 1]) + ") is lost";
    if (!(3.0 * d1 < d2)) return "a required corner is not clearly closest to one output point";
    if (!bind(q, p)) return "a required corner maps onto two input points";
    breaks.insert(q);
  }
  for (const auto& entry : outToIn) {
    out.coord[2 * entry.first] = in.coord[2 * entry.second];
    out.coord[2 * entry.first + 1] = in.coord[2 * entry.second + 1];
  }

  /*--- Boundary-layer walls: the same chains between the same corners. ---*/
  const std::set<unsigned long> inBreaks(required.begin(), required.end());
  for (const auto& name : blMarkers) {
    if (in.FindMarker(name) == nullptr) continue;
    std::vector<std::pair<unsigned long, unsigned long>> inEnds, outEnds;
    unsigned long inLoops = 0, outLoops = 0;
    for (const auto& piece : WallPieces(in, name, inBreaks)) {
      if (piece.closed) inLoops++;
      else inEnds.push_back(std::minmax(piece.points.front(), piece.points.back()));
    }
    for (const auto& piece : WallPieces(out, name, breaks)) {
      if (piece.closed) {
        outLoops++;
        continue;
      }
      const auto a = outToIn.find(piece.points.front()), b = outToIn.find(piece.points.back());
      if (a == outToIn.end() || b == outToIn.end()) return "wall marker " + name + " has a new chain end";
      outEnds.push_back(std::minmax(a->second, b->second));
    }
    std::sort(inEnds.begin(), inEnds.end());
    std::sort(outEnds.begin(), outEnds.end());
    if (inEnds != outEnds || inLoops != outLoops) return "the chains of wall marker " + name + " changed";
  }
  return "";
}

CBarycentricLocator::Stencil CBoundaryLayerRemesher::DonorStencil(CBarycentricLocator& locator, const su2double* x,
                                                                  const std::vector<std::string>& markers) {
  auto stencil = locator.Locate(x);
  if (stencil.inside || markers.empty()) return stencil;
  return locator.LocateOnBoundary(x, markers);
}

CSimplexMesh CBoundaryLayerRemesher::TwoPass(const CConfig& config, const CSimplexMesh& mesh,
                                             const CReferenceWall& reference, Report& report, const PassAHook& hook) {
  const auto nDim = mesh.nDim;
  if (nDim != 2) SU2_MPI::Error("ADAP_BL_METHOD= TWO_PASS is only implemented in 2D.", CURRENT_FUNCTION);
  const auto nMetric = CSimplexMesh::GetnMetric(nDim);
  const auto walls = Walls(config, mesh);
  const bool surface = config.GetAdap_Surface();
  const auto angle = SU2_TYPE::GetValue(config.GetAdap_Angle());
  const auto c = SU2_TYPE::GetValue(config.GetAdap_BL_CurvatureFactor());
  const auto gate = SU2_TYPE::GetValue(config.GetAdap_BL_GateFactor());
  const auto hmin = SU2_TYPE::GetValue(config.GetAdap_Hmin());
  const auto hmax = SU2_TYPE::GetValue(config.GetAdap_Hmax());
  const auto geomTol = GeomTol(config);
  const auto domainSize = DomainSize(mesh);
  report = Report();
  if (walls.empty()) return BoundaryLayerPass(config, mesh, report);

  std::map<std::string, const WallInfo*> wallOf;
  for (const auto& wall : walls) wallOf[wall.name] = &wall;
  std::vector<std::string> blNames;
  for (const auto& wall : walls) blNames.push_back(wall.name);

  /*--- Sensor tangential size at a point x of a wall with tangent t: the smallest extent along t of the sensor metric
   *    at x + d n on the fluid side, d = (0.1, 0.25, 0.5, 1) x thickness (interpolated, log-Euclidean). The wall points
   *    themselves are left out: there the tangential part of the sensor Hessian is not meaningful (no-slip: the MACH
   *    sensor is 0 along the wall; its Hessian at the wall mixes in the wall-normal derivatives). The extent of the unit
   *    ball along t, sqrt(t^T M^-1 t), is not shortened by a slight misalignment of a much finer normal size. ---*/
  CBarycentricLocator sensorLocator(mesh);
  auto sensorSize = [&](const std::string& name) {
    const auto thickness = wallOf.at(name)->thickness;
    return [&, thickness](const passivedouble* x, const passivedouble* t) {
      passivedouble best = std::numeric_limits<passivedouble>::max();
      for (const passivedouble side : {1.0, -1.0})
      for (const passivedouble fraction : {0.1, 0.25, 0.5, 1.0}) {
        const auto d = fraction * thickness;
        su2double y[3] = {x[0] - side * d * t[1], x[1] + side * d * t[0], 0.0};
        const auto stencil = sensorLocator.Locate(y);
        if (!stencil.inside) continue;
        const passivedouble* metrics[4];
        passivedouble weights[4], m[3];
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          metrics[k] = &mesh.metric[stencil.point[k] * nMetric];
          weights[k] = SU2_TYPE::GetValue(stencil.weight[k]);
        }
        LogEuclideanMean(nDim, stencil.nPoint, metrics, weights, m);
        const auto det = m[0] * m[2] - m[1] * m[1];
        best = std::min(best, std::sqrt((m[2] * t[0] * t[0] - 2.0 * m[1] * t[0] * t[1] + m[0] * t[1] * t[1]) / det));
      }
      return best;
    };
  };

  const auto& segments = reference.GetSegments();
  std::vector<std::vector<passivedouble>> scale(segments.size());
  CSimplexMesh meshA;

  /*--- Required corners of pass A: sharp vertices, chain ends and points shared with other markers. ---*/
  std::vector<unsigned long> fixedPoints;
  for (const auto& wall : walls) {
    const auto fixed = FixedWallPoints(mesh, wall.name, angle);
    fixedPoints.insert(fixedPoints.end(), fixed.begin(), fixed.end());
  }
  fixedPoints = UniquePoints(fixedPoints);

  for (unsigned short attempt = 0; attempt < 2; ++attempt) {
    report.attempts = attempt + 1;
    report.failedGate.clear();

    /*--- Wall size along the reference. ---*/
    std::vector<BLWallRule::SizeSamples> samples(segments.size());
    std::map<std::string, passivedouble> largestSize;
    report.nSizeConflict = 0;
    for (unsigned long iSeg = 0; iSeg < segments.size(); ++iSeg) {
      const auto it = wallOf.find(segments[iSeg].marker);
      if (it == wallOf.end()) continue;
      BLWallRule::SizeRule rule;
      rule.h0 = it->second->h0;
      rule.curvatureFactor = c;
      rule.hmin = hmin;
      rule.hmax = hmax;
      samples[iSeg] = BLWallRule::SampleSize(reference, iSeg, rule, sensorSize(segments[iSeg].marker));
      if (scale[iSeg].size() != samples[iSeg].size.size()) scale[iSeg].assign(samples[iSeg].size.size(), 1.0);
      const auto tmin = std::max(hmin, 2.0 * rule.h0);
      for (unsigned long j = 0; j < samples[iSeg].size.size(); ++j)
        samples[iSeg].size[j] = std::max(tmin, samples[iSeg].size[j] * scale[iSeg][j]);
      report.nSizeConflict += samples[iSeg].nConflict;
      if (const char* debugFile = std::getenv("SU2_BL_DEBUG_SIZES")) {  // developer output: the wall size samples
        std::ofstream out(std::string(debugFile) + "_" + std::to_string(iSeg) + "_" + std::to_string(attempt) + ".txt");
        for (unsigned long j = 0; j < samples[iSeg].s.size(); ++j) {
          passivedouble x[2], dx[2];
          reference.Evaluate(segments[iSeg], samples[iSeg].s[j], x, dx);
          const auto speed = std::hypot(dx[0], dx[1]);
          const passivedouble t[2] = {dx[0] / speed, dx[1] / speed};
          out << samples[iSeg].s[j] << " " << x[0] << " " << x[1] << " " << samples[iSeg].size[j] << " "
              << reference.Curvature(segments[iSeg], samples[iSeg].s[j]) << " "
              << sensorSize(segments[iSeg].marker)(x, t) << "\n";
        }
      }
      auto& largest = largestSize[segments[iSeg].marker];
      for (const auto t : samples[iSeg].size) largest = std::max(largest, t);
    }

    /*--- Pass-A metric: isotropic t_w + 0.25 d within the thickness of the walls, the sensor metric elsewhere. ---*/
    auto inA = mesh;
    for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) {
      const auto* x = &mesh.coord[2 * iPoint];
      passivedouble h = std::numeric_limits<passivedouble>::max();
      for (const auto& wall : walls) {
        const auto* marker = mesh.FindMarker(wall.name);
        passivedouble d = std::numeric_limits<passivedouble>::max();
        for (auto iLine = 0ul; iLine < marker->GetnElem(2) && d > 0.0; ++iLine)
          d = std::min(d, SegmentDistance(x, &mesh.coord[2 * marker->elem[2 * iLine]],
                                          &mesh.coord[2 * marker->elem[2 * iLine + 1]]));
        if (d >= wall.thickness) continue;
        const auto proj = reference.Project(x, wall.name);
        if (proj.segment < 0) continue;
        const auto tw = BLWallRule::SizeAt(samples[proj.segment], proj.s);
        h = std::min(h, tw + 0.25 * d);
      }
      if (h == std::numeric_limits<passivedouble>::max()) continue;
      /*--- No sensor cap inside the band: the sensor is fine in all directions inside a resolved boundary layer and
       *    would keep the band fine (no reset); pass B gets the sensor back from the input mesh. ---*/
      auto* m = &inA.metric[iPoint * nMetric];
      m[0] = m[2] = 1.0 / (h * h);
      m[1] = 0.0;
    }

    /*--- MMG pass A: boundary-layer walls adapted (surface), everything else required; swaps. ---*/
    CMMGInterface mmgA(config);
    auto& params = mmgA.GetParameters();
    params.surface = surface;
    params.boundaryLayer = false;
    params.swap = 1;
    params.localWallHmax = false;
    if (surface) {
      for (const auto& marker : mesh.markers)
        if (!wallOf.count(marker.name)) params.requiredMarkers.push_back(marker.name);
      params.requiredPoints = fixedPoints;
      std::vector<CMMGInterface::LocalParameter> local;
      for (const auto& marker : mesh.markers) {
        const auto it = wallOf.find(marker.name);
        if (it == wallOf.end()) continue;
        CMMGInterface::LocalParameter param;
        param.ref = marker.ref;
        param.hmin = 0.5 * std::max(hmin, 2.0 * it->second->h0);
        param.hmax = 1.5 * largestSize[marker.name];
        /*--- MMG's own curvature-based boundary size (hausd) must not override t_w: the geometry comes from the
         *    projection onto the reference and the wall gate; hausd = gate x h0 only stops MMG from coarsening it. ---*/
        param.hausd = gate * it->second->h0 * (attempt == 0 ? 1.0 : 0.5);
        local.push_back(param);
      }
      mmgA.SetLocalParameters(local);
    }
    meshA = mmgA.Adapt(inA);
    if (hook) hook(meshA, attempt);
    report.nPointA = meshA.GetnPoint();

    report.nG2 = report.nG3 = report.nG4 = report.nG5 = 0;
    report.maxResidual = report.maxArcDeviation = report.maxSizeRatio = 0.0;
    report.nUndersized = 0;
    report.nWallPointA = 0;
    for (const auto& wall : walls)
      if (const auto* marker = meshA.FindMarker(wall.name)) report.nWallPointA += UniquePoints(marker->elem).size();

    /*--- Fixed surface: G2 only reports what the wall permits; the band reset is always accepted. ---*/
    if (!surface) {
      for (const auto& wall : walls)
        for (const auto e : BLWallRule::NormalExtent(meshA, wall.name, angle))
          if (e > gate * wall.h0) report.nG2++;
      break;
    }

    /*--- G1: the other boundaries, the required corners and the wall chains unchanged (immediate fallback). ---*/
    std::set<unsigned long> breaks;
    const auto g1 = CheckOtherBoundaries(mesh, meshA, blNames, fixedPoints, 1e-9 * domainSize, breaks);
    if (!g1.empty()) {
      report.failedGate = "G1: " + g1;
      report.immediateFallback = true;
      break;
    }

    /*--- Pieces of the new walls between the corners, each on its own reference segment. ---*/
    std::map<std::string, std::vector<WallPiece>> pieces;
    for (const auto& wall : walls) {
      pieces[wall.name] = WallPieces(meshA, wall.name, breaks);
      AssignSegments(meshA, reference, wall.name, pieces[wall.name]);
    }

    /*--- Projection of the new wall points onto their own segment: sweeps of partial moves keeping every cell valid
     *    (positive area, smallest angle at least half of its value before the move). ---*/
    std::vector<std::vector<unsigned long>> star(meshA.GetnPoint());
    for (auto iElem = 0ul; iElem < meshA.GetnElem(); ++iElem)
      for (unsigned short k = 0; k < 3; ++k) star[meshA.elem[3 * iElem + k]].push_back(iElem);
    struct Move {
      unsigned long point;
      passivedouble target[2];
    };
    std::vector<Move> moves;
    for (const auto& wall : walls) {
      for (const auto& piece : pieces[wall.name]) {
        if (piece.segment < 0) continue;
        const auto n = piece.points.size();
        for (auto k = 0ul; k < n; ++k) {
          const auto p = piece.points[k];
          if (breaks.count(p) || (piece.closed && k + 1 == n)) continue;
          if (!piece.closed && (k == 0 || k + 1 == n)) continue;  // chain ends (breaks or degree != 2)
          const auto proj = reference.Project(&meshA.coord[2 * p], wall.name, piece.segment);
          if (proj.segment < 0 || proj.distance <= 0.0) continue;
          moves.push_back({p, {proj.x[0], proj.x[1]}});
        }
      }
    }
    auto starOK = [&](unsigned long p, const passivedouble* newX) {
      for (const auto e : star[p]) {
        const auto* v = &meshA.elem[3 * e];
        const passivedouble* before[3];
        for (int k = 0; k < 3; ++k) before[k] = &meshA.coord[2 * v[k]];
        const auto angleBefore = MinAngle(before[0], before[1], before[2]);
        const passivedouble* after[3];
        for (int k = 0; k < 3; ++k) after[k] = (v[k] == p) ? newX : before[k];
        if (SignedArea(after[0], after[1], after[2]) <= 0.0) return false;
        if (MinAngle(after[0], after[1], after[2]) < 0.5 * angleBefore) return false;
      }
      return true;
    };
    std::set<unsigned long> movedPoints;
    for (int sweep = 0; sweep < 20; ++sweep) {
      bool progress = false;
      for (const auto& move : moves) {
        auto* x = &meshA.coord[2 * move.point];
        const passivedouble d[2] = {move.target[0] - x[0], move.target[1] - x[1]};
        if (std::hypot(d[0], d[1]) <= 1e-15 * domainSize) continue;
        for (const auto frac : {1.0, 0.5, 0.25, 0.125}) {
          const passivedouble trial[2] = {x[0] + frac * d[0], x[1] + frac * d[1]};
          if (!starOK(move.point, trial)) continue;
          x[0] = trial[0];
          x[1] = trial[1];
          movedPoints.insert(move.point);
          progress = true;
          break;
        }
      }
      if (!progress) break;
    }
    report.nProjected = movedPoints.size();

    /*--- G2 wall lines feasible for h0; G3 geometry and G5 size per piece; G4 cells after the last move. ---*/
    std::vector<std::array<passivedouble, 2>> failures;
    for (const auto& wall : walls) {
      const auto* marker = meshA.FindMarker(wall.name);
      const auto extent = BLWallRule::NormalExtent(meshA, wall.name, angle);
      for (auto iLine = 0ul; iLine < extent.size(); ++iLine) {
        if (!(extent[iLine] > gate * wall.h0)) continue;
        const auto* a = &meshA.coord[2 * marker->elem[2 * iLine]];
        const auto* b = &meshA.coord[2 * marker->elem[2 * iLine + 1]];
        report.nG2++;
        failures.push_back({0.5 * (a[0] + b[0]), 0.5 * (a[1] + b[1])});
      }
      auto sizeAt = [&](long iSeg, passivedouble s) {
        return samples[iSeg].s.empty() ? 0.0 : BLWallRule::SizeAt(samples[iSeg], s);
      };
      CheckPieces(meshA, reference, wall.name, pieces[wall.name], geomTol, sizeAt, report, failures);
    }
    report.nG4 = CountInverted(meshA, &failures);
    if (failures.empty()) break;
    report.failedGate = "G2 " + std::to_string(report.nG2) + ", G3 " + std::to_string(report.nG3) + ", G4 " +
                        std::to_string(report.nG4) + ", G5 " + std::to_string(report.nG5) + " failures";

    /*--- Retry: wall size x 0.7 within 2 t_w of the failures (and half the Hausdorff distance, above). ---*/
    for (const auto& x : failures) {
      for (const auto& wall : walls) {
        const auto proj = reference.Project(x.data(), wall.name);
        if (proj.segment < 0 || proj.distance > wall.thickness) continue;
        auto& smp = samples[proj.segment];
        for (unsigned long j = 0; j < smp.s.size(); ++j)
          if (std::fabs(smp.s[j] - proj.s) <= 2.0 * smp.size[j]) scale[proj.segment][j] = 0.7;
      }
    }
  }

  if (!report.failedGate.empty()) {
    report.passA = false;
    if (SU2_MPI::GetRank() == MASTER_NODE)
      cout << "WARNING: TWO_PASS pass A rejected after " << report.attempts << " attempt(s) (" << report.failedGate
           << (report.immediateFallback ? ", no retry" : "")
           << "); one-pass boundary-layer remesh of the input mesh instead." << endl;
    return BoundaryLayerPass(config, mesh, report);
  }
  report.passA = true;

  /*--- Pass B: the sensor metric of the input mesh on the pass-A points (log-Euclidean, barycentric; boundary points
   *    outside the input mesh from the closest face of their own markers), bounded. ---*/
  std::vector<std::vector<std::string>> markersOf(meshA.GetnPoint());
  for (const auto& marker : meshA.markers)
    for (const auto p : UniquePoints(marker.elem)) markersOf[p].push_back(marker.name);
  CBarycentricLocator locator(mesh);
  const auto armax = SU2_TYPE::GetValue(config.GetAdap_ARmax());
  for (auto iPoint = 0ul; iPoint < meshA.GetnPoint(); ++iPoint) {
    su2double x[3] = {0.0, 0.0, 0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = meshA.coord[iPoint * nDim + iDim];
    const auto stencil = DonorStencil(locator, x, markersOf[iPoint]);
    const passivedouble* metrics[4];
    passivedouble weights[4];
    for (unsigned short k = 0; k < stencil.nPoint; ++k) {
      metrics[k] = &mesh.metric[stencil.point[k] * nMetric];
      weights[k] = SU2_TYPE::GetValue(stencil.weight[k]);
    }
    auto* m = &meshA.metric[iPoint * nMetric];
    LogEuclideanMean(nDim, stencil.nPoint, metrics, weights, m);
    BoundMetric(nDim, hmin, hmax, armax, m);
  }
  return BoundaryLayerPass(config, meshA, report);
}

std::string CBoundaryLayerRemesher::CheckReference(const CConfig& config, const CSimplexMesh& mesh,
                                                   const CReferenceWall& reference, passivedouble* distance) {
  const auto angle = SU2_TYPE::GetValue(config.GetAdap_Angle());
  if (!(std::fabs(reference.GetCornerAngle() - angle) <= 1e-9 * std::max(1.0, std::fabs(angle))))
    return "was made with the corner angle " + Number(reference.GetCornerAngle()) + ", not ADAP_ANGLE= " +
           Number(angle);
  const auto names = WallNames(config, mesh);
  for (const auto& name : names) {
    bool found = false;
    for (const auto& seg : reference.GetSegments()) found = found || (seg.marker == name);
    if (!found) return "has no segment of the boundary-layer marker " + name;
  }
  const auto dist = reference.MaxDistance(mesh, names);
  const auto tol = std::max(10.0 * GeomTol(config), 1e-9 * DomainSize(mesh));
  if (distance != nullptr) *distance = dist;
  if (!(dist <= tol))
    return "does not match the wall of the mesh (largest distance " + Number(dist) + ", more than " + Number(tol) + ")";
  return "";
}

bool CBoundaryLayerRemesher::CreatesReference(const CConfig& config) {
  return config.GetAdap_BL_ReferenceRebase() || !config.GetRestart();
}

std::string CBoundaryLayerRemesher::PrepareReference(const CConfig& config, const CSimplexMesh& mesh, bool create,
                                                     CReferenceWall& reference, std::string& info) {
  const auto& file = config.GetAdap_BL_Reference();
  const std::string hint =
      " Set ADAP_BL_REFERENCE_REBASE= YES to rebuild it from the current wall (the wall then follows the current mesh "
      "from now on).";
  if (create) {
    reference = CReferenceWall(mesh, WallNames(config, mesh), SU2_TYPE::GetValue(config.GetAdap_Angle()));
    reference.Write(file);
    info = "fitted to the current wall (" + std::to_string(reference.GetSegments().size()) + " segments) and written to " +
           file + (config.GetAdap_BL_ReferenceRebase() ? " (ADAP_BL_REFERENCE_REBASE= YES)." : ".");
    return "";
  }
  CReferenceWall read;
  std::string why;
  if (!read.Read(file, &why))
    return "The boundary-layer reference wall " + file + " " + why +
           ". A restart (RESTART_SOL= YES) reads the reference wall written by the first run." + hint;
  passivedouble dist = 0.0;
  const auto error = CheckReference(config, mesh, read, &dist);
  if (!error.empty()) return "The boundary-layer reference wall " + file + " " + error + "." + hint;
  reference = std::move(read);
  info = "read from " + file + " (" + std::to_string(reference.GetSegments().size()) +
         " segments; largest distance of the wall from it " + Number(dist) + ").";
  return "";
}

CRemeshResult CBoundaryLayerRemesher::Remesh(const CConfig& config, const CGeometry& geometry,
                                             const su2activematrix& metric) {
  const int rank = SU2_MPI::GetRank();
  const auto startTime = SU2_MPI::Wtime();
  const auto mesh = CMMGInterface::ExtractMesh(config, geometry, metric);

  CSimplexMesh adapted;
  Report report;
  if (rank == MASTER_NODE) {
    cout << endl << "---------------------- Remesh (MMG, TWO_PASS boundary layer) ---------------------" << endl;
    /*--- The reference wall: created on the first remesh of a fresh run (or rebased), read on a restart, kept in
     *    memory for the later remeshes of this run (SERIAL_BL_FIX_PLAN.md 2.2). Never refitted silently. ---*/
    std::string error, info;
    if (!reference) {
      const bool create = CreatesReference(config);
      auto fresh = std::make_unique<CReferenceWall>();
      error = PrepareReference(config, mesh, create, *fresh, info);
      if (error.empty()) reference = std::move(fresh);
    } else {
      passivedouble dist = 0.0;
      error = CheckReference(config, mesh, *reference, &dist);
      if (!error.empty()) error = "The boundary-layer reference wall of this run " + error + ".";
      info = "kept from the first remesh (largest distance of the wall from it " + Number(dist) + ").";
    }
    if (!error.empty()) SU2_MPI::Error(error, CURRENT_FUNCTION);
    cout << "Reference wall " << info << endl;
    adapted = TwoPass(config, mesh, *reference, report);
  }
  const auto mmgTime = SU2_MPI::Wtime();

  CRemeshResult result;
  result.slices = CReaderSlices::FromComplete(adapted, MASTER_NODE);
  result.markers = result.slices.markersWithElements;
  result.status = CRemeshResult::Status::COMPLETE;

  if (rank == MASTER_NODE) {
    cout << "TWO_PASS: pass A " << (report.passA ? "accepted" : "rejected (one-pass fallback)") << " after "
         << report.attempts << " attempt(s)";
    if (!report.failedGate.empty()) cout << ", last failed gate: " << report.failedGate;
    cout << "; " << mesh.GetnPoint() << " -> " << report.nPointA << " (pass A) -> " << report.nPointB
         << " points (pass B)." << endl;
    cout << "  gates of the last pass A: G2 " << report.nG2 << ", G3 " << report.nG3 << ", G4 " << report.nG4
         << ", G5 " << report.nG5 << " failures; wall points " << report.nWallPointA << ", projected "
         << report.nProjected << ", largest distance from the reference: points " << report.maxResidual
         << ", arcs from the lines " << report.maxArcDeviation << "; largest L / t_w " << report.maxSizeRatio
         << ", lines with L / t_w < 0.5: " << report.nUndersized << "." << endl;
    cout << "  wall size samples where max(ADAP_HMIN, 2 h0) exceeds the curvature cap: " << report.nSizeConflict
         << "; wall points whose normal size the fixed-wall floor raised > 1.1x: " << report.nFloorRaised
         << " (largest ratio " << report.maxFloorRatio << ")." << endl;
    cout << "  band points where the sensor is finer than the boundary-layer normal size: " << report.nSensorLoss
         << "; metric check of the boundary-layer pass: " << report.nMetricChecked << " fixed boundary points, "
         << report.nMetricViolations << " coarser (" << report.nMetricCornerViolations << " corners), worst ratio "
         << report.metricWorstRatio << "." << endl;
    cout << "  final wall: largest L sin(turn/2) / h0 = " << report.maxExtentRatio << ", lines with h0_eff > 1.1 h0: "
         << report.nExtentAbove << "; remesh " << mmgTime - startTime << " s." << endl;
  }
  return result;
}
