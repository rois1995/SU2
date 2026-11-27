/*!
 * \file CAdaptationOptions.hpp
 * \brief Scalar adaptation defaults shared with configuration fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "../basic_types/datatype_structure.hpp"

namespace SU2Adaptation {

inline su2double EffectiveNorm(bool goal, bool supplied, su2double scalar) {
  if (goal && !supplied) return 1.0;
  return scalar;
}

}  // namespace SU2Adaptation
