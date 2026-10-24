// BL_FIRST_ROW_DESIGN rev 2.1, DC-9: TEST-ONLY [NativeFirstRowDC9] harness, a copy of the DC-8 harness with the DC-9
// construction changes marked "DC-9" below: (a) W crease-strip spacing (no metric-short wall edge when another split
// point avoids it) and chain collapses/moves of metric-short wall edges along their feature chain (CollapseVertex /
// MoveVertex keep every incident reference facet, so a point only moves along its crease line; original corners are
// never collapsed or moved); (b) lazy R2 excess certificate (added excess first; the removed side is integrated only
// while it can still decide) and a coordinate-keyed composed-length memo; (c) sector lifts whose junction neighbour is
// already lifted (cut path through a link vertex above the lid plane, otherwise a Prepare-kind link split above the
// band); (d) ColumnSplit of a one-column wall/carrier junction edge. Limits and gates are unchanged.
// DC-8 text: extends DC-7 (W -> R1 -> R2 -> V) by
// the four routes DC-7 named as missing, as one connected step, with production primitives only:
//  (a) carrier sector lifts: a wall vertex whose carrier fan spans several canonical charts that one lift point cannot
//      serve gets one bound lift point per chart seam (BindNewPoint on both seam charts); the vertex star is split along
//      shortest link paths from each seam to its junction (gap tetrahedra), the corner column takes the rim triangles.
//      A corner cell owning a wall face and a foreign carrier face is first separated (InsertEdgeCavity, Prepare kind).
//  (b) structured crease strips in W: crease edges (convex D1 edges detected from the authority facets with the
//      production rule) are split first to the along-crease station target; every other long wall edge is split at its
//      crossing with a D1 level line rho_k (unit metric spacing of the production law h0 + ln(HGRAD) rho) when it
//      crosses one, otherwise at the metric midpoint, so wall edges end parallel to the crease, graded from h0;
//      metric-short wall edges (< 0.25) left by converging level points are then collapsed (CollapseVertex).
//  (c) bounded clearance route: band height down to the feasible LP window (1-T)h0(1+5%) before any closure growth;
//      R1 with the finite candidate set of §8.3 (Reconnect of each link edge, MoveVertex kernel direction + halvings,
//      CollapseVertex coarsening, Steiner at {2,3}H) and explicit limit labels. Caps unchanged (64/128).
//  (d) V: ApexMove and ColumnSplit for column demand, exact on-edge split points (exact OnEdge predicate, otherwise no
//      SplitEdge trial), no InsertEdgeCavity trial on chart/marker junctions, topology-keyed rejection cache (no
//      repeated identical rejected trials), balanced split point computed once per edge, wall-first edge order.
// Performance: per phase seconds, peak RSS, FrozenField counters; per-operation counts/seconds; cached cell shapes,
// edge lengths and first-height verdicts (cells and vertices are immutable by identity). Final gates are unchanged
// and absolute; the final mesh is re-audited with a fresh field and no caches. Nothing here is production code.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLFirstRow9.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
namespace dc9 {
// ---- loaders: DC-7 copies whose frozen field carries a metric identity (enables exact edge-length reuse) and timings
// DC-13 first-height ramp diagnostic: every wall marker's own first height becomes max(h0, h0Scale x h0) in the
// production wall policies before the target is composed (1 = unchanged, the default of every earlier harness). In a
// configuration, ADAP_BL_FIRST_HEIGHT (per marker) and ADAP_BL_PROTECT_DISTANCE (the BL budget requires it >= every
// first height; set to h0 in these configurations) are scaled.
double h0Scale=1;
double ScaledHeight(double h){return std::max(h,h0Scale*h);}
std::string ScaleBLConfig(const std::string& text) {
  if(h0Scale==1)return text;std::istringstream lines(text);std::ostringstream out;size_t replaced=0;
  for(std::string line;std::getline(lines,line);){
    if(line.rfind("ADAP_BL_FIRST_HEIGHT",0)==0){const auto l=line.find('('),r=line.rfind(')');if(l==std::string::npos||r==std::string::npos||r<l)throw std::runtime_error("DC13 ADAP_BL_FIRST_HEIGHT format");
      std::stringstream list(line.substr(l+1,r-l-1));std::ostringstream o;o<<std::setprecision(17)<<"ADAP_BL_FIRST_HEIGHT= (";bool first=true;
      for(std::string v;std::getline(list,v,',');){o<<(first?" ":", ")<<ScaledHeight(std::stod(v));first=false;}o<<" )";line=o.str();++replaced;}
    else if(line.rfind("ADAP_BL_PROTECT_DISTANCE",0)==0){const auto e=line.find('=');std::ostringstream o;o<<std::setprecision(17)<<"ADAP_BL_PROTECT_DISTANCE= "<<ScaledHeight(std::stod(line.substr(e+1)));line=o.str();}
    out<<line<<"\n";}
  if(replaced!=1)throw std::runtime_error("DC13 ramp: ADAP_BL_FIRST_HEIGHT not found once");
  return out.str();
}
std::unique_ptr<State> LoadA1(const std::string& wp0b) {
  const std::string annulus=wp0b+"/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1";
  ReferenceState rejected;if(!LoadReference(annulus+"_rejected_rank0.native_ref",rejected)||!rejected.original||!rejected.original->Mapped())throw std::runtime_error("DC7 annulus reference");
  std::ifstream cfgRaw(annulus+".cfg");if(!cfgRaw)throw std::runtime_error("DC7 annulus cfg");std::stringstream cfgText;cfgText<<cfgRaw.rdbuf();
  std::istringstream cfgFile(ScaleBLConfig(cfgText.str()));  // DC-13 ramp diagnostic (identity at h0Scale 1)
  std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
  std::ifstream in(wp0b+"/annulus_case.txt");std::string tag;int version;in>>tag>>version;if(tag!="SU2_JUNCTION_ANNULUS")throw std::runtime_error("DC7 annulus case");
  std::string key;double core=0;Id recordedCell=0;double recordedQ=0,recordedJ=0;in>>key>>core;in>>key>>recordedCell>>recordedQ>>recordedJ;
  size_t count=0;in>>key>>count;std::vector<DonorCell> donors;
  for(size_t i=0;i<count;++i){DonorCell d;int extension=0;in>>d.cell.id>>extension;for(unsigned k=0;k<4;++k){auto& n=d.cell.v[k];auto& m=d.sensor[k];in>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;}
    if(extension)d.extension_component=0;donors.push_back(d);}
  std::string name;size_t n;in>>name>>n;if(name!="input")throw std::runtime_error("DC7 annulus block");std::vector<OwnedCell> input;
  for(size_t i=0;i<n;++i){OwnedCell row;in>>row.cell.id;for(unsigned k=0;k<4;++k)in>>row.cell.v[k].id>>row.cell.v[k].p.x>>row.cell.v[k].p.y>>row.cell.v[k].p.z;
    for(unsigned k=0;k<4;++k){int present;in>>present>>row.physical[k].facet;row.physical[k].present=present;}input.push_back(row);}
  auto layers=CBoundaryLayerMetric::FromNativeReference(rejected,*config);
  if(config->GetAdap_BL_Budget()){layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));layers->FreezeOuterNormal();}
  auto authority=rejected.original->Parameters()->Authority();BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:rejected.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto x:f.v)points.emplace(x.id,BoundaryPoint{x,authority->Vertex(x.id)});}
  auto s=std::make_unique<State>();s->name="annulus_A1";
  s->reference=PlanarReference::WithParameters(rejected.original,std::make_shared<BoundaryParameterState>(authority,points,skin));
  s->facets=rejected.facets;s->markers=rejected.marker_names;s->mapped=true;s->core=core;
  for(unsigned short b=0;b<config->GetnAdap_BL();++b)for(size_t m=0;m<rejected.marker_names.size();++m)if(rejected.marker_names[m]==config->GetAdap_BL(b).marker) {
    s->walls.insert(int(m));s->heights[int(m)]=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);
    s->h0=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);s->growth=SU2_TYPE::GetValue(config->GetAdap_BL(b).growth);s->thickness=SU2_TYPE::GetValue(config->GetAdap_BL(b).thickness);}
  s->heights.tolerance=HeightTolerance;
  for(const auto& row:input)s->mesh.emplace(row.cell.id,row);
  s->layers=layers;s->field=std::make_unique<FrozenField>(donors,SU2NativeBL3D::Composition(layers,core),true,std::vector<Id>{0},SU2NativeBL3D::EdgeGeometry(layers,core),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);return s;
}
// Faceted SU2_JUNCTION_CASE fixture with the production D1 law (ADAP_HGRAD 1.3, default ADAP_ANGLE 45, partners are
// the non-wall facets, "symmetry" reflective) and the production composition.
std::unique_ptr<State> LoadTube(const std::string& path) {
  Id center=0;
  auto s=LoadCaseWith(path,center,[](std::shared_ptr<CBoundaryLayerMetric> layers,double core){return Compose(layers,core);});
  std::vector<CBoundaryLayerMetric::Wall> walls;CBoundaryLayerMetric::FeatureOptions options;options.hgrad=1.3;
  for(int m:s->walls){CBoundaryLayerMetric::Wall w;w.layer.marker=s->markers.at(m);w.layer.firstHeight=s->h0;w.layer.growth=s->growth;w.layer.thickness=s->thickness;
    std::map<Id,unsigned long> index;
    for(const auto& f:s->facets)if(f.marker==m){for(auto x:f.v){const auto ins=index.emplace(x.id,index.size());if(ins.second){w.coord.push_back(x.p.x);w.coord.push_back(x.p.y);w.coord.push_back(x.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
    walls.push_back(std::move(w));}
  for(const auto& f:s->facets)if(!s->walls.count(f.marker)){CBoundaryLayerMetric::FeatureFace p;p.reflective=s->markers.at(f.marker).find("sym")!=std::string::npos;
    for(auto x:f.v){p.coord.push_back(x.p.x);p.coord.push_back(x.p.y);p.coord.push_back(x.p.z);}options.faces.push_back(std::move(p));}
  s->layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),45.0,std::move(options));
  // same six covering uniform-sensor donors as LoadCaseWith
  Point lo=s->mesh.begin()->second.cell.v[0].p,hi=lo;for(const auto& r:s->mesh)for(auto x:r.second.cell.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
  const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
  const Tensor sensor{s->core,0,0,s->core,0,s->core};std::vector<DonorCell> donors;
  auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
  const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
  for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
    if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),true,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core),std::make_shared<const FrozenMetricIdentity>());
  s->heights.tolerance=HeightTolerance;s->lengths.clear();dc7::MakeMapped(*s);return s;
}
// M6 neighbourhood (dc7_m6_prepare.py): fresh adopted Y=0 faceted authority of the cut, real selected sensor on the
// cut's own original cells (<= 256 donors), production layers from the saved-state reference/config (ADAP_ANGLE 60).
struct M6Field { std::shared_ptr<CBoundaryLayerMetric> layers;double h0=0,growth=0,thickness=0;std::string wall; };
std::map<std::string,M6Field> m6cache;
std::unique_ptr<State> LoadM6Cut(const std::string& path) {
  std::ifstream in(path);if(!in)throw std::runtime_error("DC7 missing "+path);std::string tag;int version=0;in>>tag>>version;
  if(tag!="SU2_DC7_M6"||version!=1)throw std::runtime_error("DC7 bad M6 cut header");
  auto s=std::make_unique<State>();std::string cfg,ref;double parameter=-1;std::map<Id,std::pair<Point,Tensor>> nodes;std::vector<std::pair<Id,std::array<Id,4>>> cells;
  std::vector<std::tuple<Id,int,std::array<Id,3>>> facets;std::vector<std::pair<std::array<Id,3>,std::pair<Point,Tensor>>> collar;Id centre=UINT64_MAX;
  for(std::string key;in>>key;) {
    if(key=="end")break;
    if(key=="centre")in>>centre;else if(key=="extension"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,3> f;Point p;Tensor t;in>>id>>f[0]>>f[1]>>f[2]>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;collar.push_back({f,{p,t}});}}
    else if(key=="name")in>>s->name;else if(key=="cfg")in>>cfg;else if(key=="ref")in>>ref;else if(key=="parameter")in>>parameter;else if(key=="core")in>>s->core;
    else if(key=="markers"){size_t n;in>>n;s->markers.resize(n);for(auto& m:s->markers)in>>m;}
    else if(key=="walls"){size_t n;in>>n;for(size_t k=0;k<n;++k){int m;in>>m;s->walls.insert(m);}}
    else if(key=="nodes"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;Point p;Tensor t;in>>id>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;nodes[id]={p,t};}}
    else if(key=="cells"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,4> v;in>>id>>v[0]>>v[1]>>v[2]>>v[3];cells.push_back({id,v});}}
    else if(key=="facets"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;int m;std::array<Id,3> v;in>>id>>m>>v[0]>>v[1]>>v[2];facets.emplace_back(id,m,v);}}
    else throw std::runtime_error("DC7 unknown M6 key "+key);
    if(!in)throw std::runtime_error("DC7 truncated M6 cut");
  }
  auto& field=m6cache[cfg+"|"+ref+"|"+std::to_string(parameter)];
  if(!field.layers) {
    std::ifstream cfgFile(cfg);if(!cfgFile)throw std::runtime_error("DC7 M6 cfg");std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
    ReferenceState reference;if(!LoadReference(ref,reference)||!reference.original)throw std::runtime_error("DC7 M6 reference");
    field.layers=CBoundaryLayerMetric::FromNativeReference(reference,*config);
    field.layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));
    field.layers->SelectOuterNormal(parameter);field.layers->FreezeOuterNormal();
    field.h0=SU2_TYPE::GetValue(config->GetAdap_BL(0).firstHeight);field.growth=SU2_TYPE::GetValue(config->GetAdap_BL(0).growth);
    field.thickness=SU2_TYPE::GetValue(config->GetAdap_BL(0).thickness);field.wall=config->GetAdap_BL(0).marker;
  }
  s->h0=field.h0;s->growth=field.growth;s->thickness=field.thickness;s->layers=field.layers;
  std::vector<Cell> mesh;std::vector<DonorCell> donors;
  for(const auto& c:cells){Cell cell{c.first,{}};for(unsigned k=0;k<4;++k)cell.v[k]={c.second[k],nodes.at(c.second[k]).first};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);mesh.push_back(cell);}
  {// the real selected sensor of the cut centre node, frozen uniform on six covering donors (the cut exceeds the
   // 256-donor FrozenField bound; within a cut of a few cm the P1 sensor variation is neglected and reported)
   Point lo=mesh.front().v[0].p,hi=lo;for(const auto& c:mesh)for(auto x:c.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
   const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
   const Tensor sensor=nodes.at(centre).second;
   auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
   const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
   for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
     if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
  std::map<Face,Face> skin;for(const auto& c:mesh)for(auto f:CellFaces(c))skin[Sorted(f)]=f;
  std::map<Face,int> uses;for(const auto& c:mesh)for(auto f:CellFaces(c))++uses[Sorted(f)];
  std::map<Face,Id> physical;
  for(const auto& f:facets){const auto key=Sorted(std::get<2>(f));if(uses.at(key)!=1)throw std::runtime_error("DC7 M6 facet is not a skin face.");
    const auto oriented=skin.at(key);Facet facet;facet.id=std::get<0>(f);facet.marker=std::get<1>(f);
    for(unsigned k=0;k<3;++k)facet.v[k]={oriented[k],nodes.at(oriented[k]).first};s->facets.push_back(facet);physical[key]=facet.id;}
  s->reference=std::make_shared<PlanarReference>(s->facets);
  for(const auto& c:mesh){OwnedCell row;row.cell=c;const auto f=CellFaces(c);for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}s->mesh.emplace(c.id,row);}
  for(int m:s->walls)s->heights[m]=s->h0;s->heights.tolerance=HeightTolerance;
  if(!collar.empty())throw std::runtime_error("DC7 collar donors are not used with the covering sensor");
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),true,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);dc7::MakeMapped(*s);return s;
}
}  // namespace dc9

TEST_CASE("DC-9 W(crease strips, chain pass) -> R1 -> R2(sector order, lazy certificate) -> V(ApexMove, ColumnSplit incl. junction)", "[.][NativeFirstRowDC9]") {
  dc9::sectorFirst=std::getenv("SU2_DC9_SECTOR_FIRST")!=nullptr;struct SectorReset{~SectorReset(){dc9::sectorFirst=false;}} sectorReset;  // DC-14: env -> switch
  // Evidence of this harness was produced under the former production default: keep StrictExcess pinned.
  struct PinStrictExcess{SU2Native3D::RefineAcceptance a=SU2Native3D::GetRefineAcceptance();PinStrictExcess(){SU2Native3D::SetRefineAcceptance(SU2Native3D::RefineAcceptance::StrictExcess);}~PinStrictExcess(){SU2Native3D::SetRefineAcceptance(a);}} pinStrictExcess;
  using namespace dc9;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC9_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC9_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC9_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC9_CASES");REQUIRE(only);
  const char* binary=std::getenv("SU2_DC9_BINARY_SHA");const std::string sha=binary?binary:"unknown";
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    // V limits: 8192 work / 120 s (reference) unless SU2_DC9_V_WORK / SU2_DC9_V_SECONDS are set (separate campaign).
    const size_t vWork=std::getenv("SU2_DC9_V_WORK")?std::stoul(std::getenv("SU2_DC9_V_WORK")):8192;
    const double vSeconds=std::getenv("SU2_DC9_V_SECONDS")?std::stod(std::getenv("SU2_DC9_V_SECONDS")):120;
    const std::string stopAfter=std::getenv("SU2_DC9_STOP_AFTER")?std::getenv("SU2_DC9_STOP_AFTER"):"";  // diagnostics only
    std::ostringstream limits;limits<<std::setprecision(12)<<"\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"V\":["<<vWork<<","<<vSeconds<<"],\"sector_first\":"<<bool(std::getenv("SU2_DC9_SECTOR_FIRST"))<<"}";
    // resume only a complete full-campaign result of the same binary and the same limits/options; otherwise recompute
    if(stopAfter.empty()){std::ifstream old(root+"/dc9_"+name+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find(limits.str())!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos)continue;}
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;geometryHits=0;
    const auto start=Clock::now();std::ostringstream json,log;
    json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"binary_sha256\":\""<<sha<<"\","<<limits.str();
    auto load=[&]()->std::unique_ptr<State>{
      if(name=="annulus")return LoadA1(fixtures);
      if(name.rfind("m6_",0)==0)return LoadM6Cut(cuts+"/"+name+".txt");
      return LoadTube(fixtures+"/"+name+".txt");};
    std::unique_ptr<State> s;{Scope sc("Load");s=load();}Rows rows;DetectCreases(*s);
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped
        <<",\"d1\":{\"segments\":"<<s->layers->GetFeatures().segments<<",\"images\":"<<s->layers->GetFeatures().images<<",\"corners\":"<<s->layers->GetFeatures().corners
        <<",\"angle\":"<<s->layers->GetFeatures().angle<<",\"slope\":"<<s->layers->GetFeatures().slope<<",\"R_f\":"<<s->layers->GetFeatures().maximumRadius
        <<",\"creases_in_cut_test_side\":"<<d1.creases.size()<<"}}";
    std::cout<<"DC9 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<" creases="<<d1.creases.size()<<std::endl;
    const auto w=PhaseW(*s,8192,120,log);
    json<<",\"W\":"<<JsonStage(w)<<",\"W_counts\":{\"stations\":"<<w.stations<<",\"level_splits\":"<<w.levels<<",\"bisections\":"<<w.bisections<<",\"short_edge_collapses\":"<<w.collapses<<",\"unmeasurable\":"<<w.unmeasurable
        <<",\"spacing_alternatives\":"<<w.spacing_alternatives<<",\"spacing_fallbacks\":"<<w.spacing_fallbacks<<",\"chain_collapses\":"<<w.chain_collapses<<",\"chain_moves\":"<<w.chain_moves
        <<",\"short_wall_edges_after\":"<<w.short_after<<",\"short_census\":{";
    {bool k=false;for(const auto& c:w.short_census){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}"
        <<",\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    std::cout<<"DC9 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<" stations="<<w.stations<<" levels="<<w.levels<<" bisections="<<w.bisections<<" collapses="<<w.collapses
             <<" chain_collapses="<<w.chain_collapses<<" chain_moves="<<w.chain_moves<<" short_after="<<w.short_after<<" t="<<w.seconds<<std::endl;
    if(stopAfter=="W"){std::ofstream(root+"/dc9_"+name+"_W.json")<<json.str()<<"}\n";std::ofstream(root+"/dc9_"+name+"_W.log")<<log.str();continue;}
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"collapses\":"<<r1.collapses<<",\"steiner\":"<<r1.steiner<<",\"exhausted_vertices\":"<<r1.exhausted<<'}';
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC9 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<" t="<<r1.seconds<<std::endl;
    const auto r2=PhaseR2(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"low_band_transactions\":"<<r2.low_band<<",\"sector_transactions\":"<<r2.sector_tx<<",\"gap_cells\":"<<r2.gaps<<",\"corner_separations\":"<<r2.separations
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC9 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" t="<<r2.seconds<<" last="<<r2.last.substr(0,300)<<std::endl;
    if(stopAfter=="R2"){std::ofstream(root+"/dc9_"+name+"_R2.json")<<json.str()<<"}\n";std::ofstream(root+"/dc9_"+name+"_R2.log")<<log.str();continue;}
    {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
    const auto v=PhaseV(*s,rows,vWork,vSeconds,log);
    json<<",\"V\":"<<JsonStage(v)<<",\"V_counts\":{\"sweeps\":"<<v.sweeps<<",\"column_demand_unrouted\":"<<v.column_demand<<",\"apex_moves\":"<<v.apex_moves<<",\"column_splits\":"<<v.column_splits<<",\"junction_column_splits\":"<<v.junction_splits
        <<",\"rejection_cache_skips\":"<<v.cache_skips<<",\"exact_on_edge_splits\":"<<v.exact_splits<<",\"no_exact_on_edge_point\":"<<v.no_exact_point<<",\"junction_insert_skips\":"<<v.junction_skips
        <<",\"with_complete_rows\":"<<(r2.unrowed==0)<<",\"demand\":{";
    {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
    {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
    ExportGates(*s,rows,root+"/dc9_"+name+"_final.mesh");
    const auto before=Seconds(start);
    {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
         <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
     json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';}
    json<<",\"perf\":"<<JsonPerf()<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(root+"/dc9_"+name+".json")<<json.str();std::ofstream(root+"/dc9_"+name+".log")<<log.str();
    std::cout<<"DC9 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}
