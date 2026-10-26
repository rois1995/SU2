// DC-11 V diagnosis: TEST-ONLY diagnostics (no production code, no acceptance change).
//  [NativeVDiagDC11]       (Q4) DC-11 W -> R1 -> R2(single) -> V with the 2D split rule; every k-th D-floor refusal of a
//                          long-edge split is re-examined read-only: the refused fresh patch plus a ring of outside cells
//                          is optimized by the existing RepairMetricConnectivity (swaps only = empty movable set; swaps +
//                          vertex movement) and its minimum q/J is compared with the absolute gates (.20/.05) and with the
//                          relative floor min(old enlarged minimum, .20/.05); Admit8 (Refine, size insertion) is reported.
//                          Nothing of the experiment is applied: V continues exactly as in the DC-11 campaign.
//  [NativeVolumeOnlyDC11]  (Q3) the production Refine engine on the same cut input with no first-height constraint
//                          (empty wall_heights) and the D1-free BL target ("base"), the option-A target ("optionA") or the
//                          sensor alone ("sensor"); bounded time; its own stop/remaining counts and, for a mesh of at most
//                          30000 cells, the harness final gates with the same target.
namespace dc11diag {
using namespace dc11;
struct MinShape { double q=1,J=1; };
MinShape MinOf(State& s,const std::vector<Cell>& cells){MinShape m;for(const auto& c:cells){const auto sh=Shape(s,c);m.q=std::min(m.q,sh.q);m.J=std::min(m.J,sh.J);}return m;}
std::vector<Id> Neighbours(State& s,const std::set<Id>& in,const std::vector<Face>& faces) {
  std::set<Id> out;
  for(const auto& f:faces){const auto a=s.vertex_cells.find(f[0]),b=s.vertex_cells.find(f[1]),c=s.vertex_cells.find(f[2]);
    if(a==s.vertex_cells.end()||b==s.vertex_cells.end()||c==s.vertex_cells.end())continue;
    for(Id id:a->second)if(!in.count(id)&&b->second.count(id)&&c->second.count(id))out.insert(id);}
  return {out.begin(),out.end()};
}
std::string JS(const std::string& x){return junction_wp0::Json(x);}
}  // namespace dc11diag

TEST_CASE("DC-11 diagnosis: refused splits re-examined with a local swap/smooth", "[.][NativeVDiagDC11]") {
  using namespace dc11;using namespace dc11diag;
  struct RestorePolicy{RefineAcceptance a=GetRefineAcceptance();~RestorePolicy(){SetRefineAcceptance(a);dc9::splitPolicy=dc9::SplitPolicy{};dc11::floorHook=nullptr;}} restorePolicy;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC11_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC11_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC11_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC11_CASES");REQUIRE(only);
  const size_t samples=std::getenv("SU2_DC11_DIAG_SAMPLES")?std::stoul(std::getenv("SU2_DC11_DIAG_SAMPLES")):20;
  const size_t stride=std::getenv("SU2_DC11_DIAG_STRIDE")?std::stoul(std::getenv("SU2_DC11_DIAG_STRIDE")):5;
  const double vSeconds=std::getenv("SU2_DC11_V_SECONDS")?std::stod(std::getenv("SU2_DC11_V_SECONDS")):240;
  const bool interiorOnly=std::getenv("SU2_DC11_DIAG_INTERIOR")&&std::string(std::getenv("SU2_DC11_DIAG_INTERIOR"))=="1";
  unsetenv("SU2_DC9_SECTOR_FIRST");
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    splitPolicy=SplitPolicy{};splitPolicy.rule=1;SetRefineAcceptance(RefineAcceptance::SplitInsertion);
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;geometryHits=0;
    std::ostringstream log;const std::string tag=root+"/vdiag_"+name;
    std::unique_ptr<State> s;
    if(name=="annulus"){optionA=std::make_shared<OptionA>();s=LoadA1(fixtures);}else if(name.rfind("m6_",0)==0)s=LoadM6CutA(cuts+"/"+name+".txt");else s=LoadTubeA(fixtures+"/"+name+".txt");
    Rows10 rows;DetectCreases(*s);
    PhaseW10(*s,8192,120,log);PhaseR1(*s,rows,4096,120,log);
    PhaseR2(*s,static_cast<Rows&>(rows),600,log);
    std::cout<<"VDIAG "<<name<<" after R2 cells="<<s->mesh.size()<<std::endl;
    std::ofstream out(tag+(interiorOnly?"_interior":"")+".jsonl");out<<std::setprecision(6);
    size_t seen=0,taken=0;std::set<std::vector<Id>> done;
    // (a) every D-floor refusal: Admit8 checks height and measurement before the floor and, under the 2D rule, a size
    // insertion has no later condition, so a floor refusal with positive fresh q and J is refused by the floor alone
    std::map<std::string,std::array<size_t,6>> census;  // op -> {all, q>0&&J>0, new>=.10, new>=.05, old>=floor, requested>1.6}
    floorHook=[&](const std::string& op,const std::vector<Id>& ids,const Patch& old,const Patch& fresh,const Decision& d) {
      {auto& c=census[op];++c[0];c[1]+=d.other_q>0&&d.other_J>0;c[2]+=d.other_q>=.10&&d.other_J>=.05;c[3]+=d.other_q>=.05&&d.other_J>=.02;
       c[4]+=d.q_old>=MinimumMeanRatio&&d.J_old>=MinimumScaledJacobian;c[5]+=splitPolicy.requested>(long double)SplitInsertionLength;}
      if(taken>=samples||op.find("+repair")!=std::string::npos||op=="RepairMetricConnectivity"||op.rfind("SplitGrow",0)==0)return;
      if(interiorOnly&&!old.surface.empty())return;  // SU2_DC11_DIAG_INTERIOR=1: cavities without physical faces only
      if(!done.insert(std::vector<Id>(ids.begin(),ids.end())).second)return;
      if(seen++%stride)return;
      ++taken;const auto saved=splitPolicy;const long double requested=splitPolicy.requested;
      ColumnIndex ci;ci.Build(*s,rows);for(Id id:FanCells(*s,rows))ci.cells.insert(id);
      std::set<Id> frozen=rows.Apices();for(const auto& r:rows.lift){frozen.insert(r.first);frozen.insert(r.second);}
      // location of the refused split
      std::set<Id> oldV,freshV;for(const auto& c:old.cells)for(auto n:c.v)oldV.insert(n.id);for(const auto& c:fresh.cells)for(auto n:c.v)freshV.insert(n.id);
      std::vector<Id> added;for(Id x:freshV)if(!oldV.count(x))added.push_back(x);
      std::set<int> markers;for(const auto& f:old.surface)markers.insert(s->reference->Get(f.facet).marker);
      out<<"{\"case\":\""<<name<<"\",\"op\":"<<JS(op)<<",\"requested\":"<<double(requested)<<",\"star\":"<<ids.size()<<",\"fresh\":"<<fresh.cells.size()<<",\"new_points\":"<<added.size()
         <<",\"shape_measured\":true"
         <<",\"old_q\":"<<d.q_old<<",\"old_J\":"<<d.J_old<<",\"fresh_q\":"<<d.other_q<<",\"fresh_J\":"<<d.other_J<<",\"physical_markers\":[";
      {bool k=false;for(int m:markers){out<<(k?",":"")<<JS(s->markers.at(m));k=true;}}out<<"],\"levels\":[";
      // bad fresh children: faces not containing a new point are skin faces of the old star
      std::set<Id> in(ids.begin(),ids.end());std::vector<Face> badSkin,allSkin;
      for(const auto& c:fresh.cells){const auto sh=Shape(*s,c);for(auto f:CellFaces(c)){bool hasNew=false;for(Id x:f)for(Id y:added)hasNew=hasNew||x==y;if(hasNew)continue;allSkin.push_back(Sorted(f));if(sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian)badSkin.push_back(Sorted(f));}}
      std::vector<Id> ringBad=Neighbours(*s,in,badSkin),ring1=Neighbours(*s,in,allSkin);
      std::vector<Id> ring2=ring1;{std::set<Id> in1=in;in1.insert(ring1.begin(),ring1.end());std::vector<Face> fs;for(Id r:ring1)for(auto f:CellFaces(s->mesh.at(r).cell))fs.push_back(Sorted(f));for(Id x:Neighbours(*s,in1,fs))ring2.push_back(x);}
      const std::vector<std::pair<std::string,std::vector<Id>>> levels{{"ring_bad",ringBad},{"ring1",ring1},{"ring2",ring2}};
      bool firstLevel=true;
      for(const auto& level:levels) {
        std::vector<Id> ring;bool blocked=false;for(Id r:level.second){if(ci.cells.count(r)){blocked=true;continue;}ring.push_back(r);}
        for(int mode=0;mode<2;++mode) {
          out<<(firstLevel?"":",")<<"{\"level\":\""<<level.first<<"\",\"mode\":\""<<(mode?"swap+move":"swap")<<"\",\"ring\":"<<ring.size()<<",\"ring_column_cells_excluded\":"<<blocked;firstLevel=false;
          std::vector<Id> idsE=ids;idsE.insert(idsE.end(),ring.begin(),ring.end());
          if(idsE.size()>MaximumCavityCells||fresh.cells.size()+ring.size()>MaximumReplacementCells){out<<",\"skipped\":\"size\"}";continue;}
          try {
            Patch oldE=PatchOf(*s,idsE);Patch fe;fe.cells=fresh.cells;for(Id r:ring)fe.cells.push_back(s->mesh.at(r).cell);
            fe.surface=fresh.surface;if(!ring.empty()){const auto pr=PatchOf(*s,ring);fe.surface.insert(fe.surface.end(),pr.surface.begin(),pr.surface.end());}
            fe.parameters=fresh.parameters;for(const auto& p:oldE.parameters)fe.parameters.emplace(p.first,p.second);s->reference->ExportParameters(fe);
            const auto oldMin=MinOf(*s,oldE.cells),before=MinOf(*s,fe.cells);
            std::set<Id> movable;if(mode)for(const auto& c:fe.cells)for(auto n:c.v)if(!frozen.count(n.id))movable.insert(n.id);
            Patch rep;ConnectivityRepairReport report;std::string why;bool better=false;
            try{better=RepairMetricConnectivity(fe,*s->field,*s->reference,s->heights,s->next_cell+8192,rep,report,why,128,false,&movable);}catch(const std::exception& x){why=x.what();better=false;}
            if(better){for(size_t i=0;i<rep.cells.size();++i)rep.cells[i].id=s->next_cell+8192+i;try{s->reference->ExportParameters(rep);}catch(const std::exception& x){better=false;why=x.what();}}
            const auto after=better?MinOf(*s,rep.cells):before;
            const double relQ=std::min<double>(MinimumMeanRatio,oldMin.q),relJ=std::min<double>(MinimumScaledJacobian,oldMin.J);
            out<<",\"old_min_q\":"<<oldMin.q<<",\"old_min_J\":"<<oldMin.J<<",\"before_q\":"<<before.q<<",\"before_J\":"<<before.J<<",\"improved\":"<<better<<",\"after_q\":"<<after.q<<",\"after_J\":"<<after.J
               <<",\"absolute_ok\":"<<(after.q>=MinimumMeanRatio&&after.J>=MinimumScaledJacobian)<<",\"relative_ok\":"<<(after.q>=relQ&&after.J>=relJ)
               <<",\"flips\":"<<report.flips<<",\"moves\":"<<report.vertex_moves<<",\"removals\":"<<report.removals<<",\"trials\":"<<report.trials<<",\"why\":"<<JS(why.substr(0,160));
            if(better){splitPolicy.requested=requested;const auto a=Admit8(*s,oldE,rep,Kind::Refine);out<<",\"admit8\":"<<a.ok<<",\"admit8_reason\":"<<JS(a.reason.substr(0,120));}
          } catch(const std::exception& x){out<<",\"error\":"<<JS(std::string(x.what()).substr(0,160));}
          out<<"}";
        }
      }
      out<<"]}\n";out.flush();splitPolicy=saved;
      std::cout<<"VDIAG "<<name<<" sample "<<taken<<"/"<<samples<<std::endl;
    };
    const auto v=PhaseV11(*s,rows,1u<<20,vSeconds,64,false,log);floorHook=nullptr;
    std::cout<<"VDIAG "<<name<<" V "<<v.outcome<<" floor refusals seen(distinct stars)="<<seen<<" sampled="<<taken<<std::endl;
    out<<"{\"case\":\""<<name<<"\",\"census\":{";{bool k=false;for(const auto& c:census){out<<(k?",":"")<<JS(c.first)<<":["<<c.second[0]<<","<<c.second[1]<<","<<c.second[2]<<","<<c.second[3]<<","<<c.second[4]<<","<<c.second[5]<<"]";k=true;}}
    out<<"},\"census_columns\":\"all,positive q and J,q>=.10&J>=.05,q>=.05&J>=.02,old>=floor,requested>1.6\"}\n";
    out<<"{\"case\":\""<<name<<"\",\"summary\":true,\"distinct_floor_refusals\":"<<seen<<",\"sampled\":"<<taken<<",\"stride\":"<<stride<<",\"V_outcome\":\""<<v.outcome<<"\",\"V_seconds\":"<<vSeconds<<"}\n";
  }
}

TEST_CASE("DC-11 diagnosis: production Refine on the cut input without a first-height constraint", "[.][NativeVolumeOnlyDC11]") {
  using namespace dc11;using namespace dc11diag;
  struct Restore{RefineAcceptance a=GetRefineAcceptance();~Restore(){SetRefineAcceptance(a);}} restore;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC11_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC11_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC11_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC11_CASES");REQUIRE(only);
  const std::string target=std::getenv("SU2_DC11_VO_TARGET")?std::getenv("SU2_DC11_VO_TARGET"):"base";
  REQUIRE((target=="base"||target=="optionA"||target=="sensor"));
  const std::string accept=std::getenv("SU2_DC11_VO_ACCEPT")?std::getenv("SU2_DC11_VO_ACCEPT"):"split";
  REQUIRE((accept=="split"||accept=="strict"));
  const double seconds=std::getenv("SU2_DC11_VO_SECONDS")?std::stod(std::getenv("SU2_DC11_VO_SECONDS")):600;
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    SetRefineAcceptance(accept=="split"?RefineAcceptance::SplitInsertion:RefineAcceptance::StrictExcess);
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();
    const bool isM6=name.rfind("m6_",0)==0;
    auto s=isM6?LoadM6CutA(cuts+"/"+name+".txt"):LoadTubeA(fixtures+"/"+name+".txt");
    // the covering donors of the loaders (same sensor: M6 the cut centre node's sensor, tubes the uniform core sensor)
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
    MetricComposition composition;MetricEdgeBreakpoints edges;
    if(target=="base"){composition=SU2NativeBL3D::Composition(probeBase,s->core);edges=SU2NativeBL3D::EdgeGeometry(probeBase,s->core);}
    else if(target=="optionA"){composition=CompositionA(probeBase,probeFull,s->core,optionA);edges=EdgeGeometryA(probeBase,probeFull,s->core,optionA);}
    EngineOptions options;options.maximum_seconds=seconds;options.edge_breakpoints=edges;options.timings=false;
    {long double scale=0;for(const auto& f:s->facets)for(auto x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
     options.surface_tolerance=double(1e-13L*std::max(1.L,scale));}  // the mapped-authority error bound of dc7::MakeMapped
    const auto cells0=s->mesh.size();const auto start=Clock::now();
    // SU2_DC11_VO_PLAIN=1: the plain facet-local PlanarReference of the cut facets (no harness mapped-parameter overlay)
    const bool plain=std::getenv("SU2_DC11_VO_PLAIN")&&std::string(std::getenv("SU2_DC11_VO_PLAIN"))=="1";
    if(plain){s->reference=std::make_shared<PlanarReference>(s->facets);s->mapped=false;s->overlay.clear();options.surface_tolerance=0;}
    const auto r=Refine(world,s->mesh,donors,*s->reference,composition,options);
    const double used=Seconds(start);const auto& st=r.stats;
    std::ostringstream json;json<<std::setprecision(10)<<"{\"case\":\""<<name<<"\",\"target\":\""<<target<<"\",\"acceptance\":\""<<accept<<"\",\"wall_heights\":\"empty\",\"seconds_limit\":"<<seconds
      <<",\"input_cells\":"<<cells0<<",\"valid\":"<<r.valid<<",\"stop\":\""<<EngineStopName(st.stop)<<"\",\"reason\":"<<JS(r.reason.substr(0,400))<<",\"seconds\":"<<used
      <<",\"cells\":"<<r.cells.size()<<",\"global_work\":"<<st.global_work<<",\"work_limit\":"<<st.work_limit<<",\"accepted\":"<<st.accepted<<",\"rejected\":"<<st.rejected
      <<",\"size_progress\":"<<st.size_progress<<",\"quality_progress\":"<<st.quality_progress<<",\"inserted\":"<<st.inserted<<",\"moved\":"<<st.moved<<",\"reconnected\":"<<st.reconnected
      <<",\"repair_proposals\":"<<st.repair_proposals<<",\"repair_fallbacks\":"<<st.repair_fallbacks<<",\"fallback_accepted\":"<<st.fallback_accepted<<",\"recovery_starts\":"<<st.recovery_starts
      <<",\"metric_trials\":"<<st.metric_trials<<",\"metric_shape_rejected\":"<<st.metric_shape_rejected<<",\"metric_progress_rejected\":"<<st.metric_progress_rejected
      <<",\"metric_measurement_rejected\":"<<st.metric_measurement_rejected<<",\"geometry_trials\":"<<st.geometry_trials<<",\"geometry_rejected\":"<<st.geometry_rejected
      <<",\"oversized_stars\":"<<st.oversized_stars<<",\"remaining_edges\":"<<st.remaining_edges<<",\"remaining_repairs\":"<<st.remaining_repairs<<",\"cached_invalid_cells\":"<<st.cached_invalid_cells
      <<",\"last_metric_rejection\":{\"shape_measured\":"<<(st.last_metric_rejection.shape_measured?"true":"false")<<",\"q_new\":";
    if(st.last_metric_rejection.shape_measured)json<<st.last_metric_rejection.minimum_mean_ratio;else json<<"null";
    json<<"}";
    std::cout<<"VOLONLY "<<name<<" "<<target<<" valid="<<r.valid<<" stop="<<EngineStopName(st.stop)<<" cells="<<r.cells.size()<<" t="<<used<<" reason="<<r.reason.substr(0,200)<<std::endl;
    if(!r.cells.empty()&&r.cells.size()<=30000) {
      s->mesh=r.cells;if(r.reference)s->reference=r.reference;s->overlay.clear();s->heights=WallHeights{};s->lengths.clear();ResetCaches();
      s->field=std::make_unique<FrozenField>(donors,composition,false,std::vector<Id>{},edges);Index(*s);
      try{const auto g=Final(*s);json<<",\"final\":{\"cells\":"<<g.cells<<",\"shape_measured\":true,\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max\":"<<double(g.lmax)<<",\"long_edges\":"<<g.long_edges
        <<",\"long_interior\":"<<g.long_interior<<",\"long_other_physical\":"<<g.long_other_physical<<",\"long_wall\":"<<g.long_wall<<",\"bad_shape\":"<<g.bad_shape<<",\"unmeasurable\":"<<g.unmeasurable<<"}";
        std::cout<<"VOLONLY "<<name<<" final long="<<g.long_edges<<" bad="<<g.bad_shape<<" lmax="<<double(g.lmax)<<" qmin="<<g.q<<std::endl;}
      catch(const std::exception& x){json<<",\"final_error\":"<<JS(x.what());}
    }
    json<<",\"peak_rss_kB\":"<<PeakKB()<<"}\n";
    std::ofstream(root+"/volonly_"+name+"_"+target+"_"+accept+(plain?"_plain":"")+".json")<<json.str();
  }
}
