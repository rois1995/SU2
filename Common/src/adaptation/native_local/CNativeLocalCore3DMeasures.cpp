/*!
 * \file CNativeLocalCore3DMeasures.cpp
 * \brief Local double-arithmetic length and shape with the production exact orientation predicate.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace SU2NativeLocal3D {
namespace {
Point Difference(Point b, Point a) { return {b.x - a.x, b.y - a.y, b.z - a.z}; }

double FloatOrientation(Point a, Point b, Point c, Point d) {
  const auto u = Difference(b, a), v = Difference(c, a), w = Difference(d, a);
  return u.x * (v.y * w.z - v.z * w.y) + u.y * (v.z * w.x - v.x * w.z) + u.z * (v.x * w.y - v.y * w.x);
}

double NormSquared(Point a, Point b, const Tensor& m) {
  const auto d = Difference(b, a);
  return d.x * (m.xx * d.x + m.xy * d.y + m.xz * d.z) + d.y * (m.xy * d.x + m.yy * d.y + m.yz * d.z) +
         d.z * (m.xz * d.x + m.yz * d.y + m.zz * d.z);
}

double MetricLength(Point a, Point b, const Tensor& ma, const Tensor& mb, bool trial = false) {
  const double first = std::sqrt(NormSquared(a, b, ma));
  const double last = std::sqrt(NormSquared(a, b, mb));
  const double largest = std::max(first, last);
  if (!std::isfinite(first) || !std::isfinite(last) || largest <= 0) {
    if (trial) return first == 0 && last == 0 ? 0.0 : std::numeric_limits<double>::infinity();
    throw std::runtime_error("Native local core encountered an unrepresentable metric edge.");
  }
  const double r = std::min(first, last) / largest;
  return largest * (2. / 3.) * (1 + r + r * r) / (1 + r);
}

Tensor MeanTensor(const std::array<const Tensor*, 4>& tensors) {
  Tensor mean{0, 0, 0, 0, 0, 0};
  for (const auto* m : tensors) {
    mean.xx += 0.25 * m->xx;
    mean.xy += 0.25 * m->xy;
    mean.xz += 0.25 * m->xz;
    mean.yy += 0.25 * m->yy;
    mean.yz += 0.25 * m->yz;
    mean.zz += 0.25 * m->zz;
  }
  return mean;
}
}  // namespace

bool SampleMovedTensorFloat(const std::vector<DonorCell>& donors, Point point, Tensor& tensor, size_t* chosen) {
  size_t best = donors.size();
  double score = -std::numeric_limits<double>::infinity();
  std::array<double, 4> weights{};
  for (size_t i = 0; i < donors.size(); ++i) {
    const auto& v = donors[i].cell.v;
    const auto a = v[0].p, b = v[1].p, c = v[2].p, d = v[3].p;
    const double volume = FloatOrientation(a, b, c, d);
    if (!(volume > 0) || !std::isfinite(volume)) continue;
    const double x = FloatOrientation(a, point, c, d) / volume;
    const double y = FloatOrientation(a, b, point, d) / volume;
    const double z = FloatOrientation(a, b, c, point) / volume;
    const std::array<double, 4> trial{1 - x - y - z, x, y, z};
    if (!std::all_of(trial.begin(), trial.end(), [](double w) { return std::isfinite(w); })) continue;
    const double least = *std::min_element(trial.begin(), trial.end());
    if (least < score || (least == score && !(donors[i].Key() < donors[best].Key()))) continue;
    best = i;
    score = least;
    weights = trial;
  }
  if (best == donors.size()) return false;
  const double scale = *std::max_element(weights.begin(), weights.end());
  double sum = 0;
  for (auto& w : weights) {
    w = std::max(0., w) / scale;
    sum += w;
  }
  tensor = {0, 0, 0, 0, 0, 0};
  for (size_t k = 0; k < 4; ++k) {
    const auto& t = donors[best].sensor[k];
    const auto w = weights[k] / sum;
    tensor.xx += w * t.xx;
    tensor.xy += w * t.xy;
    tensor.xz += w * t.xz;
    tensor.yy += w * t.yy;
    tensor.yz += w * t.yz;
    tensor.zz += w * t.zz;
  }
  if (chosen) *chosen = best;
  return true;
}

std::vector<Id> UnresolvedSurfacePoints(const Owned& owned) {
  std::vector<Id> unresolved;
  for (const auto& entry : owned) {
    const auto& row = entry.second;
    for (unsigned face = 0; face < 4; ++face) {
      if (!row.physical[face].present) continue;
      for (unsigned a = 0; a < 4; ++a) {
        if (a == face) continue;
        for (unsigned b = a + 1; b < 4; ++b) {
          if (b == face) continue;
          const auto& first = row.cell.v[a];
          const auto& last = row.cell.v[b];
          if (MetricLength(first.p, last.p, row.sensor[a], row.sensor[b]) <= LongEdge) continue;
          unresolved.push_back(first.id);
          unresolved.push_back(last.id);
        }
      }
    }
  }
  std::sort(unresolved.begin(), unresolved.end());
  unresolved.erase(std::unique(unresolved.begin(), unresolved.end()), unresolved.end());
  return unresolved;
}

bool Mesh::Positive(Vertices vertices) const {
  return SU2Native3D::Orientation(points_[vertices[0]].node.p, points_[vertices[1]].node.p, points_[vertices[2]].node.p,
                                  points_[vertices[3]].node.p) > 0;
}

bool Mesh::FloatPositive(Vertices vertices) const {
  const auto p = [&](size_t k) { return points_[vertices[k]].node.p; };
  const auto volume = FloatOrientation(p(0), p(1), p(2), p(3));
  return volume > 0 && std::isfinite(volume);
}

bool Mesh::TrialPositive(Vertices vertices) const {
  return options_.float_trials ? FloatPositive(vertices) : Positive(vertices);
}

bool Mesh::ExactlyPositive(const Plan& plan) const {
  return std::all_of(plan.fresh.begin(), plan.fresh.end(), [this](const auto& row) { return Positive(row.vertices); });
}

bool Mesh::TrialEdgeShorter(const PointRow& a, const PointRow& b, double limit) {
  return MetricLength(a.node.p, b.node.p, a.tensor, b.tensor, true) < limit;
}

double Mesh::Length(const PointRow& a, const PointRow& b) {
  return MetricLength(a.node.p, b.node.p, a.tensor, b.tensor);
}

double Mesh::Length(Edge edge) const { return Length(points_[edge[0]], points_[edge[1]]); }

namespace {
MetricMeasures NodalShape(const SU2Native3D::Tetrahedron& tet, const std::array<const Tensor*, 4>& tensors) {
  const auto m = MeanTensor(tensors);
  const double det =
      m.xx * (m.yy * m.zz - m.yz * m.yz) - m.xy * (m.xy * m.zz - m.xz * m.yz) + m.xz * (m.xy * m.yz - m.xz * m.yy);
  const double volume = FloatOrientation(tet[0], tet[1], tet[2], tet[3]) * std::sqrt(det);
  const auto root = std::cbrt(std::abs(volume) / 2);
  double sum = 0, longest = 0;
  std::array<double, 4> product{1, 1, 1, 1};
  for (unsigned i = 0; i < 4; ++i) {
    for (unsigned j = i + 1; j < 4; ++j) {
      const auto squared = NormSquared(tet[i], tet[j], m);
      sum += squared;
      longest = std::max(longest, squared);
      product[i] *= squared;
      product[j] *= squared;
    }
  }
  const auto sign = std::copysign(1., volume);
  const double largest = *std::max_element(product.begin(), product.end());
  const double q = sign * 12 * root * root / sum;
  const double j = sign * std::min(1., std::sqrt(2.) * std::abs(volume) / std::sqrt(largest));
  if (!std::isfinite(q) || !std::isfinite(j))
    throw std::runtime_error("Native local core encountered an unrepresentable metric shape.");
  return {q, j, std::sqrt(sum / 6), std::sqrt(longest)};
}

double ShapeMargin(const MetricMeasures& shape) {
  return std::min(shape.mean_ratio / SU2Native3D::MinimumMeanRatio,
                  shape.minimum_scaled_jacobian / SU2Native3D::MinimumScaledJacobian);
}
}  // namespace

MetricMeasures Mesh::Shape(Vertices vertices) const {
  SU2Native3D::Tetrahedron tet;
  std::array<const Tensor*, 4> tensors;
  for (unsigned k = 0; k < 4; ++k) {
    tet[k] = points_[vertices[k]].node.p;
    tensors[k] = &points_[vertices[k]].tensor;
  }
  return NodalShape(tet, tensors);
}

double Mesh::Margin(Vertices vertices) const { return ShapeMargin(Shape(vertices)); }

double NodalMargin(const OwnedCell& row) {
  SU2Native3D::Tetrahedron tet;
  std::array<const Tensor*, 4> tensors;
  for (unsigned k = 0; k < 4; ++k) {
    tet[k] = row.cell.v[k].p;
    tensors[k] = &row.sensor[k];
  }
  return ShapeMargin(NodalShape(tet, tensors));
}

CollapseProbe FP3Probe() {
  static const auto probe = [] {
    const auto* value = std::getenv("SU2_NATIVE_FP3_PROBE");
    if (!value || !value[0] || value[1]) return CollapseProbe::None;
    if (value[0] == '1') return CollapseProbe::Core;
    if (value[0] == '2') return CollapseProbe::Engine;
    if (value[0] == '3') return CollapseProbe::FP3B;
    if (value[0] == '4') return CollapseProbe::FP3D;
    return CollapseProbe::None;
  }();
  return probe;
}
}  // namespace SU2NativeLocal3D
