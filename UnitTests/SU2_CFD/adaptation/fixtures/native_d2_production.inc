// BL_REMESH_PLAN rev 3, WP-C: the D2 witnesses (and controls) through the PRODUCTION owner-pinned composed edge
// integration (SU2NativeBL3D::EdgeGeometry -> CBoundaryLayerMetric::EdgeBreakpoints owners -> ApplyPointOnChord ->
// FrozenField::TargetEdgeLength), unchanged 65,536-evaluation budget and 1e-9 relative + 1e-12 absolute tolerance.
// Each edge must converge; with SU2_D2_PIN_RECORD (the test-only pinned replica JSONL of the D2 helper) its length
// must equal the replica's converged total as binary64. Hidden: needs the M6 band case and the witness list.
// Uses swbi_dc2 (native_band_dc2.inc) for the case and the production wall, d2_replay (native_d2_replay.inc) for
// the six canonical covering donors that the replica integrated over.
TEST_CASE("D2 witnesses through the production owner-pinned edge integration", "[.][NativeD2Production]") {
  using namespace SU2Native3D;
  World world;REQUIRE(world.size==1);
  const char* root=std::getenv("SU2_D2_CASE");REQUIRE(root);
  const char* list=std::getenv("SU2_D2_WITNESSES");REQUIRE(list);
  const char* record=std::getenv("SU2_D2_PIN_RECORD");
  const auto t0=swbi_dc2::Clock::now();
  const auto c=swbi_dc2::Load(root);const auto n=swbi_dc2::Build(c);
  FrozenField field(d2_replay::Donors(c),SU2NativeBL3D::Composition(n.layers,c.core),false,std::vector<Id>{},
                    SU2NativeBL3D::EdgeGeometry(n.layers,c.core));
  std::map<int64_t,double> replica;
  if(record) {
    std::ifstream in(record);REQUIRE(in);
    for(std::string line;std::getline(in,line);) {
      const auto k=line.find("\"index\":"),ok=line.find("\"replica_ok\":1"),total=line.find("\"replica_total\":");
      if(k!=std::string::npos && ok!=std::string::npos && total!=std::string::npos)
        replica[std::atoll(line.c_str()+k+8)]=std::strtod(line.c_str()+total+16,nullptr);
    }
  }
  std::ifstream in(list,std::ios::binary|std::ios::ate);REQUIRE(in);const size_t count=size_t(in.tellg())/24;in.seekg(0);
  std::vector<int64_t> w(3*count);in.read(reinterpret_cast<char*>(w.data()),std::streamsize(24*count));REQUIRE(in);
  size_t converged=0,compared=0,equal=0;long double worst=0;std::string first_failure;
  for(size_t i=0;i<count;++i) {
    const Point a=swbi_dc2::At(c.pts,w[3*i+1]),b=swbi_dc2::At(c.pts,w[3*i+2]);
    try {
      const auto length=field.TargetEdgeLength(a,b);++converged;
      const auto found=replica.find(w[3*i]);
      if(found!=replica.end()) {
        ++compared;equal+=double(length)==found->second;
        worst=std::max(worst,std::abs(length-(long double)found->second)/std::max((long double)found->second,1e-300L));
      }
    } catch(const std::exception& e) {
      if(first_failure.empty())first_failure="index "+std::to_string(w[3*i])+": "+e.what();
    }
  }
  std::cout<<"D2 production pinned: "<<converged<<"/"<<count<<" converged, "<<equal<<"/"<<compared
           <<" equal to the pinned replica, worst relative difference "<<double(worst)<<", "
           <<swbi_dc2::Seconds(t0)<<" s"<<(first_failure.empty() ? "" : ", first failure "+first_failure)<<'\n';
  CHECK(converged==count);
  if(record){CHECK(compared==count);CHECK(equal==compared);}
}
