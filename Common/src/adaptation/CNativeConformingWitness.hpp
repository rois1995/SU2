/*!
 * \file CNativeConformingWitness.hpp
 * \brief Internal witness closure declaration for the affine geometry tests.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "../../include/adaptation/CNativeAffineReference.hpp"

namespace SU2NativeGeometry {
namespace witness {

struct Chord {
  std::array<Point, 3> points;
  double error = 0;
};

struct Counters {
  size_t splits = 0, children = 0, chain2 = 0, chain3 = 0, chain4 = 0, chain5_8 = 0;
  size_t reject_dead_end = 0, reject_long = 0, reject_fan_key = 0, reject_not_finite = 0;
  size_t reject_orientation = 0, reject_projection = 0;
};

struct Result {
  std::vector<Chord> chords;
  std::vector<size_t> origin;
  Counters counters;
};

Result Close(const std::vector<Chord>& input, double resolution, AtlasBudget& budget);

} // namespace witness
} // namespace SU2NativeGeometry
