// DC-13: TEST-ONLY [NativeFirstRowDC13] harness (DC13_PLAN rev 2). DC-12's W -> R1 -> R2 -> G -> V12 pipeline with the
// DC-13 production optimizer (O4m min-max smoothing, O7/O8/O7L vertex insertion) and two diagnostics requested by the
// coordinator: the per-marker first-height multiplier (dc10::h0Scale, every wall marker's own h0 -> max(h0, m h0), no
// case switch) and the halving chain (each step starts from the previous output mesh in the same frozen sensor field
// with the next multiplier: the first children are split in the wall-normal direction at the new height, then V).
// Final gates are unchanged and absolute. Nothing here is production code except the optimizer it calls.
namespace dc13 {
using namespace dc12;
std::string JsonOpt13(const OptimizeReport& r) {
  std::string base=JsonOpt(r);base.pop_back();std::ostringstream o;o<<base<<",\"O7_shell_insertions\":"<<r.shell_insertions<<",\"O8_neighbourhood_insertions\":"<<r.neighbourhood_insertions
    <<",\"O7L_length_insertions\":"<<r.length_insertions<<",\"placements\":"<<r.placements<<",\"placement_refusals\":"<<r.placement_refusals<<",\"placement_evaluations\":"<<r.placement_evaluations<<"}";
  return o.str();
}
// Halving step (wall-normal split of the near-wall layer): every wall face's first child (face + apex p) is coned from
// a point q on the segment from p to its normal foot, at the face's (new) prescribed height: the new first child is
// (face, q) with the same foot, the three other cells (q, wall edge, p). Certified first-row route (A3); registrations
// follow the new apex. q lies strictly inside the old first child when the old altitude exceeds the new height.
struct HalveStats { size_t faces=0,split=0,skipped_low=0,refused=0;std::map<std::string,size_t> why; };
HalveStats HalveFirstRow(State& s,Rows10& rows,std::set<Face>& certified) {
  Scope scope("DC13 HalveFirstRow");HalveStats st;
  std::vector<std::pair<Id,Face>> faces;for(const auto& wf:WallFaces(s))faces.push_back(wf);
  for(const auto& wf:faces){++st.faces;if(!s.mesh.count(wf.first))continue;
    const Cell c=s.mesh.at(wf.first).cell;Face out{};for(auto f:CellFaces(c))if(Sorted(f)==wf.second)out=f;
    Node apex{};for(auto n:c.v)if(!Has(wf.second,n.id))apex=n;
    const V a=Of(NodePoint(s,out[0])),b=Of(NodePoint(s,out[1])),cc=Of(NodePoint(s,out[2]));const V n=-1.L*Outward(a,b,cc);
    const long double alt=Dot(Of(apex.p)-a,n),h=FaceH0(s,wf.second);
    if(!(alt>h*(1+HeightTolerance))){++st.skipped_low;continue;}  // already within the new window (or lower)
    Patch old;try{old=PatchOf(s,{wf.first});}catch(const std::exception&){++st.refused;++st.why["patch"];continue;}
    const Node q{s.next_node,Pt(Of(apex.p)-(alt-h)*n)};
    Patch fresh;fresh.surface=old.surface;fresh.parameters=old.parameters;std::string why;
    try{if(!Cone(old.cells,q,s.next_cell,fresh.cells,why)){++st.refused;++st.why["cone: "+Key(why)];continue;}s.reference->ExportParameters(fresh);}
    catch(const std::exception& x){++st.refused;++st.why[std::string("cone: ")+x.what()];continue;}
    bool ok=true;for(const auto& x:fresh.cells)ok=ok&&Positive(x.v)&&Valid(Shape(s,x));
    if(!ok){++st.refused;++st.why["validity"];continue;}
    std::string hw;if(!HeightT(s,fresh,hw)){++st.refused;++st.why["height: "+Key(hw)];continue;}
    Apply(s,{wf.first},fresh);rows.apex[wf.second]=q.id;certified.insert(wf.second);++st.split;
  }
  return st;
}
// Census of the cells of the input that survive unchanged (cell keys) and of the vertices kept/added/removed.
struct Change { size_t cells_before=0,cells_after=0,cells_unchanged=0,vertices_before=0,vertices_after=0,vertices_kept=0;long double max_kept_displacement=0; };
Change Compare(const Owned& before,const State& s) {
  Change c;c.cells_before=before.size();c.cells_after=s.mesh.size();std::set<std::array<Id,4>> keys;std::map<Id,Point> vb;
  for(const auto& r:before){keys.insert(CellKey(r.second.cell));for(auto n:r.second.cell.v)vb[n.id]=n.p;}
  std::map<Id,Point> va;for(const auto& r:s.mesh){c.cells_unchanged+=keys.count(CellKey(r.second.cell));for(auto n:r.second.cell.v)va[n.id]=n.p;}
  c.vertices_before=vb.size();c.vertices_after=va.size();
  for(const auto& v:vb){const auto f=va.find(v.first);if(f==va.end())continue;++c.vertices_kept;c.max_kept_displacement=std::max(c.max_kept_displacement,Norm(Of(f->second)-Of(v.second)));}
  return c;
}
}  // namespace dc13

TEST_CASE("DC-13 sliver-removing build-then-optimize, first-height ramp and halving chain", "[.][NativeFirstRowDC13]") {
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
      <<",\"lift\":\"single\",\"h0_scale\":"<<scale<<",\"chain\":\""<<env("SU2_DC13_CHAIN")<<"\",\"reflective\":\""<<env("SU2_DC12_REFLECTIVE")<<"\",\"inputs_sha256\":\""<<inputSha<<"\"}";
    std::string tag=root+"/dc13_"+name+(scale!=1?"_h0x"+Fmt(scale):"");
    {std::ifstream old(tag+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&inputSha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos
        &&saved.find("\"agrees_with_cached_final\":1")!=std::string::npos&&saved.size()>2&&saved.substr(saved.size()-2)=="}\n"
        &&digest(tag+"_final.mesh")!="missing"&&saved.find("\"final_mesh_fnv64\":\""+digest(tag+"_final.mesh")+"\"")!=std::string::npos)continue;}
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
        json<<",\"halve\":{\"faces\":"<<hv.faces<<",\"split\":"<<hv.split<<",\"already_within\":"<<hv.skipped_low<<",\"refused\":"<<hv.refused<<",\"why\":{";
        {bool k=false;for(const auto& d:hv.why){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
        {Scope sc("JsonFinal after halve");json<<",\"after_halve\":"<<JsonFinal(*s,rows);}
        std::cout<<"DC13 "<<name<<" step "<<step<<" x"<<m<<" halved="<<hv.split<<" refused="<<hv.refused<<std::endl;
      }
      const auto v=PhaseV12(*s,rows,G,vWork,vSeconds,vSweeps,certified,log);
      json<<",\"V\":{\"stage\":"<<JsonStage(v)<<",\"sweeps\":"<<v.sweeps<<",\"refine_attempts\":"<<v.refine_attempts<<",\"first_row\":{\"tried\":"<<v.first.tried<<",\"inserted\":"<<v.first.inserted<<",\"relocated\":"<<v.first.relocated
          <<"},\"seconds\":{\"refine\":"<<v.refine_seconds<<",\"optimize\":"<<v.optimize_seconds<<"},\"accepted_by\":{";
      {bool k=false;for(const auto& d:v.acceptedBy){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"demand\":{";
      {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"per_sweep\":[";
      {bool k=false;for(const auto& r:v.per_sweep){json<<(k?",":"")<<"["<<r.cells<<","<<r.long_edges<<","<<r.bad<<","<<r.height<<","<<r.work<<","<<r.seconds<<"]";k=true;}}json<<"],\"optimizer\":[";
      {bool k=false;for(const auto& r:v.opt){json<<(k?",":"")<<JsonOpt13(r);k=true;}}json<<"]}";
      {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
      json<<",\"bands_final\":"<<JsonBands(BandCensus(*s))<<",\"layers_final\":"<<JsonLayers(CountLayers(*s,G,eligible))<<",\"first_row_audit\":"<<JsonAudit(FirstRowAudit(*s,snapshot,certified))
          <<",\"stale_apex_registrations\":"<<StaleRegistrations(*s,rows);
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
