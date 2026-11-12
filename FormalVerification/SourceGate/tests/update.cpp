#include "SU2_CFD/include/variables/CVariable.hpp"
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

static_assert(std::is_same<su2double, double>::value, "passive double only");
static_assert(sizeof(unsigned long) == 8 && sizeof(double) == 8, "LP64 binary64 only");
static_assert(std::numeric_limits<double>::is_iec559, "IEEE-754 host required");

class Fixture final : public CVariable {
 public:
  Fixture(unsigned long rows, unsigned long cols) {
    nPoint = rows;
    nVar = cols;
    Solution.resize(rows, cols);
    Solution_Old.resize(rows, cols);
    for (unsigned long p = 0; p < rows; ++p)
      for (unsigned long v = 0; v < cols; ++v) {
        Solution(p, v) = 11 + p * cols + v;
        Solution_Old(p, v) = 2 + p * cols + v;
      }
  }
  double& current(unsigned long p, unsigned long v) { return Solution(p, v); }
  double& old(unsigned long p, unsigned long v) { return Solution_Old(p, v); }
};

std::uint64_t bits(double x) {
  std::uint64_t result;
  std::memcpy(&result, &x, sizeof result);
  return result;
}

int main() {
  unsigned long checks = 0;
  const double pairs[][2] = {{2, 3}, {0.0, -0.0}, {-0.0, -0.0},
    {std::numeric_limits<double>::denorm_min(), std::numeric_limits<double>::denorm_min()},
    {1.0, 0x1p-53}, {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()},
    {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
    {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()},
    {std::numeric_limits<double>::quiet_NaN(), 3}};
  for (unsigned long rows : {1UL, 2UL, 7UL})
    for (unsigned long cols : {1UL, 4UL, 5UL})
      for (unsigned long point = 0; point < rows; ++point)
        for (unsigned long component = 0; component < cols; ++component)
          for (const auto& pair : pairs) {
            Fixture fixture(rows, cols);
            fixture.old(point, component) = pair[0];
            const double expected = pair[0] + pair[1];
            fixture.AddSolution(point, component, pair[1]);
            for (unsigned long p = 0; p < rows; ++p)
              for (unsigned long v = 0; v < cols; ++v) {
                const bool target = p == point && v == component;
                const double current = target ? expected : 11 + p * cols + v;
                const double old = target ? pair[0] : 2 + p * cols + v;
                ++checks;
                if (bits(fixture.current(p, v)) != bits(current) || bits(fixture.old(p, v)) != bits(old)) {
                  std::cerr << "contract failed: current=" << fixture.current(p, v)
                            << " expected=" << current << " old=" << fixture.old(p, v)
                            << " expectedOld=" << old << '\n';
                  return 1;
                }
              }
          }
  std::cout << checks << " bitwise destination/frame checks passed\n";
}
