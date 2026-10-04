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
    layers.Apply(coord, metric, smallest);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short i = 0, k = 0; i < nDim; ++i)
        for (unsigned short j = i; j < nDim; ++j, ++k)
          withBL.metric[iPoint * nMetric + k] = SU2_TYPE::GetValue(metric[iPoint].m[i][j]);
  }

  /*--- Floor activity at the wall points: wall-normal size (vertex normal) raised by the fixed wall lines. ---*/
  report.nFloorRaised = 0;
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
        if (normalSize(&floored.metric[p * nMetric]) > 1.1 * normalSize(&withBL.metric[p * nMetric]))
          report.nFloorRaised++;
      }
    }
  }

  CMMGInterface mmg(config);
  auto& params = mmg.GetParameters();
  params.surface = false;
  params.boundaryLayer = true;
  params.swap = config.GetAdap_BL_Swap() ? 1 : 0;
  auto adapted = mmg.Adapt(withBL);

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

CSimplexMesh CBoundaryLayerRemesher::TwoPass(const CConfig& config, const CSimplexMesh& mesh,
                                             const CReferenceWall& reference, Report& report) {
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
  passivedouble minH0 = std::numeric_limits<passivedouble>::max();
  for (const auto& wall : walls) minH0 = std::min(minH0, wall.h0);
  const auto geomTol = (config.GetAdap_BL_GeomTol() > 0.0) ? SU2_TYPE::GetValue(config.GetAdap_BL_GeomTol())
                                                             : 0.25 * minH0;
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
    std::vector<unsigned long> fixedPoints;
    for (const auto& wall : walls) {
      const auto fixed = FixedWallPoints(mesh, wall.name, angle);
      fixedPoints.insert(fixedPoints.end(), fixed.begin(), fixed.end());
    }
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
    report.nPointA = meshA.GetnPoint();

    std::vector<std::array<passivedouble, 2>> failures;
    if (surface) {
      /*--- Gate (i): the other boundaries unchanged (their points restored to the exact input coordinates). ---*/
      const auto tol = 1e-9 * domainSize;
      for (const auto& marker : mesh.markers) {
        if (wallOf.count(marker.name)) continue;
        const auto* out = meshA.FindMarker(marker.name);
        std::set<unsigned long> inPoints(marker.elem.begin(), marker.elem.end());
        std::set<unsigned long> outPoints(out->elem.begin(), out->elem.end());
        bool same = (inPoints.size() == outPoints.size() && marker.elem.size() == out->elem.size());
        for (const auto q : outPoints) {
          if (!same) break;
          passivedouble bestDist = std::numeric_limits<passivedouble>::max();
          unsigned long best = 0;
          for (const auto p : inPoints) {
            const auto d = std::hypot(mesh.coord[2 * p] - meshA.coord[2 * q], mesh.coord[2 * p + 1] - meshA.coord[2 * q + 1]);
            if (d < bestDist) {
              bestDist = d;
              best = p;
            }
          }
          if (bestDist > tol) {
            same = false;
            break;
          }
          meshA.coord[2 * q] = mesh.coord[2 * best];
          meshA.coord[2 * q + 1] = mesh.coord[2 * best + 1];
        }
        if (!same) report.failedGate = "other boundaries changed (" + marker.name + ")";
      }

      /*--- Projection of the new wall points onto the reference: sweeps of partial moves keeping every cell valid
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
        const auto fixed = FixedWallPoints(meshA, wall.name, angle);
        const auto* marker = meshA.FindMarker(wall.name);
        std::set<unsigned long> points(marker->elem.begin(), marker->elem.end());
        for (const auto p : points) {
          if (fixed.count(p)) continue;
          const auto proj = reference.Project(&meshA.coord[2 * p], wall.name);
          if (proj.segment < 0 || proj.distance <= 0.0) continue;
          moves.push_back({p, {proj.x[0], proj.x[1]}});
        }
      }
      auto starOK = [&](unsigned long p, const passivedouble* newX) {
        const passivedouble old[2] = {meshA.coord[2 * p], meshA.coord[2 * p + 1]};
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
        (void)old;
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
    }

    /*--- Gates (ii) wall lines feasible for h0, (iii) wall points on the reference. ---*/
    report.maxResidual = 0.0;
    report.maxMidpointDeviation = 0.0;
    report.nWallPointA = 0;
    for (const auto& wall : walls) {
      const auto* marker = meshA.FindMarker(wall.name);
      const auto extent = BLWallRule::NormalExtent(meshA, wall.name, angle);
      for (auto iLine = 0ul; iLine < extent.size(); ++iLine) {
        const auto* a = &meshA.coord[2 * marker->elem[2 * iLine]];
        const auto* b = &meshA.coord[2 * marker->elem[2 * iLine + 1]];
        const passivedouble mid[2] = {0.5 * (a[0] + b[0]), 0.5 * (a[1] + b[1])};
        if (extent[iLine] > gate * wall.h0) failures.push_back({mid[0], mid[1]});
        const auto proj = reference.Project(mid, wall.name);
        if (proj.segment >= 0) report.maxMidpointDeviation = std::max(report.maxMidpointDeviation, proj.distance);
      }
      std::set<unsigned long> points(marker->elem.begin(), marker->elem.end());
      report.nWallPointA += points.size();
      for (const auto p : points) {
        const auto proj = reference.Project(&meshA.coord[2 * p], wall.name);
        if (proj.segment < 0) continue;
        report.maxResidual = std::max(report.maxResidual, proj.distance);
        if (surface && proj.distance > geomTol) failures.push_back({meshA.coord[2 * p], meshA.coord[2 * p + 1]});
      }
    }
    if (!failures.empty() && report.failedGate.empty())
      report.failedGate = std::to_string(failures.size()) + " wall lines/points fail the wall gates";
    if (!surface) {
      /*--- Fixed wall: the gates only report what the wall permits; the band reset is always accepted. ---*/
      report.failedGate.clear();
      break;
    }
    if (report.failedGate.empty()) break;

    /*--- Retry: wall size x 0.7 near the failures. ---*/
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
      cout << "WARNING: TWO_PASS pass A rejected after " << report.attempts << " attempts (" << report.failedGate
           << "); one-pass boundary-layer remesh of the input mesh instead." << endl;
    return BoundaryLayerPass(config, mesh, report);
  }
  report.passA = true;

  /*--- Pass B: the sensor metric of the input mesh on the pass-A points (log-Euclidean, barycentric), bounded. ---*/
  CBarycentricLocator locator(mesh);
  const auto armax = SU2_TYPE::GetValue(config.GetAdap_ARmax());
  for (auto iPoint = 0ul; iPoint < meshA.GetnPoint(); ++iPoint) {
    su2double x[3] = {0.0, 0.0, 0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = meshA.coord[iPoint * nDim + iDim];
    const auto stencil = locator.Locate(x);
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

CRemeshResult CBoundaryLayerRemesher::Remesh(const CConfig& config, const CGeometry& geometry,
                                             const su2activematrix& metric) {
  const int rank = SU2_MPI::GetRank();
  const auto startTime = SU2_MPI::Wtime();
  const auto mesh = CMMGInterface::ExtractMesh(config, geometry, metric);

  CSimplexMesh adapted;
  Report report;
  if (rank == MASTER_NODE) {
    cout << endl << "---------------------- Remesh (MMG, TWO_PASS boundary layer) ---------------------" << endl;
    std::vector<std::string> blNames;
    passivedouble minH0 = std::numeric_limits<passivedouble>::max();
    for (unsigned short iBL = 0; iBL < config.GetnAdap_BL(); ++iBL) {
      blNames.push_back(config.GetAdap_BL(iBL).marker);
      minH0 = std::min(minH0, SU2_TYPE::GetValue(config.GetAdap_BL(iBL).firstHeight));
    }
    const auto geomTol = (config.GetAdap_BL_GeomTol() > 0.0) ? SU2_TYPE::GetValue(config.GetAdap_BL_GeomTol())
                                                               : 0.25 * minH0;
    const auto& file = config.GetAdap_BL_Reference();
    CReferenceWall reference;
    const bool read = !config.GetAdap_BL_ReferenceRebase() && reference.Read(file);
    if (read) {
      const auto dist = reference.MaxDistance(mesh, blNames);
      const auto tol = std::max(10.0 * geomTol, 1e-9 * DomainSize(mesh));
      if (!(dist <= tol)) {
        SU2_MPI::Error("The boundary-layer reference wall " + file + " does not match the wall of the mesh (largest " +
                           "distance " + std::to_string(dist) + "). Set ADAP_BL_REFERENCE_REBASE= YES to rebuild it "
                           "from the current wall (the wall then follows the current mesh from now on).",
                       CURRENT_FUNCTION);
      }
      cout << "Reference wall read from " << file << " (" << reference.GetSegments().size()
           << " segments; largest distance of the wall from it " << dist << ")." << endl;
    } else {
      reference = CReferenceWall(mesh, blNames, SU2_TYPE::GetValue(config.GetAdap_Angle()));
      reference.Write(file);
      cout << "Reference wall fitted to the current wall (" << reference.GetSegments().size()
           << " segments) and written to " << file << "." << endl;
    }
    adapted = TwoPass(config, mesh, reference, report);
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
    cout << "  wall points after pass A " << report.nWallPointA << ", projected onto the reference " << report.nProjected
         << ", largest distance from the reference: points " << report.maxResidual << ", line midpoints "
         << report.maxMidpointDeviation << "." << endl;
    cout << "  wall size samples where max(ADAP_HMIN, 2 h0) exceeds the curvature cap: " << report.nSizeConflict
         << "; wall points whose normal size the fixed-wall floor raised > 1.1x: " << report.nFloorRaised << "."
         << endl;
    cout << "  final wall: largest L sin(turn/2) / h0 = " << report.maxExtentRatio << ", lines with h0_eff > 1.1 h0: "
         << report.nExtentAbove << "; remesh " << mmgTime - startTime << " s." << endl;
  }
  return result;
}
