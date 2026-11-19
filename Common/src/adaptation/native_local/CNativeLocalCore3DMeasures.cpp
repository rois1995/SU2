/*!
 * \file CNativeLocalCore3DMeasures.cpp
 * \brief Local double-arithmetic length and shape with the production exact orientation predicate.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <cmath>
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

bool Mesh::Positive(Vertices vertices) const {
  return SU2Native3D::Orientation(points_[vertices[0]].node.p, points_[vertices[1]].node.p, points_[vertices[2]].node.p,
                                  points_[vertices[3]].node.p) > 0;
}

double Mesh::Length(Edge edge) const {
  const auto& a = points_[edge[0]];
  const auto& b = points_[edge[1]];
  const double first = std::sqrt(NormSquared(a.node.p, b.node.p, a.tensor));
  const double last = std::sqrt(NormSquared(a.node.p, b.node.p, b.tensor));
  const double largest = std::max(first, last);
  if (!std::isfinite(first) || !std::isfinite(last) || largest <= 0)
    throw std::runtime_error("Native local core encountered an unrepresentable metric edge.");
  const double r = std::min(first, last) / largest;
  return largest * (2. / 3.) * (1 + r + r * r) / (1 + r);
}

MetricMeasures Mesh::Shape(Vertices vertices) const {
  SU2Native3D::Tetrahedron tet;
  std::array<const Tensor*, 4> tensors;
  for (unsigned k = 0; k < 4; ++k) {
    tet[k] = points_[vertices[k]].node.p;
    tensors[k] = &points_[vertices[k]].tensor;
  }
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

double Mesh::Margin(Vertices vertices) const {
  const auto shape = Shape(vertices);
  return std::min(shape.mean_ratio / SU2Native3D::MinimumMeanRatio,
                  shape.minimum_scaled_jacobian / SU2Native3D::MinimumScaledJacobian);
}
}  // namespace SU2NativeLocal3D
