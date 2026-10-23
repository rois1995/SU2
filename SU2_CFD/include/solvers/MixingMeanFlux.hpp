#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace MixingMeanFlux {
using Flux = std::array<double, 5>; // mass, axial momentum, axial torque, radial momentum, total energy
/* Frozen geometry: the same linear change of basis applies to fluxes and their directions. */
inline Flux Cylindrical(const Flux& cartesian, double x, double y) {
  const double r = std::hypot(x, y);
  if (!(r > 0) || !std::isfinite(r)) throw std::runtime_error("Invalid axial mixing radius");
  return {{cartesian[0], cartesian[3], x*cartesian[2]-y*cartesian[1],
           (x*cartesian[1]+y*cartesian[2])/r, cartesian[4]}};
}
struct VertexJacobian {
  unsigned long point = 0;
  double x = 0, y = 0, weight = 0;
  bool fixedMomentum = false;
  std::array<double, 25> value{}; // Cylindrical numerical flux rows, Cartesian conservative-state columns.
  Flux Apply(const double* direction) const {
    Flux result{};
    for (size_t i=0; i<5; ++i) for (size_t j=0; j<5; ++j) result[i] += value[5*i+j]*direction[j];
    return result;
  }
};

inline std::vector<double> Edges(const std::vector<double>& radii) {
  if (radii.size() < 2 || !std::isfinite(radii.front()) || radii.front() < 0) throw std::runtime_error("Conservative axial mixing requires at least two radial planes");
  std::vector<double> edge(radii.size()+1);
  edge.front()=radii.front()*radii.front(); edge.back()=radii.back()*radii.back();
  for (size_t i=1; i<radii.size(); ++i) {
    if (!std::isfinite(radii[i]) || !(radii[i]>radii[i-1])) throw std::runtime_error("Mixing radii must increase strictly");
    const double radius=0.5*(radii[i-1]+radii[i]); edge[i]=radius*radius;
  }
  const double inner=edge.front(), height=edge.back()-inner;
  if (!(height>0) || !std::isfinite(height)) throw std::runtime_error("Invalid annular area interval");
  for (auto& value:edge)value=(value-inner)/height;
  return edge;
}
/* Conservative overlap in normalized annular area connects corresponding hub/shroud streamtubes. */
inline std::vector<Flux> Common(const std::vector<double>& local_radius, const std::vector<Flux>& local,
                               double local_blades, const std::vector<double>& donor_radius,
                               const std::vector<Flux>& donor, double donor_blades) {
  const auto a=Edges(local_radius), b=Edges(donor_radius);
  if (local.size()!=local_radius.size() || donor.size()!=donor_radius.size() ||
      !(local_blades>0) || !(donor_blades>0) || !std::isfinite(local_blades) || !std::isfinite(donor_blades)) throw std::runtime_error("Invalid mixing flux dimensions or pitch");
  for (const auto& row:local)for(double value:row)if(!std::isfinite(value))throw std::runtime_error("Nonfinite local mixing flux");
  for (const auto& row:donor)for(double value:row)if(!std::isfinite(value))throw std::runtime_error("Nonfinite donor mixing flux");
  std::vector<Flux> common(local.size());
  for (size_t i=0;i<local.size();++i) {
    for (size_t v=0;v<5;++v) common[i][v]=0.5*local[i][v];
    for (size_t j=0;j<donor.size();++j) {
      const double overlap=std::max(0.0,std::min(a[i+1],b[j+1])-std::max(a[i],b[j]));
      const double weight=overlap/(b[j+1]-b[j])*donor_blades/local_blades;
      for (size_t v=0;v<5;++v) common[i][v]-=0.5*weight*donor[j][v];
    }
  }
  return common;
}
/* Area weights preserve all five cylindrical integrals, including exact torque at varying radius. */
inline Flux CartesianCorrection(const Flux& difference, double weight, double x, double y) {
  const double r=std::hypot(x,y);
  if (!(r>0) || !std::isfinite(weight) || weight<0) throw std::runtime_error("Invalid axial mixing vertex");
  const double radial=weight*difference[3], tangential=weight*difference[2]/r;
  return {{weight*difference[0], (x*radial-y*tangential)/r,
           (y*radial+x*tangential)/r, weight*difference[1], weight*difference[4]}};
}
}
