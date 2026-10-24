// DC-10: TEST-ONLY [NativeFirstRowDC10] harness (CREASE_FAN_PLAN). It reuses the DC-9 harness (namespace dc9: caches,
// lazy excess certificate, coordinate length memo, ApexLP/ConeColumn, R1, ApexMove, ColumnSplit incl. the junction
// variant, final gates and export) and adds, as one connected step:
//  (A) the option-A target (user decision 2026-10-23): the D1 crease law is kept off the wall. The composed target is
//      the production BL composition without D1 (no detected feature) blended in logarithm towards the production composition
//      with D1 by a weight w(x) that is 1 inside the convex fluid wedge of a crease (the directions whose closest wall
//      point is the crease) and 0 on and above the adjacent wall faces (smoothstep in the angle from the wedge's
//      boundary planes, radial fade at the crease line). One definition for W, R, V, the final gates and the fresh
//      re-audit. Thin strips between two parallel convex creases bounding one plane face (a blunt TE base) get >= 4
//      wall cells across in W (a construction rule with its own census, not a metric term). Geometric only: creases are detected from the
//      authority facets with the production rule (ADAP_ANGLE, convexity), as in dc9::DetectCreases.
//  (W) plain wall resolve to that target (metric bisection of long wall edges); no crease stations, level lines or
//      needle strips.
//  (R) the first row with per-side lifts and an explicit crease fan (SU2_DC10_LIFT=fan), or the DC-9 single-direction
//      lift (SU2_DC10_LIFT=single, the diagnostic of CREASE_FAN_PLAN step 1).
// Final gates are unchanged and absolute. Nothing here is production code.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLOptionA10.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
namespace dc10 {
// ---- loaders with the option-A field (A1 has no crease: the dc9 loader is the same target) --------------------------
std::unique_ptr<State> LoadTubeA(const std::string& path) {
  Id center=0;loadedArtificial.clear();
  auto s=LoadCaseWith(path,center,[](std::shared_ptr<CBoundaryLayerMetric> layers,double core){return Compose(layers,core);});
  s->h0=ScaledHeight(s->h0);  // DC-13 ramp diagnostic (one policy per junction fixture: every wall marker's own h0)
  // Fixture metadata: the junction-case format declares a reflective carrier by its marker name ("symmetry"); this one
  // loader-side reading defines both the production layers' reflective partners and the option-A marker roles.
  // DC-12: an explicit reflective marker list (case configuration, as MARKER_SYM) replaces the name test when given.
  std::map<int,int> roles;for(size_t m=0;m<s->markers.size();++m)roles[int(m)]=s->walls.count(int(m))?1:
    (reflectiveByRole?reflectiveMarkers.count(s->markers[m])>0:s->markers[m].find("sym")!=std::string::npos)?0:2;
  auto make=[&](double hgrad){std::vector<CBoundaryLayerMetric::Wall> walls;CBoundaryLayerMetric::FeatureOptions options;options.hgrad=hgrad;
    for(int m:s->walls){CBoundaryLayerMetric::Wall w;w.layer.marker=s->markers.at(m);w.layer.firstHeight=s->h0;w.layer.growth=s->growth;w.layer.thickness=s->thickness;
      std::map<Id,unsigned long> index;
      for(const auto& f:s->facets)if(f.marker==m){for(auto x:f.v){const auto ins=index.emplace(x.id,index.size());if(ins.second){w.coord.push_back(x.p.x);w.coord.push_back(x.p.y);w.coord.push_back(x.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
      walls.push_back(std::move(w));}
    for(const auto& f:s->facets)if(!s->walls.count(f.marker)){CBoundaryLayerMetric::FeatureFace p;p.reflective=roles.at(f.marker)==0;
      for(auto x:f.v){p.coord.push_back(x.p.x);p.coord.push_back(x.p.y);p.coord.push_back(x.p.z);}options.faces.push_back(std::move(p));}
    return std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),45.0,std::move(options));};
  s->layers=make(1.3);auto base=make(0);probeBase=base;probeFull=s->layers;
  optionA=BuildOptionA(s->facets,roles,s->layers->GetFeatures().angle,s->layers->GetFeatures().maximumRadius,s->h0);optionA->distanceLayers=base;optionA->core=s->core;
  Point lo=s->mesh.begin()->second.cell.v[0].p,hi=lo;for(const auto& r:s->mesh)for(auto x:r.second.cell.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
  const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
  const Tensor sensor{s->core,0,0,s->core,0,s->core};std::vector<DonorCell> donors;
  auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
  const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
  for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
    if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}
  s->field=std::make_unique<FrozenField>(std::move(donors),CompositionA(base,s->layers,s->core,optionA),true,std::vector<Id>{},EdgeGeometryA(base,s->layers,s->core,optionA),std::make_shared<const FrozenMetricIdentity>());
  s->heights.tolerance=HeightTolerance;s->lengths.clear();dc7::MakeMapped(*s);return s;
}
size_t m6Snapped=0;
struct M6FieldA { std::shared_ptr<CBoundaryLayerMetric> layers,base;double h0=0,growth=0,thickness=0;std::map<std::string,int> kinds;std::set<std::string> walls; };
std::map<std::string,M6FieldA> m6cacheA;
std::unique_ptr<State> LoadM6CutA(const std::string& path) {
  std::ifstream in(path);if(!in)throw std::runtime_error("DC10 missing "+path);std::string tag;int version=0;in>>tag>>version;
  if(tag!="SU2_DC7_M6"||version!=1)throw std::runtime_error("DC10 bad M6 cut header");
  auto s=std::make_unique<State>();std::string cfg,ref;double parameter=-1;std::map<Id,std::pair<Point,Tensor>> nodes;std::vector<std::pair<Id,std::array<Id,4>>> cells;
  std::vector<std::tuple<Id,int,std::array<Id,3>>> facets;size_t collar=0;Id centre=UINT64_MAX;
  for(std::string key;in>>key;) {
    if(key=="end")break;
    if(key=="centre")in>>centre;else if(key=="extension"){size_t n;in>>n;collar=n;for(size_t k=0;k<n;++k){Id id;std::array<Id,3> f;Point p;Tensor t;in>>id>>f[0]>>f[1]>>f[2]>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;}}
    else if(key=="name")in>>s->name;else if(key=="cfg")in>>cfg;else if(key=="ref")in>>ref;else if(key=="parameter")in>>parameter;else if(key=="core")in>>s->core;
    else if(key=="markers"){size_t n;in>>n;s->markers.resize(n);for(auto& m:s->markers)in>>m;}
    else if(key=="walls"){size_t n;in>>n;for(size_t k=0;k<n;++k){int m;in>>m;s->walls.insert(m);}}
    else if(key=="nodes"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;Point p;Tensor t;in>>id>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;nodes[id]={p,t};}}
    else if(key=="cells"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,4> v;in>>id>>v[0]>>v[1]>>v[2]>>v[3];cells.push_back({id,v});}}
    else if(key=="facets"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;int m;std::array<Id,3> v;in>>id>>m>>v[0]>>v[1]>>v[2];facets.emplace_back(id,m,v);}}
    else throw std::runtime_error("DC10 unknown M6 key "+key);
    if(!in)throw std::runtime_error("DC10 truncated M6 cut");
  }
  if(collar)throw std::runtime_error("DC10 collar donors are not used with the covering sensor");
  auto& field=m6cacheA[cfg+"|"+ref+"|"+std::to_string(parameter)+"|h0x"+std::to_string(h0Scale)];
  if(!field.layers) {
    std::ifstream cfgFile(cfg);if(!cfgFile)throw std::runtime_error("DC10 M6 cfg");std::stringstream text;text<<cfgFile.rdbuf();
    ReferenceState reference;if(!LoadReference(ref,reference)||!reference.original)throw std::runtime_error("DC10 M6 reference");
    // The cut was adopted with |y| <= 1e-10 snapped to the Y = 0 plane (dc7_m6_prepare.py); the BL walls are built from
    // the same adopted coordinates, so wall points of the cut lie exactly on the BL wall (otherwise the off-wall
    // radial normal of a 1e-10 offset is rounding noise, which D1 used to mask on the root line).
    for(auto& f:reference.facets)for(auto& x:f.v)if(x.p.y!=0&&std::abs(x.p.y)<=1e-10){x.p.y=0;++m6Snapped;}
    for(int pass=0;pass<2;++pass) {
      std::string cfgText=text.str();
      cfgText=ScaleBLConfig(cfgText);  // DC-13 ramp diagnostic (dc9::h0Scale, identity at 1)
      if(pass==1){std::istringstream lines(cfgText);std::ostringstream out;size_t replaced=0;
        // ADAP_HGRAD must stay > 1 (config check); ADAP_ANGLE is read only by the D1 detection (BuildFeatures), so a
        // feature angle no wall edge can exceed disables D1 and leaves every other BL ingredient unchanged
        for(std::string line;std::getline(lines,line);){if(line.rfind("ADAP_ANGLE",0)==0){line="ADAP_ANGLE= 179.999";++replaced;}out<<line<<"\n";}
        if(replaced!=1)throw std::runtime_error("DC10 M6 cfg: ADAP_ANGLE not found once");cfgText=out.str();}
      std::istringstream buffer(cfgText);std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(buffer,SU2_COMPONENT::SU2_CFD,false);}
      auto layers=CBoundaryLayerMetric::FromNativeReference(reference,*config);
      layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));
      layers->SelectOuterNormal(parameter);layers->FreezeOuterNormal();
      (pass?field.base:field.layers)=layers;
      if(!pass){for(unsigned short m=0;m<config->GetnMarker_CfgFile();++m){const auto name=config->GetMarker_CfgFile_TagBound(m);field.kinds[name]=config->GetMarker_CfgFile_KindBC(name);}
        for(unsigned short b=0;b<config->GetnAdap_BL();++b)field.walls.insert(config->GetAdap_BL(b).marker);
        field.h0=SU2_TYPE::GetValue(config->GetAdap_BL(0).firstHeight);field.growth=SU2_TYPE::GetValue(config->GetAdap_BL(0).growth);field.thickness=SU2_TYPE::GetValue(config->GetAdap_BL(0).thickness);}
    }
    if(field.base->GetFeatures().segments!=0)throw std::runtime_error("DC10 M6 base layers still carry D1");
  }
  s->h0=field.h0;s->growth=field.growth;s->thickness=field.thickness;s->layers=field.layers;probeBase=field.base;probeFull=field.layers;
  std::vector<Cell> mesh;std::vector<DonorCell> donors;
  for(const auto& c:cells){Cell cell{c.first,{}};for(unsigned k=0;k<4;++k)cell.v[k]={c.second[k],nodes.at(c.second[k]).first};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);mesh.push_back(cell);}
  {Point lo=mesh.front().v[0].p,hi=lo;for(const auto& c:mesh)for(auto x:c.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
   const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
   const Tensor sensor=nodes.at(centre).second;
   auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
   const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
   for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
     if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
  std::map<Face,Face> skin;for(const auto& c:mesh)for(auto f:CellFaces(c))skin[Sorted(f)]=f;
  std::map<Face,int> uses;for(const auto& c:mesh)for(auto f:CellFaces(c))++uses[Sorted(f)];
  std::map<Face,Id> physical;
  for(const auto& f:facets){const auto key=Sorted(std::get<2>(f));if(uses.at(key)!=1)throw std::runtime_error("DC10 M6 facet is not a skin face.");
    const auto oriented=skin.at(key);Facet facet;facet.id=std::get<0>(f);facet.marker=std::get<1>(f);
    for(unsigned k=0;k<3;++k)facet.v[k]={oriented[k],nodes.at(oriented[k]).first};s->facets.push_back(facet);physical[key]=facet.id;}
  s->reference=std::make_shared<PlanarReference>(s->facets);
  for(const auto& c:mesh){OwnedCell row;row.cell=c;const auto f=CellFaces(c);for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}s->mesh.emplace(c.id,row);}
  for(int m:s->walls)s->heights[m]=s->h0;s->heights.tolerance=HeightTolerance;
  std::map<int,int> roles;loadedArtificial.clear();
  for(size_t m=0;m<s->markers.size();++m){const auto& name=s->markers[m];const auto k=field.kinds.find(name);
    if(k==field.kinds.end()&&!s->walls.count(int(m)))loadedArtificial.insert(int(m));
    if(s->walls.count(int(m))!=field.walls.count(name))throw std::runtime_error("DC10 M6 wall markers differ from the configuration");
    roles[int(m)]=s->walls.count(int(m))?1:k==field.kinds.end()?0:k->second==SYMMETRY_PLANE?0:2;
    // the inherited rim accounting (dc9::Rim) names the artificial cut boundary "CUT": the role decides the name
    if(k==field.kinds.end()&&!s->walls.count(int(m)))s->markers[m]="CUT";else if(name=="CUT")throw std::runtime_error("DC10 configured marker named CUT");}
  optionA=BuildOptionA(s->facets,roles,s->layers->GetFeatures().angle,s->layers->GetFeatures().maximumRadius,s->h0);optionA->distanceLayers=field.base;optionA->core=s->core;
  s->field=std::make_unique<FrozenField>(std::move(donors),CompositionA(field.base,field.layers,s->core,optionA),true,std::vector<Id>{},EdgeGeometryA(field.base,field.layers,s->core,optionA),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);dc7::MakeMapped(*s);return s;
}
}  // namespace dc10

TEST_CASE("DC-10 option-A wall target, W -> R1 -> R2(single | per-side + crease fan) -> V", "[.][NativeFirstRowDC10]") {
  // Evidence of this harness was produced under the former production default: keep StrictExcess pinned.
  struct PinStrictExcess{SU2Native3D::RefineAcceptance a=SU2Native3D::GetRefineAcceptance();PinStrictExcess(){SU2Native3D::SetRefineAcceptance(SU2Native3D::RefineAcceptance::StrictExcess);}~PinStrictExcess(){SU2Native3D::SetRefineAcceptance(a);}} pinStrictExcess;
  using namespace dc10;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC10_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC10_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC10_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC10_CASES");REQUIRE(only);
  const char* binary=std::getenv("SU2_DC10_BINARY_SHA");const std::string sha=binary?binary:"unknown";
  const std::string lift=std::getenv("SU2_DC10_LIFT")?std::getenv("SU2_DC10_LIFT"):"fan";
  REQUIRE((lift=="single"||lift=="fan"));
  unsetenv("SU2_DC9_SECTOR_FIRST");  // inherited DC-9 ordering option is not part of DC-10 (not in the resume key)
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    const size_t vWork=std::getenv("SU2_DC10_V_WORK")?std::stoul(std::getenv("SU2_DC10_V_WORK")):8192;
    const double vSeconds=std::getenv("SU2_DC10_V_SECONDS")?std::stod(std::getenv("SU2_DC10_V_SECONDS")):120;
    const std::string stopAfter=std::getenv("SU2_DC10_STOP_AFTER")?std::getenv("SU2_DC10_STOP_AFTER"):"";  // diagnostics only
    const std::string inputSha=std::getenv("SU2_DC10_INPUT_SHA")?std::getenv("SU2_DC10_INPUT_SHA"):"unknown";  // runtime inputs (run_case.sh)
    std::ostringstream limits;limits<<std::setprecision(12)<<"\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"V\":["<<vWork<<","<<vSeconds<<"],\"lift\":\""<<lift<<"\",\"inputs_sha256\":\""<<inputSha<<"\"}";
    const std::string tag=root+"/dc10_"+name;
    if(stopAfter.empty()){std::ifstream old(tag+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&inputSha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos
        &&saved.find("\"agrees_with_cached_final\":1")!=std::string::npos&&saved.size()>2&&saved.substr(saved.size()-2)=="}\n")continue;}
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
    std::cout<<"DC10 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<" creases="<<optionA->creases.size()<<" strips="<<optionA->strips.size()<<std::endl;
    if(const char* probe=std::getenv("SU2_DC10_PROBE")){  // diagnostics: pointwise option-A tensor along a segment
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
    std::cout<<"DC10 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<" splits="<<w.splits<<" t="<<w.seconds<<std::endl;
    if(stopAfter=="W"){std::ofstream(tag+"_W.json")<<json.str()<<"}\n";std::ofstream(tag+"_W.log")<<log.str();continue;}
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"collapses\":"<<r1.collapses<<",\"steiner\":"<<r1.steiner<<",\"exhausted_vertices\":"<<r1.exhausted<<'}';
    if(lift=="fan"){const auto rw=PhaseR1w(*s,rows,4096,120,log);json<<",\"R1w\":"<<JsonStage(rw)<<",\"R1w_counts\":{\"ridges\":"<<rw.ridges<<",\"inserted\":"<<rw.inserted<<",\"present\":"<<rw.present<<'}';
      std::cout<<"DC10 "<<name<<" R1w ridges="<<rw.ridges<<" inserted="<<rw.inserted<<" present="<<rw.present<<" t="<<rw.seconds<<std::endl;}
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC10 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<" t="<<r1.seconds<<std::endl;
    R2Stage10 r2;if(lift=="single")static_cast<R2Stage&>(r2)=PhaseR2(*s,static_cast<Rows&>(rows),600,log);else r2=PhaseR2_10(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"low_band_transactions\":"<<r2.low_band<<",\"sector_transactions\":"<<r2.sector_tx<<",\"gap_cells\":"<<r2.gaps<<",\"corner_separations\":"<<r2.separations
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"ridge_lifts\":"<<r2.ridges<<",\"fan_cells_built\":"<<r2.fan_cells
        <<",\"fan_cells_now\":"<<FanCells(*s,rows).size()<<",\"ridge_gap_cells\":"<<r2.gap_cells<<",\"fan_check_failures\":{";
    {bool k=false;for(const auto& c:r2.fanChecks){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"},\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC10 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" t="<<r2.seconds<<" last="<<r2.last.substr(0,300)<<std::endl;
    if(stopAfter=="R2"){std::ofstream(tag+"_R2.json")<<json.str()<<"}\n";std::ofstream(tag+"_R2.log")<<log.str();continue;}
    {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
    const auto v=lift=="single"?PhaseV(*s,static_cast<Rows&>(rows),vWork,vSeconds,log):PhaseV10(*s,rows,vWork,vSeconds,log);
    json<<",\"V\":"<<JsonStage(v)<<",\"V_counts\":{\"sweeps\":"<<v.sweeps<<",\"column_demand_unrouted\":"<<v.column_demand<<",\"apex_moves\":"<<v.apex_moves<<",\"column_splits\":"<<v.column_splits<<",\"junction_column_splits\":"<<v.junction_splits
        <<",\"rejection_cache_skips\":"<<v.cache_skips<<",\"exact_on_edge_splits\":"<<v.exact_splits<<",\"no_exact_on_edge_point\":"<<v.no_exact_point<<",\"junction_insert_skips\":"<<v.junction_skips
        <<",\"with_complete_rows\":"<<(r2.unrowed==0)<<",\"demand\":{";
    {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
    {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
    {size_t fanBad=0;const auto fan=FanCells(*s,rows);for(Id id:fan)fanBad+=Bad(MeshShape(*s,id));json<<",\"final_fan\":{\"cells\":"<<fan.size()<<",\"bad\":"<<fanBad<<"}";}
    ExportGates10(*s,rows,tag+"_final.mesh");
    const auto before=Seconds(start);
    const auto distanceEvaluations=optionA->distanceEvaluations;
    {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;m6cacheA.clear();auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
         <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
     json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';}
    json<<",\"perf\":"<<JsonPerf()<<",\"wall_distance_evaluations\":"<<distanceEvaluations<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(tag+".json")<<json.str();std::ofstream(tag+".log")<<log.str();
    std::cout<<"DC10 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}
