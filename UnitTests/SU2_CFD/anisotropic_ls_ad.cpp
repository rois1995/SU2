/* Standalone forward/reverse AD checks of QR and scalar fallback; build with either CoDiPack type. */
#include "../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../SU2_CFD/include/gradients/computeGradientsLeastSquares.hpp"
#include "../../SU2_CFD/include/limiters/computeLimiters.hpp"
#include "../../SU2_CFD/include/numerics/util.hpp"
#include <array>
#include <iostream>

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
#error "This check needs a CoDiPack active type."
#endif

struct Stencil {
  su2activematrix coord;
  std::vector<unsigned long> neighbors;
  explicit Stencil(size_t dim) : coord(2 * dim + 2, dim) {
    coord = su2double(0.0);
    for (size_t i = 1; i < coord.rows(); ++i) neighbors.push_back(i);
  }
  auto GetPoints(size_t) const -> const std::vector<unsigned long>& { return neighbors; }
  auto GetCoord(size_t point) -> su2double* { return coord[point]; }
};

template<size_t dim>
bool check(bool weighted, unsigned mode) {
  su2double parameter = 1.2;
#ifdef CODI_REVERSE_TYPE
  auto& tape = su2double::getTape();
  tape.reset();
  tape.setActive();
  tape.registerInput(parameter);
#else
  SU2_TYPE::SetDerivative(parameter, 1.0);
#endif
  Stencil nodes(dim);
  const su2double height = mode == 1 ? su2double(1e-6 * parameter) : su2double(mode == 2 ? 1e-4 : 1e-6);
  const su2double slope = mode == 0 ? parameter : su2double(1.2);
  for (size_t axis = 0; axis < dim; ++axis) {
    for (size_t side = 0; side < 2; ++side) {
      const size_t point = 2 * axis + side + 1;
      const su2double sign = side == 0 ? -1.0 : 1.0;
      if (axis == 0) {
        const su2double length = mode == 2 ? su2double(parameter * (side == 0 ? -.5 : 1.0)) : sign;
        nodes.coord(point, 0) = length * cos(.37);
        nodes.coord(point, 1) = length * sin(.37);
      } else if (axis == 1) {
        nodes.coord(point, 0) = -sign * sin(.37) * height;
        nodes.coord(point, 1) = sign * cos(.37) * height;
      } else {
        nodes.coord(point, 2) = sign;
      }
    }
  }
  /* The last neighbor coincides with the center, exercising zero rows and the WLS distance guard. */
  su2activematrix field(nodes.coord.rows(), 3);
  field = su2double(0.0);
  C3DDoubleMatrix R(1, dim, dim, 0.0), gradient(1, 3, dim, 7.0);
  for (size_t p = 0; p < field.rows(); ++p) {
    field(p, 1) = slope * nodes.coord(p, 0) - 2.0 * nodes.coord(p, 1);
    if (dim == 3) field(p, 1) += 3.0 * nodes.coord(p, 2);
    if (mode == 2) field(p, 1) = nodes.coord(p, 0) * nodes.coord(p, 0);
    field(p, 2) = 1.0;
  }
  for (const auto p : nodes.neighbors) {
    su2double weight = 1.0;
    if (weighted) {
      const su2double squared = GeometryToolbox::SquaredNorm(dim, nodes.GetCoord(p));
      if (!(squared > 0.0)) continue;
      weight = 1.0 / squared;
    }
    for (size_t i = 0; i < dim; ++i)
      for (size_t j = i; j < dim; ++j) R(0, i, j) += nodes.coord(p, i) * nodes.coord(p, j) * weight;
  }
  su2double inverse[dim][dim];
  bool valid = detail::leastSquaresInverse<dim>(0, R, inverse, true) == weighted;
  detail::solveLeastSquaresQR<dim>(0, 1, 3, nodes, weighted, field, R, gradient);
  su2double objective = 0.0;
  for (size_t d = 0; d < dim; ++d) {
    passivedouble expected = d == 0 ? 1.2 : (d == 1 ? -2.0 : 3.0);
    if (mode == 2)
      expected = (weighted ? .25 : .7) * 1.2 * pow(cos(.37), 2) * (d == 0 ? cos(.37) : (d == 1 ? sin(.37) : 0.0));
    valid &= fabs(SU2_TYPE::GetValue(gradient(0, 1, d)) - expected) < 1e-8;
    valid &= SU2_TYPE::GetValue(gradient(0, 0, d)) == 7.0;
    valid &= SU2_TYPE::GetValue(gradient(0, 2, d)) == 0.0;
    objective += (d + 1) * gradient(0, 1, d);
  }
#ifdef CODI_REVERSE_TYPE
  tape.registerOutput(objective);
  tape.setPassive();
  objective.setGradient(1.0);
  tape.evaluate();
  const auto derivative = parameter.getGradient();
#else
  const auto derivative = SU2_TYPE::GetDerivative(objective);
#endif
  passivedouble expectedDerivative = mode == 0 ? 1.0 : 0.0;
  if (mode == 2) expectedDerivative = (weighted ? .25 : .7) * pow(cos(.37), 2) * (cos(.37) + 2 * sin(.37));
  valid &= std::isfinite(derivative) && fabs(derivative - expectedDerivative) < 1e-7;
  std::cout << "dim=" << dim << " weighted=" << weighted << " mode=" << mode
            << " derivative=" << derivative << " valid=" << valid << '\n';
  return valid;
}

bool checkBounds(bool admissible) {
  su2double parameter = 1.2;
#ifdef CODI_REVERSE_TYPE
  auto& tape = su2double::getTape();
  tape.reset();
  tape.setActive();
  tape.registerInput(parameter);
#else
  SU2_TYPE::SetDerivative(parameter, 1.0);
#endif
  const su2double face = admissible ? parameter : su2double(std::numeric_limits<double>::infinity());
  const su2double cell = parameter * parameter;
  su2double objective = boundedReconstruction(face, cell, 0.0, 2.0);
#ifdef CODI_REVERSE_TYPE
  tape.registerOutput(objective);
  tape.setPassive();
  objective.setGradient(1.0);
  tape.evaluate();
  const auto derivative = parameter.getGradient();
#else
  const auto derivative = SU2_TYPE::GetDerivative(objective);
#endif
  const bool valid = std::isfinite(derivative) && fabs(derivative - (admissible ? 1.0 : 2.4)) < 1e-12 &&
                     fabs(SU2_TYPE::GetValue(objective) - (admissible ? 1.2 : 1.44)) < 1e-12;
  std::cout << "bounds admissible=" << admissible << " derivative=" << derivative << " valid=" << valid << '\n';
  return valid;
}

int main() {
  bool valid = true;
  for (const bool weighted : {false, true})
    for (unsigned mode = 0; mode < 3; ++mode) valid &= check<2>(weighted, mode) && check<3>(weighted, mode);
  valid &= checkBounds(true) && checkBounds(false);
  return valid ? 0 : 1;
}
