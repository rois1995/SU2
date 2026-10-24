/*!
 * \file BLSplit11.inl
 * \brief 2D split rule pieces of DC-11 (split growth, V11).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc11.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// DC-11: (history: formerly test-only) [NativeFirstRowDC11] harness (SPLIT_RULE_PLAN, user decision 2026-10-23: 3D split acceptance follows
// the 2D rule). It reuses DC-10 unchanged (option-A target, W, R1, R1w, R2 single | per-side + crease fan, ApexMove,
// ColumnSplit, final gates, export, fresh re-audit) and switches the per-operation size certificates by dc9::splitPolicy
// (rule 1 = 2D split rule; rule 0 = the DC-10 certificates, the A/B control) together with the production
// RefineAcceptance. V = PhaseV10 with the halves guard, a sweep cap and per-op/per-sweep work counts.
// Final gates are unchanged and absolute. (DC-14: production.)
namespace dc11 {
using namespace dc10;
constexpr long double HalfGuard=0.7L;
// Diagnostics only (V diagnosis, native_v_diag_dc11.inc): called on every D-floor refusal; unset = no effect.
inline std::function<void(const std::string&,const std::vector<Id>&,const Patch&,const Patch&,const Decision&)> floorHook;
struct SweepRow { size_t cells=0,long_edges=0,bad=0,height=0,work=0;double seconds=0; };
// ---- iteration 2: the 2D SplitPatch cavity growth in 3D (CNativeMesh2D.hpp SplitPatch ~548-628, consider()) ----------
// A split at p of an interior edge whose children fall under the shape target grows its cavity by face-adjacent
// interior cells (no physical face, outside columns/fan cells) and re-cones the cavity from p. Each step takes the
// candidate with the largest drop of the penalty sum(max(.22 - q, 0)) over the fresh cells plus the added old cells,
// whose fresh minimum q is not below min(current fresh minimum, q of the added cell) (2D consider()); it stops at fresh
// min q >= .22, no gain, 12 steps or 64 old cells. Candidates: cells across the skin faces of fresh cells with q < .22.
// Admission is the ordinary Refine (D.1 floors; an absorbed vertex keeps the strict excess decrease).
struct GrowStats { size_t calls=0,grown=0,trials=0,steps=0,no_view=0,no_gain=0,blocked=0; };
inline bool SplitGrow(State& s,Edge e,Point p,const ColumnCellSet& blocked,std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why,GrowStats& gs) {
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
inline VStage11 PhaseV11(State& s,Rows10& rows,size_t work,double seconds,size_t maxSweeps,bool grow,std::ostream& log) {
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
  auto build=[&]{const bool fresh=ci.Build(s,rows);if(!rows.fan.empty())for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;
    if(fresh){apexIds.clear();for(const auto& r:ci.apexFace)apexIds.insert(r.first);}splitPolicy.relocated=&apexIds;};
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
