#pragma once

#include <algorithm>
#include <utility>

/* The configured band is a fraction of span height, independent of radius. */
template<class T>
std::pair<T, T> GilesRelaxation(const T& span, const T& hub, const T& shroud,
                              const T& fraction, const T& average, const T& fourier,
                              const T& endwall_average) {
  const T width = (shroud - hub) * fraction;
  const T distance = std::min(span - hub, shroud - span);
  if (width <= 0 || distance > width) return {average, fourier};
  return {endwall_average + (average - endwall_average) * distance / width, T(0)};
}
