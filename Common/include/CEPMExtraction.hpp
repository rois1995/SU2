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

#include <memory>

#include "basic_types/datatype_structure.hpp"
#include "option_structure.hpp"

/*! \brief Field-sensitivity site storage, defined with the field replay. Held through a shared_ptr so that
 *         CEPMExtraction (a member of CConfig) stays usable while the type is incomplete. */
class CEPMFieldSites;

/* Rank-local runtime state for post-convergence classical EPM parameter recordings. */
struct CEPMExtraction {
  /*! \brief Extraction strategy. FIELD records every site once; SCALAR_REPLAY keeps the scalar-amplitude replay. */
  using Mode = EPM_EXTRACTION_MODE;
  /*! \brief Kind of the site a recording belongs to. */
  enum class SiteKind : int { EDGE = 0, BOUNDARY = 1, NODE = 2 };
  /*! \brief Position of the evaluation currently being recorded. */
  struct Cursor {
    SiteKind kind = SiteKind::EDGE;
    unsigned long a = 0, b = 0;
    bool set = false;
  };

  bool enabled = false;
  Mode mode = Mode::FIELD;
  bool extract_only = false;
  Cursor cursor;
  std::shared_ptr<CEPMFieldSites> field;
  bool active = false;
  su2double amplitude = 0.0;
  unsigned short component = 1;
  bool permute = false;
  double angle = 0.0;
  unsigned short probe_pair = 3;
  unsigned long ambiguous = 0;
  unsigned long evaluations = 0;
  unsigned long stress_evaluations = 0;
  static constexpr double gap_tolerance = 1e-10;
};
