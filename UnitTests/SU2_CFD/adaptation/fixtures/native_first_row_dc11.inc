// DC-11: TEST-ONLY [NativeFirstRowDC11] harness (SPLIT_RULE_PLAN, user decision 2026-10-23: 3D split acceptance follows
// the 2D rule). It reuses DC-10 unchanged (option-A target, W, R1, R1w, R2 single | per-side + crease fan, ApexMove,
// ColumnSplit, final gates, export, fresh re-audit) and switches the per-operation size certificates by dc9::splitPolicy
// (rule 1 = 2D split rule; rule 0 = the DC-10 certificates, the A/B control) together with the production
// RefineAcceptance. V = PhaseV10 with the halves guard, a sweep cap and per-op/per-sweep work counts.
// Final gates are unchanged and absolute. Nothing here is production code.
namespace dc11 {
using namespace dc10;
constexpr long double HalfGuard=0.7L;
// Diagnostics only (V diagnosis, native_v_diag_dc11.inc): called on every D-floor refusal; unset = no effect.
std::function<void(const std::string&,const std::vector<Id>&,const Patch&,const Patch&,const Decision&)> floorHook;
struct SweepRow { size_t cells=0,long_edges=0,bad=0,height=0,work=0;double seconds=0; };
// ---- iteration 2: the 2D SplitPatch cavity growth in 3D (CNativeMesh2D.hpp SplitPatch ~548-628, consider()) ----------
// A split at p of an interior edge whose children fall under the shape target grows its cavity by face-adjacent
// interior cells (no physical face, outside columns/fan cells) and re-cones the cavity from p. Each step takes the
// candidate with the largest drop of the penalty sum(max(.22 - q, 0)) over the fresh cells plus the added old cells,
// whose fresh minimum q is not below min(current fresh minimum, q of the added cell) (2D consider()); it stops at fresh
// min q >= .22, no gain, 12 steps or 64 old cells. Candidates: cells across the skin faces of fresh cells with q < .22.
// Admission is the ordinary Refine (D.1 floors; an absorbed vertex keeps the strict excess decrease).
struct GrowStats { size_t calls=0,grown=0,trials=0,steps=0,no_view=0,no_gain=0,blocked=0; };
bool SplitGrow(State& s,Edge e,Point p,const std::set<Id>& blocked,std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why,GrowStats& gs) {
  Scope scope("V SplitGrow");++gs.calls;
  const auto star=Star(s,e);if(star.empty()||star.size()>MaximumCavityCells){why="grow: edge star exceeds 64";return false;}
  auto interior=[&](Id id){if(blocked.count(id))return false;const auto& r=s.mesh.at(id);for(unsigned k=0;k<4;++k)if(r.physical[k].present)return false;return true;};
  std::set<Id> cav(star.begin(),star.end());for(Id id:cav)if(!interior(id)){++gs.blocked;why="grow: physical or protected edge star";return false;}
  const Node point{s.next_node,p};
  auto cone=[&](const std::set<Id>& C,std::vector<Cell>& out)->bool{std::vector<Cell> in;for(Id id:C)in.push_back(s.mesh.at(id).cell);std::string w;out.clear();
    try{return Cone(in,point,s.next_cell,out,w);}catch(const std::exception&){return false;}};
  auto penalty=[&](const std::vector<Cell>& cs,double& mn,std::vector<double>* qs=nullptr){double pen=0;mn=1;if(qs)qs->clear();
    for(const auto& c:cs){const double q=layer2d::ShapeOf(s,c).q;pen+=std::max(.22-q,0.);mn=std::min(mn,q);if(qs)qs->push_back(q);}return pen;};
  std::vector<Cell> cur;if(!cone(cav,cur)){++gs.no_view;why="grow: the point does not see its edge star";return false;}
  std::vector<double> qs;double curMin=1;double curPen=penalty(cur,curMin,&qs);bool grown=false;
  for(unsigned step=0;step<12&&curMin<.22;++step) {
    std::set<Id> cand;
    for(size_t i=0;i<cur.size();++i){if(qs[i]>=.22)continue;const auto& c=cur[i];
      for(unsigned k=0;k<4;++k){std::array<Id,3> f;unsigned m=0;bool apex=false;for(unsigned j=0;j<4;++j)if(j!=k){f[m++]=c.v[j].id;}
        for(unsigned j=0;j<3;++j)apex=apex||f[j]==point.id;if(apex)continue;
        const auto a=s.vertex_cells.find(f[0]),b=s.vertex_cells.find(f[1]),d=s.vertex_cells.find(f[2]);if(a==s.vertex_cells.end()||b==s.vertex_cells.end()||d==s.vertex_cells.end())continue;
        for(Id id:a->second)if(!cav.count(id)&&b->second.count(id)&&d->second.count(id))cand.insert(id);}}
    double bestGain=1e-5,bestPen=0,bestMin=0;std::set<Id> bestCav;std::vector<Cell> bestCells;std::vector<double> bestQs;
    for(Id nb:cand) {
      if(!interior(nb)||cav.size()+1>MaximumCavityCells)continue;
      auto C2=cav;C2.insert(nb);std::vector<Cell> out;++gs.trials;if(!cone(C2,out))continue;
      if(out.size()>MaximumReplacementCells)continue;
      double mn=1;std::vector<double> q2;const double pen=penalty(out,mn,&q2);const double qa=MeshShape(s,nb).q;
      if(mn+1e-7<std::min(curMin,qa))continue;
      const double gain=curPen+std::max(.22-qa,0.)-pen;
      if(gain>bestGain){bestGain=gain;bestPen=pen;bestMin=mn;bestCav=C2;bestCells=out;bestQs=q2;}
    }
    if(bestCav.empty()){++gs.no_gain;break;}
    cav=bestCav;cur=bestCells;qs=bestQs;curPen=bestPen;curMin=bestMin;grown=true;++gs.steps;
  }
  if(!grown){why="grow: no improving enlargement";return false;}
  ++gs.grown;ids.assign(cav.begin(),cav.end());
  try{old=PatchOf(s,ids);fresh=Patch{};fresh.cells=cur;fresh.surface=old.surface;fresh.parameters=old.parameters;s.reference->ExportParameters(fresh);}
  catch(const std::exception& x){why=std::string("grow: ")+x.what();return false;}
  return true;
}
struct VStage11 : VStage { GrowStats grow;size_t stale_skips=0,halves_guard=0;std::map<std::string,size_t> acceptedBy,floorBuckets;std::vector<SweepRow> per_sweep; };
VStage11 PhaseV11(State& s,Rows10& rows,size_t work,double seconds,size_t maxSweeps,bool grow,std::ostream& log) {
  Scope scope("PhaseV");
  VStage11 g;const bool rule2d=splitPolicy.rule==1;std::set<Id> apexIds;Begin(s,g);const auto t0=Clock::now();
  auto limited=[&]{return g.work>=work||Seconds(t0)>=seconds;};
  struct Reset{~Reset(){splitPolicy.requested=-1;splitPolicy.relocated=nullptr;splitPolicy.relocatedCount=0;}} resetPolicy;
  std::set<std::vector<Id>> rejected;
  auto keyOf=[&](Id tag,const std::vector<Id>& head,const std::vector<Id>& ids){std::vector<Id> k{tag};k.insert(k.end(),head.begin(),head.end());k.push_back(UINT64_MAX);k.insert(k.end(),ids.begin(),ids.end());std::sort(k.begin()+head.size()+2,k.end());return k;};
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    ++g.work;const auto d=Admit8(s,old,fresh,kind);
    if(!d.ok){++g.rejected;++g.reasons[op+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=op+": "+d.reason;
      if(floorHook&&d.reason.rfind("D floor",0)==0)floorHook(op,ids,old,fresh,d);
      if(d.reason.rfind("D floor",0)==0){  // witness: was the old cavity above the floor (absolute refusal) and how low is the new cell
        const bool abs=d.q_old>=MinimumMeanRatio&&d.J_old>=MinimumScaledJacobian;const double q=d.other_q;
        ++g.floorBuckets[std::string(abs?"old>=floor":"old<floor")+(q<.05?" new q<.05":q<.1?" new q<.10":q<.15?" new q<.15":q<.2?" new q<.20":" new J only")];}
      return false;}
    Apply(s,ids,fresh);++g.accepted;++g.acceptedBy[op];return true;};
  ColumnIndex ci;
  auto build=[&]{ci.Build(s,rows);for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;
    apexIds.clear();for(const auto& r:ci.apexFace)apexIds.insert(r.first);splitPolicy.relocated=&apexIds;};
  // DC-11 halves guard (rule 1): a split is requested only if its two halves cannot both be < 0.7, i.e. the split edge (for a
  // ColumnSplit the longer of the wall edge and its lid edge) is >= 1.4 (metric length is additive along a segment).
  auto columnSplit=[&](Edge w,std::string& why)->bool{
    if(rule2d){long double lw=INFINITY,ll=INFINITY;try{lw=Len(s,w[0],w[1]);}catch(const std::exception&){}
      const auto la=rows.lift.find(w[0]),lb=rows.lift.find(w[1]);
      if(la!=rows.lift.end()&&lb!=rows.lift.end()){try{ll=Len(s,std::min(la->second,lb->second),std::max(la->second,lb->second));}catch(const std::exception&){}}else ll=0;
      const long double l=std::max(lw,ll);  // an unmeasurable edge (INFINITY) never triggers the guard
      if(l<2*HalfGuard){++g.halves_guard;why="ColumnSplit: halves guard (split edges < 1.4)";return false;}
      splitPolicy.requested=l;}
    struct Clear{~Clear(){splitPolicy.requested=-1;}} clear;
    const bool ok=ColumnSplit(s,rows,w,g,why,rejected);if(ok)++g.acceptedBy["ColumnSplit"];return ok;};
  auto ridgeEdge=[&](Edge w){if(rows.copies.count(w[0])||rows.copies.count(w[1]))return true;
    for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))for(Id x:r.first)if(rows.copies.count(x))return true;return false;};
  for(;;++g.sweeps) {
    if(limited()){g.outcome="limit";break;}
    if(g.sweeps>=maxSweeps){g.outcome="sweep cap";break;}
    build(); bool changed=false;g.column_demand=0;g.demand.clear();
    auto touchesColumn=[&](const std::vector<Id>& ids){for(Id id:ids)if(ci.cells.count(id))return true;return false;};
    // wall-first: wall/seam edges, other physical, interior; longest first in each class
    std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;
    for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
      const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l>MaximumMetricEdge){const int k=EdgeClass(s,e);longs.emplace_back(k==1||k==2?0:k==3?1:2,-l,e);}}}
    std::sort(longs.begin(),longs.end());
    std::set<Face> movedThisSweep;
    for(const auto& row:longs) {
      if(limited())break;const Edge e=std::get<2>(row);const auto star=Star(s,e);if(star.empty())continue;
      if(touchesColumn(star)) {
        // route column demand: lid/lateral edge of a wall edge (a,b) -> ColumnSplit; apex edge -> ApexMove, then ColumnSplit
        auto base=[&](Id x)->Id{const auto u=ci.unlift.find(x);if(u!=ci.unlift.end())return u->second;return rows.lift.count(x)?x:UINT64_MAX;};
        const Id b0=base(e[0]),b1=base(e[1]);bool done=false;std::string why;
        Face apexKey{};bool apex=false;for(Id x:e){const auto f=ci.apexFace.find(x);if(f!=ci.apexFace.end()){apexKey=f->second;apex=true;}}
        if(apex){if(!movedThisSweep.count(apexKey)){movedThisSweep.insert(apexKey);const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
            if(rejected.count(k)){++g.cache_skips;++perf.cache_skips;why="cached rejection (unchanged cavity)";}else{done=ApexMove(s,rows,apexKey,g,why);if(!done)rejected.insert(k);}}
          if(!done){long double best=-1;Edge w{0,0};for(int i=0;i<3;++i){const Id x=apexKey[i],y=apexKey[(i+1)%3];long double l=0;try{l=Len(s,x,y);}catch(const std::exception&){l=INFINITY;}if(l>best){best=l;w={std::min(x,y),std::max(x,y)};}}
            if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}}
        else if(b0!=UINT64_MAX&&b1!=UINT64_MAX&&b0!=b1){const Edge w{std::min(b0,b1),std::max(b0,b1)};if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}
        else why="column demand without a column route";
        if(done){changed=true;build(); }else{++g.column_demand;++g.demand[Key(why)];g.last="column demand: "+why;}
        continue;
      }
      const auto key=keyOf(1,{e[0],e[1]},star);if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;continue;}
      // the sweep-start length can be stale (an earlier proposal may have moved a retained vertex): remeasure
      long double now=-std::get<1>(row);try{now=Len(s,e[0],e[1]);}catch(const std::exception&){now=INFINITY;}
      if(!(now>MaximumMetricEdge)){++g.stale_skips;continue;}
      if(rule2d&&now<2*HalfGuard){++g.halves_guard;continue;}
      splitPolicy.requested=rule2d?now:-1;
      bool taken=false;
      size_t variantIndex=0;
      for(auto& variant:RefineVariantsOf(s,e,g)) {
        const size_t vi=variantIndex++;splitPolicy.relocatedCount=0;
        if(limited())break;std::vector<Id> ids;Patch old,fresh;std::string reason;bool built=false;
        try{built=variant.second(ids,old,fresh,reason);}catch(const std::exception& x){reason=x.what();built=false;}
        // wall variants 1 (RefitWallSplit) and 2 (PlaceSplitPatch) give each moved apex a new identity from next_node+1
        // on, one for one (the split point is next_node): certified relocations, as the production engine does
        if(built&&variant.first=="wall"&&(vi==1||vi==2)){std::set<Id> before;for(const auto& c:old.cells)for(auto n:c.v)before.insert(n.id);
          std::set<Id> moved;for(const auto& c:fresh.cells)for(auto n:c.v)if(n.id>=s.next_node+1&&!before.count(n.id))moved.insert(n.id);splitPolicy.relocatedCount=moved.size();}
        if(!built){++g.work;++g.rejected;++g.reasons["Refine build: "+Key(reason)];continue;}
        if(touchesColumn(ids)){++g.rejected;++g.reasons["Refine: protected column"];continue;}
        Patch repaired;ConnectivityRepairReport report;std::string why;bool better=false;
        {Scope sc("V RepairMetricConnectivity(fresh)");try{better=RepairMetricConnectivity(fresh,*s.field,*s.reference,s.heights,s.next_cell+4096,repaired,report,why,128,false);}catch(const std::exception&){better=false;}}
        if(better){for(size_t i=0;i<repaired.cells.size();++i)repaired.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(repaired);}catch(const std::exception&){better=false;}}
        {Scope sc("V Admit");if(better&&attempt(variant.first+"+repair",Kind::Refine,ids,old,repaired)){taken=true;break;}
         if(attempt(variant.first,Kind::Refine,ids,old,fresh)){taken=true;break;}}
      }
      splitPolicy.relocatedCount=0;
      if(!taken&&grow&&!limited()) {  // iteration 2: 2D SplitPatch cavity growth (SplitGrow), interior edges only
        bool junction=false;const bool physical=SameChartJunction(s,e,junction);
        if(!physical){const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);Point p{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
          try{p=s.field->BalancedSplitPoint(a,b);}catch(const std::exception&){}
          std::vector<Id> ids;Patch old,fresh;std::string why;bool built=false;++g.work;
          try{built=SplitGrow(s,e,p,ci.cells,ids,old,fresh,why,g.grow);}catch(const std::exception& x){why=x.what();built=false;}
          if(!built){++g.rejected;++g.reasons["SplitGrow build: "+Key(why)];}
          else{Patch repaired;ConnectivityRepairReport report;std::string w;bool better=false;
            {Scope sc("V RepairMetricConnectivity(fresh)");try{better=RepairMetricConnectivity(fresh,*s.field,*s.reference,s.heights,s.next_cell+4096,repaired,report,w,128,false);}catch(const std::exception&){better=false;}}
            if(better){for(size_t i=0;i<repaired.cells.size();++i)repaired.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(repaired);}catch(const std::exception&){better=false;}}
            Scope sc("V Admit");if(better&&attempt("SplitGrow+repair",Kind::Refine,ids,old,repaired))taken=true;else if(attempt("SplitGrow",Kind::Refine,ids,old,fresh))taken=true;}}}
      splitPolicy.requested=-1;
      if(!taken)rejected.insert(key);
      changed|=taken;if(taken)build(); 
    }
    // bad cells: column cells -> ApexMove; others -> RepairMetricConnectivity outside columns (as DC-7)
    std::vector<std::pair<double,Id>> bad;for(const auto& row:s.mesh){const auto sh=MeshShape(s,row.first);if(Bad(sh))bad.emplace_back(sh.q,row.first);}
    std::sort(bad.begin(),bad.end());
    for(const auto& row:bad) {
      if(limited())break;if(!s.mesh.count(row.second))continue;
      if(ci.cells.count(row.second)){Face apexKey{};bool found=false;for(auto n:s.mesh.at(row.second).cell.v){const auto f=ci.apexFace.find(n.id);if(f!=ci.apexFace.end()){apexKey=f->second;found=true;}}
        if(!found||movedThisSweep.count(apexKey))continue;movedThisSweep.insert(apexKey);
        const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
        if(rejected.count(k)){++g.cache_skips;++perf.cache_skips;continue;}std::string why;
        if(ApexMove(s,rows,apexKey,g,why)){changed=true;build(); }else{rejected.insert(k);++g.demand["bad column cell: "+Key(why)];}
        continue;}
      std::set<Id> star;for(auto n:s.mesh.at(row.second).cell.v)for(Id id:StarOf(s,n.id))if(!ci.cells.count(id))star.insert(id);
      if(star.size()>MaximumCavityCells){star.clear();const auto& c=s.mesh.at(row.second).cell;for(Id id:Star(s,{std::min(c.v[0].id,c.v[1].id),std::max(c.v[0].id,c.v[1].id)}))if(!ci.cells.count(id))star.insert(id);}
      if(star.empty()||star.size()>MaximumCavityCells){++g.work;++g.rejected;++g.reasons["Repair build: star exceeds 64"];continue;}
      std::vector<Id> ids(star.begin(),star.end());const auto key=keyOf(2,{row.second},ids);if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;continue;}
      Patch old,fresh;std::string reason;ConnectivityRepairReport report;
      std::set<Id> movable,frozen=rows.Apices();for(const auto& r:rows.lift){frozen.insert(r.first);frozen.insert(r.second);}
      for(Id id:ids)for(auto n:s.mesh.at(id).cell.v)if(!frozen.count(n.id))movable.insert(n.id);
      bool ok=false;{Scope sc("V RepairMetricConnectivity(bad)");try{old=PatchOf(s,ids);ok=RepairMetricConnectivity(old,*s.field,*s.reference,s.heights,s.next_cell,fresh,report,reason,64,true,&movable);}catch(const std::exception& x){reason=x.what();}}
      if(!ok){++g.work;++g.rejected;++g.reasons["Repair build: "+Key(reason)];rejected.insert(key);continue;}
      // production repair moves vertices in place and may keep cell identities: fresh identities keep every cache valid
      for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
      bool acc=false;{Scope sc("V Admit");acc=attempt("RepairMetricConnectivity",Kind::Repair,ids,old,fresh);}
      if(acc){changed=true;build(); }else rejected.insert(key);
    }
    const auto gates=Final(s);
    log<<"DC11 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" column_demand="<<g.column_demand<<" work="<<g.work<<" cache_skips="<<g.cache_skips<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC11 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" work="<<g.work<<" t="<<Seconds(t0)<<std::endl;
    g.per_sweep.push_back({gates.cells,gates.long_edges,gates.bad_shape,gates.height_failures,g.work,Seconds(t0)});
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
}  // namespace dc11

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
