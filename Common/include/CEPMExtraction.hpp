/*!
 * \file CEPMExtraction.hpp
 * \brief Runtime state for classical EPM parameter extraction.
 * \author SU2 Contributors
 * \version 8.6.0 "Harrier"
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

#include "basic_types/datatype_structure.hpp"

/* Runtime state for serial, post-convergence classical EPM parameter recordings. */
struct CEPMExtraction {
  bool enabled = false;
  bool active = false;
  su2double amplitude = 0.0;
  unsigned short component = 1;
  bool permute = false;
  double angle = 0.0;
  unsigned short probe_pair = 3;
  unsigned long ambiguous = 0;
  unsigned long evaluations = 0;
  static constexpr double gap_tolerance = 1e-10;
};
