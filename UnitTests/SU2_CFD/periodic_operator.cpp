#include "catch.hpp"
#include "../UnitQuadTestCase.hpp"

TEST_CASE("Matrix-free interface vectors preserve periodic master constraints", "[Periodic][Mixing]") {
  UnitQuadTestCase test;
  test.config_options =
      "SOLVER=EULER\nINIT_OPTION=TD_CONDITIONS\nMESH_FORMAT=BOX\nMESH_BOX_SIZE=5,5,5\n"
      "MESH_BOX_LENGTH=1,1,1\nMESH_BOX_OFFSET=0,0,0\nMACH_NUMBER=0.2\n"
      "MARKER_CUSTOM=(x_minus,x_plus,z_minus,z_plus)\n"
      "MARKER_PERIODIC=(y_minus,y_plus,0,0,0,0,0,0,0,1,0)\n";
  test.InitConfig();
  test.InitGeometry(true);
  for (unsigned short pair=1; pair<=test.config->GetnMarker_Periodic()/2; ++pair)
    test.geometry->MatchPeriodic(test.config.get(), pair);
  test.geometry->PreprocessPeriodicComms(test.geometry.get(), test.config.get());
  test.InitSolver();
  auto* solver=test.solver[FLOW_SOL];
  CSysVector<su2double> direction(test.geometry->GetnPoint(),test.geometry->GetnPointDomain(),5,0.0);
  CSysVector<su2double> product(test.geometry->GetnPoint(),test.geometry->GetnPointDomain(),5,0.0);
  for(auto marker=0u;marker<test.config->GetnMarker_All();++marker) {
    const auto periodic=test.config->GetMarker_All_PerBound(marker);
    if (periodic==0) continue;
    for(auto vertex=0ul;vertex<test.geometry->GetnVertex(marker);++vertex) {
      const auto point=test.geometry->vertex[marker][vertex]->GetNode();
      for(auto v=0u;v<5;++v) {direction(point,v)=periodic==1 ? v+1 : -99;product(point,v)=periodic==1 ? v+1 : 2*(v+1);}
    }
  }
  solver->CommunicatePeriodicOperator(test.geometry.get(),test.config.get(),direction,false);
  solver->CommunicatePeriodicOperator(test.geometry.get(),test.config.get(),product,true);
  for(auto marker=0u;marker<test.config->GetnMarker_All();++marker) {
    const auto periodic=test.config->GetMarker_All_PerBound(marker);
    if (periodic==0) continue;
    for(auto vertex=0ul;vertex<test.geometry->GetnVertex(marker);++vertex) {
      const auto point=test.geometry->vertex[marker][vertex]->GetNode();
      if(!test.geometry->nodes->GetDomain(point))continue;
      for(auto v=0u;v<5;++v) {
        CHECK(direction(point,v)==v+1);
        CHECK(product(point,v)==(periodic==1 ? 3*(v+1) : 0));
      }
    }
  }
}

#include "../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"

TEST_CASE("Custom interface product reaches the production Krylov solve", "[Mixing][LinearSolver]") {
  UnitQuadTestCase test;
  test.AddOption("LINEAR_SOLVER=FGMRES");
  test.AddOption("LINEAR_SOLVER_PREC=JACOBI");
  test.AddOption("LINEAR_SOLVER_ERROR=1e-10");
  test.InitConfig();test.InitGeometry(true);test.InitSolver();
  auto* flow=test.solver[FLOW_SOL];
  flow->Jacobian.SetValZero();
  for(auto point=0ul;point<test.geometry->GetnPointDomain();++point)flow->Jacobian.SetVal2Diag(point,1.0);
  CSysVector<su2double> rhs(test.geometry->GetnPoint(),test.geometry->GetnPointDomain(),5,1.0);
  CSysVector<su2double> solution(test.geometry->GetnPoint(),test.geometry->GetnPointDomain(),5,0.0);
  class Twice final : public CMatrixVectorProduct<su2mixedfloat> {
   public:
    void operator()(const CSysVector<su2mixedfloat>& u,CSysVector<su2mixedfloat>& v) const override {v=2.0*u;}
  } twice;
  CSysSolve<su2mixedfloat> system;
  system.SetxIsZero(true);
  system.Solve(flow->Jacobian,rhs,solution,test.geometry.get(),test.config.get(),&twice);
  for(auto point=0ul;point<test.geometry->GetnPointDomain();++point)
    for(auto v=0u;v<5;++v)CHECK(solution(point,v)==Approx(0.5));
  solution=0.0;
  system.Solve(flow->Jacobian,rhs,solution,test.geometry.get(),test.config.get());
  for(auto point=0ul;point<test.geometry->GetnPointDomain();++point)
    for(auto v=0u;v<5;++v)CHECK(solution(point,v)==Approx(1.0));
}
