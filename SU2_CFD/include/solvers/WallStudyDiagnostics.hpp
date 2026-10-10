/* Private restart-study instrumentation; no changes to solution or MPI communication. */
#pragma once
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class WallStudyDiagnostics {
 public:
  enum Reason { ACTIVE, LOW_SEED, LOW_SOLVED, SOLID_SAMPLE, ZERO_SPEED, SOLVE_FAILURE, UNKNOWN };
 private:
  const bool enabled = [] { const auto* value = std::getenv("SU2_WALL_DIAGNOSTICS");
                            return value && std::string(value) == "1"; }();
  std::map<std::pair<std::string, unsigned>, std::array<double,14>> fluxes;
  std::map<std::string, std::vector<Reason>> previous;
  std::map<std::string, std::array<unsigned long,10>> walls;
  std::ofstream flux_file, wall_file, event_file;
  unsigned long flux_sample = 0, wall_sample = 0, outer = 0, inner = 0;
  unsigned rk = 0;
  bool output = false;

  static void Open(std::ofstream& file, const std::string& name, const char* header) {
    if (file.is_open()) return;
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file.open("wall_diagnostics/" + name);
    file << std::setprecision(17) << header << '\n';
  }
  static std::string Name(const char* kind, unsigned zone, unsigned mg, unsigned rank) {
    return std::string(kind)+"_z"+std::to_string(zone)+"_mg"+std::to_string(mg)+"_rank"+std::to_string(rank)+".csv";
  }
 public:
  bool Enabled() const { return enabled; }
  void AddFlux(const std::string& marker, unsigned span, bool viscous, unsigned dim,
               const double* residual, const double* coord, double area) {
    if (!enabled) return;
    auto& sum = fluxes[{marker,span}];
    if (!viscous) { sum[0] += 1; sum[1] += area; }
    const unsigned offset = viscous ? 8 : 2;
    sum[offset] += residual[0];
    for (unsigned d = 0; d < dim; ++d) sum[offset+1+d] += residual[1+d];
    sum[offset+4] += residual[dim+1];
    sum[offset+5] += coord[0]*residual[2]-coord[1]*residual[1];
  }
  void WriteFlux(unsigned zone, unsigned mg, unsigned rank, unsigned long outer_iter,
                 unsigned long inner_iter, unsigned step) {
    if (!enabled || fluxes.empty()) return;
    Open(flux_file, Name("giles",zone,mg,rank),
         "sample,outer,inner,rk,marker,span,owners,area,conv_mass,conv_mx,conv_my,conv_mz,conv_energy,conv_torque_z,visc_mass,visc_mx,visc_my,visc_mz,visc_energy,visc_torque_z");
    for (const auto& row : fluxes) {
      flux_file << flux_sample << ',' << outer_iter << ',' << inner_iter << ',' << step << ','
                << row.first.first << ',' << row.first.second;
      for (double value : row.second) flux_file << ',' << value;
      flux_file << '\n';
    }
    flux_file.flush();
    fluxes.clear();
    ++flux_sample;
  }
  void BeginWall(unsigned zone, unsigned mg, unsigned rank, unsigned long outer_iter,
                 unsigned long inner_iter, unsigned step, bool is_output) {
    if (!enabled) return;
    outer=outer_iter; inner=inner_iter; rk=step; output=is_output;
    walls.clear();
    Open(wall_file,Name("walls",zone,mg,rank),
         "sample,outer,inner,rk,output,marker,active,low_seed,low_solved,solid_sample,zero_speed,solve_failure,entered,left,reason_changed,near_threshold");
    Open(event_file,Name("events",zone,mg,rank),
         "sample,outer,inner,rk,output,marker,vertex,global_point,previous,current,seed_yplus,yplus");
  }
  static std::array<unsigned long,3> Transition(Reason before, Reason after) {
    return {{before!=UNKNOWN && before!=ACTIVE && after==ACTIVE,
             before==ACTIVE && after!=ACTIVE, before!=UNKNOWN && before!=after}};
  }
  void RecordWall(const std::string& marker, unsigned long vertex, unsigned long global_point,
                  Reason reason, double seed, double yplus, double threshold) {
    if (!enabled) return;
    auto& states = previous[marker];
    if (states.size()<=vertex) states.resize(vertex+1, UNKNOWN);
    auto& counts = walls[marker];
    ++counts[reason];
    const auto changes = Transition(states[vertex],reason);
    for (unsigned i=0; i<3; ++i) counts[6+i] += changes[i];
    if (std::isfinite(yplus) && std::abs(yplus-threshold)<=0.5) ++counts[9];
    if (changes[2])
      event_file << wall_sample << ',' << outer << ',' << inner << ',' << rk << ',' << output << ','
                 << marker << ',' << vertex << ',' << global_point << ',' << states[vertex] << ',' << reason
                 << ',' << seed << ',' << yplus << '\n';
    states[vertex]=reason;
  }
  void WriteWall() {
    if (!enabled) return;
    for (const auto& row : walls) {
      wall_file << wall_sample << ',' << outer << ',' << inner << ',' << rk << ',' << output << ',' << row.first;
      for (auto count : row.second) wall_file << ',' << count;
      wall_file << '\n';
    }
    wall_file.flush(); event_file.flush(); ++wall_sample;
  }
};
