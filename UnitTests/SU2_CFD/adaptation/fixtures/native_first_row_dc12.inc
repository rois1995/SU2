// DC-12: TEST-ONLY [NativeFirstRowDC12] harness (OPTIMIZE_PLAN rev 3): build-then-optimize. It reuses DC-11 (option-A
// target, W10, R1, R2 single lift, ColumnSplit/ApexMove, halves guard, rejection cache, final gates, export, re-audit) and
// adds, as one connected step: (1) validity-only construction (production ShapeAdmission::ValidityOnly, dc9 shapePolicy for
// the R2/D.2/ConeColumn first-child checks, Admit12 for every other admission: every fresh cell q > 0 and J >= 1e-3, height
// T, facet/marker/L-C, caps, the SplitPolicy rule-1 progress rules); (2) the BL growth-layer operator G (column lifts at the
// production BL law heights d' = h0 + g d of the column's own wall); (3) the certified first-child route FirstRowInsert and
// the R2 retry; (4) V12 = bounded refinement + ApexMove + the production mesh-wide optimizer OptimizeMetricMesh. Final gates
// are unchanged and absolute. Nothing here is production code except the optimizer it calls.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLGrowth12.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
TEST_CASE("DC-12 build-then-optimize: W -> R1 -> R2 -> G (growth layers) -> V12 (validity-only + optimizer)", "[.][NativeFirstRowDC12]") {
  using namespace dc12;
  auto digest=[](const std::string& path)->std::string{std::ifstream in(path,std::ios::binary);if(!in)return "missing";uint64_t h=1469598103934665603ULL;char c;
    while(in.get(c)){h^=(unsigned char)c;h*=1099511628211ULL;}std::ostringstream o;o<<std::hex<<h;return o.str();};
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC12_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC12_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC12_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC12_CASES");REQUIRE(only);
  const std::string sha=std::getenv("SU2_DC12_BINARY_SHA")?std::getenv("SU2_DC12_BINARY_SHA"):"unknown";
  const std::string inputSha=std::getenv("SU2_DC12_INPUT_SHA")?std::getenv("SU2_DC12_INPUT_SHA"):"unknown";
  auto envD=[](const char* k,double d){return std::getenv(k)?std::stod(std::getenv(k)):d;};
  const double gSeconds=envD("SU2_DC12_G_SECONDS",300),vSeconds=envD("SU2_DC12_V_SECONDS",900);
  const size_t vWork=size_t(envD("SU2_DC12_V_WORK",1u<<22)),vSweeps=size_t(envD("SU2_DC12_V_SWEEPS",64));
  const bool growth=!(std::getenv("SU2_DC12_GROWTH")&&std::string(std::getenv("SU2_DC12_GROWTH"))=="0");  // A/B diagnostic only
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    std::ostringstream limits;limits<<std::setprecision(12)<<"\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"G_seconds\":"<<gSeconds<<",\"V\":["<<vWork<<","<<vSeconds<<"],\"V_sweeps\":"<<vSweeps
      <<",\"lift\":\"single\",\"growth\":"<<growth<<",\"reflective\":\""<<(std::getenv("SU2_DC12_REFLECTIVE")?std::getenv("SU2_DC12_REFLECTIVE"):"")<<"\",\"inputs_sha256\":\""<<inputSha<<"\"}";
    const std::string tag=root+"/dc12_"+name;
    {std::ifstream old(tag+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&inputSha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos
        &&saved.find("\"agrees_with_cached_final\":1")!=std::string::npos&&saved.size()>2&&saved.substr(saved.size()-2)=="}\n"
        &&digest(tag+"_final.mesh")!="missing"&&saved.find("\"final_mesh_fnv64\":\""+digest(tag+"_final.mesh")+"\"")!=std::string::npos)continue;}
    Policies policies;
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;geometryHits=0;
    const auto start=Clock::now();std::ostringstream json,log;
    json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"binary_sha256\":\""<<sha<<"\","<<limits.str();
    // explicit reflective carrier roles of the junction-case fixtures (case configuration, as MARKER_SYM): SU2_DC12_REFLECTIVE
    reflectiveByRole=std::getenv("SU2_DC12_REFLECTIVE")!=nullptr;reflectiveMarkers.clear();
    // junction-case fixtures carry no boundary roles: DC-12 requires them explicitly (never the marker-name inference)
    if(name!="annulus"&&name.rfind("m6_",0)!=0)REQUIRE(reflectiveByRole);
    if(reflectiveByRole){std::stringstream list(std::getenv("SU2_DC12_REFLECTIVE"));for(std::string m;std::getline(list,m,',');)if(!m.empty())reflectiveMarkers.insert(m);}
    auto load=[&]()->std::unique_ptr<State>{std::unique_ptr<State> x;
      if(name=="annulus"){optionA=std::make_shared<OptionA>();x=LoadA1(fixtures);loadedArtificial.clear();}
      else if(name.rfind("m6_",0)==0)x=LoadM6CutA(cuts+"/"+name+".txt");else x=LoadTubeA(fixtures+"/"+name+".txt");
      artificialMarkers=loadedArtificial;ApplyWallPolicies(*x);return x;};
    std::unique_ptr<State> s;{Scope sc("Load");s=load();}Rows10 rows;DetectCreases(*s);
    std::set<std::string> artificialNames;for(int m:artificialMarkers)artificialNames.insert(s->markers.at(m));
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped<<",\"artificial_markers\":"<<artificialMarkers.size()<<",\"wall_heights\":{";
    {bool k=false;for(const auto& h:s->heights){json<<(k?",":"")<<"\""<<s->markers.at(h.first)<<"\":"<<h.second;k=true;}}json<<"}}";
    std::cout<<"DC12 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
    const auto w=PhaseW10(*s,8192,120,log);json<<",\"W\":"<<JsonStage(w)<<",\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    DirField field;{field=ComputeDirectionField(*s);directionField=&field.d;directionFieldUses=0;}
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after<<'}';
    const auto r2=PhaseR2(*s,static_cast<Rows&>(rows),600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed<<",\"unrowed\":"<<r2.unrowed
        <<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    json<<",\"directions\":"<<JsonDirections(field);
    std::cout<<"DC12 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" t="<<r2.seconds<<std::endl;
    {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
    const auto snapshot=FirstChildren(*s);std::set<Face> certified;
    MapLayers(*s);const auto eligible=EligibleAfterR2(*s);
    json<<",\"bands_after_R\":"<<JsonBands(BandCensus(*s));
    Growth G;if(growth)G=PhaseG(*s,rows,gSeconds,log);else G.outcome="disabled";
    json<<",\"G\":"<<JsonGrowth(*s,G)<<",\"bands_after_G\":"<<JsonBands(BandCensus(*s));
    {Scope sc("JsonFinal after_G");json<<",\"after_G\":"<<JsonFinal(*s,rows);}
    json<<",\"layers_after_G\":"<<JsonLayers(CountLayers(*s,G,eligible));
    std::cout<<"DC12 "<<name<<" G lifted="<<G.lifted<<" columns="<<G.columns.size()<<" refused="<<G.refused<<" t="<<G.seconds<<std::endl;
    const auto v=PhaseV12(*s,rows,G,vWork,vSeconds,vSweeps,certified,log);
    json<<",\"V12\":{\"stage\":"<<JsonStage(v)<<",\"sweeps\":"<<v.sweeps<<",\"refine_attempts\":"<<v.refine_attempts<<",\"halves_guard\":"<<v.halves_guard<<",\"stale_skips\":"<<v.stale_skips
        <<",\"r2_retries\":"<<v.r2_retries<<",\"r2_new_rowed\":"<<v.r2_new_rowed<<",\"first_row\":{\"tried\":"<<v.first.tried<<",\"inserted\":"<<v.first.inserted<<",\"relocated\":"<<v.first.relocated
        <<",\"refused\":"<<v.first.refused<<",\"registrations_moved\":"<<v.first.registrations_moved<<"},\"seconds\":{\"r2_retry\":"<<v.r2_seconds<<",\"first_row\":"<<v.first_seconds<<",\"refine\":"<<v.refine_seconds<<",\"optimize\":"<<v.optimize_seconds
        <<"},\"column_demand_unrouted\":"<<v.column_demand<<",\"accepted_by\":{";
    {bool k=false;for(const auto& d:v.acceptedBy){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"demand\":{";
    {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"first_row_refusals\":{";
    {bool k=false;for(const auto& d:v.first.why){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"},\"per_sweep\":[";
    {bool k=false;for(const auto& r:v.per_sweep){json<<(k?",":"")<<"["<<r.cells<<","<<r.long_edges<<","<<r.bad<<","<<r.height<<","<<r.work<<","<<r.seconds<<"]";k=true;}}json<<"],\"optimizer\":[";
    {bool k=false;for(const auto& r:v.opt){json<<(k?",":"")<<JsonOpt(r);k=true;}}json<<"],\"split_policy\":{\"size_insertions\":"<<splitPolicy.size_insertions<<",\"vertex_removal_refusals\":"<<splitPolicy.vertex_removal_refusals
        <<",\"nonsize_refusals\":"<<splitPolicy.nonsize_refusals<<",\"layer_free\":"<<splitPolicy.layer_free<<"}}";
    {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
    json<<",\"bands_final\":"<<JsonBands(BandCensus(*s))<<",\"layers_final\":"<<JsonLayers(CountLayers(*s,G,eligible))<<",\"first_row_audit\":"<<JsonAudit(FirstRowAudit(*s,snapshot,certified))
        <<",\"stale_apex_registrations\":"<<StaleRegistrations(*s,rows);
    ExportGates10(*s,rows,tag+"_final.mesh");json<<",\"final_mesh_fnv64\":\""<<digest(tag+"_final.mesh")<<"\"";
    const auto before=Seconds(start);
    {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;m6cacheA.clear();auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
         <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
     json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';}
    json<<",\"perf\":"<<JsonPerf()<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(tag+".json")<<json.str();std::ofstream(tag+".log")<<log.str();
    std::cout<<"DC12 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}

TEST_CASE("DC-12 production volume-only build-then-optimize (serial RefineOptimize, no wall heights)", "[.][NativeVolumeOnlyDC12]") {
  using namespace dc12;using namespace dc11diag;
  World world;REQUIRE(world.size==1);
  struct Restore{RefineAcceptance a=GetRefineAcceptance();ShapeAdmission b=GetShapeAdmission();~Restore(){SetRefineAcceptance(a);SetShapeAdmission(b);}} restore;
  const char* folder=std::getenv("SU2_DC12_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC12_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC12_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC12_CASES");REQUIRE(only);
  const double seconds=std::getenv("SU2_DC12_VO_SECONDS")?std::stod(std::getenv("SU2_DC12_VO_SECONDS")):900;
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
    s->reference=std::make_shared<PlanarReference>(s->facets);s->mapped=false;s->overlay.clear();
    const auto cells0=s->mesh.size();const auto start=Clock::now();
    RefineOptimizeReport report;const auto r=RefineOptimize(world,s->mesh,donors,*s->reference,composition,options,OptimizeOptions{},&report);
    const double used=Seconds(start);
    std::ostringstream json;json<<std::setprecision(10)<<"{\"case\":\""<<name<<"\",\"target\":\"base\",\"reference\":\"plain facets\",\"wall_heights\":\"empty\",\"seconds_limit\":"<<seconds
      <<",\"input_cells\":"<<cells0<<",\"valid\":"<<r.valid<<",\"stop\":\""<<EngineStopName(r.stats.stop)<<"\",\"reason\":"<<JS(r.reason.substr(0,400))<<",\"seconds\":"<<used
      <<",\"cells\":"<<r.cells.size()<<",\"rounds\":"<<report.rounds<<",\"refine_seconds\":"<<report.refine_seconds<<",\"optimize_seconds\":"<<report.optimize_seconds<<",\"round_stops\":[";
    for(size_t i=0;i<report.stops.size();++i)json<<(i?",":"")<<"[\""<<report.stops[i]<<"\","<<report.cells[i]<<"]";json<<"],\"optimizer\":[";
    for(size_t i=0;i<report.optimize.size();++i)json<<(i?",":"")<<JsonOpt(report.optimize[i]);json<<"]";
    std::cout<<"VOLONLY12 "<<name<<" valid="<<r.valid<<" stop="<<EngineStopName(r.stats.stop)<<" cells="<<r.cells.size()<<" rounds="<<report.rounds<<" t="<<used<<std::endl;
    if(!r.cells.empty()&&r.cells.size()<=60000) {
      s->mesh=r.cells;if(r.reference)s->reference=r.reference;s->overlay.clear();s->heights=WallHeights{};s->lengths.clear();ResetCaches();
      s->field=std::make_unique<FrozenField>(donors,composition,false,std::vector<Id>{},edges);Index(*s);
      try{const auto g=Final(*s);json<<",\"final\":{\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max\":"<<double(g.lmax)<<",\"long_edges\":"<<g.long_edges
        <<",\"bad_shape\":"<<g.bad_shape<<",\"unmeasurable\":"<<g.unmeasurable<<",\"complete\":"<<g.complete<<"}";
        std::cout<<"VOLONLY12 "<<name<<" final long="<<g.long_edges<<" bad="<<g.bad_shape<<" complete="<<g.complete<<std::endl;}
      catch(const std::exception& x){json<<",\"final_error\":"<<JS(x.what());}
      ExportGates10(*s,Rows10{},root+"/volonly12_"+name+"_final.mesh");
    }
    json<<",\"peak_rss_kB\":"<<PeakKB()<<"}\n";
    std::ofstream(root+"/volonly12_"+name+".json")<<json.str();
  }
}

namespace dc12 {
// Synthetic box [0,1]^3 (rotated about z by `angle`), n^3 cubes x 6 Kuhn tetrahedra: marker 0 wall z=0, 1 reflective carrier
// x=0, 2 artificial boundary, 3 second wall z=1 (topWall), 4 y=0 carrier, and optional 5/6 x=1/y=1 carriers.
struct BoxSpec { int n=4;double h0=4e-3,g=1.2,thickness=.5,h0Top=6e-3,gTop=1.15,angle=0,shear=0,shearY=0;bool topWall=false,seam=false,symmetrySides=false;
  std::array<std::string,5> names{{"wall","symmetry","outer","top","symmetry2"}}; };
std::unique_ptr<State> SyntheticBox(const BoxSpec& b) {
  auto s=std::make_unique<State>();s->name="box";const int n=b.n;
  auto id=[&](int i,int j,int k){return Id(1+i+(n+1)*(j+(n+1)*k));};
  const double ca=std::cos(b.angle),sa=std::sin(b.angle);std::map<Id,Point> nodes;std::map<Id,std::array<int,3>> ijk;
  for(int k=0;k<=n;++k)for(int j=0;j<=n;++j)for(int i=0;i<=n;++i){const double z=double(k)/n,x=double(i)/n+b.shear*z,y=double(j)/n+b.shearY*z;nodes[id(i,j,k)]={ca*x-sa*y,sa*x+ca*y,z};ijk[id(i,j,k)]={i,j,k};}
  std::vector<Cell> mesh;Id cid=1;const std::array<std::array<int,3>,6> perms{{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
  for(int k=0;k<n;++k)for(int j=0;j<n;++j)for(int i=0;i<n;++i)for(const auto& p:perms){
    std::array<int,3> c{i,j,k};std::array<Id,4> v;v[0]=id(c[0],c[1],c[2]);for(int t=0;t<3;++t){++c[p[t]];v[t+1]=id(c[0],c[1],c[2]);}
    Cell cell{cid++,{Node{v[0],nodes.at(v[0])},Node{v[1],nodes.at(v[1])},Node{v[2],nodes.at(v[2])},Node{v[3],nodes.at(v[3])}}};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);mesh.push_back(cell);}
  s->markers={b.names[0],b.names[1],b.names[2],b.names[3],b.names[4]};if(b.symmetrySides){s->markers.push_back("symmetry3");s->markers.push_back("symmetry4");}
  auto plane=[&](const Face& f)->int{std::array<std::array<int,3>,3> q;for(int t=0;t<3;++t)q[t]=ijk.at(f[t]);
    auto all=[&](int axis,int val){return q[0][axis]==val&&q[1][axis]==val&&q[2][axis]==val;};
    if(all(2,0))return 0;if(all(2,n))return b.topWall?3:2;if(all(0,0))return 1;if(b.symmetrySides&&all(0,n))return 5;
    if(all(1,0))return b.symmetrySides?4:(b.seam?4:2);if(b.symmetrySides&&all(1,n))return 6;return 2;};
  std::map<Face,Face> skin;std::map<Face,int> uses;for(const auto& c:mesh)for(auto f:CellFaces(c)){skin[Sorted(f)]=f;++uses[Sorted(f)];}
  std::map<Face,Id> physical;Id fid=1;
  for(const auto& u:uses)if(u.second==1){const auto oriented=skin.at(u.first);Facet facet;facet.id=fid++;facet.marker=plane(u.first);
    for(int t=0;t<3;++t)facet.v[t]={oriented[t],nodes.at(oriented[t])};s->facets.push_back(facet);physical[u.first]=facet.id;}
  s->reference=std::make_shared<PlanarReference>(s->facets);
  for(const auto& c:mesh){OwnedCell row;row.cell=c;const auto f=CellFaces(c);for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}s->mesh.emplace(c.id,row);}
  s->walls.insert(0);s->heights[0]=b.h0;if(b.topWall){s->walls.insert(3);s->heights[3]=b.h0Top;}
  s->h0=b.topWall?std::min(b.h0,b.h0Top):b.h0;s->growth=b.g;s->thickness=b.thickness;s->core=1;s->heights.tolerance=HeightTolerance;
  std::vector<CBoundaryLayerMetric::Wall> walls;CBoundaryLayerMetric::FeatureOptions options;options.hgrad=1.3;
  for(int m:s->walls){CBoundaryLayerMetric::Wall w;w.layer.marker=s->markers.at(m);w.layer.firstHeight=m==0?b.h0:b.h0Top;w.layer.growth=m==0?b.g:b.gTop;w.layer.thickness=b.thickness;
    std::map<Id,unsigned long> index;
    for(const auto& f:s->facets)if(f.marker==m){for(auto x:f.v){const auto ins=index.emplace(x.id,index.size());if(ins.second){w.coord.push_back(x.p.x);w.coord.push_back(x.p.y);w.coord.push_back(x.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
    walls.push_back(std::move(w));}
  for(const auto& f:s->facets)if(!s->walls.count(f.marker)){CBoundaryLayerMetric::FeatureFace p;p.reflective=f.marker==1||f.marker==4||(b.symmetrySides&&(f.marker==5||f.marker==6));
    for(auto x:f.v){p.coord.push_back(x.p.x);p.coord.push_back(x.p.y);p.coord.push_back(x.p.z);}options.faces.push_back(std::move(p));}
  s->layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),45.0,std::move(options));
  std::vector<DonorCell> donors;{Point lo{-2,-2,-1},hi{3,3,2};const Tensor sensor{s->core,0,0,s->core,0,s->core};
    auto corner=[&](unsigned c){return Node{Id(1)<<40|c,{c&1?hi.x:lo.x,c&2?hi.y:lo.y,c&4?hi.z:lo.z}};};
    const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
    for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
      if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),true,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);dc7::MakeMapped(*s);return s;
}
// DC-16: roles of the box markers by index (0, 3 walls; 1, 4 reflective carriers; 2 artificial), never by name
inline std::map<int,int> BoxRoles(bool symmetrySides=false){auto r=std::map<int,int>{{0,1},{1,0},{2,2},{3,1},{4,0}};if(symmetrySides){r[5]=0;r[6]=0;}return r;}
std::map<int,long double> MarkerAreas(State& s,size_t& unbound) {
  std::map<int,long double> area;std::map<Face,int> uses;unbound=0;for(const auto& r:s.mesh)for(auto f:CellFaces(r.second.cell))++uses[Sorted(f)];
  for(const auto& r:s.mesh){const auto f=CellFaces(r.second.cell);for(unsigned k=0;k<4;++k){if(uses.at(Sorted(f[k]))!=1)continue;if(!r.second.physical[k].present){++unbound;continue;}
    const int m=s.reference->Get(r.second.physical[k].facet).marker;const V a=Of(NodePoint(s,f[k][0])),b=Of(NodePoint(s,f[k][1])),c=Of(NodePoint(s,f[k][2]));area[m]+=Norm(Cross(b-a,c-a))/2;}}
  return area;
}
}  // namespace dc12

TEST_CASE("DC-12 growth layers on synthetic boxes: fills, law heights, carriers, stops, renaming, two walls", "[NativeGrowthLayer12]") {
  using namespace dc12;World world;REQUIRE(world.size==1);
  struct Run { std::unique_ptr<State> s;Rows10 rows;Growth G;std::map<int,long double> areas;size_t walls=0;DirField field; };
  auto run=[&](const BoxSpec& b,const std::set<std::array<int,2>>& stopAt)->Run{
    Run r;Policies policies;artificialMarkers={2};perf=Perf{};ResetCaches();cachesOn=true;
    r.s=SyntheticBox(b);ApplyWallPolicies(*r.s);DetectCreases(*r.s,BoxRoles());size_t unbound=0;  // DC-16: roles by marker index (spec), as production takes them from config kinds
   r.areas=MarkerAreas(*r.s,unbound);REQUIRE(unbound==0);
    r.field=ComputeDirectionField(*r.s);directionField=&r.field.d;
    std::ostringstream log;PhaseR1(*r.s,r.rows,4096,60,log);PhaseR2(*r.s,static_cast<Rows&>(r.rows),120,log);
    std::set<Id> force;for(const auto& a:stopAt)force.insert(Id(1+a[0]+(b.n+1)*a[1]));
    r.G=PhaseG(*r.s,r.rows,120,log,force);
    for(const auto& c:r.G.columns)r.walls+=!c.stack.empty();
    return r;};
  auto check=[&](Run& r,const BoxSpec& b,const std::string& what) {
    INFO(what);State& s=*r.s;
    long double volume=0;bool positive=true;for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto o=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p);positive=positive&&o>0;volume+=o;}
    CHECK(positive);CHECK(std::abs(double(volume/6)-1)<1e-9);
    size_t unbound=0;const auto areas=MarkerAreas(s,unbound);CHECK(unbound==0);
    for(const auto& a:r.areas){const auto f=areas.find(a.first);REQUIRE(f!=areas.end());CHECK(std::abs(double(f->second-a.second))<=1e-9);}
    std::set<Face> rowed;for(const auto& wf:WallFaces(s))if(r.rows.apex.count(wf.second)){double w=0;CHECK(HeightOk(s,wf.first,&w));}
    const V normal=Unit(V{std::cos(b.angle),std::sin(b.angle),-b.shear});  // normal of the (sheared, rotated) plane x = 0
    for(const auto& c:r.G.columns) {
      REQUIRE(c.walls.size()==1);const unsigned short w=*c.walls.begin();const long double h0=FirstHeightOf(s,w),g=GrowthOf(s,w);
      long double d=c.lidDistance;
      for(Id x:c.stack){if(!s.vertex_cells.count(x))continue;const V p=Of(NodePoint(s,x));const auto dx=WallDistanceTo(s,w,p);
        CHECK(std::abs(double(dx/(h0+g*d))-1)<2e-2);d=dx;  // a unit target step reproduces the plain law within ~0.3 %
        const V root=Of(NodePoint(s,c.wall));if(std::abs(Dot(root,normal))<1e-12L)CHECK(std::abs(double(Dot(p,normal)))<=1e-12);}
    }
  };
  // 1 axis-aligned box: 12 non-rim wall columns (3 on the symmetry plane), each grows several law layers
  BoxSpec base;auto r0=run(base,{});check(r0,base,"base");
  CHECK(r0.G.columns.size()==12);for(const auto& c:r0.G.columns)CHECK(c.stack.size()>=3);
  // 2 a forced stop of one interior column: partial prisms around it, the neighbours still grow
  auto r1=run(base,{{2,2}});check(r1,base,"stopped neighbour");
  for(const auto& c:r1.G.columns){if(c.wall==Id(1+2+5*2))CHECK(c.stack.empty());else if(c.wall==Id(1+1+5*2)||c.wall==Id(1+3+5*2))CHECK(c.stack.size()>=1);}
  // 3 oblique symmetry plane (rotation about z by .3): carrier columns stay on the plane
  BoxSpec oblique;oblique.angle=.3;auto r2=run(oblique,{});check(r2,oblique,"oblique symmetry");CHECK(r2.walls>=10);
  // 4 renaming every marker (wall, symmetry, artificial) gives the identical mesh
  BoxSpec renamed;renamed.names={{"W7","PLANE","OUTER","LID","PLANE2"}};auto r3=run(renamed,{});check(r3,renamed,"renamed");
  {bool same=r3.s->mesh.size()==r0.s->mesh.size();auto a=r0.s->mesh.begin();auto b=r3.s->mesh.begin();
   for(;same&&a!=r0.s->mesh.end();++a,++b)for(unsigned k=0;k<4;++k)same=same&&a->first==b->first&&a->second.cell.v[k].id==b->second.cell.v[k].id&&SamePoint(a->second.cell.v[k].p,b->second.cell.v[k].p);
   CHECK(same);}
  // 5 two walls with different h0/growth: first rows at their own h0, columns follow their own law
  BoxSpec two;two.topWall=true;auto r4=run(two,{});check(r4,two,"two walls");
  {size_t top=0;for(const auto& c:r4.G.columns)if(*c.walls.begin()!=*r4.G.columns.front().walls.begin())++top;CHECK(top>=1);}
  // 7 inclined wall: the symmetry plane meets the wall at 65 deg (shear cot 65 = .4663): vertical columns would leave the
  // domain; on the plane the direction is exactly n - (n.s)s, every direction sees its faces (margin .2), layers stay inside
  BoxSpec inclined;inclined.shear=1/std::tan(65*std::acos(-1.)/180);auto r6=run(inclined,{});check(r6,inclined,"inclined symmetry plane");
  {const V sn=Unit(V{1,0,-inclined.shear});size_t onPlane=0;CHECK(r6.field.constrained>=3);CHECK(r6.field.low_margin==0);CHECK(r6.field.blended>=1);
   for(const auto& d:r6.field.d){const V x=Of(NodePoint(*r6.s,d.first));if(std::abs(Dot(x,sn))<1e-12L){++onPlane;CHECK(std::abs(double(Dot(d.second,sn)))<=1e-12);}}
   CHECK(onPlane>=3);
   for(const auto& c:r6.G.columns){if(std::abs(Dot(Of(NodePoint(*r6.s,c.wall)),sn))<.3)CHECK(c.stack.size()>=3);
     for(Id x:c.stack)if(r6.s->vertex_cells.count(x)){const V p=Of(NodePoint(*r6.s,x));CHECK(double(Dot(p,sn))>=-1e-12);}}}
  // 8 two inclined carriers (65 and 70 deg) meeting along an inclined seam: tangency on each plane, the seam column along
  // the planes' intersection, continuous blend, and the actual first-row lift directions equal the field directions
  BoxSpec twoPlanes;twoPlanes.shear=1/std::tan(65*std::acos(-1.)/180);twoPlanes.shearY=1/std::tan(70*std::acos(-1.)/180);twoPlanes.seam=true;
  auto r7=run(twoPlanes,{});
  {const V sx=Unit(V{1,0,-twoPlanes.shear}),sy=Unit(V{0,1,-twoPlanes.shearY});CHECK(r7.field.lines>=1);CHECK(r7.field.low_margin==0);
   for(const auto& d:r7.field.d){const V x=Of(NodePoint(*r7.s,d.first));const bool onX=std::abs(Dot(x,sx))<1e-12L,onY=std::abs(Dot(x,sy))<1e-12L;
     if(onX)CHECK(std::abs(double(Dot(d.second,sx)))<=1e-12);if(onY)CHECK(std::abs(double(Dot(d.second,sy)))<=1e-12);}
   size_t followed=0;
   for(const auto& l:r7.rows.lift){const auto f=r7.field.d.find(l.first);if(f==r7.field.d.end()||!r7.s->vertex_cells.count(l.second))continue;
     const V dir=Unit(Of(NodePoint(*r7.s,l.second))-Of(NodePoint(*r7.s,l.first)));CHECK(Norm(dir-f->second)<=1e-9L);++followed;}
   CHECK(followed>=6);
   for(const auto& c:r7.G.columns)for(Id x:c.stack)if(r7.s->vertex_cells.count(x)){const V p=Of(NodePoint(*r7.s,x));CHECK(double(Dot(p,sx))>=-1e-12);CHECK(double(Dot(p,sy))>=-1e-12);}}
  // 6 seam: a second carrier at y = 0 makes the root (0,0,0) a rank-2 seam column along x = y = 0
  BoxSpec seam;seam.seam=true;auto r5=run(seam,{});check(r5,seam,"seam");
  {bool found=false;for(const auto& c:r5.G.columns)if(c.wall==Id(1)){found=true;CHECK(!c.stack.empty());
     for(Id x:c.stack)if(r5.s->vertex_cells.count(x)){const auto p=NodePoint(*r5.s,x);CHECK(std::abs(p.x)<=1e-12);CHECK(std::abs(p.y)<=1e-12);}}
   CHECK(found);}
}

namespace dc12 {
inline std::string D9Number(double x) {
  if (!std::isfinite(x)) return "null";
  std::ostringstream o;
  o << std::setprecision(12) << x;
  return o.str();
}
struct D9StackRows {
  std::string columns, by_wall;
};
inline D9StackRows D9Columns(State& s, const Growth& growth) {
  std::map<unsigned short, std::vector<size_t>> layerCounts;
  std::map<unsigned short, size_t> wallColumns;
  std::map<unsigned short, int> markerForLayer;
  for (const auto& w : layerOfMarker) markerForLayer[w.second] = w.first;
  std::ostringstream cols;
  cols << "[";
  bool first = true;
  for (const auto& c : growth.columns) {
    if (!first) cols << ",";
    first = false;
    cols << "{\"root\":" << c.wall << ",\"top\":" << c.top << ",\"walls\":[";
    bool fw = true;
    for (unsigned short layer : c.walls) {
      if (!fw) cols << ",";
      fw = false;
      const int marker = markerForLayer.at(layer);
      cols << marker;
      ++wallColumns[layer];
      layerCounts[layer].push_back(c.stack.size());
    }
    cols << "],\"layers\":" << c.stack.size() << ",\"stopped\":" << (c.stopped ? "true" : "false")
         << ",\"stop\":" << junction_wp0::Json(c.stop) << ",\"stop_height\":" << D9Number(double(c.stopHeight)) << "}";
  }
  cols << "]";
  std::ostringstream byWall;
  byWall << "{";
  bool firstWall = true;
  for (int marker : s.walls) {
    if (!firstWall) byWall << ",";
    firstWall = false;
    const unsigned short layer = layerOfMarker.at(marker);
    auto v = layerCounts[layer];
    size_t total = 0, largest = 0;
    for (size_t count : v) {
      total += count;
      largest = std::max(largest, count);
    }
    byWall << "\"" << marker << "\":{\"layer_index\":" << layer
           << ",\"configured_h0\":" << D9Number(double(FirstHeightOf(s, layer)))
           << ",\"growth\":" << D9Number(double(GrowthOf(s, layer)))
           << ",\"thickness\":" << D9Number(double(ThicknessOf(s, layer))) << ",\"columns\":" << wallColumns[layer]
           << ",\"stack_count\":" << v.size() << ",\"largest_layers\":" << largest
           << ",\"mean_layers\":" << (v.empty() ? 0. : double(total) / v.size()) << "}";
  }
  byWall << "}";
  return {cols.str(), byWall.str()};
}
inline std::string D9FirstHeights(State& s, const Rows10& rows) {
  struct Sample {
    Id base, lift;
    double dz;
  };
  std::map<int, std::set<Id>> wallVertices;
  std::map<int, std::vector<Sample>> heights;
  std::map<int, size_t> missing;
  for (const auto& wf : WallFaces(s)) {
    const auto& row = s.mesh.at(wf.first);
    const auto faces = CellFaces(row.cell);
    for (unsigned k = 0; k < 4; ++k) {
      if (Sorted(faces[k]) != wf.second || !row.physical[k].present) continue;
      const int marker = s.reference->Get(row.physical[k].facet).marker;
      for (Id v : wf.second) wallVertices[marker].insert(v);
    }
  }
  for (int marker : s.walls) {
    for (Id base : wallVertices[marker]) {
      const auto lift = rows.lift.find(base);
      if (lift == rows.lift.end() || !s.vertex_cells.count(lift->second)) {
        ++missing[marker];
        continue;
      }
      const V a = Of(NodePoint(s, base)), b = Of(NodePoint(s, lift->second));
      heights[marker].push_back({base, lift->second, std::abs(double(b.z - a.z))});
    }
  }
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (int marker : s.walls) {
    if (!first) out << ",";
    first = false;
    const auto& values = heights[marker];
    double sum = 0, lo = INFINITY, hi = 0;
    for (const auto& sample : values) {
      sum += sample.dz;
      lo = std::min(lo, sample.dz);
      hi = std::max(hi, sample.dz);
    }
    out << "\"" << marker << "\":{\"measured_samples\":" << values.size() << ",\"missing_vertices\":" << missing[marker]
        << ",\"measurement_status\":"
        << junction_wp0::Json(values.empty() ? "unavailable: no base-to-lift samples" : "measured")
        << ",\"min_abs_dz\":" << D9Number(values.empty() ? NAN : lo)
        << ",\"max_abs_dz\":" << D9Number(values.empty() ? NAN : hi)
        << ",\"mean_abs_dz\":" << D9Number(values.empty() ? NAN : sum / values.size()) << ",\"samples\":[";
    for (size_t i = 0; i < values.size(); ++i) {
      const auto& sample = values[i];
      out << (i ? "," : "") << "{\"base\":" << sample.base << ",\"lift\":" << sample.lift
          << ",\"abs_dz\":" << D9Number(sample.dz) << "}";
    }
    out << "]}";
  }
  out << "}";
  return out.str();
}
inline std::string D9WallFirstChildHeights(const State& s) {
  std::map<int, std::vector<double>> values;
  for (const auto& row : s.mesh) {
    const auto faces = CellFaces(row.second.cell);
    for (unsigned k = 0; k < 4; ++k) {
      if (!row.second.physical[k].present) continue;
      const int marker = s.reference->Get(row.second.physical[k].facet).marker;
      if (!s.walls.count(marker)) continue;
      const V a = Of(NodePoint(s, faces[k][0]));
      const V b = Of(NodePoint(s, faces[k][1]));
      const V c = Of(NodePoint(s, faces[k][2]));
      const V normal = Unit(Cross(b - a, c - a));
      const V opposite = Of(row.second.cell.v[k].p);
      values[marker].push_back(std::abs(double(Dot(opposite - a, normal))));
    }
  }
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (int marker : s.walls) {
    if (!first) out << ",";
    first = false;
    const double requested = s.heights.at(marker);
    double sum = 0, lo = INFINITY, hi = 0, error = 0;
    for (double height : values[marker]) {
      sum += height;
      lo = std::min(lo, height);
      hi = std::max(hi, height);
      error = std::max(error, std::abs(height / requested - 1));
    }
    const auto count = values[marker].size();
    out << "\"" << marker << "\":{\"faces\":" << count << ",\"requested_h0\":" << D9Number(requested)
        << ",\"min_normal_distance\":" << D9Number(count ? lo : NAN)
        << ",\"max_normal_distance\":" << D9Number(count ? hi : NAN)
        << ",\"mean_normal_distance\":" << D9Number(count ? sum / count : NAN)
        << ",\"maximum_relative_error\":" << D9Number(count ? error : NAN)
        << ",\"height_tolerance\":" << D9Number(s.heights.tolerance) << "}";
  }
  out << "}";
  return out.str();
}
inline std::string D9MeshStats(State& s, const Gates8& gates) {
  size_t positive = 0, qBad = 0, jBad = 0;
  for (const auto& row : s.mesh) {
    if (Positive(row.second.cell.v)) ++positive;
    const auto shape = MeshShape(s, row.first);
    qBad += !std::isfinite(shape.q) || shape.q < MinimumMeanRatio;
    jBad += !std::isfinite(shape.J) || shape.J < MinimumScaledJacobian;
  }
  std::map<Face, size_t> uses;
  std::map<Face, bool> bound;
  for (const auto& row : s.mesh) {
    const auto faces = CellFaces(row.second.cell);
    for (unsigned k = 0; k < 4; ++k) {
      const auto key = Sorted(faces[k]);
      ++uses[key];
      bound[key] = bound[key] || row.second.physical[k].present;
    }
  }
  size_t nonmanifold = 0, unbound = 0, boundary = 0;
  for (const auto& f : uses) {
    if (f.second > 2)
      ++nonmanifold;
    else if (f.second == 1) {
      ++boundary;
      unbound += !bound[f.first];
    }
  }
  std::ostringstream out;
  out << "{\"cells\":" << s.mesh.size() << ",\"positive_cells\":" << positive
      << ",\"all_positive\":" << (positive == s.mesh.size() ? "true" : "false") << ",\"boundary_faces\":" << boundary
      << ",\"nonmanifold_faces\":" << nonmanifold << ",\"unbound_boundary_faces\":" << unbound
      << ",\"q_min\":" << D9Number(gates.q) << ",\"J_min\":" << D9Number(gates.J) << ",\"q_bad\":" << qBad
      << ",\"j_bad\":" << jBad << ",\"q_threshold\":" << D9Number(MinimumMeanRatio)
      << ",\"j_threshold\":" << D9Number(MinimumScaledJacobian) << ",\"size_long_edges\":" << gates.long_edges
      << ",\"size_edge_max\":" << D9Number(double(gates.lmax)) << ",\"height_failures\":" << gates.height_failures
      << ",\"complete\":" << (gates.complete ? "true" : "false") << "}";
  return out.str();
}
inline std::string D9RoomStats(const std::string& log) {
  std::vector<size_t> regions;
  std::istringstream in(log);
  std::string line;
  while (std::getline(in, line)) {
    const auto p = line.find("DC16 room ");
    if (p == std::string::npos) continue;
    const auto q = line.find("region=", p);
    if (q != std::string::npos) regions.push_back(std::stoull(line.substr(q + 7)));
  }
  std::ostringstream out;
  out << "{\"stats\":" << dc16::JsonRoom(dc16::roomStats) << ",\"logged_nonempty_regions\":" << regions.size()
      << ",\"region_sizes\":[";
  for (size_t i = 0; i < regions.size(); ++i) out << (i ? "," : "") << regions[i];
  out << "]}";
  return out.str();
}
inline std::string D9Report(const std::string& mode, const std::string& family, const BoxSpec& b, const R1Stage& r1,
                            const R2Stage& r2, const dc15::Completion& completion, const Growth& growth,
                            const Gates8& gates, State& s, const std::string& log, const Rows10& rows, double r1time,
                            double r2time, double completionTime, double growthTime,
                            const std::string& growthFailure) {
  const bool growthReturned = completion.failing_after == 0 && growthFailure.empty();
  const auto columns = growthReturned ? D9Columns(s, growth) : D9StackRows{"null", "null"};
  std::ostringstream out;
  out << std::setprecision(12) << "{\"schema\":\"D9_stack_v1\",\"mode\":" << junction_wp0::Json(mode)
      << ",\"family\":" << junction_wp0::Json(family) << ",\"n\":" << b.n
      << ",\"G\":1,\"top_wall\":" << (b.topWall ? "true" : "false")
      << ",\"symmetry_sides\":" << (b.symmetrySides ? "true" : "false") << ",\"thickness\":" << D9Number(b.thickness)
      << ",\"h0_bottom\":" << D9Number(b.h0) << ",\"h0_top\":" << (b.topWall ? D9Number(b.h0Top) : "null")
      << ",\"growth_bottom\":" << D9Number(b.g) << ",\"growth_top\":" << (b.topWall ? D9Number(b.gTop) : "null")
      << ",\"phase_limits\":{\"R1_work\":4096,\"R1_seconds\":60,\"R2_seconds\":120,\"completion_seconds\":60,"
         "\"completion_surface_tolerance\":0.0001,\"G_seconds\":120}"
      << ",\"timing_seconds\":{\"R1\":" << D9Number(r1time) << ",\"R2\":" << D9Number(r2time)
      << ",\"completion\":" << D9Number(completionTime) << ",\"G\":" << D9Number(growthTime) << "}"
      << ",\"phases\":{\"R1\":" << JsonStage(r1) << ",\"R2\":" << JsonStage(r2)
      << ",\"completion\":" << dc15::JsonCompletion(completion)
      << ",\"G_ran\":" << (completion.failing_after == 0 ? "true" : "false") << ",\"G_reason\":"
      << junction_wp0::Json(completion.failing_after == 0 ? "completion complete"
                                                          : "completion incomplete; production early stop")
      << ",\"G_returned\":" << (growthReturned ? "true" : "false")
      << ",\"G_exception\":" << junction_wp0::Json(growthFailure)
      << ",\"phase_log_file\":\"d9_stack_phase_log.txt\""
      << ",\"G\":" << (growthReturned ? JsonGrowth(s, growth) : "null")
      << "},\"R2_max\":{\"closure_vertices\":" << r2.max_closure
      << ",\"old_cells\":" << r2.max_old << ",\"fresh_cells\":" << r2.max_fresh << "}"
      << ",\"room\":" << D9RoomStats(log) << ",\"positive_validity_and_topology\":" << D9MeshStats(s, gates)
      << ",\"wall_first_child_heights\":" << D9WallFirstChildHeights(s)
      << ",\"first_height_measurement_scope\":\"vertex-lift spacings; separate from wall-face first-child altitude\""
      << ",\"first_height_measurements\":" << D9FirstHeights(s, rows) << ",\"columns_by_wall\":" << columns.by_wall
      << ",\"columns\":" << columns.columns << ",\"acceptance_claim\":false}\n";
  return out.str();
}
inline void D9ConfigureDirections(State& state, DirField& field) {
  dc9::ensureDirectionField = [&state, &field](State& current) {
    if (&current != &state) throw std::logic_error("D9 direction refresh changed private state.");
    if (current.publication_failed) throw std::runtime_error("D9 direction refresh follows failed publication.");
    if (current.direction_revision == current.boundary_revision) return;
    field = ComputeDirectionField(current);
    current.direction_revision = current.boundary_revision;
    dc9::directionField = &field.d;
  };
  dc9::ensureDirectionField(state);
  dc9::directionFieldUses = 0;
}
inline void D9Measure(const std::string& mode, const std::string& family, const BoxSpec& b, const std::string& output,
                      World& world) {
  Policies policies;
  DirField field;
  struct ReleaseDirections {
    ~ReleaseDirections() {
      dc9::ensureDirectionField = {};
      dc9::directionField = nullptr;
      dc9::directionFieldUses = 0;
    }
  } release;
  dc9::ensureDirectionField = {};
  artificialMarkers = {2};
  perf = Perf{};
  ResetCaches();
  cachesOn = true;
  balancedCache.clear();
  dc16::roomStats = dc16::RoomStats{};
  dc9::directionField = nullptr;
  dc9::directionFieldUses = 0;
  const auto roles = BoxRoles(b.symmetrySides);
  std::unique_ptr<State> s = SyntheticBox(b);
  REQUIRE(s->markers.size() == (b.symmetrySides ? 7 : 5));
  if (b.symmetrySides) {
    REQUIRE(roles.at(1) == 0);
    REQUIRE(roles.at(4) == 0);
    REQUIRE(roles.at(5) == 0);
    REQUIRE(roles.at(6) == 0);
  }
  ApplyWallPolicies(*s);
  DetectCreases(*s, roles);
  Rows10 rows;
  D9ConfigureDirections(*s, field);
  std::ostringstream log;
  const auto t0 = Clock::now();
  const auto r1 = PhaseR1(*s, rows, 4096, 60, log);
  const double r1time = Seconds(t0);
  const auto t1 = Clock::now();
  const auto r2 = PhaseR2(*s, static_cast<Rows&>(rows), 120, log);
  const double r2time = Seconds(t1);
  std::set<Face> certified;
  const auto t2 = Clock::now();
  const auto completion = dc15::CompleteFirstRow(*s, rows, world, 1e-4, 60, certified, log);
  const double completionTime = Seconds(t2);
  Growth growth;
  std::string growthFailure;
  double growthTime = 0;
  if (completion.failing_after == 0) {
    const auto t3 = Clock::now();
    try {
      growth = PhaseG(*s, rows, 120, log);
    } catch (const std::exception& error) {
      growthFailure = error.what();
      log << "D9 observed production growth stop: " << growthFailure << '\n';
    }
    growthTime = Seconds(t3);
  }
  const auto gates = Final(*s);
  const std::string report = D9Report(mode, family, b, r1, r2, completion, growth, gates, *s, log.str(), rows, r1time,
                                      r2time, completionTime, growthTime, growthFailure);
  std::ofstream phaseLog("d9_stack_phase_log.txt");
  phaseLog << log.str();
  phaseLog.close();
  REQUIRE(phaseLog.good());
  std::ofstream out(output);
  REQUIRE(out.good());
  out << report;
  out.close();
  REQUIRE(out.good());
  std::cout << "D9_STACK " << mode << " cells=" << s->mesh.size() << " completion=" << completion.outcome
            << " G=" << (completion.failing_after == 0) << "\n";
}
}  // namespace dc12

TEST_CASE("D9 serial stack measurement on synthetic boxes", "[.][NativeD9SerialStack]") {
  using namespace dc12;
  World world;
  REQUIRE(world.size == 1);
  const char* modeEnv = std::getenv("SU2_D9_STACK_MODE");
  REQUIRE(modeEnv);
  const std::string mode(modeEnv);
  const char* outputEnv = std::getenv("SU2_D9_STACK_JSON");
  REQUIRE(outputEnv);
  const std::string output(outputEnv);
  BoxSpec b;
  std::string family;
  if (mode == "M12_control" || mode == "M12_meet") {
    family = "M12";
    b.topWall = true;
    b.symmetrySides = true;
    b.thickness = mode == "M12_control" ? .4 : .6;
  } else if (mode == "M10_n1" || mode == "M10_n2" || mode == "M10_n4") {
    family = "M10";
    b.n = mode == "M10_n1" ? 1 : mode == "M10_n2" ? 2 : 4;
  } else
    FAIL("unsupported SU2_D9_STACK_MODE");
  REQUIRE(b.n > 0);
  REQUIRE(b.thickness > 0);
  D9Measure(mode, family, b, output, world);
}
