// DC-11: TEST-ONLY [NativeFirstRowDC11] harness (SPLIT_RULE_PLAN, user decision 2026-10-23: 3D split acceptance follows
// the 2D rule). It reuses DC-10 unchanged (option-A target, W, R1, R1w, R2 single | per-side + crease fan, ApexMove,
// ColumnSplit, final gates, export, fresh re-audit) and switches the per-operation size certificates by dc9::splitPolicy
// (rule 1 = 2D split rule; rule 0 = the DC-10 certificates, the A/B control) together with the production
// RefineAcceptance. V = PhaseV10 with the halves guard, a sweep cap and per-op/per-sweep work counts.
// Final gates are unchanged and absolute. Nothing here is production code.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLSplit11.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
TEST_CASE("DC-11 2D split rule, option-A target, W -> R1 -> R2(single | fan) -> V", "[.][NativeFirstRowDC11]") {
  using namespace dc11;
  struct RestorePolicy{RefineAcceptance a=GetRefineAcceptance();bool had=std::getenv("SU2_DC9_SECTOR_FIRST")!=nullptr;std::string v=had?std::getenv("SU2_DC9_SECTOR_FIRST"):"";
    ~RestorePolicy(){SetRefineAcceptance(a);dc9::splitPolicy=dc9::SplitPolicy{};if(had)setenv("SU2_DC9_SECTOR_FIRST",v.c_str(),1);}} restorePolicy;
  // FNV-1a 64 digest of an artifact (resume provenance: the saved final mesh must exist and match its recorded digest)
  auto digest=[](const std::string& path)->std::string{std::ifstream in(path,std::ios::binary);if(!in)return "missing";uint64_t h=1469598103934665603ULL;char c;
    while(in.get(c)){h^=(unsigned char)c;h*=1099511628211ULL;}std::ostringstream o;o<<std::hex<<h;return o.str();};
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC11_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC11_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC11_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC11_CASES");REQUIRE(only);
  const char* binary=std::getenv("SU2_DC11_BINARY_SHA");const std::string sha=binary?binary:"unknown";
  const std::string lift=std::getenv("SU2_DC11_LIFT")?std::getenv("SU2_DC11_LIFT"):"fan";
  REQUIRE((lift=="single"||lift=="fan"));
  const std::string rule=std::getenv("SU2_DC11_SPLIT_RULE")?std::getenv("SU2_DC11_SPLIT_RULE"):"2d";  // 2d | strict (A/B control)
  REQUIRE((rule=="2d"||rule=="strict"));
  unsetenv("SU2_DC9_SECTOR_FIRST");  // inherited DC-9 ordering option is not part of DC-10 (not in the resume key)
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    const size_t vWork=std::getenv("SU2_DC11_V_WORK")?std::stoul(std::getenv("SU2_DC11_V_WORK")):8192;
    const double vSeconds=std::getenv("SU2_DC11_V_SECONDS")?std::stod(std::getenv("SU2_DC11_V_SECONDS")):120;
    const size_t vSweeps=std::getenv("SU2_DC11_V_SWEEPS")?std::stoul(std::getenv("SU2_DC11_V_SWEEPS")):64;
    const bool grow=std::getenv("SU2_DC11_GROW")&&std::string(std::getenv("SU2_DC11_GROW"))=="1";  // iteration 2 (SplitGrow)
    const std::string stopAfter=std::getenv("SU2_DC11_STOP_AFTER")?std::getenv("SU2_DC11_STOP_AFTER"):"";  // diagnostics only
    const std::string inputSha=std::getenv("SU2_DC11_INPUT_SHA")?std::getenv("SU2_DC11_INPUT_SHA"):"unknown";  // runtime inputs (run_case.sh)
    std::ostringstream limits;limits<<std::setprecision(12)<<"\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"V\":["<<vWork<<","<<vSeconds<<"],\"V_sweeps\":"<<vSweeps<<",\"lift\":\""<<lift<<"\",\"split_rule\":\""<<rule<<"\",\"grow\":"<<grow<<",\"inputs_sha256\":\""<<inputSha<<"\"}";
    const std::string tag=root+"/dc11_"+name;
    if(stopAfter.empty()){std::ifstream old(tag+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&inputSha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos
        &&saved.find("\"agrees_with_cached_final\":1")!=std::string::npos&&saved.size()>2&&saved.substr(saved.size()-2)=="}\n"
        &&saved.find("\"final_mesh_fnv64\":\""+digest(tag+"_final.mesh")+"\"")!=std::string::npos&&digest(tag+"_final.mesh")!="missing")continue;}
    splitPolicy=SplitPolicy{};splitPolicy.rule=rule=="2d"?1:0;
    SetRefineAcceptance(rule=="2d"?RefineAcceptance::SplitInsertion:RefineAcceptance::StrictExcess);
    auto policyJson=[&]{std::ostringstream o;o<<"{\"size_insertions\":"<<splitPolicy.size_insertions<<",\"vertex_removal_checks\":"<<splitPolicy.vertex_removal_checks
      <<",\"vertex_removal_refusals\":"<<splitPolicy.vertex_removal_refusals<<",\"nonsize_checks\":"<<splitPolicy.nonsize_checks<<",\"nonsize_refusals\":"<<splitPolicy.nonsize_refusals
      <<",\"layer_free\":"<<splitPolicy.layer_free<<",\"layer_checked\":"<<splitPolicy.layer_checked<<"}";return o.str();};
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;geometryHits=0;
    const auto start=Clock::now();std::ostringstream json,log;
    json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"binary_sha256\":\""<<sha<<"\","<<limits.str();
    auto load=[&]()->std::unique_ptr<State>{
      if(name=="annulus"){optionA=std::make_shared<OptionA>();return LoadA1(fixtures);}
      if(name.rfind("m6_",0)==0)return LoadM6CutA(cuts+"/"+name+".txt");
      return LoadTubeA(fixtures+"/"+name+".txt");};
    std::unique_ptr<State> s;{Scope sc("Load");s=load();}Rows10 rows;DetectCreases(*s);
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped
        <<",\"d1\":{\"segments\":"<<s->layers->GetFeatures().segments<<",\"images\":"<<s->layers->GetFeatures().images<<",\"corners\":"<<s->layers->GetFeatures().corners
        <<",\"angle\":"<<s->layers->GetFeatures().angle<<",\"slope\":"<<s->layers->GetFeatures().slope<<",\"R_f\":"<<s->layers->GetFeatures().maximumRadius
        <<",\"creases_in_cut_test_side\":"<<d1.creases.size()<<"},\"option_A\":"<<OptionACheck(*s,nullptr)<<",\"bl_wall_vertices_snapped_to_y0\":"<<m6Snapped<<"}";
    std::cout<<"DC11 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<" creases="<<optionA->creases.size()<<" strips="<<optionA->strips.size()<<std::endl;
    if(const char* probe=std::getenv("SU2_DC11_PROBE")){  // diagnostics: pointwise option-A tensor along a segment
      std::stringstream in(probe);std::array<double,6> c{};char comma;for(int k=0;k<6;++k){in>>c[k];if(k<5)in>>comma;}
      const Tensor sensor{s->core,0,0,s->core,0,s->core};std::ofstream out(tag+"_probe.txt");out<<std::setprecision(17);
      const Point pa{c[0],c[1],c[2]},pb{c[3],c[4],c[5]};
      auto ownFull=std::make_shared<CBoundaryLayerMetric::ChordOwners>(),ownBase=std::make_shared<CBoundaryLayerMetric::ChordOwners>();
      const auto cutsFull=probeFull->EdgeBreakpoints({pa.x,pa.y,pa.z},{pb.x,pb.y,pb.z},1.5,16,ownFull.get());
      const auto cutsBase=probeBase->EdgeBreakpoints({pa.x,pa.y,pa.z},{pb.x,pb.y,pb.z},1.5,16,ownBase.get());
      out<<"cuts_full "<<cutsFull.size()<<" pieces "<<ownFull->Pieces()<<" cuts_base "<<cutsBase.size()<<" pieces "<<ownBase->Pieces()<<"\n";
      for(int i=0;i<=2000;++i){const long double t=i/2000.L;const Point p{double(c[0]+t*(c[3]-c[0])),double(c[1]+t*(c[4]-c[1])),double(c[2]+t*(c[5]-c[2]))};
        const auto m=optionA->Compose(*probeBase,*probeFull,s->core,p,sensor,nullptr,0);const auto mb=SU2NativeBL3D::Compose(*probeBase,s->core,p,sensor);
        const auto pf=optionA->Compose(*probeBase,*probeFull,s->core,p,sensor,ownFull.get(),t);const auto pbb=SU2NativeBL3D::Compose(*probeBase,s->core,p,sensor,ownBase.get(),t);
        out<<double(t)<<' '<<double(optionA->Weight(Of(p)))<<' '<<m.xx<<' '<<m.xy<<' '<<m.xz<<' '<<m.yy<<' '<<m.yz<<' '<<m.zz<<"  base "<<mb.xx<<' '<<mb.xy<<' '<<mb.xz<<' '<<mb.yy<<' '<<mb.yz<<' '<<mb.zz
           <<" pinned "<<pf.xx<<' '<<pf.xy<<' '<<pf.xz<<' '<<pf.yy<<' '<<pf.yz<<' '<<pf.zz<<" pinnedbase "<<pbb.xx<<' '<<pbb.xy<<' '<<pbb.xz<<' '<<pbb.yy<<' '<<pbb.yz<<' '<<pbb.zz;
        const auto mf=SU2NativeBL3D::Compose(*probeFull,s->core,p,sensor);const auto pff=SU2NativeBL3D::Compose(*probeFull,s->core,p,sensor,ownFull.get(),t);
        const V e=Of(pb)-Of(pa);auto norm=[&](const Tensor& m){return std::sqrt(e.x*e.x*m.xx+e.y*e.y*m.yy+e.z*e.z*m.zz+2*(e.x*e.y*m.xy+e.x*e.z*m.xz+e.y*e.z*m.yz));};
        out<<" full "<<mf.xx<<' '<<mf.xy<<' '<<mf.xz<<' '<<mf.yy<<' '<<mf.yz<<' '<<mf.zz<<" norms "<<double(norm(m))<<' '<<double(norm(mb))<<' '<<double(norm(mf))<<' '<<double(norm(pf))<<' '<<double(norm(pff))<<"\n";}
      continue;}
    const auto w=PhaseW10(*s,8192,120,log);
    json<<",\"W\":"<<JsonStage(w)<<",\"W_counts\":{\"splits\":"<<w.splits<<",\"unmeasurable\":"<<w.unmeasurable<<",\"strip_splits\":"<<w.strip_splits<<",\"strip_faces\":"<<w.strip_faces<<",\"strip_cells_across_min\":"<<w.strip_cells_min<<"},\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    std::cout<<"DC11 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<" splits="<<w.splits<<" t="<<w.seconds<<std::endl;
    if(stopAfter=="W"){std::ofstream(tag+"_W.json")<<json.str()<<"}\n";std::ofstream(tag+"_W.log")<<log.str();continue;}
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"collapses\":"<<r1.collapses<<",\"steiner\":"<<r1.steiner<<",\"exhausted_vertices\":"<<r1.exhausted<<'}';
    if(lift=="fan"){const auto rw=PhaseR1w(*s,rows,4096,120,log);json<<",\"R1w\":"<<JsonStage(rw)<<",\"R1w_counts\":{\"ridges\":"<<rw.ridges<<",\"inserted\":"<<rw.inserted<<",\"present\":"<<rw.present<<'}';
      std::cout<<"DC11 "<<name<<" R1w ridges="<<rw.ridges<<" inserted="<<rw.inserted<<" present="<<rw.present<<" t="<<rw.seconds<<std::endl;}
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC11 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<" t="<<r1.seconds<<std::endl;
    R2Stage10 r2;if(lift=="single")static_cast<R2Stage&>(r2)=PhaseR2(*s,static_cast<Rows&>(rows),600,log);else r2=PhaseR2_10(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"low_band_transactions\":"<<r2.low_band<<",\"sector_transactions\":"<<r2.sector_tx<<",\"gap_cells\":"<<r2.gaps<<",\"corner_separations\":"<<r2.separations
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"ridge_lifts\":"<<r2.ridges<<",\"fan_cells_built\":"<<r2.fan_cells
        <<",\"fan_cells_now\":"<<FanCells(*s,rows).size()<<",\"ridge_gap_cells\":"<<r2.gap_cells<<",\"fan_check_failures\":{";
    {bool k=false;for(const auto& c:r2.fanChecks){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"},\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC11 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" t="<<r2.seconds<<" last="<<r2.last.substr(0,300)<<std::endl;
    if(stopAfter=="R2"){std::ofstream(tag+"_R2.json")<<json.str()<<"}\n";std::ofstream(tag+"_R2.log")<<log.str();continue;}
    {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
    json<<",\"split_policy_after_R\":"<<policyJson();
    const auto v=PhaseV11(*s,rows,vWork,vSeconds,vSweeps,grow,log);
    json<<",\"split_policy_after_V\":"<<policyJson()<<",\"V11\":{\"halves_guard\":"<<v.halves_guard<<",\"accepted_by\":{";
    {bool k=false;for(const auto& d:v.acceptedBy){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"per_sweep\":[";
    {bool k=false;for(const auto& r:v.per_sweep){json<<(k?",":"")<<"["<<r.cells<<","<<r.long_edges<<","<<r.bad<<","<<r.height<<","<<r.work<<","<<r.seconds<<"]";k=true;}}json<<"],\"completed_sweeps\":"<<v.per_sweep.size()<<",\"stale_skips\":"<<v.stale_skips<<",\"grow\":{\"calls\":"<<v.grow.calls<<",\"grown\":"<<v.grow.grown
      <<",\"trials\":"<<v.grow.trials<<",\"steps\":"<<v.grow.steps<<",\"no_view\":"<<v.grow.no_view<<",\"no_gain\":"<<v.grow.no_gain<<",\"blocked\":"<<v.grow.blocked<<"},\"floor_refusals\":{";
    {bool k=false;for(const auto& d:v.floorBuckets){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
    json<<",\"V\":"<<JsonStage(v)<<",\"V_counts\":{\"sweeps\":"<<v.sweeps<<",\"column_demand_unrouted\":"<<v.column_demand<<",\"apex_moves\":"<<v.apex_moves<<",\"column_splits\":"<<v.column_splits<<",\"junction_column_splits\":"<<v.junction_splits
        <<",\"rejection_cache_skips\":"<<v.cache_skips<<",\"exact_on_edge_splits\":"<<v.exact_splits<<",\"no_exact_on_edge_point\":"<<v.no_exact_point<<",\"junction_insert_skips\":"<<v.junction_skips
        <<",\"with_complete_rows\":"<<(r2.unrowed==0)<<",\"demand\":{";
    {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
    {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
    {size_t fanBad=0;const auto fan=FanCells(*s,rows);for(Id id:fan)fanBad+=Bad(MeshShape(*s,id));json<<",\"final_fan\":{\"cells\":"<<fan.size()<<",\"bad\":"<<fanBad<<"}";}
    ExportGates10(*s,rows,tag+"_final.mesh");json<<",\"final_mesh_fnv64\":\""<<digest(tag+"_final.mesh")<<"\"";
    const auto before=Seconds(start);
    const auto distanceEvaluations=optionA->distanceEvaluations;
    {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;m6cacheA.clear();auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
         <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
     json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';}
    json<<",\"perf\":"<<JsonPerf()<<",\"wall_distance_evaluations\":"<<distanceEvaluations<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(tag+".json")<<json.str();std::ofstream(tag+".log")<<log.str();
    std::cout<<"DC11 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}
