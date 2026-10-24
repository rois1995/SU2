// DC-13: TEST-ONLY [NativeFirstRowDC13] harness (DC13_PLAN rev 2). DC-12's W -> R1 -> R2 -> G -> V12 pipeline with the
// DC-13 production optimizer (O4m min-max smoothing, O7/O8/O7L vertex insertion) and two diagnostics requested by the
// coordinator: the per-marker first-height multiplier (dc10::h0Scale, every wall marker's own h0 -> max(h0, m h0), no
// case switch) and the halving chain (each step starts from the previous output mesh in the same frozen sensor field
// with the next multiplier: the first children are split in the wall-normal direction at the new height, then V).
// Final gates are unchanged and absolute. Nothing here is production code except the optimizer it calls.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLVolume13.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
TEST_CASE("DC-13 sliver-removing build-then-optimize, first-height ramp and halving chain", "[.][NativeFirstRowDC13]") {
  // DC-14: the former environment switches are namespace switches of the production code (test-side only)
  auto flag=[](const char* k){const char* v=std::getenv(k);return v&&std::string(v)=="1";};
  dc13::unliftedColumns=flag("SU2_DC13_UNLIFTED_COLUMNS");dc13::diagnosticCutSlide=flag("SU2_DC13_CUT_SLIDE");
  dc13::v13MappedBoundaryOps=true;  // DC-13 results were produced with O5/O6 on the mapped cut references
  struct SwitchReset{~SwitchReset(){dc13::unliftedColumns=false;dc13::diagnosticCutSlide=false;dc13::v13MappedBoundaryOps=false;}} switchReset;
  using namespace dc13;
  auto digest=[](const std::string& path)->std::string{std::ifstream in(path,std::ios::binary);if(!in)return "missing";uint64_t h=1469598103934665603ULL;char c;
    while(in.get(c)){h^=(unsigned char)c;h*=1099511628211ULL;}std::ostringstream o;o<<std::hex<<h;return o.str();};
  World world;REQUIRE(world.size==1);
  auto env=[](const char* k)->std::string{const char* v=std::getenv(k);return v?v:"";};
  const std::string root=env("SU2_DC13_ROOT");REQUIRE(!root.empty());const std::string fixtures=env("SU2_DC13_WP0B");REQUIRE(!fixtures.empty());
  const std::string cuts=env("SU2_DC13_M6"),only=env("SU2_DC13_CASES");REQUIRE(!only.empty());
  const std::string sha=env("SU2_DC13_BINARY_SHA").empty()?"unknown":env("SU2_DC13_BINARY_SHA"),inputSha=env("SU2_DC13_INPUT_SHA").empty()?"unknown":env("SU2_DC13_INPUT_SHA");
  auto envD=[&](const char* k,double d){return env(k).empty()?d:std::stod(env(k));};
  const double gSeconds=envD("SU2_DC13_G_SECONDS",300),vSeconds=envD("SU2_DC13_V_SECONDS",1500),scale=envD("SU2_DC13_H0_SCALE",1);
  const size_t vWork=size_t(envD("SU2_DC13_V_WORK",1u<<22)),vSweeps=size_t(envD("SU2_DC13_V_SWEEPS",64));
  std::vector<double> chain;{std::stringstream l(env("SU2_DC13_CHAIN"));for(std::string x;std::getline(l,x,',');)if(!x.empty())chain.push_back(std::stod(x));}
  REQUIRE(scale>=1);for(double m:chain)REQUIRE(m>=1);
  struct ScaleReset{~ScaleReset(){dc10::h0Scale=1;}} scaleReset;
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    std::ostringstream limits;limits<<std::setprecision(12)<<"\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"G_seconds\":"<<gSeconds<<",\"V\":["<<vWork<<","<<vSeconds<<"],\"V_sweeps\":"<<vSweeps
      <<",\"lift\":\"single\",\"h0_scale\":"<<scale<<",\"chain\":\""<<env("SU2_DC13_CHAIN")<<"\",\"redistribute\":\""<<env("SU2_DC13_REDISTRIBUTE")<<"\",\"reflective\":\""<<env("SU2_DC12_REFLECTIVE")<<"\",\"inputs_sha256\":\""<<inputSha<<"\"}";
    std::string tag=root+"/dc13_"+name+(scale!=1?"_h0x"+Fmt(scale):"");
    // resume only when the LAST requested step has a complete receipt
    const std::string lastTag=chain.empty()?tag:tag+"_step"+std::to_string(chain.size())+"_h0x"+Fmt(chain.back());
    {std::ifstream old(lastTag+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&inputSha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos
        &&saved.find("\"agrees_with_cached_final\":1")!=std::string::npos&&saved.size()>2&&saved.substr(saved.size()-2)=="}\n"
        &&digest(lastTag+"_final.mesh")!="missing"&&saved.find("\"final_mesh_fnv64\":\""+digest(lastTag+"_final.mesh")+"\"")!=std::string::npos)continue;}
    Policies policies;
    reflectiveByRole=std::getenv("SU2_DC12_REFLECTIVE")!=nullptr;reflectiveMarkers.clear();
    if(name!="annulus"&&name.rfind("m6_",0)!=0)REQUIRE(reflectiveByRole);
    if(reflectiveByRole){std::stringstream list(env("SU2_DC12_REFLECTIVE"));for(std::string m;std::getline(list,m,',');)if(!m.empty())reflectiveMarkers.insert(m);}
    auto load=[&](double m)->std::unique_ptr<State>{dc10::h0Scale=m;std::unique_ptr<State> x;
      if(name=="annulus"){optionA=std::make_shared<OptionA>();x=LoadA1(fixtures);loadedArtificial.clear();}
      else if(name.rfind("m6_",0)==0)x=LoadM6CutA(cuts+"/"+name+".txt");else x=LoadTubeA(fixtures+"/"+name+".txt");
      artificialMarkers=loadedArtificial;ApplyWallPolicies(*x);return x;};
    // one pipeline run; step 0 builds W -> R1 -> R2 -> G -> V, a chain step halves the first row and runs V
    std::unique_ptr<State> s;Rows10 rows;Growth G;DirField field;std::map<Id,WallOf> eligible;
    for(size_t step=0;step<=chain.size();++step) {
      const double m=step?chain[step-1]:scale;const std::string stepTag=step?tag+"_step"+std::to_string(step)+"_h0x"+Fmt(m):tag;
      perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;geometryHits=0;
      const auto start=Clock::now();std::ostringstream json,log;
      json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"step\":"<<step<<",\"h0_scale_step\":"<<m<<",\"binary_sha256\":\""<<sha<<"\","<<limits.str();
      Owned input;std::set<Face> certified;std::map<Face,std::array<Id,4>> snapshot;
      if(step==0) {
        {Scope sc("Load");s=load(m);}DetectCreases(*s);
        json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"wall_heights\":{";
        {bool k=false;for(const auto& h:s->heights){json<<(k?",":"")<<"\""<<s->markers.at(h.first)<<"\":"<<h.second;k=true;}}json<<"}}";
        std::cout<<"DC13 "<<name<<" x"<<m<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
        const auto w=PhaseW10(*s,8192,120,log);json<<",\"W\":"<<JsonStage(w);
        field=ComputeDirectionField(*s);directionField=&field.d;directionFieldUses=0;
        const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1);
        const auto r2=PhaseR2(*s,static_cast<Rows&>(rows),600,log);
        json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed<<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<"}";
        json<<",\"directions\":"<<JsonDirections(field);
        {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
        snapshot=FirstChildren(*s);MapLayers(*s);eligible=EligibleAfterR2(*s);
        G=PhaseG(*s,rows,gSeconds,log);
        json<<",\"G\":"<<JsonGrowth(*s,G)<<",\"bands_after_G\":"<<JsonBands(BandCensus(*s));
        {Scope sc("JsonFinal after_G");json<<",\"after_G\":"<<JsonFinal(*s,rows);}
        std::cout<<"DC13 "<<name<<" x"<<m<<" R2 rowed="<<r2.rowed<<" G lifted="<<G.lifted<<" t="<<Seconds(start)<<std::endl;
      } else {
        // same frozen sensor field at the next multiplier: the target, the wall policies and heights are rebuilt, the
        // mesh, its boundary parameters and the first-row registry are kept
        input=s->mesh;auto fresh=load(m);s->field=std::move(fresh->field);s->layers=fresh->layers;s->h0=fresh->h0;s->heights=fresh->heights;
        s->growth=fresh->growth;s->thickness=fresh->thickness;s->lengths.clear();ResetCaches();DetectCreases(*s);MapLayers(*s);Index(*s);
        {Scope sc("JsonFinal before step");json<<",\"before_step\":"<<JsonFinal(*s,rows);}
        snapshot=FirstChildren(*s);
        const auto hv=HalveFirstRow(*s,rows,certified);
        const bool redistribute=!(std::getenv("SU2_DC13_REDISTRIBUTE")&&std::string(std::getenv("SU2_DC13_REDISTRIBUTE"))=="0");
        ColumnStats cs;if(redistribute)cs=ColumnInsert(*s,rows);
        json<<",\"column_insert\":{\"enabled\":"<<redistribute<<",\"columns\":"<<cs.columns<<",\"unlifted_columns\":"<<cs.unlifted<<",\"inserted\":"<<cs.inserted<<",\"refused\":"<<cs.refused<<",\"why\":{";
        {bool k=false;for(const auto& d:cs.why){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
        std::cout<<"DC13 "<<name<<" step "<<step<<" column insertions="<<cs.inserted<<" refused="<<cs.refused<<std::endl;
        json<<",\"halve\":{\"faces\":"<<hv.faces<<",\"split\":"<<hv.split<<",\"placed\":"<<hv.placed<<",\"already_within\":"<<hv.skipped_low<<",\"refused\":"<<hv.refused<<",\"why\":{";
        {bool k=false;for(const auto& d:hv.why){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
        {Scope sc("JsonFinal after halve");json<<",\"after_halve\":"<<JsonFinal(*s,rows);}
        std::cout<<"DC13 "<<name<<" step "<<step<<" x"<<m<<" halved="<<hv.split<<" refused="<<hv.refused<<std::endl;
      }
      v13R2Retry=step==0;const auto v=PhaseV13(*s,rows,G,vWork,vSeconds,vSweeps,certified,log);v13R2Retry=true;
      json<<",\"V\":{\"stage\":"<<JsonStage(v)<<",\"sweeps\":"<<v.sweeps<<",\"refine_attempts\":"<<v.refine_attempts<<",\"first_row\":{\"tried\":"<<v.first.tried<<",\"inserted\":"<<v.first.inserted<<",\"relocated\":"<<v.first.relocated
          <<"},\"seconds\":{\"refine\":"<<v.refine_seconds<<",\"optimize\":"<<v.optimize_seconds<<"},\"accepted_by\":{";
      {bool k=false;for(const auto& d:v.acceptedBy){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"demand\":{";
      {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"per_sweep\":[";
      {bool k=false;for(const auto& r:v.per_sweep){json<<(k?",":"")<<"["<<r.cells<<","<<r.long_edges<<","<<r.bad<<","<<r.height<<","<<r.work<<","<<r.seconds<<"]";k=true;}}json<<"],\"optimizer\":[";
      {bool k=false;for(const auto& r:v.opt){json<<(k?",":"")<<JsonOpt13(r);k=true;}}json<<"],\"by_class\":{";
      {bool k=false;for(const auto& c:v.byClass){json<<(k?",":"")<<"\""<<c.first<<"\":{\"attempts\":"<<c.second.attempts<<",\"accepted\":{";bool k2=false;for(const auto& a:c.second.accepted){json<<(k2?",":"")<<junction_wp0::Json(a.first)<<':'<<a.second;k2=true;}
        json<<"},\"refusals\":{";k2=false;std::vector<std::pair<size_t,std::string>> top;for(const auto& r:c.second.reasons)top.push_back({r.second,r.first});std::sort(top.rbegin(),top.rend());
        for(size_t i=0;i<top.size()&&i<12;++i){json<<(k2?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;k2=true;}json<<"}}";k=true;}}
      json<<"},\"demand_v13\":{\"classes\":\"0 interior, 1 wall, 2 seam, 3 carrier, 4 artificial\"}}";
      {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
      json<<",\"bad_witness\":"<<BadWitness(*s,rows);
      json<<",\"bands_final\":"<<JsonBands(BandCensus(*s))<<",\"layers_final\":"<<JsonLayers(CountLayers(*s,G,eligible))<<",\"first_row_audit\":"<<JsonAudit(FirstRowAudit(*s,snapshot,certified))
          <<",\"stale_apex_registrations\":"<<StaleRegistrations(*s,rows)
          <<",\"stale_lid_registrations\":"<<std::count_if(rows.lift.begin(),rows.lift.end(),[&](const std::pair<const Id,Id>& r){return !s->vertex_cells.count(r.second);});
      if(step){const auto c=Compare(input,*s);json<<",\"change\":{\"cells_before\":"<<c.cells_before<<",\"cells_after\":"<<c.cells_after<<",\"cells_unchanged\":"<<c.cells_unchanged
          <<",\"vertices_before\":"<<c.vertices_before<<",\"vertices_after\":"<<c.vertices_after<<",\"vertices_kept\":"<<c.vertices_kept<<",\"max_kept_displacement\":"<<double(c.max_kept_displacement)<<"}";}
      ExportGates10(*s,rows,stepTag+"_final.mesh");json<<",\"final_mesh_fnv64\":\""<<digest(stepTag+"_final.mesh")<<"\"";
      const auto before=Seconds(start);
      {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;m6cacheA.clear();auto fresh=load(m);fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
       json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
           <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
       json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';
       std::cout<<"DC13 "<<name<<" step "<<step<<" x"<<m<<" final cells="<<s->mesh.size()<<" long="<<ga.long_edges<<" bad="<<ga.bad_shape<<" h="<<ga.height_failures<<" complete="<<ga.complete<<" t="<<Seconds(start)<<std::endl;}
      json<<",\"perf\":"<<JsonPerf()<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
      std::ofstream(stepTag+".json")<<json.str();std::ofstream(stepTag+".log")<<log.str();
    }
  }
}

TEST_CASE("DC-13 production volume-only build-then-optimize on the promoted faceted reference", "[.][NativeVolumeOnlyDC13]") {
  using namespace dc12;using namespace dc11diag;
  World world;REQUIRE(world.size==1);
  struct Restore{RefineAcceptance a=GetRefineAcceptance();ShapeAdmission b=GetShapeAdmission();~Restore(){SetRefineAcceptance(a);SetShapeAdmission(b);}} restore;
  const char* folder=std::getenv("SU2_DC13_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC13_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC13_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC13_CASES");REQUIRE(only);
  const double seconds=std::getenv("SU2_DC13_VO_SECONDS")?std::stod(std::getenv("SU2_DC13_VO_SECONDS")):900;
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    SetRefineAcceptance(RefineAcceptance::SplitInsertion);
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();
    const bool isM6=name.rfind("m6_",0)==0;
    auto s=isM6?LoadM6CutA(cuts+"/"+name+".txt"):LoadTubeA(fixtures+"/"+name+".txt");
    Tensor sensor{s->core,0,0,s->core,0,s->core};
    if(isM6){std::ifstream in(cuts+"/"+name+".txt");std::string key;Id centre=UINT64_MAX;bool found=false;
      for(std::string line;std::getline(in,line);){std::istringstream l(line);l>>key;if(key=="centre")l>>centre;
        else if(centre!=UINT64_MAX&&!key.empty()&&std::isdigit((unsigned char)key[0])&&Id(std::stoull(key))==centre&&line.find(' ')!=std::string::npos){
          double x,y,z;std::istringstream r(line);Id id;r>>id>>x>>y>>z>>sensor.xx>>sensor.xy>>sensor.xz>>sensor.yy>>sensor.yz>>sensor.zz;found=bool(r);break;}}
      REQUIRE(found);}
    std::vector<DonorCell> donors;
    {Point lo=s->mesh.begin()->second.cell.v[0].p,hi=lo;for(const auto& r:s->mesh)for(auto x:r.second.cell.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
     const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
     auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
     const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
     for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
       if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
    const auto composition=SU2NativeBL3D::Composition(probeBase,s->core);const auto edges=SU2NativeBL3D::EdgeGeometry(probeBase,s->core);
    EngineOptions options;options.maximum_seconds=seconds;options.edge_breakpoints=edges;options.timings=false;
    // DC13_PLAN sect. 5: the plain faceted reference is promoted to the mapped authority (production
    // PromoteFacetedReference, 1e-13 relative declared error) so crease/junction points bind to their original feature;
    // mesh faces are bound to the canonical facet of their component (as production import binds through FindFace)
    double declared=0;const auto plain=std::make_shared<PlanarReference>(s->facets);
    s->reference=PromoteFacetedReference(plain,s->facets,1e-13,declared);s->mapped=true;s->overlay.clear();
    for(auto& row:s->mesh)for(auto& ph:row.second.physical)if(ph.present)ph.facet=s->reference->Canonical(ph.facet);
    options.surface_tolerance=std::max(options.surface_tolerance,declared);
    const auto cells0=s->mesh.size();const auto start=Clock::now();
    RefineOptimizeReport report;const auto r=RefineOptimize(world,s->mesh,donors,*s->reference,composition,options,OptimizeOptions{},&report);
    const double used=Seconds(start);
    std::ostringstream json;json<<std::setprecision(10)<<"{\"case\":\""<<name<<"\",\"target\":\"base\",\"reference\":\"promoted facets\",\"declared_error\":"<<declared<<",\"wall_heights\":\"empty\",\"seconds_limit\":"<<seconds
      <<",\"input_cells\":"<<cells0<<",\"valid\":"<<r.valid<<",\"stop\":\""<<EngineStopName(r.stats.stop)<<"\",\"reason\":"<<JS(r.reason.substr(0,400))<<",\"seconds\":"<<used
      <<",\"cells\":"<<r.cells.size()<<",\"rounds\":"<<report.rounds<<",\"refine_seconds\":"<<report.refine_seconds<<",\"optimize_seconds\":"<<report.optimize_seconds<<",\"round_stops\":[";
    for(size_t i=0;i<report.stops.size();++i)json<<(i?",":"")<<"[\""<<report.stops[i]<<"\","<<report.cells[i]<<"]";json<<"],\"optimizer\":[";
    for(size_t i=0;i<report.optimize.size();++i)json<<(i?",":"")<<JsonOpt(report.optimize[i]);json<<"]";
    std::cout<<"VOLONLY13 "<<name<<" valid="<<r.valid<<" stop="<<EngineStopName(r.stats.stop)<<" cells="<<r.cells.size()<<" rounds="<<report.rounds<<" t="<<used<<std::endl;
    if(!r.cells.empty()&&r.cells.size()<=60000) {
      s->mesh=r.cells;if(r.reference)s->reference=r.reference;s->overlay.clear();s->heights=WallHeights{};s->lengths.clear();ResetCaches();
      s->field=std::make_unique<FrozenField>(donors,composition,false,std::vector<Id>{},edges);Index(*s);
      try{const auto g=Final(*s);json<<",\"final\":{\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max\":"<<double(g.lmax)<<",\"long_edges\":"<<g.long_edges
        <<",\"bad_shape\":"<<g.bad_shape<<",\"unmeasurable\":"<<g.unmeasurable<<",\"complete\":"<<g.complete<<"}";
        std::cout<<"VOLONLY13 "<<name<<" final long="<<g.long_edges<<" bad="<<g.bad_shape<<" complete="<<g.complete<<std::endl;}
      catch(const std::exception& x){json<<",\"final_error\":"<<JS(x.what());}
      ExportGates10(*s,Rows10{},root+"/volonly13_"+name+"_final.mesh");
    }
    json<<",\"peak_rss_kB\":"<<PeakKB()<<"}\n";
    std::ofstream(root+"/volonly13_"+name+".json")<<json.str();
  }
}


TEST_CASE("DC-13 ApexPlace and the halving step on a synthetic box: window, positivity, skin, registrations", "[NativeApexPlace13]") {
  using namespace dc13;World world;REQUIRE(world.size==1);
  Policies policies;artificialMarkers={2};perf=Perf{};ResetCaches();cachesOn=true;
  BoxSpec b;auto s=SyntheticBox(b);ApplyWallPolicies(*s);DetectCreases(*s);size_t unbound=0;const auto areas=MarkerAreas(*s,unbound);REQUIRE(unbound==0);
  auto field=ComputeDirectionField(*s);directionField=&field.d;Rows10 rows;std::ostringstream log;
  PhaseR1(*s,rows,4096,60,log);PhaseR2(*s,static_cast<Rows&>(rows),120,log);REQUIRE(!rows.apex.empty());
  std::set<Face> passing;  // the wall faces whose first child passes after R2 (R2 does not row every face)
  auto checkMesh=[&](const char* what){INFO(what);long double volume=0;bool positive=true;
    for(const auto& row:s->mesh){const auto& c=row.second.cell;const auto o=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p);positive=positive&&o>0;volume+=o;}
    CHECK(positive);CHECK(std::abs(double(volume/6)-1)<1e-9);size_t u=0;const auto now=MarkerAreas(*s,u);CHECK(u==0);
    for(const auto& a:areas){const auto f=now.find(a.first);REQUIRE(f!=now.end());CHECK(std::abs(double(f->second-a.second))<=1e-9);}
    for(const auto& wf:WallFaces(*s))if(passing.count(wf.second)){double w=0;CHECK(HeightOk(*s,wf.first,&w));}
    CHECK(StaleRegistrations(*s,rows)==0);};
  for(const auto& wf:WallFaces(*s)){double w=0;if(HeightOk(*s,wf.first,&w))passing.insert(wf.second);}
  REQUIRE(passing.size()>=rows.apex.size()/2);
  checkMesh("after R2");
  // ApexPlace on every registered face: never worse than its star minimum, first child within the window
  VStage g;std::set<Face> certified;size_t placed=0;const auto keys=rows.apex;
  for(const auto& r:keys){std::string why;ResetCaches();if(ApexPlace(*s,rows,r.first,g,why,certified))++placed;}
  ResetCaches();checkMesh("after ApexPlace");CHECK(placed>=1);
  // halving step at twice the multiplier-free height: every first child split at h0/2 inside the window
  for(auto& h:s->heights)h.second/=2;ResetCaches();
  const auto hv=HalveFirstRow(*s,rows,certified);CHECK(hv.split>=1);CHECK(hv.refused==0);
  {std::set<Face> split;for(const auto& wf:WallFaces(*s)){double w=0;ResetCaches();if(HeightOk(*s,wf.first,&w))split.insert(wf.second);}
   size_t kept=0;for(const auto& f:passing)kept+=split.count(f);CHECK(kept==passing.size());passing=split;}
  ResetCaches();checkMesh("after halving");
}
