// Standalone provider probe: reads physical x,y,z rows from stdin, writes E in m^2/s^2.
#include "../../../Common/include/toolboxes/stress_field.hpp"
#include <iostream>

int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::runtime_error("usage: stress_field_probe field.suf");
    const StressField::Field field(argv[1]);
    std::cerr << field.Summary() << '\n';
    std::cout << std::setprecision(17);
    StressField::Point point{};
    while (std::cin >> point[0] >> point[1] >> point[2]) {
      const auto tensor = field.Evaluate(point);
      std::cout << tensor[0][0] << ' ' << tensor[1][1] << ' ' << tensor[2][2] << ' ' << tensor[0][1] << ' '
                << tensor[0][2] << ' ' << tensor[1][2] << '\n';
    }
    if (!std::cin.eof()) throw std::runtime_error("invalid query row");
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
