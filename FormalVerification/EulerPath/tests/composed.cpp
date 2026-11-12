#include "Common/include/linear_algebra/CSysVector.hpp"
#include "SU2_CFD/include/variables/CVariable.hpp"
#include <iostream>
#include <type_traits>

static_assert(std::is_same<su2double, double>::value, "passive double only");
static_assert(sizeof(unsigned long) == 8 && sizeof(double) == 8, "LP64 only");

class Fixture final : public CVariable {
 public:
  Fixture() {
    nPoint = 3;
    nVar = 4;
    Solution.resize(nPoint, nVar);
    Solution_Old.resize(nPoint, nVar);
    Delta_Time.resize(nPoint);
    for (unsigned long p = 0; p < nPoint; ++p) {
      Delta_Time(p) = 1 + p;
      for (unsigned long c = 0; c < nVar; ++c) {
        Solution(p, c) = 97;
        Solution_Old(p, c) = 20 + 4*p + c;
      }
    }
  }
};
struct GeometryNodes {
  double GetVolume(unsigned long) const { return 2; }
  double GetPeriodicVolume(unsigned long) const { return 0; }
};
struct Geometry { GeometryNodes storage; GeometryNodes* nodes = &storage; };

struct Identity {
#include "selected-apply.inc"
};

void update(Fixture* nodes, Geometry* geometry, unsigned long iPoint,
            const double* residual, const double* trunc) {
#include "selected-scale.inc"
  Identity preconditioner;
  for (unsigned short iVar = 0; iVar < 4; ++iVar) {
    const double Res = preconditioner.apply(iVar, residual, trunc);
    switch (EULER_EXPLICIT) {
#include "selected-call.inc"
      default: std::abort();
    }
  }
}

template<size_t N> struct Flux {
  static constexpr size_t StaticSize = 4;
  simd::Array<double, N> values[4];
  const simd::Array<double, N>& operator[](size_t i) const { return values[i]; }
};

template<size_t N> bool check(unsigned long& checks) {
  for (unsigned pattern = 0; pattern < (1u << N); ++pattern) {
    Fixture state;
    Geometry geometry;
    CSysVector<double> residual(3, 3, 4, 0.0);
    double expected[3][4];
    simd::Array<unsigned long, N> left, right;
    simd::Array<double, N> mask;
    Flux<N> flux;
    for (unsigned p = 0; p < 3; ++p)
      for (unsigned c = 0; c < 4; ++c)
        expected[p][c] = residual(p, c) = 5 + p + c;
    for (unsigned k = 0; k < N; ++k) {
      left[k] = k % 3;
      right[k] = (k + 1) % 3;
      mask[k] = bool(pattern & (1u << k));
      for (unsigned c = 0; c < 4; ++c) {
        flux.values[c][k] = 3 + k + c;
        if (mask[k]) {
          expected[left[k]][c] += 3 + k + c;
          expected[right[k]][c] -= 3 + k + c;
        }
      }
    }
    residual.UpdateBlocks(left, right, flux, mask);
    const double trunc[4] = {1, 2, 3, 4};
    for (unsigned p = 0; p < 3; ++p) {
      update(&state, &geometry, p, residual.GetBlock(p), trunc);
      for (unsigned c = 0; c < 4; ++c) {
        const double wanted = 20 + 4*p + c - (double(1+p)/2)*(expected[p][c] + trunc[c]);
        ++checks;
        if (residual(p, c) != expected[p][c] || state.GetSolution(p, c) != wanted ||
            state.GetSolution_Old(p, c) != 20 + 4*p + c) {
          std::cerr << "composed contract failed: N=" << N << " mask=" << pattern
                    << " point=" << p << " component=" << c << " residual=" << residual(p, c)
                    << " expectedResidual=" << expected[p][c] << " state=" << state.GetSolution(p, c)
                    << " expectedState=" << wanted << '\n';
          return false;
        }
      }
    }
  }
  return true;
}
int main() {
  unsigned long checks = 0;
  if (!check<2>(checks) || !check<4>(checks) || !check<8>(checks)) return 1;
  std::cout << checks << " composed destination/residual/old-state checks passed; host SIMD lanes="
            << simd::Array<double>::Size << '\n';
}
