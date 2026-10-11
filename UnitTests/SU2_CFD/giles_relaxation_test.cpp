#include "../../SU2_CFD/include/solvers/GilesRelaxation.hpp"
#include <cassert>
#include <cmath>
int main() {
  const auto check = [](double radius) {
    const auto end = GilesRelaxation(radius, radius, radius+0.1, 0.05, 0.3, 0.4, 0.05);
    assert(end.first == 0.05 && end.second == 0);
    const auto half = GilesRelaxation(radius+0.0025, radius, radius+0.1, 0.05, 0.3, 0.4, 0.05);
    assert(std::abs(half.first-0.175) < 1e-11 && half.second == 0);
    const auto middle = GilesRelaxation(radius+0.05, radius, radius+0.1, 0.05, 0.3, 0.4, 0.05);
    assert(middle.first == 0.3 && middle.second == 0.4);
    const auto outer = GilesRelaxation(radius+0.1, radius, radius+0.1, 0.05, 0.3, 0.4, 0.05);
    assert(std::abs(outer.first-0.05) < 1e-11 && outer.second == 0);
  };
  check(0.0); check(0.4); check(100.0);
  assert(GilesRelaxation(0.4,0.4,0.5,0.0,0.3,0.4,0.05).first == 0.3);
  assert(GilesRelaxation(0.4,0.4,0.4,0.05,0.3,0.4,0.05).second == 0.4);
}
