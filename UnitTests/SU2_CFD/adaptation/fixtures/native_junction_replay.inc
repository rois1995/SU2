// ROBUST_JUNCTION_PLAN rev4 WP-0(b): TEST-ONLY prototype of D2 column points and D3 single/bounded-compound
// insertion through the existing operators and unchanged gates. Not production code; nothing here is promoted.
// Outcomes per target: 1 single, 2 bounded compound, 3 blocked (witness kept), 0 not needed (no long edge sees it).
// DC-14: the pipeline code of this file moved to SU2_CFD/include/adaptation/native_bl3d/BLState.inl (production, inline); the
// fixture loaders and TEST_CASEs below stay test-only.
namespace junction_wp0 {
std::unique_ptr<State> LoadCase(const std::string& path,Id& center) {
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
  s->field=std::make_unique<FrozenField>(std::move(donors),Compose(s->layers,s->core),false,std::vector<Id>{},Bands(s->layers));
  Prepare(*s);return s;
}
}  // namespace junction_wp0

TEST_CASE("WP-0(b) D2 column points and D3 insertion through existing operators", "[.][NativeJunctionReplay3D]") {
  using namespace junction_wp0;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_JUNCTION_WP0B_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* only=std::getenv("SU2_JUNCTION_WP0B_CASES");const std::string selected=only?only:"";
  const bool resume=std::getenv("SU2_JUNCTION_WP0B_RESUME")!=nullptr;
  auto wanted=[&](const std::string& name){return selected.empty()||(","+selected+",").find(","+name+",")!=std::string::npos;};
  auto done=[&](const std::string& name){return resume&&std::ifstream("wp0b_"+name+".json").good();};
  const std::string annulus=root+"/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1";
  if(wanted("annulus")&&!done("annulus")) {
    std::ostringstream log,json;json<<std::setprecision(17);
    ReferenceState rejected;REQUIRE(LoadReference(annulus+"_rejected_rank0.native_ref",rejected));REQUIRE(rejected.original);REQUIRE(rejected.original->Mapped());
    std::ifstream cfgFile(annulus+".cfg");REQUIRE(cfgFile.good());std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
    std::ifstream in(root+"/annulus_case.txt");std::string tag;int version;in>>tag>>version;REQUIRE(tag=="SU2_JUNCTION_ANNULUS");
    std::string key;double core=0;Id recordedCell=0;double recordedQ=0,recordedJ=0;in>>key>>core;REQUIRE(key=="core");in>>key>>recordedCell>>recordedQ>>recordedJ;REQUIRE(key=="recorded");
    size_t count=0;in>>key>>count;REQUIRE(key=="donors");std::vector<DonorCell> donors;
    for(size_t i=0;i<count;++i){DonorCell d;int extension=0;in>>d.cell.id>>extension;for(unsigned k=0;k<4;++k){auto& n=d.cell.v[k];auto& m=d.sensor[k];in>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;}
      if(extension)d.extension_component=0;donors.push_back(d);}
    auto block=[&](const std::string& expected){std::string name;size_t n;in>>name>>n;REQUIRE(name==expected);std::vector<OwnedCell> rows;
      for(size_t i=0;i<n;++i){OwnedCell row;in>>row.cell.id;for(unsigned k=0;k<4;++k)in>>row.cell.v[k].id>>row.cell.v[k].p.x>>row.cell.v[k].p.y>>row.cell.v[k].p.z;
        for(unsigned k=0;k<4;++k){int present;in>>present>>row.physical[k].facet;row.physical[k].present=present;}rows.push_back(row);}REQUIRE(bool(in));return rows;};
    const auto input=block("input"),before=block("before"),after=block("after");
    std::vector<std::tuple<std::string,bool,std::map<Id,Point>>> variants;in>>key>>count;REQUIRE(key=="variants");
    for(size_t i=0;i<count;++i){std::string word,name;int swap;size_t n;in>>word>>name>>swap>>n;REQUIRE(word=="variant");std::map<Id,Point> coords;
      for(size_t k=0;k<n;++k){Id id;Point p;in>>id>>p.x>>p.y>>p.z;coords[id]=p;}variants.emplace_back(name,swap!=0,coords);}
    in>>key;REQUIRE(key=="end");
    auto layers=CBoundaryLayerMetric::FromNativeReference(rejected,*config);
    if(config->GetAdap_BL_Budget()){layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));layers->FreezeOuterNormal();}
    auto make=[&](std::shared_ptr<const PlanarReference> reference,const std::vector<OwnedCell>& cells,const std::string& name) {
      auto s=std::make_unique<State>();s->name=name;s->reference=reference;s->facets=rejected.facets;s->markers=rejected.marker_names;s->mapped=true;s->core=core;
      for(unsigned short b=0;b<config->GetnAdap_BL();++b)for(size_t m=0;m<rejected.marker_names.size();++m)if(rejected.marker_names[m]==config->GetAdap_BL(b).marker) {
        s->walls.insert(int(m));s->heights[int(m)]=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);
        s->h0=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);s->growth=SU2_TYPE::GetValue(config->GetAdap_BL(b).growth);s->thickness=SU2_TYPE::GetValue(config->GetAdap_BL(b).thickness);}
      for(const auto& row:cells)s->mesh.emplace(row.cell.id,row);
      s->layers=layers;s->field=std::make_unique<FrozenField>(donors,Compose(layers,core),false,std::vector<Id>{0},Bands(layers));
      Prepare(*s);return s;
    };
    auto patch=[&](const std::vector<OwnedCell>& rows){Patch p;for(const auto& row:rows){p.cells.push_back(row.cell);const auto f=CellFaces(row.cell);
      for(unsigned k=0;k<4;++k)if(row.physical[k].present)p.surface.push_back({f[k],row.physical[k].facet});}return p;};
    auto parameters=[&](const std::string& file){std::ifstream raw(file,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(raw)),{});
      RecordStream stream(static_cast<const std::vector<char>&>(bytes));uint64_t n=0;stream(n);BoundaryPoints points;for(uint64_t i=0;i<n;++i){BoundaryPoint p;stream(p);points[p.node.id]=p;}REQUIRE(stream.End());return points;};
    std::vector<OwnedCell> state;{std::ifstream raw(annulus+"_rejected_rank0.bin",std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(raw)),{});
      RecordStream stream(static_cast<const std::vector<char>&>(bytes));uint64_t n=0;stream(n);for(uint64_t i=0;i<n;++i){OwnedCell row;stream(row);state.push_back(row);}REQUIRE(stream.End());}
    auto rejectedState=make(rejected.original,state,"annulus_rejected");
    // 1. Native replay fidelity: the recorded weakest cell of the saved candidate.
    auto trialBefore=patch(before),trialAfter=patch(after);
    trialBefore.parameters=parameters(annulus+"_rejected_rank0_trial_before.parameters.bin");trialAfter.parameters=parameters(annulus+"_rejected_rank0_trial_after.parameters.bin");
    double nativeQ=-1,nativeJ=-1;
    for(const auto& c:trialAfter.cells)if(c.id==recordedCell){const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},rejectedState->field->Query(Centroid(c)).target);nativeQ=m.mean_ratio;nativeJ=m.minimum_scaled_jacobian;}
    log<<std::setprecision(17)<<"WP0B_ANNULUS_FIDELITY q="<<nativeQ<<" recorded="<<recordedQ<<" J="<<nativeJ<<" recordedJ="<<recordedJ<<"\n";
    CHECK(nativeQ==Approx(recordedQ).epsilon(1e-12));CHECK(nativeJ==Approx(recordedJ).epsilon(1e-12));
    json<<"{\"case\":\"annulus\",\"fidelity\":{\"cell\":"<<recordedCell<<",\"native_q\":"<<nativeQ<<",\"recorded_q\":"<<recordedQ<<",\"native_J\":"<<nativeJ<<",\"recorded_J\":"<<recordedJ<<'}';
    // 2. WP-0(a) witnesses natively: shape and Refine progress of the saved candidate and each variant.
    json<<",\"wp0a_native\":[";
    for(size_t vi=0;vi<=variants.size();++vi) {
      Patch fresh=trialAfter;std::string name="saved";
      if(vi){const auto& row=variants[vi-1];name=std::get<0>(row);for(auto& c:fresh.cells)for(auto& n:c.v){const auto f=std::get<2>(row).find(n.id);if(f!=std::get<2>(row).end())n.p=f->second;}
        if(std::get<1>(row)) {
          // Boundary 2-2 swap of symmetry edge 1-5 to 0-13663 with its two-cell volume shell (as WP-0(a)).
          std::vector<Cell> shell,kept;for(const auto& c:fresh.cells){bool a=false,b=false;for(auto n:c.v){a=a||n.id==1;b=b||n.id==5;}(a&&b?shell:kept).push_back(c);}
          REQUIRE(shell.size()==2);std::map<Id,Node> nodes;for(const auto& c:shell)for(auto n:c.v)nodes.emplace(n.id,n);
          Id next=1000000;for(Id apex:{Id(1),Id(5)}){std::array<Node,4> v{nodes.at(apex),nodes.at(0),nodes.at(16809),nodes.at(13663)};
            if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);kept.push_back({next++,v});}
          fresh.cells=kept;
        }}
      double q=1,j=1;for(const auto& c:fresh.cells){const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},rejectedState->field->Query(Centroid(c)).target);q=std::min(q,m.mean_ratio);j=std::min(j,m.minimum_scaled_jacobian);}
      PatchMetric measured;MetricDiagnosis d;std::string reason;
      const bool refine=ValidateMetricChange(trialBefore.cells,fresh.cells,MetricChange::Refine,*rejectedState->field,measured,reason,nullptr,nullptr,&d);
      json<<(vi?",":"")<<"{\"variant\":"<<Json(name)<<",\"q_min\":"<<q<<",\"J_min\":"<<j<<",\"shape_gate\":"<<(q>=MinimumMeanRatio&&j>=MinimumScaledJacobian)
          <<",\"refine_valid\":"<<refine<<",\"removed_excess\":"<<double(d.removed_excess)<<",\"added_excess\":"<<double(d.added_excess)<<",\"edge_max\":"<<double(measured.maximum_length)<<",\"reason\":"<<Json(reason)<<'}';
      log<<"WP0B_WP0A_NATIVE "<<name<<" q="<<q<<" J="<<j<<" refine="<<refine<<" removed="<<double(d.removed_excess)<<" added="<<double(d.added_excess)<<"\n";
    }
    json<<"]";
    // 3. Repair rejection histogram on the saved candidate with the C3 fix (candidate's own ID range).
    {
      Id first=UINT64_MAX;for(const auto& c:trialAfter.cells)first=std::min(first,c.id);
      json<<",\"repair\":[";
      for(int retire=1;retire>=0;--retire) {
        Patch repaired;ConnectivityRepairReport report;std::string reason;
        const bool progress=RepairMetricConnectivity(trialAfter,*rejectedState->field,*rejectedState->reference,rejectedState->heights,first,repaired,report,reason,256,retire!=0);
        double q=1,j=1;if(progress)for(const auto& c:repaired.cells){const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},rejectedState->field->Query(Centroid(c)).target);q=std::min(q,m.mean_ratio);j=std::min(j,m.minimum_scaled_jacobian);}
        std::string refineReason;bool refine=false;MetricDiagnosis d;
        if(progress){PatchMetric measured;refine=ValidateMetricChange(trialBefore.cells,repaired.cells,MetricChange::Refine,*rejectedState->field,measured,refineReason,nullptr,nullptr,&d);}
        json<<(retire?"":",")<<"{\"retire_points\":"<<retire<<",\"progress\":"<<progress<<",\"trials\":"<<report.trials<<",\"flips\":"<<report.flips<<",\"moves\":"<<report.moves
            <<",\"vertex_moves\":"<<report.vertex_moves<<",\"coordinated\":"<<report.coordinated<<",\"removals\":"<<report.removals<<",\"solve_rejected\":"<<report.solve_rejected
            <<",\"move_geometry_rejected\":"<<report.move_geometry_rejected<<",\"move_height_rejected\":"<<report.move_height_rejected<<",\"move_loss_rejected\":"<<report.move_loss_rejected
            <<",\"before_loss\":"<<report.before_loss<<",\"after_loss\":"<<report.after_loss<<",\"complete_shape\":"<<report.complete_shape<<",\"q_min\":"<<q<<",\"J_min\":"<<j
            <<",\"refine_valid\":"<<refine<<",\"removed_excess\":"<<double(d.removed_excess)<<",\"added_excess\":"<<double(d.added_excess)<<",\"reason\":"<<Json(reason)<<",\"refine_reason\":"<<Json(refineReason)<<'}';
        log<<"WP0B_REPAIR retire="<<retire<<" progress="<<progress<<" trials="<<report.trials<<" flips="<<report.flips<<" vertex_moves="<<report.vertex_moves<<" coordinated="<<report.coordinated
           <<" height_rejected="<<report.move_height_rejected<<" geometry_rejected="<<report.move_geometry_rejected<<" loss_rejected="<<report.move_loss_rejected<<" q="<<q<<" refine="<<refine<<"\n";
      }
      json<<"]";
    }
    // 4. D2/D3 sequences on the saved rejected state and on the exact input after existing first-row construction.
    std::vector<Id> roots;for(const auto& f:rejected.facets)if(rejectedState->walls.count(f.marker))for(auto n:f.v)if(std::find(roots.begin(),roots.end(),n.id)==roots.end())roots.push_back(n.id);
    std::sort(roots.begin(),roots.end());
    json<<",\"rejected_state\":{\"before\":";Quality(*rejectedState,json);json<<',';
    const auto rejectedSummary=Sequence(*rejectedState,roots,240,json,log);json<<",\"after\":";Quality(*rejectedState,json);json<<'}';
    // Initial reference: the same immutable authority with every original vertex bound to itself.
    auto authority=rejected.original->Parameters()->Authority();BoundaryPoints points;std::vector<SurfaceFace> skin;
    for(const auto& f:rejected.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
    auto initialReference=PlanarReference::WithParameters(rejected.original,std::make_shared<BoundaryParameterState>(authority,points,skin));
    auto inputState=make(initialReference,input,"annulus_input");
    const auto first=FirstRow(*inputState,log);
    json<<",\"input_state\":{\"first_row_built\":"<<first.first<<",\"first_row_refused\":"<<first.second<<",\"before\":";Quality(*inputState,json);json<<',';
    const auto inputSummary=Sequence(*inputState,roots,240,json,log);json<<",\"after\":";Quality(*inputState,json);json<<"}}\n";
    std::ofstream out("wp0b_annulus.json");out<<json.str();REQUIRE(bool(out));
    std::cout<<log.str()<<std::flush;
    (void)rejectedSummary;(void)inputSummary;
  }
  std::ifstream list(root+"/cases.txt");REQUIRE(list.good());
  for(std::string name;list>>name;) {
    if(!wanted(name)||done(name))continue;
    std::ostringstream log,json;json<<std::setprecision(17);Id center=UINT64_MAX;
    auto s=LoadCase(root+"/"+name+".txt",center);
    const auto first=FirstRow(*s,log);
    std::vector<Id> walls;for(const auto& f:s->facets)if(s->walls.count(f.marker))for(auto n:f.v)if(std::find(walls.begin(),walls.end(),n.id)==walls.end())walls.push_back(n.id);
    std::sort(walls.begin(),walls.end());
    if(center!=UINT64_MAX) {
      // M6: the three root (partner-incident) wall vertices and two plain wing vertices nearest the root point.
      Point c;for(const auto& f:s->facets)for(auto n:f.v)if(n.id==center)c=n.p;
      std::vector<std::pair<long double,Id>> rootRank,plainRank;
      for(Id v:walls){bool partner=false;Point p;for(const auto& f:s->facets)for(auto n:f.v)if(n.id==v){p=n.p;partner=partner||!s->walls.count(f.marker);}
        (partner?rootRank:plainRank).emplace_back(Norm(Of(p)-Of(c)),v);}
      std::sort(rootRank.begin(),rootRank.end());std::sort(plainRank.begin(),plainRank.end());walls.clear();
      for(size_t k=0;k<rootRank.size()&&k<3;++k)walls.push_back(rootRank[k].second);
      for(size_t k=0;k<plainRank.size()&&k<2;++k)walls.push_back(plainRank[k].second);
    }
    json<<"{\"case\":"<<Json(name)<<",\"first_row_built\":"<<first.first<<",\"first_row_refused\":"<<first.second<<",\"before\":";Quality(*s,json);json<<',';
    Sequence(*s,walls,center!=UINT64_MAX?300:240,json,log);json<<",\"after\":";Quality(*s,json);json<<"}\n";
    std::ofstream out("wp0b_"+name+".json");out<<json.str();REQUIRE(bool(out));
    std::cout<<log.str()<<std::flush;
  }
}
