// DC-14: tests of the production FIRST_ROW construction (SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp): the
// coordinate registry continuation, one fresh event and one halving event on a synthetic box with two wall markers
// and two symmetry planes through the production entry, and the TEST-ONLY frozen-field replay of the full M6 through
// the production driver backend ([NativeFrozenBL14], hidden).
namespace dc14 {
using namespace dc13;
using SU2NativeBLPipeline3D::RowRegistry;
std::unique_ptr<CConfig> BoxConfig(const std::string& extra) {
  std::stringstream ss(
      "SOLVER= NAVIER_STOKES\nMESH_FORMAT= BOX\nINIT_OPTION= TD_CONDITIONS\nREYNOLDS_NUMBER= 1e6\n"
      "MARKER_HEATFLUX= (wall, 0.0, top, 0.0)\nMARKER_SYM= (symmetry, symmetry2)\nMARKER_FAR= (outer)\n"
      "COMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\nADAP_HMIN= 1e-6\nADAP_HMAX= 10\nADAP_REMESHER= NATIVE_CAVITY\n"
      "ADAP_BL_METHOD= FIRST_ROW\nADAP_BL_MARKER= (wall, top)\nADAP_BL_FIRST_HEIGHT= (4e-3, 6e-3)\nADAP_BL_GROWTH= (1.2, 1.15)\n"
      "ADAP_BL_THICKNESS= (0.5)\nADAP_ANGLE= 60\n" + extra);
  std::unique_ptr<CConfig> config;
  {Mute mute; config = std::make_unique<CConfig>(ss, SU2_COMPONENT::SU2_CFD, false);}
  return config;
}
}  // namespace dc14

TEST_CASE("DC-14 FIRST_ROW registry: exact coordinate encoding, digest, sidecar round trip", "[NativeBLRegistry14]") {
  using namespace SU2NativeBLPipeline3D;
  RowRegistry r;
  const RowRegistry::P a{0.1, -2.0000000000000004, 1e-300}, b{std::nextafter(1.0, 2.0), -0.0, 3.14159265358979}, c{7, 8, 9};
  r.lift = {{a, b}};
  r.apex = {{{a, b, c}, c}};
  r.overrides = {{{a, b, c}, {c, b, a}}};
  r.extras = {{{a, b, c}, {{{c, a, b}, true}, {{b, c, a}, false}}}};
  r.digest = 1234567890123ULL; r.unsupported = 2;
  RowRegistry back; std::string why;
  REQUIRE(RowRegistry::Decode(r.Encode(), back, why));
  CHECK(back.lift == r.lift); CHECK(back.apex == r.apex); CHECK(back.overrides == r.overrides); CHECK(back.extras == r.extras);
  CHECK(back.digest == r.digest); CHECK(back.unsupported == 2);
  CHECK_FALSE(RowRegistry::Decode("SU2_NATIVE_ROWS 2\n", back, why));
  CHECK_FALSE(RowRegistry::Decode(r.Encode().substr(0, r.Encode().size() / 2), back, why));
  // digest: independent of cell order and of repeated vertices, sensitive to one coordinate bit
  Owned one, two;
  Cell c1{1, {Node{1, {0, 0, 0}}, Node{2, {1, 0, 0}}, Node{3, {0, 1, 0}}, Node{4, {0, 0, 1}}}};
  Cell c2{2, {Node{2, {1, 0, 0}}, Node{3, {0, 1, 0}}, Node{4, {0, 0, 1}}, Node{5, {1, 1, 1}}}};
  one[1].cell = c1; one[2].cell = c2; two[7].cell = c2; two[9].cell = c1;
  CHECK(CoordinateDigest(one) == CoordinateDigest(two));
  two[7].cell.v[3].p.x = std::nextafter(1.0, 2.0);
  CHECK(CoordinateDigest(one) != CoordinateDigest(two));
  // sidecar file round trip
  Continuation cont; cont.committed = std::make_shared<RowRegistry>(r);
  const std::string path = "dc14_registry_test.native_rows";
  WriteContinuation(path, cont);
  Continuation loaded; REQUIRE(LoadContinuation(path, loaded, why));
  CHECK(loaded.committed->extras == r.extras); CHECK(loaded.source == path);
  std::remove(path.c_str());
  Continuation missing; CHECK_FALSE(LoadContinuation(path, missing, why)); CHECK(why == "missing");
}

TEST_CASE("DC-14 FIRST_ROW entry: two wall markers, two symmetry planes, fresh event then halving event", "[NativeBL14Entry]") {
  using namespace dc14;
  using namespace SU2NativeBLPipeline3D;
  World world; REQUIRE(world.size == 1);
  auto config = BoxConfig("ADAP_LOOP= YES\nADAP_SIZES= (1000, 1000)\nADAP_BL_H0_RAMP= 1\n");
  REQUIRE(config->GetAdap_BL_H0Ramp() == 1);
  dc12::BoxSpec spec; spec.topWall = true; spec.seam = true;
  std::unique_ptr<junction_wp0::State> box;
  {dc12::Policies scoped; box = dc12::SyntheticBox(spec);}  // the fixture builder only (its field is not used)
  ReferenceState state;
  state.facets = box->facets; state.marker_names = box->markers;
  state.original = std::make_shared<PlanarReference>(box->facets);  // plain faceted: the entry promotes it
  std::vector<DonorCell> donors;
  for (const auto& row : box->mesh) { DonorCell d; d.cell = row.second.cell; d.sensor.fill(Tensor{1, 0, 0, 1, 0, 1}); donors.push_back(d); }
  long double volume = 0; for (const auto& row : box->mesh) { const auto& v = row.second.cell.v; volume += Orientation(v[0].p, v[1].p, v[2].p, v[3].p); }
  auto skinAreas = [](const Owned& cells, const PlanarReference& ref) {
    std::map<int, long double> area; std::map<Face, int> uses;
    for (const auto& r : cells) for (auto f : junction_wp0::CellFaces(r.second.cell)) ++uses[junction_wp0::Sorted(f)];
    size_t unbound = 0;
    for (const auto& r : cells) { const auto f = junction_wp0::CellFaces(r.second.cell);
      for (unsigned k = 0; k < 4; ++k) { if (uses.at(junction_wp0::Sorted(f[k])) != 1) continue; if (!r.second.physical[k].present) { ++unbound; continue; }
        std::array<junction_wp0::V, 3> q; for (unsigned t = 0; t < 3; ++t) for (auto n : r.second.cell.v) if (n.id == f[k][t]) q[t] = junction_wp0::Of(n.p);
        area[ref.Get(r.second.physical[k].facet).marker] += junction_wp0::Norm(junction_wp0::Cross(q[1] - q[0], q[2] - q[0])) / 2; } }
    area[-1] = unbound; return area; };
  const auto areas0 = skinAreas(box->mesh, *state.original);
  Continuation cont;
  Owned current = box->mesh;
  for (unsigned long event = 0; event < 2; ++event) {
    INFO("event " << event);
    config->SetAdap_BL_RampEvent(event);
    CHECK(config->GetAdap_BL(0).firstHeight == Approx(event ? 4e-3 : 8e-3));
    auto policy = MakeTargetPolicy(*config, state, 1.0, -1);
    CHECK(policy->roles.at(0) == 1); CHECK(policy->roles.at(3) == 1); CHECK(policy->roles.at(1) == 0); CHECK(policy->roles.at(4) == 0);
    CHECK(policy->roles.at(2) == 2);
    EventControls ctl; ctl.seconds = 400; ctl.event = event; ctl.ramp_factor = config->GetAdap_BL_RampFactor();
    ctl.final_cycle = config->GetAdap_BL_RampFinal(); ctl.report_prefix = "dc14_box_event" + std::to_string(event);
    std::string report;
    auto result = RunEvent(world, current, donors, state, *policy, *config, cont, ctl, report);
    INFO(report.substr(0, 4000));
    REQUIRE(!result.cells.empty());
    if (!event) REQUIRE(result.valid);  // intermediate acceptance: positive cells, every first height within T of h0_k
    CHECK(report.find("\"audit\":") != std::string::npos);
    CHECK(report.find(event ? "\"chain_step\":1" : "\"chain_step\":0") != std::string::npos);
    // every cell positive, volume and every marker's skin area unchanged (planar box), skin fully bound
    long double vol = 0; bool positive = true;
    for (const auto& row : result.cells) { const auto& v = row.second.cell.v; const auto o = Orientation(v[0].p, v[1].p, v[2].p, v[3].p); positive = positive && o > 0; vol += o; }
    CHECK(positive); CHECK(std::abs(double(vol - volume)) <= 1e-9 * double(volume));
    REQUIRE(result.reference); CHECK(result.reference->Mapped()); REQUIRE(result.parameters);
    const auto areas = skinAreas(result.cells, *result.reference);
    for (const auto& a : areas0) { const auto f = areas.find(a.first); REQUIRE(f != areas.end()); CHECK(std::abs(double(f->second - a.second)) <= 1e-9); }
    REQUIRE(cont.pending); CHECK(cont.pending->digest == CoordinateDigest(result.cells));
    CHECK(!cont.pending->lift.empty()); CHECK(!cont.pending->apex.empty());
    if (event) {
      CHECK(report.find("\"halve\":{") != std::string::npos);
      CHECK(report.find("\"completion\":{") != std::string::npos);
    }
    // commit (the remesher does this in onAccepted) and continue from the published state
    if (!result.valid) break;  // only an accepted event continues
    cont.committed = cont.pending; cont.source = "memory";
    current = result.cells; state.original = result.reference;
  }
}

TEST_CASE("DC-14 frozen-field FIRST_ROW event through the production driver backend (full M6)", "[.][NativeFrozenBL14]") {
  // TEST-ONLY replay: the frozen original sensor (P1 on the original mesh, donor file) is evaluated at the nodes of the
  // event's input mesh; the production MakeRemesher/Remesh run unchanged (FIRST_ROW, the configured ramp event); the
  // accepted mesh is written with its .native_ref and .native_rows sidecars; a JSON receipt is written in any case.
  World world;
  auto env = [](const char* k) -> std::string { const char* v = std::getenv(k); return v ? v : ""; };
  const auto cfg = env("SU2_DC14_CONFIG"), donorFile = env("SU2_DC14_DONORS"), out = env("SU2_DC14_OUT");
  REQUIRE(!cfg.empty()); REQUIRE(!donorFile.empty()); REQUIRE(!out.empty());
  const unsigned long event = std::stoul(env("SU2_DC14_EVENT").empty() ? "0" : env("SU2_DC14_EVENT"));
  const double parameter = env("SU2_DC14_OUTER_PARAMETER").empty() ? -1 : std::stod(env("SU2_DC14_OUTER_PARAMETER"));
  // DC-17 TEST-ONLY diagnostics: SU2_DC17_V_SECONDS caps V, SU2_DC17_OLD_HALVING=1 runs the DC-16 halving
  dc17::diagnosticVSeconds = env("SU2_DC17_V_SECONDS").empty() ? -1 : std::stod(env("SU2_DC17_V_SECONDS"));
  dc17::rebuildFirstRow = env("SU2_DC17_OLD_HALVING") != "1";
  dc17::probePath = env("SU2_DC17_PROBE");
  struct Dc17Reset { ~Dc17Reset() { dc17::diagnosticVSeconds = -1; dc17::rebuildFirstRow = true; dc17::probePath.clear(); } } dc17Reset;
  class Driver : public CSinglezoneDriver {
   public:
    using CSinglezoneDriver::CSinglezoneDriver;
    CConfig& Config() { return *config_container[ZONE_0]; }
    CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
    std::unique_ptr<CRemesher> Backend(double p, const std::string& prefix) {
      nativeOuterNormalParameter = p; nativeConstructionPrefix = prefix; nativeConstructionExport = true; return MakeRemesher(); }
    SU2Native3D::ReferenceState& Reference() { return *nativeReference3D; }
    SU2NativeBLPipeline3D::Continuation& Rows() { return *nativeFirstRow3D; }
  };
  const auto t0 = std::chrono::steady_clock::now();
  Driver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  auto& geometry = driver.Geometry(); REQUIRE(geometry.GetnDim() == 3);
  auto& config = driver.Config();
  REQUIRE(config.GetKind_Adap_BL_Method() == ADAP_BL_METHOD::FIRST_ROW);
  config.SetAdap_BL_RampEvent(event);
  // frozen original sensor (P1 donors of the original mesh)
  std::vector<DonorCell> donors;
  {std::ifstream in(donorFile, std::ios::binary); uint64_t n = 0; in.read(reinterpret_cast<char*>(&n), 8); REQUIRE(n > 0); REQUIRE(n < 20000000);
   donors.resize(n);
   for (auto& d : donors) { for (unsigned k = 0; k < 4; ++k) { uint64_t id; double x[3], m[6]; in.read(reinterpret_cast<char*>(&id), 8);
       in.read(reinterpret_cast<char*>(x), 24); in.read(reinterpret_cast<char*>(m), 48);
       d.cell.v[k] = Node{id, {x[0], x[1], x[2]}}; d.sensor[k] = Tensor{m[0], m[1], m[2], m[3], m[4], m[5]}; }
     d.cell.id = &d - donors.data();
     if (Orientation(d.cell.v[0].p, d.cell.v[1].p, d.cell.v[2].p, d.cell.v[3].p) < 0) { std::swap(d.cell.v[0], d.cell.v[1]); std::swap(d.sensor[0], d.sensor[1]); } }
   REQUIRE(bool(in));}
  const size_t limit = donors.size();
  FrozenField frozen(std::move(donors), {}, false, {}, {}, {}, true, limit);
  su2activematrix sensor(geometry.GetnPointDomain(), 6);
  for (unsigned long p = 0; p < sensor.rows(); ++p) {
    const auto* x = geometry.nodes->GetCoord(p);
    const auto t = frozen.Query({SU2_TYPE::GetValue(x[0]), SU2_TYPE::GetValue(x[1]), SU2_TYPE::GetValue(x[2])}).sensor;
    sensor(p, 0) = t.xx; sensor(p, 1) = t.xy; sensor(p, 2) = t.xz; sensor(p, 3) = t.yy; sensor(p, 4) = t.yz; sensor(p, 5) = t.zz;
  }
  const double prepared = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  auto backend = driver.Backend(parameter, out);
  const auto* native = dynamic_cast<CNativeRemesher3D*>(backend.get()); REQUIRE(native);
  const auto start = std::chrono::steady_clock::now();
  auto result = backend->Remesh(config, geometry, sensor);
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  const auto& stats = native->Statistics();
  const bool complete = result.status == CRemeshResult::Status::COMPLETE;
  bool written = false;
  if (complete) {
    if (result.onAccepted) result.onAccepted();
    MeshSolution candidate(&config, result.slices, 0);
    CMeshOutput::WriteMesh(&config, &candidate.Fine(), out);
    SU2Native3D::WriteReference(out + ".su2.native_ref", driver.Reference());
    SU2NativeBLPipeline3D::WriteContinuation(out + ".su2.native_rows", driver.Rows());
    written = true;
  }
  if (world.rank == 0) {
    std::ofstream receipt(out + "_receipt.json");
    receipt << std::setprecision(12) << "{\"event\":" << event << ",\"ramp_factor\":" << config.GetAdap_BL_RampFactor()
            << ",\"final_cycle\":" << config.GetAdap_BL_RampFinal() << ",\"input_points\":" << geometry.GetGlobal_nPointDomain()
            << ",\"complete\":" << complete << ",\"written\":" << written << ",\"initial_cells\":" << stats.initial_cells
            << ",\"final_cells\":" << stats.final_cells << ",\"complexity\":{\"value\":" << stats.complexity.value << ",\"uncertainty\":"
            ;if(std::isfinite(stats.complexity.uncertainty))receipt<<stats.complexity.uncertainty;else receipt<<"null";
    receipt << ",\"estimated\":" << stats.complexity.estimated << ",\"reason\":" << junction_wp0::Json(stats.complexity.reason)
            << ",\"target\":" << SU2_TYPE::GetValue(config.GetAdap_Complexity()) << "},\"prepare_seconds\":" << prepared
            << ",\"remesh_seconds\":" << seconds << ",\"peak_rss_kB\":" << dc9::PeakKB() << ",\"construction\":"
            << (stats.construction_report.empty() ? std::string("null") : stats.construction_report.substr(0, stats.construction_report.size() - 1)) << "}\n";
  }
  driver.Finalize();
  REQUIRE(complete);
  REQUIRE(written);
}

TEST_CASE("DC-17 shared column lids use every incident current wall height", "[NativeBL17Lid]") {
  using namespace dc12;
  Policies scoped;
  for(double angle:{0.,.7,1.3}) {
    BoxSpec spec;spec.n=1;spec.angle=angle;auto s=SyntheticBox(spec);
    auto delta=[&](double x,double z) {return V{x*std::cos(angle),x*std::sin(angle),z};};
    // The failing M6 columns had only about2.01 new h0: their midpoint leaves almost no apex room.
    const auto d=delta(.02,2.01028*spec.h0);
    CHECK(dc17::ColumnLidFraction(*s,1,d)==Approx(1.5/2.01028));
    s->walls.insert(1);s->heights[1]=.012;
    CHECK(dc17::ColumnLidFraction(*s,1,d)==Approx(.9));
    CHECK_FALSE(std::isfinite(dc17::ColumnLidFraction(*s,1,delta(.01,.02))));
    CHECK_FALSE(std::isfinite(dc17::ColumnLidFraction(*s,1,delta(-.02,.02))));
    CHECK_FALSE(std::isfinite(dc17::ColumnLidFraction(*s,UINT64_MAX,d)));
  }
}

TEST_CASE("First-row movement distinguishes requested heights from quality repair", "[NativeBL17Lid]") {
  using namespace dc12;
  Policies scoped;BoxSpec spec;spec.n=1;auto s=SyntheticBox(spec);
  const auto wf=WallFaces(*s).front();const auto old=PatchOf(*s,{wf.first});
  CHECK(FirstRowKind(*s,old)==Kind::Layer);
  s->heights[0]=1.;s->h0=1.;
  CHECK(FirstRowKind(*s,old)==Kind::Repair);
  s->heights[0]=2.;s->h0=2.;
  CHECK(FirstRowKind(*s,old)==Kind::Layer);
}
