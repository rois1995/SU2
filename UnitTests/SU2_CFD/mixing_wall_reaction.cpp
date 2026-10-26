#include "catch.hpp"
#include "../UnitQuadTestCase.hpp"
#include "../../SU2_CFD/include/solvers/MixingMeanFlux.hpp"
#include <array>

TEST_CASE("Mixing flux and strong-wall reaction close the constrained momentum budget",
          "[MixingConservation][WallReaction]") {
  UnitQuadTestCase test;
  test.config_options =
      "SOLVER=NAVIER_STOKES\nINIT_OPTION=TD_CONDITIONS\nMESH_FORMAT=BOX\n"
      "MESH_BOX_SIZE=5,5,5\nMESH_BOX_LENGTH=1,1,1\nMESH_BOX_OFFSET=0.5,0.5,0\n"
      "MACH_NUMBER=0.2\nMARKER_HEATFLUX=(y_minus,0,y_plus,0)\n"
      "MARKER_CUSTOM=(x_minus,x_plus,z_minus,z_plus)\n"
      "VISCOSITY_MODEL=CONSTANT_VISCOSITY\nTIME_DISCRE_FLOW=EULER_IMPLICIT\n";
  test.InitConfig();
  test.InitGeometry(true);
  CNSSolver solver(test.geometry.get(), test.config.get(), MESH_0);
  auto* geometry = test.geometry.get();
  auto* config = test.config.get();
  short station = -1;
  std::vector<unsigned short> walls;
  for (auto marker=0u; marker<geometry->GetnMarker(); ++marker) {
    if (config->GetMarker_All_TagBound(marker)=="z_minus") station=marker;
    if (config->GetViscous_Wall(marker)) walls.push_back(marker);
  }
  double localArea=0, area=0;
  if (station>=0) for (auto vertex=0ul;vertex<geometry->GetnVertex(station);++vertex) {
    const auto point=geometry->vertex[station][vertex]->GetNode();
    if (geometry->nodes->GetDomain(point))
      localArea+=GeometryToolbox::Norm(3,geometry->vertex[station][vertex]->GetNormal());
  }
  SU2_MPI::Allreduce(&localArea,&area,1,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
  REQUIRE(area>0);
  solver.LinSysRes.SetValZero();
  solver.Jacobian.SetValZero();
  const MixingMeanFlux::Flux target{{2,3,4,5,6}};
  if (station>=0) for (auto vertex=0ul;vertex<geometry->GetnVertex(station);++vertex) {
    const auto point=geometry->vertex[station][vertex]->GetNode();
    if (!geometry->nodes->GetDomain(point)) continue;
    const auto* xyz=geometry->nodes->GetCoord(point);
    const double weight=GeometryToolbox::Norm(3,geometry->vertex[station][vertex]->GetNormal())/area;
    const auto correction=MixingMeanFlux::CartesianCorrection(target,weight,xyz[0],xyz[1]);
    for (auto var=0u;var<5;++var) solver.LinSysRes(point,var)+=correction[var];
  }
  auto before=solver.LinSysRes;
  for (auto marker:walls)
    static_cast<CSolver&>(solver).BC_HeatFlux_Wall(geometry,nullptr,nullptr,nullptr,config,marker);

  auto cylindrical=[](const MixingMeanFlux::Flux& f,double x,double y) {
    return MixingMeanFlux::Flux{{f[0],f[3],x*f[2]-y*f[1],(x*f[1]+y*f[2])/std::hypot(x,y),f[4]}};
  };
  std::array<double,15> local{},global{};
  unsigned long localShared=0,shared=0;
  if (station>=0) for (auto vertex=0ul;vertex<geometry->GetnVertex(station);++vertex) {
    const auto point=geometry->vertex[station][vertex]->GetNode();
    if (!geometry->nodes->GetDomain(point)) continue;
    bool constrained=false;
    for (auto marker:walls) constrained=constrained || geometry->nodes->GetVertex(point,marker)>=0;
    localShared+=constrained;
    const auto* xyz=geometry->nodes->GetCoord(point);
    MixingMeanFlux::Flux ledger{},projected{},reaction{};
    for (auto var=0u;var<5;++var) {
      ledger[var]=before(point,var);projected[var]=solver.LinSysRes(point,var);
      // Reaction is the exact change made by the production strong-wall assembly.
      reaction[var]=projected[var]-ledger[var];
      CHECK(projected[var]==Approx(constrained && var>=1 && var<=3 ? 0.0 : ledger[var]));
    }
    const auto a=cylindrical(ledger,xyz[0],xyz[1]);
    const auto b=cylindrical(projected,xyz[0],xyz[1]);
    const auto c=cylindrical(reaction,xyz[0],xyz[1]);
    for (auto var=0u;var<5;++var) {local[var]+=a[var];local[5+var]+=b[var];local[10+var]+=c[var];}
  }
  SU2_MPI::Allreduce(local.data(),global.data(),15,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&localShared,&shared,1,MPI_UNSIGNED_LONG,MPI_SUM,SU2_MPI::GetComm());
  REQUIRE(shared>0);
  for (auto var=0u;var<5;++var) {
    CHECK(global[var]==Approx(target[var]));
    CHECK(global[5+var]==Approx(global[var]+global[10+var]));
  }
  CHECK(global[10]==Approx(0)); // No mass reaction at this stationary adiabatic wall.
  CHECK(global[14]==Approx(0));
  CHECK(std::abs(global[11])>0); // Axial and torque response really reach constrained nodes.
  CHECK(std::abs(global[12])>0);
}
