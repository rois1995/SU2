/*!
 * \file CAdapSensors.hpp
 * \brief Ordered expressions and staged input gradients for mesh adaptation.
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

#include <map>
#include <string>
#include <vector>

#include "../../../Common/include/toolboxes/expression_toolbox.hpp"

class CConfig;
class CGeometry;
class CSolver;
class CVariable;

/*!
 * \brief Bound per flow solver, without references to the mesh or donor solvers.
 * Expressions use cached flow primitives and current conservative density/energy and scalar solutions.
 * Sampling only writes adaptation arrays and the shared gradient workspace.
 */
class CAdapSensors {
public:
  enum class KIND { PRIMITIVE, SCALAR, MACH, ENERGY, DENSITY, TOTALPRESSURE, GRADIENT, STAGE };
  struct Symbol {
    KIND kind;
    unsigned short index = 0, solver = 0, dim = 0;
  };
  struct Stage {
    std::string name;
    mel::ExpressionTree<passivedouble> tree;
    std::vector<Symbol> symbols;
  };

  /*--- Validate the complete grammar before calling MEL's permissive parser. Throws invalid_argument. ---*/
  static std::string ValidateExpression(const std::string& expression);

  CAdapSensors(const CConfig& config, const CGeometry& geometry, CSolver* const* solvers);
  void Sample(CSolver& flow, CGeometry& geometry, const CConfig& config, CSolver* const* solvers);

  unsigned short GetnWork() const { return nWork; }
  unsigned short GetnStaged() const { return inputs.size(); }
  const std::vector<Stage>& GetStages() const { return stages; }

private:
  std::vector<Stage> stages;
  std::vector<Symbol> selected, inputs;
  std::map<std::string, Symbol> available;
  std::map<std::string, unsigned short> inputSlots;
  unsigned short nDim = 0, nWork = 0;
  bool velocityBlock = false;
  su2double gamma = 1.4;

  Symbol Resolve(const std::string& name, CSolver* const* solvers);
  su2double Value(const Symbol& symbol, unsigned long point, CSolver* const* solvers) const;
};
