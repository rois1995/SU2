// BL_REMESH_PLAN rev 3, DC-5: TEST-ONLY [NativeLayer2DWay3D] harness. Nothing here is production code or promoted.
// Existing operators + D.1 admission + LayerConstruct + a D.3 coordinated first row, run twice per case on the SAME
// deterministic proposal stream: admission A (current absolute shape on every fresh cell) and admission D (D.1:
// first children absolute, LayerConstruct other fresh cells positivity only, other kinds not-worse-than-before).
// Both admissions use the decided T = 0.10 first-height tolerance test-side (CheckFirstHeight report), so the
// comparison isolates D.1. Final gates are unchanged and absolute; the final mesh is re-audited with a fresh field.
// Reuses junction_wp0 (fixtures/native_junction_replay.inc) for state, loading, visibility and patches.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLLayer.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
namespace layer2d {
std::shared_ptr<const D1Law> LoadD1(const std::string& path) {
  std::ifstream in(path);if(!in)return nullptr;std::string tag;in>>tag;if(tag!="SU2_D1_FEATURES")throw std::runtime_error("DC5 bad D1 file");
  auto law=std::make_shared<D1Law>();size_t n=0;in>>law->h0>>law->g>>law->T>>law->hgrad>>law->Rf>>n;
  for(size_t i=0;i<n;++i){D1Law::Seg s;in>>s.a.x>>s.a.y>>s.a.z>>s.b.x>>s.b.y>>s.b.z>>s.marker;law->segments.push_back(s);}
  in>>n;for(size_t i=0;i<n;++i){D1Law::Corner c;in>>c.c.x>>c.c.y>>c.c.z>>c.marker;law->corners.push_back(c);}
  if(!in)throw std::runtime_error("DC5 truncated D1 file");return law;
}
// ---- fixture loader with an injected composition (copy of junction_wp0::LoadCase; donors are private) ----------
std::unique_ptr<State> LoadCaseWith(const std::string& path,Id& center,const std::function<MetricComposition(std::shared_ptr<CBoundaryLayerMetric>,double)>& compose) {
  std::ifstream in(path);if(!in)throw std::runtime_error("WP0B missing case "+path);
  std::string tag;int version=0;in>>tag>>version;if(tag!="SU2_JUNCTION_CASE"||version!=1)throw std::runtime_error("WP0B bad case header.");
  auto s=std::make_unique<State>();Tensor sensor;std::map<Id,Point> nodes;std::vector<std::pair<Id,std::array<Id,4>>> cells;
  std::vector<std::tuple<Id,int,std::array<Id,3>>> facets;center=UINT64_MAX;
  for(std::string key;in>>key;) {
    if(key=="end")break;
    if(key=="name")in>>s->name;
    else if(key=="markers"){size_t n;in>>n;s->markers.resize(n);for(auto& m:s->markers)in>>m;}
    else if(key=="policy")in>>s->h0>>s->growth>>s->thickness;
    else if(key=="walls"){size_t n;in>>n;for(size_t k=0;k<n;++k){int m;in>>m;s->walls.insert(m);}}
    else if(key=="core")in>>s->core;
    else if(key=="sensor")in>>sensor.xx>>sensor.xy>>sensor.xz>>sensor.yy>>sensor.yz>>sensor.zz;
    else if(key=="nodes"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;Point p;in>>id>>p.x>>p.y>>p.z;nodes[id]=p;}}
    else if(key=="cells"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,4> v;in>>id>>v[0]>>v[1]>>v[2]>>v[3];cells.push_back({id,v});}}
    else if(key=="facets"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;int m;std::array<Id,3> v;in>>id>>m>>v[0]>>v[1]>>v[2];facets.emplace_back(id,m,v);}}
    else if(key=="center")in>>center;
    else throw std::runtime_error("WP0B unknown case key "+key);
    if(!in)throw std::runtime_error("WP0B truncated case "+path);
  }
  std::vector<Cell> mesh;
  for(const auto& c:cells){Cell cell{c.first,{}};for(unsigned k=0;k<4;++k)cell.v[k]={c.second[k],nodes.at(c.second[k])};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);mesh.push_back(cell);}
  std::map<Face,Face> skin;for(const auto& c:mesh)for(auto f:CellFaces(c))skin[Sorted(f)]=f;
  std::map<Face,int> uses;for(const auto& c:mesh)for(auto f:CellFaces(c))++uses[Sorted(f)];
  std::map<Face,Id> physical;
  for(const auto& f:facets){const auto key=Sorted(std::get<2>(f));if(uses.at(key)!=1)throw std::runtime_error("WP0B facet is not a skin face.");
    const auto oriented=skin.at(key);Facet facet;facet.id=std::get<0>(f);facet.marker=std::get<1>(f);
    for(unsigned k=0;k<3;++k)facet.v[k]={oriented[k],nodes.at(oriented[k])};s->facets.push_back(facet);physical[key]=facet.id;}
  s->reference=std::make_shared<PlanarReference>(s->facets);
  std::vector<DonorCell> donors;Point lo=mesh.front().v[0].p,hi=lo;
  for(const auto& c:mesh){OwnedCell row;row.cell=c;const auto f=CellFaces(c);for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
    s->mesh.emplace(c.id,row);for(auto n:c.v){lo={std::min(lo.x,n.p.x),std::min(lo.y,n.p.y),std::min(lo.z,n.p.z)};hi={std::max(hi.x,n.p.x),std::max(hi.y,n.p.y),std::max(hi.z,n.p.z)};}}
  {
    // The proxy sensor is uniform, so six covering source tetrahedra reproduce its P1 field exactly within the
    // 256-donor frozen-field bound; the BL composition is evaluated at every actual query as in the engine.
    const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
    auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
    const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
    for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);
      for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
      if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);
      d.sensor.fill(sensor);donors.push_back(d);}
  }
  std::vector<CBoundaryLayerMetric::Wall> walls;
  for(int m:s->walls){CBoundaryLayerMetric::Wall w;w.layer.marker=s->markers.at(m);w.layer.firstHeight=s->h0;w.layer.growth=s->growth;w.layer.thickness=s->thickness;
    std::map<Id,unsigned long> index;
    for(const auto& f:s->facets)if(f.marker==m){for(auto n:f.v){const auto ins=index.emplace(n.id,index.size());if(ins.second){w.coord.push_back(n.p.x);w.coord.push_back(n.p.y);w.coord.push_back(n.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
    walls.push_back(std::move(w));s->heights[m]=s->h0;}
  s->layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls));
  s->field=std::make_unique<FrozenField>(std::move(donors),compose(s->layers,s->core),false,std::vector<Id>{},Bands(s->layers));
  Prepare(*s);return s;
}

// ---- A1 exact annulus input (byte-identical annulus_n1 inputs/config; same construction as WP-0(b) "annulus_input") ----
std::unique_ptr<State> LoadAnnulus(const std::string& wp0b) {
  const std::string annulus=wp0b+"/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1";
  ReferenceState rejected;if(!LoadReference(annulus+"_rejected_rank0.native_ref",rejected)||!rejected.original||!rejected.original->Mapped())throw std::runtime_error("DC5 annulus reference");
  std::ifstream cfgFile(annulus+".cfg");if(!cfgFile)throw std::runtime_error("DC5 annulus cfg");
  std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
  std::ifstream in(wp0b+"/annulus_case.txt");std::string tag;int version;in>>tag>>version;if(tag!="SU2_JUNCTION_ANNULUS")throw std::runtime_error("DC5 annulus case");
  std::string key;double core=0;Id recordedCell=0;double recordedQ=0,recordedJ=0;in>>key>>core;in>>key>>recordedCell>>recordedQ>>recordedJ;
  size_t count=0;in>>key>>count;std::vector<DonorCell> donors;
  for(size_t i=0;i<count;++i){DonorCell d;int extension=0;in>>d.cell.id>>extension;for(unsigned k=0;k<4;++k){auto& n=d.cell.v[k];auto& m=d.sensor[k];in>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;}
    if(extension)d.extension_component=0;donors.push_back(d);}
  auto block=[&](const std::string& expected){std::string name;size_t n;in>>name>>n;if(name!=expected)throw std::runtime_error("DC5 annulus block");std::vector<OwnedCell> rows;
    for(size_t i=0;i<n;++i){OwnedCell row;in>>row.cell.id;for(unsigned k=0;k<4;++k)in>>row.cell.v[k].id>>row.cell.v[k].p.x>>row.cell.v[k].p.y>>row.cell.v[k].p.z;
      for(unsigned k=0;k<4;++k){int present;in>>present>>row.physical[k].facet;row.physical[k].present=present;}rows.push_back(row);}return rows;};
  const auto input=block("input");
  auto layers=CBoundaryLayerMetric::FromNativeReference(rejected,*config);
  if(config->GetAdap_BL_Budget()){layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));layers->FreezeOuterNormal();}
  auto authority=rejected.original->Parameters()->Authority();BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:rejected.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
  auto s=std::make_unique<State>();s->name="annulus_input";
  s->reference=PlanarReference::WithParameters(rejected.original,std::make_shared<BoundaryParameterState>(authority,points,skin));
  s->facets=rejected.facets;s->markers=rejected.marker_names;s->mapped=true;s->core=core;
  for(unsigned short b=0;b<config->GetnAdap_BL();++b)for(size_t m=0;m<rejected.marker_names.size();++m)if(rejected.marker_names[m]==config->GetAdap_BL(b).marker) {
    s->walls.insert(int(m));s->heights[int(m)]=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);
    s->h0=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);s->growth=SU2_TYPE::GetValue(config->GetAdap_BL(b).growth);s->thickness=SU2_TYPE::GetValue(config->GetAdap_BL(b).thickness);}
  for(const auto& row:input)s->mesh.emplace(row.cell.id,row);
  s->layers=layers;s->field=std::make_unique<FrozenField>(donors,Compose(layers,core),false,std::vector<Id>{0},Bands(layers));
  Prepare(*s);return s;
}

}  // namespace layer2d

TEST_CASE("DC-5 2D-way layer construction: admission A vs D.1 on identical proposals", "[.][NativeLayer2DWay3D]") {
  // Evidence of this harness was produced under the former production default: keep StrictExcess pinned.
  struct PinStrictExcess{SU2Native3D::RefineAcceptance a=SU2Native3D::GetRefineAcceptance();PinStrictExcess(){SU2Native3D::SetRefineAcceptance(SU2Native3D::RefineAcceptance::StrictExcess);}~PinStrictExcess(){SU2Native3D::SetRefineAcceptance(a);}} pinStrictExcess;
  using namespace layer2d;using junction_wp0::State;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC5_ROOT");REQUIRE(folder);const std::string root(folder);   // bl_remesh_v1/dc5
  const char* wp0b=std::getenv("SU2_DC5_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);      // robust_junction_v1/wp0b
  const char* only=std::getenv("SU2_DC5_CASES");const std::string selected=only?only:"";
  auto wanted=[&](const std::string& n){return selected.empty()||(","+selected+",").find(","+n+",")!=std::string::npos;};
  auto done=[&](const std::string& n){return std::ifstream(root+"/dc5_"+n+".json").good();};
  // A1: exact annulus end-to-end (D1 inactive: DC-1 found no feature), unchanged 8192 work / 120 s.
  if(wanted("annulus")&&!done("annulus")) {
    std::ostringstream json,log;json<<std::setprecision(12)<<"{\"case\":\"annulus\",\"limits\":{\"work\":8192,\"seconds\":120}";
    for(Adm adm:{Adm::A,Adm::D}) {
      auto s=LoadAnnulus(fixtures);const auto first=FirstRow(*s,log);
      const auto run=Engine(*s,adm,Limits{8192,120},log);const auto g=FinalGates(*s);const char* tag=adm==Adm::A?"A":"D";
      json<<",\""<<tag<<"\":"<<Json(run,g)<<",\""<<tag<<"_first_row\":["<<first.first<<','<<first.second<<']';
      Export(*s,root+"/dc5_annulus_"+tag+"_final.mesh");
      auto fresh=LoadAnnulus(fixtures);fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=FinalGates(*fresh);
      json<<",\""<<tag<<"_reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max\":"<<double(ga.lmax)<<",\"height_failures\":"<<ga.height_failures<<",\"complete\":"<<ga.complete<<'}';
    }
    json<<"}\n";std::ofstream(root+"/dc5_annulus.json")<<json.str();std::cout<<log.str()<<std::flush;
  }
  // Generic fixtures (proxy sensor); D1 from the DC-1 export when present. Admissions run on fresh identical loads.
  std::vector<std::string> cases{"tube_root_z","tube_root_x","two_tube_y","m6_root_te_h0.02","m6_root_le_h0.02","m6_root_te_h0.002","m6_root_le_h0.002"};
  for(const auto& name:cases) {
    if(!wanted(name)||done(name))continue;
    std::ostringstream json,log;json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\"";
    auto d1=LoadD1(root+"/d1_"+name+".txt");
    auto load=[&](std::shared_ptr<size_t> applied){Id center;
      return LoadCaseWith(fixtures+"/"+name+".txt",center,[&](std::shared_ptr<CBoundaryLayerMetric> layers,double core){
        return d1?ComposeD1(layers,core,d1,applied):Compose(layers,core);});};
    // fidelity: D1-free test composition equals the production composition bitwise at sample points
    {auto s=load(nullptr);auto plain=ComposeD1(s->layers,s->core,nullptr,nullptr);auto prod=Compose(s->layers,s->core);double worst=0,scale=0;
     for(const auto& row:s->mesh){const auto c=Centroid(row.second.cell);const Tensor t{s->core,0,0,s->core,0,s->core};const auto x=plain(c,t),y=prod(c,t);
       worst=std::max({worst,std::abs(x.xx-y.xx),std::abs(x.yy-y.yy),std::abs(x.zz-y.zz),std::abs(x.xy-y.xy),std::abs(x.xz-y.xz),std::abs(x.yz-y.yz)});
       scale=std::max({scale,std::abs(y.xx),std::abs(y.yy),std::abs(y.zz)});}
     json<<",\"composition_fidelity_max_abs\":"<<worst<<",\"composition_fidelity_rel_to_max_entry\":"<<worst/scale<<",\"d1\":"<<(d1?"true":"false")<<",\"d1_segments\":"<<(d1?d1->segments.size():0);CHECK(worst<=1e-12*scale);}
    for(Adm adm:{Adm::A,Adm::D}) {
      auto applied=std::make_shared<size_t>(0);auto s=load(applied);
      const auto first=FirstRow(*s,log);
      const auto run=Engine(*s,adm,Limits{8192,600},log);const auto g=FinalGates(*s);
      json<<",\""<<(adm==Adm::A?"A":"D")<<"\":"<<Json(run,g)<<",\""<<(adm==Adm::A?"A":"D")<<"_first_row\":["<<first.first<<','<<first.second<<"],\""<<(adm==Adm::A?"A":"D")<<"_d1_samples\":"<<*applied;
      Export(*s,root+"/dc5_"+name+"_"+(adm==Adm::A?"A":"D")+"_final.mesh");
      // independent re-audit with a fresh field (no caches)
      auto fresh=load(nullptr);fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=FinalGates(*fresh);
      json<<",\""<<(adm==Adm::A?"A":"D")<<"_reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max\":"<<double(ga.lmax)<<",\"complete\":"<<ga.complete<<'}';
    }
    json<<"}\n";std::ofstream(root+"/dc5_"+name+".json")<<json.str();std::cout<<log.str()<<std::flush;
  }
}
