#include "catch.hpp"

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CAdaptationOptions.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "SimplexMeshTestCase.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

namespace {

const std::string base = "SOLVER= EULER\nMESH_FORMAT= BOX\nMGLEVEL= 0\n"
                         "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n";
const std::string loop = "COMPUTE_METRIC= YES\nADAP_LOOP= YES\nADAP_SIZES= (5000, 10000)\n";

class Capture {
  std::ostringstream output;
  std::streambuf* previous = std::cout.rdbuf(output.rdbuf());

 public:
  ~Capture() { std::cout.rdbuf(previous); }
  std::string Text() const { return output.str(); }
};

std::unique_ptr<CConfig> Config(const std::string& options) {
  std::istringstream input(base + options);
  return std::make_unique<CConfig>(input, SU2_COMPONENT::SU2_CFD, false);
}

struct ZoneConfigFile {
  char name[MAX_STRING_SIZE] = "def1_zone.cfg";
  explicit ZoneConfigFile(const std::string& options) {
    std::ofstream output(name);
    output << options;
    output.close();
    REQUIRE(output.good());
  }
  ~ZoneConfigFile() { std::remove(name); }
};

void CheckNorm(const CConfig& config, double scalar, double level0, double level1) {
  CHECK(SU2_TYPE::GetValue(config.GetAdap_Norm()) == Approx(scalar));
  CHECK(SU2_TYPE::GetValue(config.GetAdap_Hgrad()) == Approx(1.6));
  REQUIRE(config.GetnAdap_Levels() == 2);
  CHECK(SU2_TYPE::GetValue(config.GetAdap_Level(0).norm) == Approx(level0));
  CHECK(SU2_TYPE::GetValue(config.GetAdap_Level(1).norm) == Approx(level1));
}

}  // namespace

TEST_CASE("DEF1 GOAL scalar norm decision", "[.][def1_normal]") {
  using SU2Adaptation::EffectiveNorm;
  CHECK(SU2_TYPE::GetValue(EffectiveNorm(false, false, 2.0)) == Approx(2.0));
  CHECK(SU2_TYPE::GetValue(EffectiveNorm(false, true, 3.0)) == Approx(3.0));
  CHECK(SU2_TYPE::GetValue(EffectiveNorm(true, false, 2.0)) == Approx(1.0));
  CHECK(SU2_TYPE::GetValue(EffectiveNorm(true, true, 1.0)) == Approx(1.0));
  CHECK(SU2_TYPE::GetValue(EffectiveNorm(true, true, 2.0)) == Approx(2.0));
}

TEST_CASE("DEF1 direct config defaults and level norm precedence", "[.][def1_normal]") {
  Capture capture;
  const auto mach = loop + "ADAP_SENSOR= (MACH)\n";
  const auto omitted = Config(mach);
  CHECK(omitted->GetKind_Adap_Remesher() == ADAP_REMESHER::MMG);
  CheckNorm(*omitted, 2.0, 2.0, 2.0);
  CheckNorm(*Config(mach + "ADAP_NORMS= NONE\n"), 2.0, 2.0, 2.0);
  CheckNorm(*Config(mach + "ADAP_NORMS= (3, 4)\n"), 2.0, 3.0, 4.0);
}

#ifdef CODI_REVERSE_TYPE
TEST_CASE("DEF1 AD GOAL config defaults and inherited scalar", "[.][def1_ad]") {
  const auto goal = loop + "ADAP_SENSOR= (GOAL)\nMATH_PROBLEM= DISCRETE_ADJOINT\nTIME_DISCRE_FLOW= EULER_IMPLICIT\n";
  const std::string warning = "ADAP_NORM= 2 is used as given";
  SECTION("Omitted scalar uses L1, including an inherited default") {
    Capture capture;
    const auto parent = Config(goal);
    CheckNorm(*parent, 1.0, 1.0, 1.0);
    ZoneConfigFile overlay("");
    CConfig inherited(parent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CheckNorm(inherited, 1.0, 1.0, 1.0);
    CHECK(capture.Text().find("L1-optimal") == std::string::npos);
  }
  SECTION("Explicit scalar is retained and still warned about") {
    Capture capture;
    const auto parent = Config(goal + "ADAP_NORM= 2\n");
    CheckNorm(*parent, 2.0, 2.0, 2.0);
    ZoneConfigFile overlay("");
    CConfig inherited(parent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CheckNorm(inherited, 2.0, 2.0, 2.0);
    const auto output = capture.Text();
    const auto first = output.find(warning);
    REQUIRE(first != std::string::npos);
    CHECK(output.find(warning, first + warning.size()) != std::string::npos);
  }
  SECTION("Per-level list supersedes the implicit GOAL scalar") {
    Capture capture;
    CheckNorm(*Config(goal + "ADAP_NORMS= (2, 3)\n"), 1.0, 2.0, 3.0);
    CHECK(capture.Text().find("L1-optimal") == std::string::npos);
  }
}
#endif

TEST_CASE("DEF1 configuration reads and inherits both Hausdorff forms", "[.][def1_normal]") {
  Capture capture;
  for (const auto& text : std::vector<std::pair<std::string, double>>{
           {"", 0.01}, {"0.01", 0.01}, {"(0.01)", 0.01}, {"0.0045", 0.0045}}) {
    CAPTURE(text.first);
    const auto config = Config(text.first.empty() ? "" : "ADAP_HAUSD= " + text.first + "\n");
    CHECK(SU2_TYPE::GetValue(config->GetAdap_Hausd()) == Approx(text.second));
    CHECK(config->GetAdap_MarkerHausd().empty());
  }

  const std::string list = "ADAP_HAUSD= (FAR, 1.0, WALL, 1.234567890123456e-3)\n";
  const std::map<std::string, double> values{{"FAR", 1.0}, {"WALL", 1.234567890123456e-3}};
  const auto listParent = Config(list);
  const auto scalarParent = Config("ADAP_HAUSD= 0.07\n");
  CHECK(SU2_TYPE::GetValue(listParent->GetAdap_Hausd()) == Approx(0.01));
  CHECK(listParent->GetAdap_MarkerHausd() == values);
  {
    ZoneConfigFile overlay("");
    CConfig inheritedList(listParent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(SU2_TYPE::GetValue(inheritedList.GetAdap_Hausd()) == Approx(0.01));
    CHECK(inheritedList.GetAdap_MarkerHausd() == values);
    CConfig inheritedScalar(scalarParent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(SU2_TYPE::GetValue(inheritedScalar.GetAdap_Hausd()) == Approx(0.07));
    CHECK(inheritedScalar.GetAdap_MarkerHausd().empty());
  }
  {
    ZoneConfigFile overlay("ADAP_HAUSD= (0.07)\n");
    CConfig inherited(listParent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(SU2_TYPE::GetValue(inherited.GetAdap_Hausd()) == Approx(0.07));
    CHECK(inherited.GetAdap_MarkerHausd().empty());
  }
  {
    ZoneConfigFile overlay(list);
    CConfig inherited(scalarParent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(SU2_TYPE::GetValue(inherited.GetAdap_Hausd()) == Approx(0.01));
    CHECK(inherited.GetAdap_MarkerHausd() == values);
  }
}

#ifdef HAVE_MMG
namespace {

CSimplexMesh TwoMarkers(unsigned short dim) {
  const auto bottom = dim - 1;
  auto mesh = simplex_test::MakeSimplexMesh(dim, 2, [&](const passivedouble* point) {
    return point[bottom] == 0 ? "WALL" : "FAR";
  });
  REQUIRE(mesh.markers.size() == 2);
  REQUIRE(mesh.markers[0].name == "FAR");
  REQUIRE(mesh.markers[1].name == "WALL");
  mesh.markers[0].ref = 1;
  mesh.markers[1].ref = 2;
  const auto components = CSimplexMesh::GetnMetric(dim);
  mesh.metric.resize(mesh.GetnPoint() * components);
  for (unsigned long point = 0; point < mesh.GetnPoint(); ++point)
    for (unsigned short i = 0, k = 0; i < dim; ++i)
      for (unsigned short j = i; j < dim; ++j, ++k)
        mesh.metric[point * components + k] = i == j ? 4.0 : 0.0;
  return mesh;
}

struct ParameterLog {
  std::vector<std::map<std::string, std::string>> globals, locals;
  explicit ParameterLog(const std::string& output) {
    std::istringstream lines(output);
    for (std::string line; std::getline(lines, line);) {
      std::istringstream words(line);
      std::string label;
      words >> label;
      if (label != "MMG_PARAMETERS" && label != "MMG_LOCAL_PARAMETER") continue;
      auto& rows = label == "MMG_PARAMETERS" ? globals : locals;
      std::map<std::string, std::string> row;
      for (std::string field; words >> field;) {
        const auto equal = field.find('=');
        REQUIRE(equal != std::string::npos);
        row.emplace(field.substr(0, equal), field.substr(equal + 1));
      }
      rows.push_back(std::move(row));
    }
  }
};

struct LocalRow {
  std::string marker;
  CMMGInterface::LocalParameter value;
};

ParameterLog Applied(const CSimplexMesh& mesh, const std::string& options, bool wall,
                     const std::vector<CMMGInterface::LocalParameter>* overrideRows = nullptr) {
  Capture capture;
  const auto config = Config("ADAP_HMIN= 0.1\nADAP_HMAX= 3\nWRT_PERFORMANCE= YES\n" + options);
  CMMGInterface mmg(*config);
  if (wall) mmg.GetParameters().boundaryLayerMarkers = {"WALL"};
  if (overrideRows) mmg.SetLocalParameters(*overrideRows);
  mmg.SetMesh(mesh);
  CHECK(mmg.Remesh() != CMMGInterface::Status::STRONGFAILURE);
  return ParameterLog(capture.Text());
}

void CheckApplied(const ParameterLog& log, unsigned short dim, double hausd, const std::vector<LocalRow>& rows) {
  REQUIRE(log.globals.size() == 1);
  const auto& global = log.globals.front();
  CHECK(std::stoi(global.at("dim")) == dim);
  CHECK(std::stod(global.at("hmin")) == 0.1);
  CHECK(std::stod(global.at("hmax")) == 3.0);
  CHECK(std::stod(global.at("hgrad")) == 1.6);
  CHECK(std::stod(global.at("hausd")) == hausd);
  CHECK(std::stoul(global.at("local_rows")) == rows.size());
  REQUIRE(log.locals.size() == rows.size());
  for (size_t i = 0; i < rows.size(); ++i) {
    const auto& observed = log.locals[i];
    const auto& expected = rows[i];
    CHECK(observed.at("marker") == expected.marker);
    CHECK(std::stoi(observed.at("ref")) == expected.value.ref);
    CHECK(std::stod(observed.at("hmin")) == expected.value.hmin);
    CHECK(std::stod(observed.at("hmax")) == expected.value.hmax);
    CHECK(std::stod(observed.at("hausd")) == expected.value.hausd);
  }
}

}  // namespace

TEST_CASE("DEF1 MMG setters combine marker Hausdorff and wall sizing", "[.][def1_normal]") {
  // Use one wall and one far marker on edges and triangles, with both local Hausdorff values requiring full precision.
  const double farHausd = 0.1234567890123456, wallHausd = 0.01234567890123456;
  const std::string list = "ADAP_HAUSD= (FAR, 0.1234567890123456, WALL, 0.01234567890123456)\n";
  const std::string scalar = "ADAP_HAUSD= (0.03)\n";
  const std::vector<CMMGInterface::LocalParameter> empty;
  const std::vector<CMMGInterface::LocalParameter> overrideRows{{1, 0.27, 0.91, 0.044}, {2, 0.21, 0.85, 0.032}};
  for (const auto dim : {2, 3}) {
    CAPTURE(dim);
    const auto mesh = TwoMarkers(dim);
    const auto wallMax = dim == 2 ? 1.0 : std::sqrt(2.0);
    const LocalRow far{"FAR", {1, 0.1, 3.0, farHausd}}, wall{"WALL", {2, 0.1, 3.0, wallHausd}};
    // Automatic list rows without the wall rule use the two global sizes.
    CheckApplied(Applied(mesh, list, false), dim, 0.01, {far, wall});
    CheckApplied(Applied(mesh, list + "ADAP_BL_LOCAL_HMAX= NO\n", true), dim, 0.01, {far, wall});
    const LocalRow mergedWall{"WALL", {2, 0.1, wallMax, wallHausd}};
    // Preserve the wall rule's own order and check appended list-only rows by reference.
    const auto merged = Applied(mesh, list, true);
    const auto wallFirst = merged.locals.size() == 2 && merged.locals.front().at("marker") == "WALL";
    const std::vector<LocalRow> combined = wallFirst ? std::vector<LocalRow>{mergedWall, far}
                                                     : std::vector<LocalRow>{far, mergedWall};
    CheckApplied(merged, dim, 0.01, combined);
    CheckApplied(Applied(mesh, list, true, &empty), dim, 0.01, combined);
    CheckApplied(Applied(mesh, list, true, &overrideRows), dim, 0.01,
                 {{"FAR", overrideRows[0]}, {"WALL", overrideRows[1]}});
    CheckApplied(Applied(mesh, "", true), dim, 0.01, {{"WALL", {2, 0.1, wallMax, 0.01}}});
    CheckApplied(Applied(mesh, scalar, true), dim, 0.03, {{"WALL", {2, 0.1, wallMax, 0.03}}});
    CheckApplied(Applied(mesh, scalar, false), dim, 0.03, {});
  }
}
#endif

TEST_CASE("DEF1 fatal configuration and marker subprocess", "[.][def1_fatal]") {
  const auto* setting = std::getenv("SU2_DEF1_FATAL_CASE");
  REQUIRE(setting != nullptr);
  const std::string which(setting);
  const std::map<std::string, std::string> inputs{
      {"empty_middle", "ADAP_HAUSD= (WALL,,0.02)\n"},
      {"empty_first", "ADAP_HAUSD= (,WALL,0.02)\n"},
      {"empty_last", "ADAP_HAUSD= (WALL,0.02,)\n"},
      {"odd", "ADAP_HAUSD= (WALL,0.02,FAR)\n"},
      {"junk", "ADAP_HAUSD= (WALL,1e-3junk)\n"},
      {"zero", "ADAP_HAUSD= (WALL,0)\n"},
      {"negative", "ADAP_HAUSD= (WALL,-0.02)\n"},
      {"nonfinite", "ADAP_HAUSD= (WALL,nan)\n"},
      {"duplicate", "ADAP_HAUSD= (WALL,0.02,WALL,0.03)\n"},
      {"native", "ADAP_REMESHER= NATIVE_CAVITY\nADAP_HAUSD= (NOT_DECLARED,0.02)\n"},
      {"native_above", "ADAP_REMESHER= NATIVE_CAVITY\nADAP_HAUSD= (x_minus,1.001e100)\n"},
      {"native_bl", "ADAP_REMESHER= NATIVE_CAVITY\nADAP_HAUSD= (x_minus,0.02)\nADAP_BL_MARKER= (x_minus)\n"},
  };
  const auto input = inputs.find(which);
  if (input != inputs.end()) {
    (void)Config(input->second);
  } else {
#ifdef HAVE_MMG
    REQUIRE((which == "absent" || which == "absent_override"));
    const auto config = Config("ADAP_HAUSD= (MISSING, 0.02)\n");
    CMMGInterface mmg(*config);
    if (which == "absent_override") mmg.SetLocalParameters({{1, 0.1, 0.5, 0.03}});
    mmg.SetMesh(TwoMarkers(2));
#else
    FAIL("DEF1 requires MMG for the mesh-membership error tests.");
#endif
  }
  FAIL("The requested DEF1 error returned instead of aborting.");
}

TEST_CASE("TOL1 native list uses the smallest configured value after marker setup", "[.][TOL1]") {
  const std::string native = "ADAP_REMESHER= NATIVE_CAVITY\n";
  const std::string list = "ADAP_HAUSD= (x_plus, 1e100, x_minus, 1e-4)\n";
  const auto config = Config(native + list);
  REQUIRE(config->GetAdap_MarkerHausd().size() == 2);
  CHECK(config->GetAdap_MarkerHausd().at("x_plus") == 1e100);
  CHECK(SU2_TYPE::GetValue(config->GetAdap_Hausd()) == 1e-4);
  CHECK(SU2_TYPE::GetValue(Config(list)->GetAdap_Hausd()) == 0.01);
  CHECK(SU2_TYPE::GetValue(Config(native + "ADAP_HAUSD= (0.0002)\n")->GetAdap_Hausd()) == 0.0002);
  {
    ZoneConfigFile overlay("ADAP_HAUSD= 0.0003\n");
    CConfig child(config.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(child.GetAdap_MarkerHausd().empty());
    CHECK(SU2_TYPE::GetValue(child.GetAdap_Hausd()) == 0.0003);
  }
  {
    const auto parent = Config(native + "ADAP_HAUSD= 0.0003\n");
    ZoneConfigFile overlay(list);
    CConfig child(parent.get(), overlay.name, SU2_COMPONENT::SU2_CFD, 0, 1, false);
    CHECK(child.GetAdap_MarkerHausd() == config->GetAdap_MarkerHausd());
    CHECK(SU2_TYPE::GetValue(child.GetAdap_Hausd()) == 1e-4);
  }
}
