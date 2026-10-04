/*!
 * \file CAdapSensors.cpp
 * \brief Custom adaptation sensors, evaluated at each instantaneous metric sample.
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

#include "../../include/adaptation/CAdapSensors.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../include/solvers/CSolver.hpp"
#include "../../include/variables/CPrimitiveIndices.hpp"
#include "../../include/gradients/computeGradientsGreenGauss.hpp"
#include "../../include/gradients/computeGradientsLeastSquares.hpp"

namespace {

/*--- Recursive descent is used only to validate grammar/arity and parenthesize unary signs.
 *     MEL supplies the expression tree and active evaluation. ---*/
class Validator {
  std::vector<std::string> tokens;
  size_t pos = 0;

  [[noreturn]] void Fail() const { throw std::invalid_argument("Invalid ADAP_CUSTOM_SENSORS expression syntax."); }
  bool Take(const std::string& token) {
    if (pos == tokens.size() || tokens[pos] != token) return false;
    ++pos;
    return true;
  }
  std::string Expression() {
    auto result = Product();
    while (pos < tokens.size() && (tokens[pos] == "+" || tokens[pos] == "-")) {
      const auto op = tokens[pos++];
      result += op + Product();
    }
    return result;
  }
  std::string Product() {
    auto result = Operand();
    while (pos < tokens.size() && (tokens[pos] == "*" || tokens[pos] == "/")) {
      const auto op = tokens[pos++];
      result += op + Operand();
    }
    return result;
  }
  std::string Operand() {
    if (pos == tokens.size()) Fail();
    if (tokens[pos] == "+" || tokens[pos] == "-") {
      const auto sign = tokens[pos++];
      return "(" + sign + Operand() + ")";
    }
    if (Take("(")) {
      const auto value = Expression();
      if (!Take(")")) Fail();
      return "(" + value + ")";
    }
    const auto token = tokens[pos++];
    if (!std::isalnum(static_cast<unsigned char>(token[0])) && token[0] != '.') Fail();
    if (!Take("(")) return token;
    static const std::map<std::string, unsigned short> arities = {
        {"sqrt", 1}, {"cbrt", 1}, {"pow", 2}, {"hypot", 2}, {"log", 1}, {"exp", 1}, {"fabs", 1},
        {"fmax", 2}, {"fmin", 2}, {"cos", 1}, {"sin", 1}, {"tan", 1}, {"acos", 1}, {"asin", 1},
        {"atan", 1}, {"atan2", 2}};
    const auto it = arities.find(token);
    if (it == arities.end()) throw std::invalid_argument("Unknown adaptation sensor function: " + token);
    auto result = token + "(" + Expression();
    unsigned short nargs = 1;
    while (Take(",")) {
      result += "," + Expression();
      ++nargs;
    }
    if (!Take(")") || nargs != it->second) throw std::invalid_argument("Wrong arity for sensor function: " + token);
    return result + ")";
  }

public:
  explicit Validator(const std::string& expression) {
    /*--- Do not merge whitespace between operands: "1 2" and "a b" must be errors. ---*/
    static const std::regex number("([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][+-]?[0-9]+)?");
    for (size_t i = 0; i < expression.size();) {
      const auto c = static_cast<unsigned char>(expression[i]);
      if (std::isspace(c)) { ++i; continue; }
      const auto first = i;
      if (std::isalpha(c)) {
        while (i < expression.size()) {
          const auto x = static_cast<unsigned char>(expression[i]);
          if (!(std::isalnum(x) || x == '_' || x == '[' || x == ']')) break;
          ++i;
        }
      } else if (std::isdigit(c) || c == '.') {
        std::smatch match;
        const auto tail = expression.substr(i);
        if (!std::regex_search(tail, match, number, std::regex_constants::match_continuous)) Fail();
        i += match.length();
        try {
          if (!std::isfinite(static_cast<passivedouble>(std::stod(match.str())))) Fail();
        } catch (const std::out_of_range&) { Fail(); }
      } else {
        if (std::string("+-*/,()").find(c) == std::string::npos) Fail();
        ++i;
      }
      tokens.push_back(expression.substr(first, i - first));
      if (tokens.size() * 2 > 200) throw std::invalid_argument("Adaptation sensor expression exceeds 200-node bound.");
      if (tokens.size() > 1) {
        const auto& a = tokens[tokens.size() - 2];
        const auto& b = tokens.back();
        if ((a == "+" || a == "-") && (b == "+" || b == "-"))
          throw std::invalid_argument("Adjacent signs are unsupported; write 2-(-1), not 2- -1.");
      }
    }
  }
  std::string Run() {
    if (tokens.empty()) Fail();
    const auto result = Expression();
    if (pos != tokens.size()) Fail();
    return result;
  }
};

}  // namespace

std::string CAdapSensors::ValidateExpression(const std::string& expression) {
  return Validator(expression).Run();
}

CAdapSensors::Symbol CAdapSensors::Resolve(const std::string& name, CSolver* const* solvers) {
  const auto found = available.find(name);
  if (found != available.end()) return found->second;

  if (name.find("GRAD_") == 0) {
    const auto axis = name.find_last_of('_');
    if (axis == std::string::npos || axis + 2 != name.size())
      throw std::invalid_argument("Invalid gradient symbol: " + name);
    const auto dim = std::string("XYZ").find(name.back());
    if (dim >= nDim) throw std::invalid_argument("Gradient direction not available: " + name);
    const auto source = name.substr(5, axis - 5);
    const auto input = Resolve(source, solvers);
    if (input.kind != KIND::PRIMITIVE && input.kind != KIND::SCALAR && input.kind != KIND::DENSITY)
      throw std::invalid_argument("Only primitive and scalar input gradients are supported: " + name);
    auto slot = inputSlots.find(source);
    if (slot == inputSlots.end()) {
      const auto same = std::find_if(inputs.begin(), inputs.end(), [&](const Symbol& field) {
        return field.kind == input.kind && field.index == input.index && field.solver == input.solver;
      });
      const auto index = static_cast<unsigned short>(same - inputs.begin());
      if (same == inputs.end()) {
        if (inputs.size() == 20) throw std::invalid_argument("Custom sensors require more than 20 adaptation work columns.");
        inputs.push_back(input);
      }
      slot = inputSlots.emplace(source, index).first;
    }
    return {KIND::GRADIENT, slot->second, 0, static_cast<unsigned short>(dim)};
  }

  static const std::regex scalar("(TURB|SPECIES|SCALAR)\\[(0|[1-9][0-9]*)\\]");
  std::smatch match;
  if (std::regex_match(name, match, scalar)) {
    const unsigned short solver = match[1] == "TURB" ? TURB_SOL : SPECIES_SOL;
    if (!solvers[solver]) throw std::invalid_argument("Inactive solver in adaptation symbol: " + name);
    unsigned long index;
    try { index = std::stoul(match[2]); }
    catch (const std::exception&) { throw std::invalid_argument("Invalid scalar index: " + name); }
    if (index >= solvers[solver]->GetnVar()) throw std::invalid_argument("Scalar index out of range: " + name);
    return {KIND::SCALAR, static_cast<unsigned short>(index), solver};
  }
  std::string message = "Unknown, forward/self-referenced or unavailable adaptation symbol: " + name + "\nAvailable:";
  for (const auto& entry : available) message += " " + entry.first;
  message += " GRAD_<primitive>_<X|Y|Z>; TURB[i], SPECIES[i], SCALAR[i] and their gradients when active.";
  throw std::invalid_argument(message);
}

CAdapSensors::CAdapSensors(const CConfig& config, const CGeometry& geometry, CSolver* const* solvers)
    : nDim(geometry.GetnDim()), gamma(config.GetGamma()) {
  const auto indices = PrimitiveNameToIndexMap(CPrimitiveIndices<unsigned short>(false, false, nDim, 0));
  const auto kind = config.GetKind_Solver();
  for (const auto& entry : indices) {
    if (entry.first == "LAMINAR_VISCOSITY" || entry.first == "THERMAL_CONDUCTIVITY" || entry.first == "CP_TOTAL") {
      if (kind == MAIN_SOLVER::EULER) continue;
    }
    if (entry.first == "EDDY_VISCOSITY" && kind != MAIN_SOLVER::RANS) continue;
    available.emplace(entry.first, Symbol{KIND::PRIMITIVE, entry.second});
  }
  available["DENSITY"] = {KIND::DENSITY};
  available["ENERGY"] = {KIND::ENERGY};
  available["MACH"] = {KIND::MACH};
  available["TOTALPRESSURE"] = {KIND::TOTALPRESSURE};

  /*--- A full leading vector block is required for reflection at oblique symmetry planes.
   *     Scan all definitions, including intermediates used by a selected cascade. ---*/
  bool cartesian = false;
  for (const auto& definition : config.GetAdap_CustomSensors()) {
    const auto expression = ValidateExpression(definition.second);
    std::vector<std::string> symbols;
    mel::Parse<passivedouble>(expression, symbols);
    for (const auto& symbol : symbols) {
      if (symbol.find("GRAD_VELOCITY_") == 0) velocityBlock = true;
      if (symbol == "VELOCITY_X" || symbol == "VELOCITY_Y" || symbol == "VELOCITY_Z" || symbol.find("GRAD_") == 0)
        cartesian = true;
    }
  }
  if (cartesian) {
    for (unsigned short marker = 0; marker < config.GetnMarker_Periodic(); ++marker) {
      const auto* rotation = config.GetPeriodicRotAngles(config.GetMarker_PerBound(marker));
      for (unsigned short dim = 0; dim < 3; ++dim)
        if (SU2_TYPE::GetValue(rotation[dim]) != 0.0)
          throw std::invalid_argument("Cartesian velocity/gradient custom sensors do not support rotational periodicity.");
    }
  }
  if (velocityBlock) {
    for (unsigned short dim = 0; dim < nDim; ++dim) {
      const auto name = std::string("VELOCITY_") + "XYZ"[dim];
      inputSlots[name] = inputs.size();
      inputs.push_back(available.at(name));
    }
  }
  for (const auto& definition : config.GetAdap_CustomSensors()) {
    if (stages.size() == std::numeric_limits<unsigned short>::max())
      throw std::invalid_argument("Too many custom adaptation definitions.");
    Stage stage;
    stage.name = definition.first;
    std::vector<std::string> symbols;
    stage.tree = mel::Parse<passivedouble>(ValidateExpression(definition.second), symbols);
    for (const auto& name : symbols) stage.symbols.push_back(Resolve(name, solvers));
    available[stage.name] = {KIND::STAGE, static_cast<unsigned short>(stages.size())};
    stages.push_back(std::move(stage));
  }
  for (unsigned short sensor = 0; sensor < config.GetnAdap_Sensor(); ++sensor)
    selected.push_back(Resolve(config.GetAdap_Sensor(sensor), solvers));
  nWork = std::max(selected.size(), inputs.size());
  if (nWork > 20) throw std::invalid_argument("Custom sensors require more than 20 adaptation work columns.");
  if (cartesian) {
    bool symmetry = false, even = true;
    for (unsigned short marker = 0; marker < config.GetnMarker_All(); ++marker)
      symmetry |= config.GetMarker_All_KindBC(marker) == SYMMETRY_PLANE;
    for (const auto& definition : config.GetAdap_CustomSensors())
      even &= definition.second.find("fabs(") != std::string::npos || definition.second.find("pow(") != std::string::npos;
    if (symmetry && !even && SU2_MPI::GetRank() == MASTER_NODE)
      std::cout << "Warning: selected custom adaptation sensors must be mirror-even at MARKER_SYM. "
                   "Cartesian components may be odd; use fabs or an even power. Odd sensors have incorrect boundary Hessians."
                << std::endl;
  }

  /*--- Resize before acquiring any array views. Hessians retain nSelected columns. ---*/
  auto* nodes = solvers[FLOW_SOL]->GetNodes();
  nodes->GetAuxVar_Adapt().resize(geometry.GetnPoint(), nWork) = su2double(0.0);
  nodes->GetGradient_Adapt().resize(geometry.GetnPoint(), nWork, nDim, 0.0);
}

su2double CAdapSensors::Value(const Symbol& symbol, unsigned long point, CSolver* const* solvers) const {
  const auto* nodes = solvers[FLOW_SOL]->GetNodes();
  switch (symbol.kind) {
    case KIND::PRIMITIVE: return nodes->GetPrimitive(point, symbol.index);
    case KIND::SCALAR: return solvers[symbol.solver]->GetNodes()->GetSolution(point, symbol.index);
    case KIND::DENSITY: return nodes->GetDensity(point);
    case KIND::ENERGY: return nodes->GetEnergy(point);
    case KIND::MACH: return sqrt(nodes->GetVelocity2(point) / pow(nodes->GetSoundSpeed(point), 2));
    case KIND::TOTALPRESSURE: {
      const auto mach2 = nodes->GetVelocity2(point) / pow(nodes->GetSoundSpeed(point), 2);
      return nodes->GetPressure(point) * pow(1.0 + 0.5 * (gamma - 1.0) * mach2, gamma / (gamma - 1.0));
    }
    default: throw std::logic_error("Unexpected custom adaptation input kind.");
  }
}

void CAdapSensors::Sample(CSolver& flow, CGeometry& geometry, const CConfig& config, CSolver* const* solvers) {
  auto* nodes = flow.GetNodes();
  auto& auxiliary = nodes->GetAuxVar_Adapt();
  auto& gradient = nodes->GetGradient_Adapt();
  CVectorOfMatrix gradIn;
  if (!inputs.empty()) {
    auxiliary = su2double(0.0);
    std::fill(gradient.data(), gradient.data() + gradient.size(), su2double(0.0));
    for (unsigned long point = 0; point < geometry.GetnPoint(); ++point)
      for (unsigned short input = 0; input < inputs.size(); ++input)
        auxiliary(point, input) = Value(inputs[input], point, solvers);
    flow.InitiateComms(&geometry, &config, MPI_QUANTITIES::AUXVAR_ADAPT);
    flow.CompleteComms(&geometry, &config, MPI_QUANTITIES::AUXVAR_ADAPT);
    const auto idxVel = velocityBlock ? 0 : -1;
    if (config.GetKind_Hessian_Method() == GREEN_GAUSS)
      computeGradientsGreenGauss(&flow, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_ADAPT_GG, geometry, config,
                                 auxiliary, 0, inputs.size(), idxVel, gradient, false);
    else
      computeGradientsLeastSquares(&flow, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_ADAPT_LS, geometry, config,
                                   true, auxiliary, 0, inputs.size(), idxVel, gradient, nodes->GetRmatrix(), false);
    gradIn.resize(geometry.GetnPointDomain(), inputs.size(), nDim, 0.0);
    for (unsigned long point = 0; point < geometry.GetnPointDomain(); ++point)
      for (unsigned short input = 0; input < inputs.size(); ++input)
        for (unsigned short dim = 0; dim < nDim; ++dim) gradIn(point, input, dim) = gradient(point, input, dim);
  }
  auxiliary = su2double(0.0);
  std::fill(gradient.data(), gradient.data() + gradient.size(), su2double(0.0));
  std::vector<su2double> values(stages.size());
  const auto nChecks = stages.size() + selected.size();
  std::vector<unsigned long> invalid(nChecks, 0), first(nChecks, std::numeric_limits<unsigned long>::max());
  const auto check = [&](size_t stage, unsigned long point, const su2double& value) {
    if (!std::isfinite(SU2_TYPE::GetValue(value))) {
      ++invalid[stage];
      first[stage] = std::min(first[stage], geometry.nodes->GetGlobalIndex(point));
    }
  };
  for (unsigned long point = 0; point < geometry.GetnPointDomain(); ++point) {
    const auto read = [&](const Symbol& symbol) -> su2double {
      if (symbol.kind == KIND::STAGE) return values[symbol.index];
      if (symbol.kind == KIND::GRADIENT) return gradIn(point, symbol.index, symbol.dim);
      return Value(symbol, point, solvers);
    };
    for (unsigned short stage = 0; stage < stages.size(); ++stage) {
      values[stage] = mel::Eval<su2double>(stages[stage].tree, [&](int index) { return read(stages[stage].symbols[index]); });
      check(stage, point, values[stage]);
    }
    for (unsigned short sensor = 0; sensor < selected.size(); ++sensor) {
      auxiliary(point, sensor) = read(selected[sensor]);
      check(stages.size() + sensor, point, auxiliary(point, sensor));
    }
  }
  /*--- Every rank completes the point loop before reporting non-finite intermediates. ---*/
  for (size_t stage = 0; stage < nChecks; ++stage) {
    unsigned long count = 0, global = 0;
    count = CPassiveComm::AllreduceSum(invalid[stage]);
    if (!count) continue;
    global = CPassiveComm::AllreduceMin(first[stage]);
    passivedouble localCoord[3] = {}, coord[3] = {};
    for (unsigned long point = 0; point < geometry.GetnPointDomain(); ++point)
      if (geometry.nodes->GetGlobalIndex(point) == global)
        for (unsigned short dim = 0; dim < nDim; ++dim) localCoord[dim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(point, dim));
    CPassiveComm::Allreduce(localCoord, coord, 3, CPassiveComm::Op::SUM);
    std::ostringstream message;
    const auto name = stage < stages.size() ? stages[stage].name : config.GetAdap_Sensor(stage - stages.size());
    message << "Non-finite custom adaptation stage " << name << ": " << count
            << " owned points; first global point " << global << " at (" << coord[0] << ", " << coord[1] << ", " << coord[2] << ").";
    throw std::runtime_error(message.str());
  }
}
