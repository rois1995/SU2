// BL_FIRST_ROW_DESIGN rev 2.1, DC-7: TEST-ONLY [NativeFirstRowDC7] harness of the WP-D construction order
// W (wall/seam/junction subdivision to the composed D1 surface target, no row present) -> R1 (clearance conditioning)
// -> R2 (closed row transactions T(V) = LiftCone + FaceApex, apex window decided by the exact three-variable LP over
// the whole height interval, persistent columns col(f) = star(p_f)) -> V (existing volume operators, columns
// protected). Nothing here is production code or promoted. Production primitives only: SplitEdge,
// InsertEdgeCavity, MoveVertex, Reconnect, Cone, ValidatePrescribedInterface, CheckFirstHeight, FindFace,
// BindNewPoint, RepairMetricConnectivity, FrozenField with the production composition (SU2NativeBL3D) and the
// production D1 law of CBoundaryLayerMetric. Admission (user decision U1): first children absolute q >= .20,
// J >= .05 and |h-h0|/h0 <= T; layer-construct fresh cells positivity only; other kinds not worse; Prepare
// transactions (W, R1, before any row) geometry/positivity only. Final gates are unchanged and absolute.
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLFirstRow7.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
namespace dc7 {
// Faceted fixture -> v6 mapped-component authority (as the A1 reference: MappedFacetReference with released certified
// planar seams, 1e-13 relative mapping error per facet, every original vertex bound to itself). Crease and junction
// points then bind to their original edge feature (production SplitEdge/BindNewPoint) instead of requiring an
// exactly representable on-edge binary64 point.
void MakeMapped(State& s) {
  long double scale=0;for(const auto& f:s.facets)for(auto x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  std::map<Id,double> errors;for(const auto& f:s.facets)errors.emplace(f.id,double(1e-13L*std::max(1.L,scale)));
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(s.reference,errors,7,SIZE_MAX,true);
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:s.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto x:f.v)points.emplace(x.id,BoundaryPoint{x,authority->Vertex(x.id)});}
  s.reference=PlanarReference::WithParameters(s.reference,std::make_shared<BoundaryParameterState>(authority,points,skin));
  s.mapped=true;s.overlay.clear();Prepare(s);
}
// ---- loaders with the production composition (SU2NativeBL3D) and the production D1 law -----------------------------
std::unique_ptr<State> LoadA1(const std::string& wp0b) {
  const std::string annulus=wp0b+"/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1";
  ReferenceState rejected;if(!LoadReference(annulus+"_rejected_rank0.native_ref",rejected)||!rejected.original||!rejected.original->Mapped())throw std::runtime_error("DC7 annulus reference");
  std::ifstream cfgFile(annulus+".cfg");if(!cfgFile)throw std::runtime_error("DC7 annulus cfg");
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
  s->layers=layers;s->field=std::make_unique<FrozenField>(donors,SU2NativeBL3D::Composition(layers,core),false,std::vector<Id>{0},SU2NativeBL3D::EdgeGeometry(layers,core));
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
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),false,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core));
  s->heights.tolerance=HeightTolerance;s->lengths.clear();MakeMapped(*s);return s;
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
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),false,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core));
  Prepare(*s);MakeMapped(*s);return s;
}
}  // namespace dc7

TEST_CASE("DC-7 W -> R1 -> R2 closed row transactions -> V with protected columns", "[.][NativeFirstRowDC7]") {
  // Evidence of this harness was produced under the former production default: keep StrictExcess pinned.
  struct PinStrictExcess{SU2Native3D::RefineAcceptance a=SU2Native3D::GetRefineAcceptance();PinStrictExcess(){SU2Native3D::SetRefineAcceptance(SU2Native3D::RefineAcceptance::StrictExcess);}~PinStrictExcess(){SU2Native3D::SetRefineAcceptance(a);}} pinStrictExcess;
  using namespace dc7;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC7_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC7_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC7_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC7_CASES");REQUIRE(only);
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty()||std::ifstream(root+"/dc7_"+name+".json").good())continue;
    const auto start=Clock::now();std::ostringstream json,log;json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"limits\":{\"work\":8192,\"seconds\":120}";
    auto load=[&]()->std::unique_ptr<State>{
      if(name=="annulus")return LoadA1(fixtures);
      if(name.rfind("m6_",0)==0)return LoadM6Cut(cuts+"/"+name+".txt");
      return LoadTube(fixtures+"/"+name+".txt");};
    auto s=load();Rows rows;
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped
        <<",\"d1\":{\"segments\":"<<s->layers->GetFeatures().segments<<",\"images\":"<<s->layers->GetFeatures().images<<",\"corners\":"<<s->layers->GetFeatures().corners
        <<",\"angle\":"<<s->layers->GetFeatures().angle<<",\"R_f\":"<<s->layers->GetFeatures().maximumRadius<<"}}";
    std::cout<<"DC7 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
    const auto w=PhaseW(*s,8192,120,log);json<<",\"W\":"<<JsonStage(w)<<",\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    std::cout<<"DC7 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"steiner\":"<<r1.steiner<<'}';
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC7 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<std::endl;
    const auto r2=PhaseR2(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC7 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" last="<<r2.last<<std::endl;
    json<<",\"after_R\":"<<JsonFinal(*s,rows);
    VStage v;
    // V runs in every case (with incomplete rows it measures the transition only; the construction has already failed)
    v=PhaseV(*s,rows,8192,120,log);json<<",\"V\":"<<JsonStage(v)<<",\"V_column_demand\":"<<v.column_demand<<",\"V_sweeps\":"<<v.sweeps<<",\"V_with_complete_rows\":"<<(r2.unrowed==0);
    json<<",\"final\":"<<JsonFinal(*s,rows);
    ExportGates(*s,rows,root+"/dc7_"+name+"_final.mesh");
    {auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"height_failures\":"<<ga.height_failures<<",\"complete\":"<<ga.complete<<'}';}
    json<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(root+"/dc7_"+name+".json")<<json.str();std::ofstream(root+"/dc7_"+name+".log")<<log.str();
    std::cout<<"DC7 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}

// M6 field export at the configured target (ADAP_ANGLE as configured): selected sensor per owned node, D1 summary,
// normalization; saved state, no CFD step, no construction. Run on 2 ranks (serial donor admission refuses M6).
TEST_CASE("DC-7 M6 selected field export", "[.][NativeFirstRowDC7Field]") {
  // Evidence of this harness was produced under the former production default: keep StrictExcess pinned.
  struct PinStrictExcess{SU2Native3D::RefineAcceptance a=SU2Native3D::GetRefineAcceptance();PinStrictExcess(){SU2Native3D::SetRefineAcceptance(SU2Native3D::RefineAcceptance::StrictExcess);}~PinStrictExcess(){SU2Native3D::SetRefineAcceptance(a);}} pinStrictExcess;
  class Driver : public CSinglezoneDriver {
   public:
    using CSinglezoneDriver::CSinglezoneDriver;
    CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
    CSolver& Solver(){return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];}
    CConfig& Config(){return *config_container[ZONE_0];}
  };
  World world;const auto* configFile=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(configFile);
  auto driver=std::make_unique<Driver>(const_cast<char*>(configFile),1,world.comm);driver->Preprocess(0);
  REQUIRE_FALSE(driver->Config().GetTime_Domain());
  const auto begin=world.seconds();driver->ComputeMetric();const auto seconds=world.seconds()-begin;
  const auto family=driver->Solver().GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->layers);
  double core=std::numeric_limits<double>::max();
  {std::ofstream out("selected_rank"+std::to_string(world.rank)+".bin",std::ios::binary);const uint64_t count=driver->Geometry().GetnPointDomain();out.write(reinterpret_cast<const char*>(&count),8);
   for(unsigned long p=0;p<count;++p){const uint64_t id=driver->Geometry().nodes->GetGlobalIndex(p);out.write(reinterpret_cast<const char*>(&id),8);
     for(unsigned k=0;k<3;++k){const double x=SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,k));out.write(reinterpret_cast<const char*>(&x),8);}
     const auto& m=family->selected.at(p);double t[9];for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){t[3*a+b]=SU2_TYPE::GetValue(m.m[a][b]);out.write(reinterpret_cast<const char*>(&t[3*a+b]),8);}
     su2double mm[3][3]={{m.m[0][0],m.m[0][1],m.m[0][2]},{m.m[1][0],m.m[1][1],m.m[1][2]},{m.m[2][0],m.m[2][1],m.m[2][2]}},vec[3][3],val[3],work[3];
     CBlasStructure::EigenDecomposition(mm,vec,val,3,work);core=std::min(core,SU2_TYPE::GetValue(*std::min_element(val,val+3)));}
   REQUIRE(bool(out));}
  core=CPassiveComm::Allreduce(core,CPassiveComm::Op::MIN,world.comm);
  const auto& f=family->layers->GetFeatures();const auto profile=family->layers->GetOuterNormal();
  if(!world.rank) {
    std::ofstream out("field.json");out<<std::setprecision(17)<<"{\"CFD_steps\":0,\"construction\":false,\"ranks\":"<<world.size<<",\"seconds\":"<<seconds
      <<",\"angle\":"<<f.angle<<",\"segments\":"<<f.segments<<",\"image_segments\":"<<f.images<<",\"corners\":"<<f.corners<<",\"image_corners\":"<<f.imageCorners
      <<",\"carrier_ends\":"<<f.carrierEnds<<",\"carrier_corners\":"<<f.carrierCorners<<",\"free_corners\":"<<f.freeCorners<<",\"planes\":"<<f.planes
      <<",\"slope\":"<<f.slope<<",\"R_f\":"<<f.maximumRadius<<",\"identity\":"<<f.identity<<",\"parameter\":"<<profile.parameter<<",\"version\":"<<profile.version
      <<",\"normalization_status\":"<<unsigned(family->normalization.status)<<",\"estimate\":"<<family->normalization.estimate.value
      <<",\"uncertainty\":";if(std::isfinite(family->normalization.estimate.uncertainty))out<<family->normalization.estimate.uncertainty;else out<<"null";
    out<<",\"family_core\":"<<family->core<<",\"sensor_core_min_eigenvalue\":"<<core<<"}\n";REQUIRE(bool(out));
  }
  {Mute mute;driver->Finalize();driver.reset();}
}
