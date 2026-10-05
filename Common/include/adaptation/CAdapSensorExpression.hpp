/*!
 * \file CAdapSensorExpression.hpp
 * \brief Shared grammar and normalization for custom adaptation sensor expressions.
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

#include <cctype>
#include <cmath>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "../toolboxes/expression_toolbox.hpp"

namespace AdapSensorExpression {

/*--- Recursive descent validates grammar/arity and normalizes operands and unary signs for MEL.
 *     MEL supplies the expression tree and active evaluation. ---*/
class Validator {
  std::vector<std::string> tokens;
  size_t pos = 0;
  std::set<std::string> identifiers;

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
      /*--- MEL supports unary minus, but treats unary plus as part of a symbol. ---*/
      return sign == "+" ? Operand() : "(" + sign + Operand() + ")";
    }
    if (Take("(")) {
      const auto value = Expression();
      if (!Take(")")) Fail();
      /*--- MEL cannot strip parentheses around a one-character atom. ---*/
      if (value.size() == 1 && std::isalnum(static_cast<unsigned char>(value[0]))) return value;
      return "(" + value + ")";
    }
    const auto token = tokens[pos++];
    if (!std::isalnum(static_cast<unsigned char>(token[0])) && token[0] != '.') Fail();
    if (!Take("(")) {
      if (!std::isalpha(static_cast<unsigned char>(token[0]))) return token;
      identifiers.insert(token);
      /*--- MEL scans digit+e/E followed by a sign even inside identifiers.
       *     Its parenthesis removal fails on one-character symbols, which cannot contain that suffix. ---*/
      return token.size() == 1 ? token : "(" + token + ")";
    }
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
  const std::set<std::string>& GetIdentifiers() const { return identifiers; }

  std::string Run() {
    if (tokens.empty()) Fail();
    const auto result = Expression();
    if (pos != tokens.size()) Fail();
    return result;
  }
};

/*--- MEL extracts distinct symbols. Check both their names and their count, without depending on traversal order. ---*/
inline void CheckSymbols(const std::string& name, const std::set<std::string>& identifiers,
                         const std::vector<std::string>& symbols) {
  if (symbols.size() != identifiers.size() || std::set<std::string>(symbols.begin(), symbols.end()) != identifiers)
    throw std::invalid_argument("Custom adaptation sensor " + name + ": MEL symbol mismatch after normalization.");
}

}  // namespace AdapSensorExpression
