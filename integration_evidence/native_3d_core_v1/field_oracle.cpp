/* Independent-reference driver: the Python oracle, not this program, computes expected values. */
#include "Common/include/adaptation/CNativeField3D.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace SU2Native3D;
int main() {
  std::cout << std::setprecision(21);
  for (;;) {
    DonorCell donor;
    if (!(std::cin >> donor.cell.v[0].p.x)) break;
    std::cin >> donor.cell.v[0].p.y >> donor.cell.v[0].p.z;
    for (size_t i = 1; i < 4; ++i) std::cin >> donor.cell.v[i].p.x >> donor.cell.v[i].p.y >> donor.cell.v[i].p.z;
    Point p;
    std::cin >> p.x >> p.y >> p.z;
    if (!std::cin) return 2;
    for (size_t i = 0; i < 4; ++i) {
      donor.cell.v[i].id = i;
      donor.sensor[i] = {double(1 + i), .125, 0, 2 + .5 * i, 0, 1e10 * (1 + i)};
    }
    try {
      FrozenField field({donor});
      const auto value = field.Query(p);
      std::cout << 1;
      for (auto w : value.weights) std::cout << ' ' << w;
      const auto m = value.sensor;
      std::cout << ' ' << m.xx << ' ' << m.xy << ' ' << m.xz << ' ' << m.yy << ' ' << m.yz << ' ' << m.zz << ' '
                << field.Statistics().kernel.exact_orientations << '\n';
    } catch (const std::runtime_error& e) {
      if (std::string(e.what()) != "Query outside native 3D immutable original donors.") throw;
      std::cout << 0 << '\n';
    }
  }
}
