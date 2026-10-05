/*!
 * \file CReferenceWall.inl
 * \brief Template part of the wall size rule (included by CReferenceWall.hpp).
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

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

template <class SensorSize>
BLWallRule::SizeSamples BLWallRule::SampleSize(const CReferenceWall& wall, unsigned long iSeg, const SizeRule& rule,
                                               const SensorSize& sensorSize, unsigned short samplesPerInterval) {
  const auto& seg = wall.GetSegments()[iSeg];
  SizeSamples out;
  const auto nInterval = seg.s.size() - 1;
  const unsigned short nPer = std::max<unsigned short>(samplesPerInterval, 1);
  const passivedouble tmin = std::max(rule.hmin, 2.0 * rule.h0);
  const passivedouble sStart = seg.s.front(), sEnd = seg.s.back();

  for (unsigned long i = 0; i < nInterval; ++i) {
    for (unsigned short j = 0; j < nPer; ++j) out.s.push_back(seg.s[i] + (seg.s[i + 1] - seg.s[i]) * j / nPer);
  }
  out.s.push_back(sEnd);

  for (const auto s : out.s) {
    passivedouble x[2], dx[2];
    wall.Evaluate(seg, s, x, dx);
    const passivedouble norm = std::sqrt(dx[0] * dx[0] + dx[1] * dx[1]);
    const passivedouble tangent[2] = {dx[0] / norm, dx[1] / norm};
    const passivedouble kappa = wall.Curvature(seg, s);
    const passivedouble cap = (kappa > 0.0) ? rule.curvatureFactor * std::sqrt(2.0 * rule.h0 / kappa)
                                            : std::numeric_limits<passivedouble>::infinity();
    const passivedouble sensor = sensorSize(x, tangent);
    passivedouble t = std::min({sensor, cap, rule.hmax});
    if (cap <= std::min(sensor, rule.hmax)) out.nCurvatureLimited++;
    if (tmin > cap) out.nConflict++;
    t = std::max(t, tmin);
    /*--- Sharp corners: small sizes into the corner (the fixed corner vertex keeps the floor of its two edges). ---*/
    if (seg.sharpStart) t = std::min(t, std::max(tmin, rule.cornerGrowth * (s - sStart)));
    if (seg.sharpEnd) t = std::min(t, std::max(tmin, rule.cornerGrowth * (sEnd - s)));
    out.size.push_back(t);
  }

  /*--- Gradation along the wall: |dt/ds| <= gradation (two sweeps; closed segments cyclic, twice). ---*/
  out.tmin = tmin;
  Grade(out, rule.gradation, seg.closed);
  return out;
}
