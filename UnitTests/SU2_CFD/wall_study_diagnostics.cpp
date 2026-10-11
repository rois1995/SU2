#include "../../SU2_CFD/include/solvers/WallStudyDiagnostics.hpp"
#include <cassert>
#include <fstream>
#include <sys/stat.h>
int main() {
  mkdir("wall_diagnostics",0700);
  using D=WallStudyDiagnostics;
  assert((D::Transition(D::UNKNOWN,D::ACTIVE)==std::array<unsigned long,3>{{0,0,0}}));
  assert((D::Transition(D::LOW_SEED,D::ACTIVE)==std::array<unsigned long,3>{{1,0,1}}));
  assert((D::Transition(D::ACTIVE,D::LOW_SOLVED)==std::array<unsigned long,3>{{0,1,1}}));
  assert((D::Transition(D::SOLID_SAMPLE,D::ZERO_SPEED)==std::array<unsigned long,3>{{0,0,1}}));
  setenv("SU2_WALL_DIAGNOSTICS","1",1);
  D diagnostic;
  double conv[5]={2,3,4,5,6}, visc[5]={0,1,2,3,4}, coord[3]={2,3,0};
  diagnostic.AddFlux("outlet",0,false,3,conv,coord,0.5);
  diagnostic.AddFlux("outlet",0,true,3,visc,coord,0.5);
  diagnostic.AddCorrection("outlet",0,conv,coord);
  diagnostic.WriteFlux(0,0,0,7,0,0);
  std::ifstream file("wall_diagnostics/giles_z0_mg0_rank0.csv");
  std::string line; std::getline(file,line); std::getline(file,line);
  assert(line=="0,7,0,0,outlet,0,1,0.5,2,3,4,5,6,-1,0,1,2,3,4,1,2,3,4,5,6,-1");
  diagnostic.BeginWall(0,0,0,7,0,0,false);
  diagnostic.RecordWall("wall",2,42,D::LOW_SEED,4,4,5); diagnostic.WriteWall();
  diagnostic.BeginWall(0,0,0,8,0,0,false);
  diagnostic.RecordWall("wall",2,42,D::ACTIVE,5,6,5); diagnostic.WriteWall();
  diagnostic.BeginWall(0,0,0,8,0,0,true);
  diagnostic.RecordWall("wall",2,42,D::LOW_SOLVED,6,4.9,5); diagnostic.WriteWall();
  std::ifstream events("wall_diagnostics/events_z0_mg0_rank0.csv");
  std::getline(events,line); std::getline(events,line);
  assert(line=="1,8,0,0,0,wall,2,42,1,0,5,6");
  std::getline(events,line); assert(line.find("2,8,0,0,1,wall,2,42,0,2,6,")==0);
  assert(!std::getline(events,line));
}
