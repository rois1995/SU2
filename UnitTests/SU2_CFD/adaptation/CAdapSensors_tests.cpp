/*!
 * \file CAdapSensors_tests.cpp
 * \brief Strict sensor grammar, binding, cascades and active gradient expressions.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CAdapSensorOptions.hpp"
#include "../../../SU2_CFD/include/adaptation/CAdapSensors.hpp"
#include "TransferTestCase.hpp"
#include "../../../SU2_CFD/include/variables/CPrimitiveIndices.hpp"

TEST_CASE("Custom adaptation definition names", "[Adaptation][CustomSensors]") {
  const auto definitions = AdapSensorOptions::Parse("a : PRESSURE; B_2 : a*a");
  REQUIRE(definitions.size() == 2);
  CHECK(definitions[0].first == "a");
  CHECK(definitions[1].second == "a*a");
  for (const auto* text : {"", "   "}) CHECK(AdapSensorOptions::Parse(text).empty());
  for (const auto* text : {"1a:1", "_a:1", "a-b:1", "a:", "a:1;", "a:1;;b:2", "a:1;a:2",
                           "MACH:1", "PRESSURE:1", "VELOCITY_X:1", "GRAD_a:1", "TURBx:1",
                           "SCALARfoo:1", "SPECIESx:1", "GOAL:1", "a"}) {
    INFO(text);
    CHECK_THROWS_AS(AdapSensorOptions::Parse(text), std::invalid_argument);
  }
}

TEST_CASE("Custom adaptation strict expression grammar", "[Adaptation][CustomSensors]") {
  for (const auto* text : {"1.2.3", "1e", "0x10", "1e+", "1e9999", "2- -1", "2+ +1", "2--1",
                           "2+-1", "2-+1", "()", "(a", "a)", "pow(a)", "fabs(a,b)", "fmax(a,)",
                           "a b", "1 2", "a**b", "a^2", "a+", "a:b", "sqrt()", "bad(a)", ""}) {
    INFO(text);
    CHECK_THROWS_AS(CAdapSensors::ValidateExpression(text), std::invalid_argument);
  }
  const std::pair<std::string, double> cases[] = {
      {"1e-3", 0.001}, {"-a", -3}, {"fmax(-1,a)", 3}, {"2*-a", -6}, {"2-(-1)", 3},
      {"fmax(a,2)", 3}, {"+a", 3}, {"pow(a,2)", 9}, {"2/+a", 2.0/3}, {".5+1.", 1.5}, {"hypot(a,4)", 5}, {"cbrt(27)", 3},
      {"log(exp(a))", 3}, {"sin(0)+cos(0)", 1}, {"fmin(a,2)", 2}};
  for (const auto& test : cases) {
    INFO(test.first);
    std::vector<std::string> symbols;
    const auto tree = mel::Parse<passivedouble>(CAdapSensors::ValidateExpression(test.first), symbols);
    const std::map<std::string, su2double> inputs = {{"a", 3.0}};
    for (const auto& symbol : symbols) REQUIRE(symbol == "a");
    const auto value = mel::Eval<su2double>(tree, [&](int index) { return inputs.at(symbols.at(index)); });
    CHECK(SU2_TYPE::GetValue(value) == Approx(test.second));
  }
  std::string edge = "a";
  for (int i = 0; i < 49; ++i) edge += "+a";
  CHECK_NOTHROW(CAdapSensors::ValidateExpression(edge));
  CHECK_THROWS_AS(CAdapSensors::ValidateExpression(edge + "+a"), std::invalid_argument);
}

namespace {
using namespace transfer_test;

std::unique_ptr<CConfig> SensorConfig(const std::string& definitions, const std::string& selected = "S") {
  return MakeConfig(2, "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nCOMPUTE_METRIC= YES\n"
                       "NUM_METHOD_HESS= WEIGHTED_LEAST_SQUARES\nADAP_SENSOR= (" + selected + ")\n"
                       "ADAP_CUSTOM_SENSORS= '" + definitions + "'\n");
}

CSimplexMesh SensorMesh() {
  return simplex_test::MakeSimplexMesh(2, 6, [](const passivedouble* x) {
    if (x[0] < 1e-10) return std::string("left");
    if (x[0] > 2.0 - 1e-10) return std::string("right");
    if (x[1] > 1.0 - 1e-10) return std::string("upper");
    return std::string(x[0] < 1.0 ? "lower_a" : "lower_b");
  });
}
}  // namespace

TEST_CASE("Custom adaptation unary plus binds and preserves grouping", "[Adaptation][CustomSensors]") {
  const std::pair<std::string, std::string> cases[] = {
      {"+PRESSURE", "PRESSURE"}, {"2/+PRESSURE", "2/PRESSURE"},
      {"+(PRESSURE*PRESSURE)", "(PRESSURE*PRESSURE)"},
      {"2/+(PRESSURE*PRESSURE)", "2/(PRESSURE*PRESSURE)"},
      {"2/+(PRESSURE+1)", "2/(PRESSURE+1)"},
      {"+(-PRESSURE)", "((-PRESSURE))"}, {"-(+PRESSURE)", "(-(PRESSURE))"},
      {"fmax(+PRESSURE,+2)", "fmax(PRESSURE,2)"},
      {"+PRESSURE+1e+3", "PRESSURE+1e+3"}};
  std::string definitions, selected;
  for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
    INFO(cases[i].first);
    const auto normalized = CAdapSensors::ValidateExpression(cases[i].first);
    CHECK(normalized == cases[i].second);
    std::vector<std::string> symbols;
    mel::Parse<passivedouble>(normalized, symbols);
    CHECK(symbols == std::vector<std::string>{"PRESSURE"});
    if (i) { definitions += "; "; selected += ", "; }
    const auto name = "S" + std::to_string(i);
    definitions += name + " : " + cases[i].first;
    selected += name;
  }
  auto config = SensorConfig(definitions, selected);
  MeshSolution state(config.get(), SensorMesh(), 0);
  auto* flow = state.solver[MESH_0][FLOW_SOL];
  auto* nodes = flow->GetNodes();
  const auto idx = CPrimitiveIndices<unsigned short>(false, false, 2, 0);
  CAdapSensors sensors(*config, state.Fine(), state.solver[MESH_0]);
  REQUIRE(sensors.GetStages().size() == sizeof(cases)/sizeof(cases[0]));
  for (const auto& stage : sensors.GetStages()) {
    REQUIRE(stage.symbols.size() == 1);
    CHECK(stage.symbols[0].kind == CAdapSensors::KIND::PRIMITIVE);
    CHECK(stage.symbols[0].index == idx.Pressure());
  }
  for (unsigned long point = 0; point < state.Fine().GetnPoint(); ++point)
    nodes->SetPrimitive(point, idx.Pressure(), 3.0 + state.Fine().nodes->GetCoord(point, 0));
  sensors.Sample(*flow, state.Fine(), *config, state.solver[MESH_0]);
  for (unsigned long point = 0; point < state.Fine().GetnPointDomain(); ++point) {
    const auto pressure = SU2_TYPE::GetValue(nodes->GetPrimitive(point, idx.Pressure()));
    const passivedouble expected[] = {pressure, 2/pressure, pressure*pressure, 2/(pressure*pressure),
                                     2/(pressure+1), -pressure, -pressure, pressure, pressure+1e3};
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
      INFO(cases[i].first);
      CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, i)) == Approx(expected[i]));
    }
  }
}

TEST_CASE("Custom adaptation solver binding rejects invalid symbols", "[Adaptation][CustomSensors]") {
  const auto mesh = SensorMesh();
  for (const auto* expression : {"S", "later", "GRAD_PRESSURE_Z", "GRAD_MACH_X", "GRAD_S_X",
                                "TURB[01]", "TURB[-1]", "TURB[99]", "TURB[0]", "SCALAR[0]",
                                "GRAD_TURB[0]_Y", "LAMINAR_VISCOSITY", "EDDY_VISCOSITY", "nan", "unknown"}) {
    INFO(expression);
    auto config = SensorConfig(std::string("S : ") + expression + "; later : 1");
    MeshSolution state(config.get(), mesh, 0);
    CHECK_THROWS_AS(CAdapSensors(*config, state.Fine(), state.solver[MESH_0]), std::invalid_argument);
  }
}

TEST_CASE("Custom adaptation cascade with active input gradient", "[Adaptation][CustomSensors]") {
  auto config = SensorConfig("G : GRAD_PRESSURE_X; S : G*G");
  MeshSolution state(config.get(), SensorMesh(), 0);
  auto* flow = state.solver[MESH_0][FLOW_SOL];
  auto* nodes = flow->GetNodes();
  su2double slope = 3.0;
#ifdef CODI_REVERSE_TYPE
  AD::Reset();
  AD::StartRecording();
  AD::RegisterInput(slope);
#elif defined(CODI_FORWARD_TYPE)
  SU2_TYPE::SetDerivative(slope, 1.0);
#endif
  const auto idx = CPrimitiveIndices<unsigned short>(false, false, 2, 0);
  for (unsigned long point = 0; point < state.Fine().GetnPoint(); ++point)
    nodes->SetPrimitive(point, idx.Pressure(), slope * state.Fine().nodes->GetCoord(point, 0));
  CAdapSensors sensors(*config, state.Fine(), state.solver[MESH_0]);
  sensors.Sample(*flow, state.Fine(), *config, state.solver[MESH_0]);
  CHECK(sensors.GetStages().size() == 2);
  CHECK(sensors.GetnStaged() == 1);
  su2double mean = 0.0;
  for (unsigned long point = 0; point < state.Fine().GetnPointDomain(); ++point) {
    CHECK(SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(point, 0)) == Approx(9.0));
    mean += nodes->GetAuxVar_Adapt(point, 0) / state.Fine().GetnPointDomain();
  }
#ifdef CODI_REVERSE_TYPE
  AD::RegisterOutput(mean);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(mean, 1.0);
  AD::ComputeAdjoint();
  CHECK(SU2_TYPE::GetDerivative(slope) == Approx(6.0));
  AD::Reset();
#elif defined(CODI_FORWARD_TYPE)
  CHECK(SU2_TYPE::GetDerivative(mean) == Approx(6.0));
#endif
  /*--- All intermediate stages are checked, even if the selected result is finite. ---*/
  auto invalidConfig = SensorConfig("BAD : sqrt(-1); S : 1");
  MeshSolution invalidState(invalidConfig.get(), SensorMesh(), 0);
  CAdapSensors invalid(*invalidConfig, invalidState.Fine(), invalidState.solver[MESH_0]);
  CHECK_THROWS_WITH(invalid.Sample(*invalidState.solver[MESH_0][FLOW_SOL], invalidState.Fine(), *invalidConfig,
                                  invalidState.solver[MESH_0]), Catch::Contains("BAD"));
}

TEST_CASE("Custom sensors validate every later instantaneous sample", "[Adaptation][CustomSensors]") {
  auto config = SensorConfig("G : GRAD_PRESSURE_X; S : sqrt(G)");
  MeshSolution state(config.get(), SensorMesh(), 0);
  auto* flow = state.solver[MESH_0][FLOW_SOL];
  const auto idx = CPrimitiveIndices<unsigned short>(false, false, 2, 0);
  CAdapSensors sensors(*config, state.Fine(), state.solver[MESH_0]);
  for (const auto slope : {1.0, -1.0}) {
    for (unsigned long point = 0; point < state.Fine().GetnPoint(); ++point)
      flow->GetNodes()->SetPrimitive(point, idx.Pressure(), slope * state.Fine().nodes->GetCoord(point, 0));
    if (slope > 0) CHECK_NOTHROW(sensors.Sample(*flow, state.Fine(), *config, state.solver[MESH_0]));
    else CHECK_THROWS_WITH(sensors.Sample(*flow, state.Fine(), *config, state.solver[MESH_0]),
                           Catch::Contains("stage S") && Catch::Contains("global point 0 at"));
  }
}

TEST_CASE("Custom adaptation active scalar bounds", "[Adaptation][CustomSensors]") {
  for (const auto* model : {"SA", "SST"}) {
    auto config = MakeConfig(2, std::string("SOLVER= RANS\nMATH_PROBLEM= DIRECT\nKIND_TURB_MODEL= ") + model +
        "\nREYNOLDS_NUMBER= 10000\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (S)\nADAP_CUSTOM_SENSORS= 'S : TURB[1]'\n");
    MeshSolution state(config.get(), SensorMesh(), 0);
    if (std::string(model) == "SA") {
      CHECK_THROWS_WITH(CAdapSensors(*config, state.Fine(), state.solver[MESH_0]), Catch::Contains("out of range"));
    } else {
      CHECK_NOTHROW(CAdapSensors(*config, state.Fine(), state.solver[MESH_0]));
    }
  }
}

TEST_CASE("Custom adaptation twenty selected sensors retain selection order", "[Adaptation][CustomSensors]") {
  std::string definitions, selected;
  for (int i = 0; i < 20; ++i) {
    if (i) { definitions += "; "; selected += ", "; }
    definitions += "A" + std::to_string(i) + " : " + std::to_string(i);
    selected += "A" + std::to_string(19-i);
  }
  auto config = SensorConfig(definitions, selected);
  MeshSolution state(config.get(), SensorMesh(), 0);
  auto* flow = state.solver[MESH_0][FLOW_SOL];
  flow->SetAuxVar_Adapt(&state.Fine(), config.get(), state.solver[MESH_0]);
  CHECK(flow->GetNodes()->GetAuxVar_Adapt().cols() == 20);
  CHECK(flow->GetNodes()->GetHessian().rows() == 20);
  for (unsigned long point = 0; point < state.Fine().GetnPointDomain(); ++point)
    for (unsigned short sensor = 0; sensor < 20; ++sensor)
      CHECK(SU2_TYPE::GetValue(flow->GetNodes()->GetAuxVar_Adapt(point, sensor)) == 19-sensor);
}
