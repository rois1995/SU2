/*!
 * \file BLGrowth12.inl
 * \brief BL growth layers G, validity-only admission Admit12, column directions with the symmetry blend, certified first-row routes, V12 (ex DC-12).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// DC-12: (history: formerly test-only) [NativeFirstRowDC12] harness (OPTIMIZE_PLAN rev 3): build-then-optimize. It reuses DC-11 (option-A
// target, W10, R1, R2 single lift, ColumnSplit/ApexMove, halves guard, rejection cache, final gates, export, re-audit) and
// adds, as one connected step: (1) validity-only construction (production ShapeAdmission::ValidityOnly, dc9 shapePolicy for
// the R2/D.2/ConeColumn first-child checks, Admit12 for every other admission: every fresh cell q > 0 and J >= 1e-3, height
// T, facet/marker/L-C, caps, the SplitPolicy rule-1 progress rules); (2) the BL growth-layer operator G (column lifts at the
// production BL law heights d' = h0 + g d of the column's own wall); (3) the certified first-child route FirstRowInsert and
// the R2 retry; (4) V12 = bounded refinement + ApexMove + the production mesh-wide optimizer OptimizeMetricMesh. Final gates
// are unchanged and absolute. (DC-14: production.)
#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
namespace dc12 {
using namespace dc11;
inline std::string Fmt(double x){std::ostringstream o;o<<std::setprecision(6)<<x;return o.str();}

// Construction admission uses topology, exact geometric positivity, wall height and frozen edge size only.
inline Decision Admit12(State& s,const Patch& old,const Patch& fresh,Kind kind) {
  Scope scope("Admit12");
  Decision base;base.old_cells=old.cells.size();base.fresh_cells=fresh.cells.size();
  if(old.cells.empty()||fresh.cells.empty()||old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){base.category=5;base.reason="replacement size";return base;}
  std::string why;FirstHeightReport hr;if(!(validityOnlyConstruction?HeightConstruct(fresh,s.heights,*s.reference,&hr,&why):HeightT(s,fresh,why))){base.category=4;base.reason=why;return base;}
  for(const auto& c:fresh.cells)if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){base.category=2;base.reason="fresh cell has nonpositive exact orientation";return base;}
  if(validityOnlyConstruction&&splitPolicy.rule==1&&(kind==Kind::Layer||(kind==Kind::Refine&&splitPolicy.requested>(long double)SplitInsertionLength))
     &&!RemovesVertex(old.cells,fresh.cells,splitPolicy.relocated,splitPolicy.relocatedCount)) {
    if(kind==Kind::Layer)++splitPolicy.layer_free;else ++splitPolicy.size_insertions;base.ok=true;return base;
  }
  PatchMetric after,before;
  try{auto reuse=ReuseOf(s,old.cells,&fresh.cells);
    after=MeasureMetricEdges(fresh.cells,*s.field,nullptr,&reuse);StoreLengths(s,after);
    for(const auto& e:after.edges)reuse.insert_or_assign(e.first,e.second);
    before=MeasureMetricEdges(old.cells,*s.field,nullptr,&reuse);StoreLengths(s,before);}
  catch(const std::exception& e){base.category=5;base.reason=e.what();return base;}
  auto shared=[](const std::map<Edge,EdgeMeasure>& others,const std::pair<const Edge,EdgeMeasure>& e){
    const auto o=others.find(e.first);return o!=others.end()&&SamePoint(o->second.a,e.second.a)&&SamePoint(o->second.b,e.second.b);};
  for(const auto& e:before.edges)if(!shared(after.edges,e))base.removed+=layer2d::ExcessOf(e.second.length);
  for(const auto& e:after.edges)if(!shared(before.edges,e))base.added+=layer2d::ExcessOf(e.second.length);
  const bool size_progress=base.removed>0&&base.added<base.removed*(1-1e-12L);
  if(kind==Kind::Refine||kind==Kind::Layer) {
    const bool removes=RemovesVertex(old.cells,fresh.cells,splitPolicy.relocated,splitPolicy.relocatedCount);
    if(kind==Kind::Layer){if(!removes||base.added<=base.removed){++splitPolicy.layer_free;base.ok=true;return base;}
      base.category=3;base.reason="layer removes a vertex and adds frozen excess";return base;}
    if(removes){if(size_progress){++splitPolicy.vertex_removal_checks;base.ok=true;return base;}
      ++splitPolicy.vertex_removal_refusals;base.category=3;base.reason="refine removes a vertex without reducing changed-edge excess";return base;}
    long double longest=splitPolicy.requested;
    if(longest<0){longest=0;for(const auto& e:before.edges)if(!shared(after.edges,e))longest=std::max(longest,e.second.length);}
    if(longest>(long double)SplitInsertionLength){++splitPolicy.size_insertions;base.ok=true;return base;}
    ++splitPolicy.nonsize_checks;bool interiorOk=base.added==0;
    if(interiorOk){std::set<Edge> in;try{in=InteriorEdges(fresh.cells);}catch(const std::exception&){interiorOk=false;}
      for(const auto& e:after.edges)if(in.count(e.first)&&e.second.length>MaximumMetricEdge)interiorOk=false;}
    if(interiorOk){base.ok=true;return base;}
    ++splitPolicy.nonsize_refusals;base.category=3;base.reason="short refine adds an oversized interior edge";return base;
  }
  if(kind==Kind::Repair){if(size_progress||base.added<=base.removed){base.ok=true;return base;}
    base.category=3;base.reason="repair adds frozen edge excess";return base;}
  base.category=3;base.reason="unknown kind";return base;
}

// ---- wall policy of a column (production layers, per wall) -------------------------------------------------------------
inline std::map<int,unsigned short> layerOfMarker;
inline void MapLayers(State& s){layerOfMarker.clear();for(int m:s.walls)for(unsigned short w=0;w<s.layers->GetnWall();++w)if(s.layers->GetLayer(w).marker==s.markers.at(m))layerOfMarker[m]=w;}
inline long double WallDistanceTo(State& s,unsigned short w,V x){const su2double X[3]={double(x.x),double(x.y),double(x.z)};return SU2_TYPE::GetValue(s.layers->Evaluate(w,X,s.core).distance);}
inline long double MinWallDistance(State& s,V x){long double d=INFINITY;for(unsigned short w=0;w<s.layers->GetnWall();++w)d=std::min(d,WallDistanceTo(s,w,x));return d;}
inline long double FirstHeightOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).firstHeight);}
inline long double GrowthOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).growth);}
inline long double ThicknessOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).thickness);}
// Per-marker first heights from the matching production wall policy (OPTIMIZE_PLAN sect. 3); s.h0 = their minimum.
inline void ApplyWallPolicies(State& s) {
  MapLayers(s);long double low=INFINITY;
  for(const auto& m:layerOfMarker){const auto h=FirstHeightOf(s,m.second);s.heights[m.first]=double(h);low=std::min(low,h);}
  if(std::isfinite(low))s.h0=double(low);s.heights.tolerance=HeightTolerance;
}

// ---- column directions (OPTIMIZE_PLAN rev 5): boundary tangency n' = n - (n.s) s, smooth blend, visibility ---------
struct DirField { std::map<Id,V> d;size_t vertices=0,constrained=0,lines=0,blended=0,fallback=0,witness=0,visibility_reduced=0,kept=0,low_margin=0;
  long double min_margin=INFINITY,max_turn_deg=0;std::vector<std::string> witnesses; };
inline long double SmoothStep(long double u){u=std::clamp(u,0.L,1.L);return u*u*(3-2*u);}
inline DirField ComputeDirectionField(State& s,long double margin=.2L) {
  Scope scope("Column directions");MapLayers(s);DirField F;
  struct Info{V x,n,c;std::vector<V> normals,basis;bool constrained=false;long double D=INFINITY,edge=0;};
  std::map<Id,Info> info;
  for(Id v:WallVertices(s)){Info I;I.x=Of(NodePoint(s,v));const auto star=StarOf(s,v);
    for(const auto& f:PhysicalOf(s,star,true))if(Has(f.key,v)){const V n=-1.L*Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));
      bool dup=false;for(const auto& x:I.normals)dup=dup||Dot(x,n)>1-1e-14L;if(!dup)I.normals.push_back(n);
      const auto it=layerOfMarker.find(f.marker);if(it!=layerOfMarker.end())I.D=std::min(I.D,ThicknessOf(s,it->second));}
    for(const auto& f:PhysicalOf(s,star,false))if(Has(f.key,v)){V r=Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));
      for(const auto& b:I.basis)r=r-Dot(r,b)*b;if(Norm(r)>1e-9L)I.basis.push_back(Unit(r));}
    {const auto nb=WallNeighbours(s,v);long double sum=0;for(Id w:nb)sum+=Norm(Of(NodePoint(s,w))-I.x);I.edge=nb.empty()?0:sum/nb.size();}
    if(I.normals.empty()||!MinNormAny(I.normals,I.n)){++F.witness;if(F.witnesses.size()<8)F.witnesses.push_back("vertex "+std::to_string(v)+": no open hemisphere");continue;}
    info.emplace(v,I);}
  F.vertices=info.size();
  for(auto& r:info){auto& I=r.second;if(I.basis.empty())continue;V d;
    if(I.basis.size()==1){const V sn=I.basis[0];d=I.n-Dot(I.n,sn)*sn;
      if(Norm(d)<1e-3L){++F.witness;if(F.witnesses.size()<8)F.witnesses.push_back("vertex "+std::to_string(r.first)+": wall almost parallel to its boundary plane");
        V m{};for(Id w:WallNeighbours(s,r.first)){const V e=Of(NodePoint(s,w))-I.x;m=m+(e-Dot(e,sn)*sn);}  // mirrored first ring: in-plane part
        if(!(Norm(m)>1e-12L*(1+I.edge)))continue;d=m;++F.fallback;}}
    else if(I.basis.size()==2){const V t=Unit(Cross(I.basis[0],I.basis[1]));d=Dot(t,I.n)<0?-1.L*t:t;++F.lines;}
    else{++F.witness;if(F.witnesses.size()<8)F.witnesses.push_back("vertex "+std::to_string(r.first)+": three boundary planes");continue;}
    I.c=Unit(d);I.constrained=true;++F.constrained;}
  std::vector<const Info*> sources;for(const auto& r:info)if(r.second.constrained)sources.push_back(&r.second);
  auto project=[](const Info& c,V n)->V{if(c.basis.size()==1)return n-Dot(n,c.basis[0])*c.basis[0];const V t=Unit(Cross(c.basis[0],c.basis[1]));return Dot(n,t)*t;};
  auto visibility=[](const Info& I,V d){long double m=INFINITY;for(const auto& n:I.normals)m=std::min(m,Dot(n,d));return m;};
  // sources grouped by their constraint (plane normal, or the two normals of a line), sign-normalized: every group
  // contributes continuously through its nearest source, so the blend never switches abruptly between planes
  auto keyOf=[](const Info& c){std::vector<long long> k;for(const auto& b:c.basis){V n=b;const long double lead=std::abs(n.x)>1e-9L?n.x:std::abs(n.y)>1e-9L?n.y:n.z;if(lead<0)n=-1.L*n;
      for(long double x:{n.x,n.y,n.z})k.push_back(std::llround(x*1e6L));}return k;};
  std::map<std::vector<long long>,std::vector<const Info*>> groups;for(const auto* c:sources)groups[keyOf(*c)].push_back(c);
  for(auto& r:info){auto& I=r.second;V d=I.n;bool use=true;
    if(I.constrained){d=I.c;if(visibility(I,d)<margin){++F.witness;++F.low_margin;use=false;
        if(F.witnesses.size()<8)F.witnesses.push_back("vertex "+std::to_string(r.first)+": tangent direction sees an incident face below the margin");}}
    else {
      V acc{};long double W=0;
      for(const auto& g:groups){const Info* c=nullptr;long double best=INFINITY;for(const auto* x:g.second){const auto dist=Norm(I.x-x->x);if(dist<best){best=dist;c=x;}}
        const long double D=std::min(I.D,3*std::max(c->edge,1e-300L));  // BL thickness, at most three wall rings of the source
        if(!(best<D))continue;const V pn=project(*c,I.n);if(!(Norm(pn)>1e-12L))continue;const long double w=SmoothStep(1-best/D);acc=acc+w*(Unit(pn)-I.n);W+=w;}
      if(W>0){const long double scale=1/std::max(1.L,W);long double lam=1;bool ok=false;
        for(int k=0;k<11;++k){const V cand=Unit(I.n+(lam*scale)*acc);if(visibility(I,cand)>=margin){d=cand;ok=true;if(k)++F.visibility_reduced;break;}lam*=.5L;}
        if(ok)++F.blended;else ++F.kept;}
      if(visibility(I,d)<margin){++F.low_margin;use=false;}}
    if(!use)continue;
    F.min_margin=std::min(F.min_margin,visibility(I,d));
    F.max_turn_deg=std::max(F.max_turn_deg,std::acos(std::clamp(Dot(d,I.n),-1.L,1.L))*180/std::acos(-1.L));
    F.d[r.first]=d;}
  return F;
}
inline std::string JsonDirections(const DirField& F){std::ostringstream o;o<<std::setprecision(6)<<"{\"wall_vertices\":"<<F.vertices<<",\"constrained_plane\":"<<F.constrained-F.lines<<",\"constrained_line\":"<<F.lines
  <<",\"blended\":"<<F.blended<<",\"blend_kept_unconstrained\":"<<F.kept<<",\"visibility_reduced\":"<<F.visibility_reduced<<",\"fallback_mirrored_ring\":"<<F.fallback<<",\"witnesses\":"<<F.witness
  <<",\"below_margin_0.2\":"<<F.low_margin<<",\"min_visibility\":"<<double(F.min_margin)<<",\"max_turn_deg\":"<<double(F.max_turn_deg)<<",\"uses_in_DirectionAt\":"<<directionFieldUses<<",\"witness_list\":[";
  for(size_t i=0;i<F.witnesses.size();++i)o<<(i?",":"")<<junction_wp0::Json(F.witnesses[i]);o<<"]}";return o.str();}

// ---- (2) BL growth layers ---------------------------------------------------------------------------------------------
struct Column { Id wall=0,top=0,lid=0;V dir;std::set<unsigned short> walls;std::vector<Id> stack;bool stopped=false;std::string stop;
  unsigned refusals=0;long double stopHeight=INFINITY,lidDistance=0;int owner=-1; };
struct Growth {
  std::vector<Column> columns;std::set<std::array<Id,4>> layerKeys;std::map<std::string,size_t> stops,refusals,accepted;
  size_t transactions=0,patch_tx=0,single_tx=0,lifted=0,refused=0,layers=0,max_old=0,max_fresh=0,work=0;double seconds=0;std::string outcome,last;
  std::map<Id,Id> current;  // stack vertex identity -> its identity now (optimizer moves)
  std::set<Id> forceStop;   // tests: wall vertices whose column never grows
  // DC-15 ClearAbove: clearance refusals answered by relocating/removing free blocking vertices, then a retry
  size_t clear_moves=0,clear_collapses=0,clear_retries=0,clear_retry_accepted=0;std::map<std::string,size_t> clear_refusals;
  std::map<std::string,size_t> fixed_blockers;  // DC-16: kind of every registry identity named as a clearance blocker
  size_t joint_retries=0,joint_accepted=0;std::map<std::string,size_t> stop_reasons;      // DC-16: transactions widened by the planned tops they depend on
};
struct Step { bool ok=false;std::string stop;V u;long double d=0,next=0;int wall=-1; };
// Target metric density along the unit direction d at x, and the distance along d from x0 at which the target metric
// length reaches `units` (trapezoid on point tensors, substeps of 1/8 of the local size, linear inverse in the last one).
inline long double Density(State& s,V x,V d){const Tensor m=s.field->Query(Pt(x)).target;
  return std::sqrt(d.x*(m.xx*d.x+m.xy*d.y+m.xz*d.z)+d.y*(m.xy*d.x+m.yy*d.y+m.yz*d.z)+d.z*(m.xz*d.x+m.yz*d.y+m.zz*d.z));}
inline long double UnitStep(State& s,V x0,V d,long double units=1) {
  long double lam=0,L=0,r0=Density(s,x0,d);
  for(int i=0;i<400;++i){if(!(r0>0))return -1;const long double h=.125L/r0,r1=Density(s,x0+(lam+h)*d,d),inc=h*(r0+r1)/2;
    if(L+inc>=units)return lam+h*(units-L)/inc;L+=inc;lam+=h;r0=r1;}
  return -1;
}
// Next layer of a column (OPTIMIZE_PLAN sect. 2, rev 5): one unit step of the target metric along the column, i.e. the row
// height follows the target's wall-normal size hn(d) (the production law h0 g^k, or its configured ADAP_BL_BUDGET outer
// profile); owner = the wall with the smallest next-row height h0 + (g-1) d at its own distance (thickness, reports).
inline Step NextLayer(State& s,const Column& c) {
  Step st;const V t=Of(NodePoint(s,c.top));long double best=INFINITY;
  for(unsigned short w:c.walls){const auto h0=FirstHeightOf(s,w),g=GrowthOf(s,w),d=WallDistanceTo(s,w,t);const auto row=h0+(g-1)*d;
    if(row<best){best=row;st.wall=w;st.d=d;}}
  if(st.wall<0){st.stop="no wall policy";return st;}
  const long double lam=UnitStep(s,t,c.dir);if(!(lam>0)){st.stop="s3 no unit metric step along the column";return st;}
  st.u=t+lam*c.dir;st.next=WallDistanceTo(s,st.wall,st.u);
  if(st.next>ThicknessOf(s,st.wall)){st.stop="s1 thickness";return st;}
  if(!(st.next>st.d)){st.stop="s3 column does not leave the wall";return st;}
  st.ok=true;return st;
}
// Smallest tangential size of the target orthogonal to the column direction at x.
inline long double TangentialSize(State& s,V x,V d) {
  const Tensor m=s.field->Query(Pt(x)).target;V e1=Cross(d,{1,0,0});if(Norm(e1)<.5L)e1=Cross(d,{0,1,0});e1=Unit(e1);const V e2=Unit(Cross(d,e1));
  auto q=[&](V a,V b){return a.x*(m.xx*b.x+m.xy*b.y+m.xz*b.z)+a.y*(m.xy*b.x+m.yy*b.y+m.yz*b.z)+a.z*(m.xz*b.x+m.yz*b.y+m.zz*b.z);};
  const long double a=q(e1,e1),b=q(e1,e2),c=q(e2,e2),lmax=(a+c)/2+std::sqrt(((a-c)/2)*((a-c)/2)+b*b);
  return lmax>0?1/std::sqrt(lmax):INFINITY;
}
struct LTx { bool ok=false;std::string cls,witness;std::vector<Id> ids;Patch fresh;std::map<Id,Id> lifted;std::vector<std::array<Id,4>> keys;size_t old_cells=0,fresh_cells=0;
  std::map<Id,Id> blockers;  // DC-15: stationary vertex of an inverted substituted cell -> a moved top of that cell
};
// One layer transaction for the tops of T: the cells of their stars above the stack are re-attached to the new points
// (R2 lift mechanics), every top face of the stack touching T is filled by the sequential decomposition in increasing id
// order of its lifted vertices; carrier faces, carrier loops, interface, D.2 and validity are checked as for R2.
inline LTx LayerTransaction(State& s,const std::function<bool(Id)>& below,const std::map<Id,V>& targets) {
  Scope scope("G LayerTransaction");
  LTx tx;std::set<Id> cavity;
  for(const auto& t:targets)for(Id id:StarOf(s,t.first))if(!below(id))cavity.insert(id);
  if(cavity.empty()){tx.cls="empty";tx.witness="no cell above the top";return tx;}
  if(cavity.size()>MaximumCavityCells){tx.cls="cap";tx.witness="above cavity of "+std::to_string(cavity.size())+" cells";tx.old_cells=cavity.size();return tx;}
  tx.ids.assign(cavity.begin(),cavity.end());tx.old_cells=cavity.size();Patch old;
  try{old=PatchOf(s,tx.ids);}catch(const std::exception& e){tx.cls="patch";tx.witness=e.what();return tx;}
  auto priv=s.reference->PrivateParameters(old);const PlanarReference& ref=priv?*priv:*s.reference;
  std::map<Id,Node> nodes;for(const auto& c:old.cells)for(auto n:c.v)nodes[n.id]=n;
  Id next=s.next_node;std::map<Id,Id> L;std::set<Id> seam;std::map<Id,Id> unlift;
  const auto carriersAll=PhysicalOf(s,cavity,false);
  for(const auto& t:targets) {
    std::set<Id> carriers,charts;for(const auto& f:carriersAll)if(Has(f.key,t.first)){carriers.insert(f.facet);charts.insert(s.reference->Canonical(f.facet));}
    Node u{next++,Pt(t.second)};
    try{u=Constrain(s,priv.get(),u,carriers,carriers);}catch(const std::exception& e){tx.cls="carrier";tx.witness="top "+std::to_string(t.first)+" binding: "+e.what();return tx;}
    nodes[u.id]=u;L[t.first]=u.id;unlift[u.id]=t.first;tx.lifted[t.first]=u.id;if(charts.size()>=2)seam.insert(u.id);
  }
  std::vector<Cell> fresh;
  // DC-15: every inverted fully substituted cell is recorded (its stationary vertices are the blockers of ClearAbove)
  for(const auto& c:old.cells){Cell m=c;for(auto& n:m.v){const auto f=L.find(n.id);if(f!=L.end())n=nodes.at(f->second);}
    if(!Positive(m.v)){if(tx.cls.empty()){tx.cls="clearance";tx.witness="moved cell "+std::to_string(c.id)+" inverts (the volume above is too close)";}
      Id top=UINT64_MAX;for(auto n:c.v)if(L.count(n.id))top=n.id;for(auto n:c.v)if(!L.count(n.id))tx.blockers.emplace(n.id,top);continue;}
    fresh.push_back(m);}
  if(tx.cls=="clearance")return tx;
  for(const auto& c:old.cells){const auto faces=CellFaces(c);
    for(unsigned k=0;k<4;++k){const Face F=faces[k];bool any=false;for(Id x:F)any=any||L.count(x);if(!any)continue;
      const auto a=s.vertex_cells.find(F[0]),b=s.vertex_cells.find(F[1]),d=s.vertex_cells.find(F[2]);Id other=UINT64_MAX;
      if(a!=s.vertex_cells.end()&&b!=s.vertex_cells.end()&&d!=s.vertex_cells.end())for(Id id:a->second)if(id!=c.id&&b->second.count(id)&&d->second.count(id))other=id;
      if(other==UINT64_MAX||cavity.count(other))continue;  // physical face or interior to the cavity
      if(!below(other)){tx.cls="topology";tx.witness="lifted face borders a cell outside cavity and stack";return tx;}
      std::vector<Id> lifted;for(Id x:F)if(L.count(x))lifted.push_back(x);std::sort(lifted.begin(),lifted.end());
      std::array<Node,3> cur{nodes.at(F[0]),nodes.at(F[1]),nodes.at(F[2])};
      for(Id x:lifted){const Node u=nodes.at(L.at(x));Cell t{0,{u,cur[0],cur[1],cur[2]}};
        if(!Positive(t.v)){tx.cls="fill";tx.witness="fill cell over a top face inverts (steep partial front)";return tx;}
        fresh.push_back(t);tx.keys.push_back(CellKey(t));for(auto& y:cur)if(y.id==x)y=u;}
    }
  }
  if(fresh.size()>MaximumReplacementCells){tx.cls="cap";tx.witness=std::to_string(fresh.size())+" fresh cells";tx.fresh_cells=fresh.size();return tx;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;tx.fresh_cells=fresh.size();
  std::vector<Face> newSkin,oldSkin;
  try{newSkin=Incidence(fresh).Boundary();oldSkin=Incidence(old.cells).Boundary();}catch(const std::exception& e){tx.cls="topology";tx.witness=e.what();return tx;}
  std::map<Face,SurfaceFace> oldPhysical;for(const auto& sf:old.surface)oldPhysical[Sorted(sf.v)]=sf;
  std::set<Face> oldOriented(oldSkin.begin(),oldSkin.end()),newOriented(newSkin.begin(),newSkin.end());
  auto oriented=[&](const std::set<Face>& set,Face f){for(int r=0;r<3;++r){if(set.count(f))return true;std::rotate(f.begin(),f.begin()+1,f.end());}return false;};
  auto markerOf=[&](Id facet){return s.reference->Get(facet).marker;};
  std::map<int,std::map<std::pair<Id,Id>,int>> oldEdges,newEdges;
  auto edges=[](std::map<std::pair<Id,Id>,int>& m,const Face& f){for(int i=0;i<3;++i){const Id a=f[i],b=f[(i+1)%3];const auto r=m.find({b,a});if(r!=m.end()){if(--r->second==0)m.erase(r);}else ++m[{a,b}];}};
  for(const auto& f:oldSkin){const auto p=oldPhysical.find(Sorted(f));
    if(p==oldPhysical.end()||s.walls.count(markerOf(p->second.facet))){if(!oriented(newOriented,f)){tx.cls="topology";tx.witness=p==oldPhysical.end()?"artificial interface changed":"wall face lost";return tx;}continue;}
    edges(oldEdges[markerOf(p->second.facet)],f);}
  // carrier marker of an old top-surface edge: the unique non-wall marker of the old physical faces containing it
  auto edgeMarker=[&](Id x,Id y)->int{int m=-1;for(const auto& sf:old.surface){const int k=markerOf(sf.facet);if(s.walls.count(k))continue;if(Has(sf.v,x)&&Has(sf.v,y)){if(m>=0&&m!=k)return -2;m=k;}}return m;};
  Patch out;out.cells=fresh;
  for(const auto& f:newSkin) {
    const auto key=Sorted(f);const auto p=oldPhysical.find(key);
    if(p!=oldPhysical.end()&&oriented(oldOriented,f)){out.surface.push_back({f,p->second.facet});const int m=markerOf(p->second.facet);if(!s.walls.count(m))edges(newEdges[m],f);continue;}
    if(oriented(oldOriented,f))continue;  // unchanged artificial face (top faces of the stack, outer interface)
    int marker=-1;{Face back=f;for(auto& x:back){const auto u=unlift.find(x);if(u!=unlift.end())x=u->second;}
      std::set<Id> distinct(back.begin(),back.end());
      if(distinct.size()==3){const auto q=oldPhysical.find(Sorted(back));if(q!=oldPhysical.end())marker=markerOf(q->second.facet);}
      else if(distinct.size()==2){marker=edgeMarker(*distinct.begin(),*distinct.rbegin());}}
    if(marker<0||s.walls.count(marker)){tx.cls="topology";std::ostringstream w;w<<"unexpected new skin face ("<<f[0]<<","<<f[1]<<","<<f[2]<<")";tx.witness=w.str();return tx;}
    std::array<Point,3> tri{nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};Id facet=0;bool found=false;
    try{found=ref.FindFace(marker,tri,facet);}catch(const std::exception&){found=false;}
    if(!found){tx.cls="carrier";tx.witness="new carrier face leaves marker "+std::to_string(marker)+" authority";return tx;}
    out.surface.push_back({f,facet});edges(newEdges[marker],f);
  }
  for(auto& m:newEdges)for(Id p:seam){std::pair<Id,Id> in{0,0},o{0,0};int nin=0,nout=0;
    for(const auto& e:m.second){if(e.first.second==p){in=e.first;++nin;}if(e.first.first==p){o=e.first;++nout;}}
    if(nin==1&&nout==1){m.second.erase(in);m.second.erase(o);const std::pair<Id,Id> joined{in.first,o.second},back{o.second,in.first};
      const auto r=m.second.find(back);if(r!=m.second.end()){if(--r->second==0)m.second.erase(r);}else ++m.second[joined];}}
  for(auto& m:oldEdges){auto& n=newEdges[m.first];if(m.second!=n){tx.cls="carrier";tx.witness="L-C: carrier boundary loop changed on marker "+std::to_string(m.first);return tx;}}
  for(auto& m:newEdges)if(!oldEdges.count(m.first)&&!m.second.empty()){tx.cls="carrier";tx.witness="L-C: new carrier patch";return tx;}
  std::string reason;
  if(!ValidatePrescribedInterface(old.cells,fresh,newSkin,reason,nullptr,MaximumCavityCells)){tx.cls="topology";tx.witness=reason;return tx;}
  try{ref.ExportParameters(out);}catch(const std::exception& e){tx.cls="carrier";tx.witness=std::string("parameters: ")+e.what();return tx;}
  {FirstHeightReport r;WallHeights none;none.tolerance=HeightTolerance;if(!CheckFirstHeight(out,none,*s.reference,r,reason)){tx.cls="carrier";tx.witness="surface: "+reason;return tx;}}
  // G never changes a first child (A3: only the certified first-row routes may): a wall face of the cavity keeps its cell
  for(const auto& sf:out.surface){if(!s.walls.count(markerOf(sf.facet)))continue;const auto key=Sorted(sf.v);std::array<Id,4> ka{},kb{};
    for(const auto& c:old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)ka=CellKey(c);}
    for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)kb=CellKey(c);}
    if(ka!=kb){tx.cls="invariant";tx.witness="the lift would change a first child";return tx;}}
  // D.2 preserves a unique positive first child and certified clearance. In ValidityOnly,
  // requested-height tolerance is provisional; strict legacy diagnostics retain HeightT.
  for(const auto& sf:out.surface){if(!s.walls.count(markerOf(sf.facet)))continue;const auto key=Sorted(sf.v);
    Patch before,after;for(const auto& c:old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)before.cells.push_back(c);}
    for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)after.cells.push_back(c);}
    before.surface=after.surface={sf};before.parameters=old.parameters;after.parameters=out.parameters;
    std::string w0,w1;
    const auto height_ok=[&](const Patch& child,std::string& witness) {
      if(child.cells.size()!=1)return false;
      const bool measured=validityOnlyConstruction
          ? HeightConstruct(child,s.heights,*s.reference,nullptr,&witness)
          : HeightT(s,child,witness);
      const auto& c=child.cells.front();
      return measured&&Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0;
    };
    const bool was=height_ok(before,w0);
    if(was&&!height_ok(after,w1)){tx.cls="invariant";tx.witness="a wall face loses its passing geometric first child";return tx;}}
  for(const auto& c:fresh)if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){tx.cls="validity";tx.witness="fresh cell has nonpositive exact orientation";return tx;}
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
// DC-15 ClearAbove (DC15_PLAN rev 2 sect. 4): a clearance refusal names the stationary vertices of every inverted
// substituted cell. A blocker x is FREE when it is no lid, apex, stack vertex, top or copy (`fixed`), lies on no physical
// face, and its whole star avoids the stack/column/layer/fan cells (`below`) and every first child - so G's below-stack
// classification, the first children and every registry identity are untouched. x is relocated along the column
// direction above the planned top (MoveVertex: a relocation of an interior vertex, admitted like G's own lift: exact
// positivity, validity q > 0 / J >= 1e-3, D.2), else collapsed onto its link neighbour farthest along the column
// (CollapseVertex, a vertex removal: also Admit12 Kind::Layer, no added frozen excess). The layer is then retried.
inline bool growthClearing=true;
inline bool ClearAbove(State& s,Growth& G,const std::function<bool(Id)>& below,const std::set<Id>& fixed,Id x,V d,V u,long double step) {
  Scope scope("G ClearAbove");
  auto refuse=[&](const char* k){++G.clear_refusals[k];return false;};
  if(!s.vertex_cells.count(x))return refuse("gone");
  if(fixed.count(x))return refuse("fixed (lid, apex, stack, top or copy)");
  const auto star=StarOf(s,x);if(star.empty()||star.size()>MaximumCavityCells)return refuse("star size");
  for(Id id:star){if(below(id))return refuse("star touches the stack or a column");if(WallChild(s,id))return refuse("star touches a first child");
    const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(f[k],x))return refuse("boundary vertex");}
  std::vector<Id> ids(star.begin(),star.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception&){return refuse("patch");}
  const V px=Of(NodePoint(s,x));const long double lift=std::max(0.L,Dot(u-px,d));
  auto valid=[&](const Patch& fresh){for(const auto& c:fresh.cells)if(!Positive(c.v))return false;std::string d2;return D2Ok(s,old,fresh,d2);};
  for(long double k:{1.L,2.L}){Patch fresh;std::string why;bool ok=false;
    try{ok=MoveVertex(old,x,{s.next_node,Pt(px+(lift+k*step)*d)},s.next_cell,*s.reference,fresh,why);}catch(const std::exception&){ok=false;}
    if(!ok||!valid(fresh))continue;
    Apply(s,ids,fresh);++G.clear_moves;return true;}
  Id y=UINT64_MAX;long double best=0;for(Id id:star)for(auto n:s.mesh.at(id).cell.v)if(n.id!=x){const auto h=Dot(Of(n.p)-px,d);if(h>best){best=h;y=n.id;}}
  if(y==UINT64_MAX)return refuse("no move; no neighbour above");
  Patch fresh;std::string why;bool ok=false;
  try{ok=CollapseVertex(old,x,y,s.next_cell,*s.reference,fresh,why);}catch(const std::exception&){ok=false;}
  if(!ok||!valid(fresh))return refuse("no valid move or collapse");
  const auto dec=Admit12(s,old,fresh,Kind::Layer);if(!dec.ok)return refuse("collapse adds frozen excess");
  Apply(s,ids,fresh);++G.clear_collapses;return true;
}
}  // namespace dc12
#include "BLDeform16.inl"
namespace dc12 {
// G: layer index by layer index; per layer, bounded wall patches of adjacent planned tops (<= 8 tops, above cavity <= 64),
// each member alone on failure, one retry of every refused column at the same index; stop rules s1-s4 (OPTIMIZE_PLAN 2).
inline Growth PhaseG(State& s,Rows10& rows,double seconds,std::ostream& log,const std::set<Id>& forceStop={},size_t maximum_work=std::numeric_limits<size_t>::max()) {
  Scope scope("PhaseG");
  Growth G;G.forceStop=forceStop;const auto t0=Clock::now();MapLayers(s);
  for(const auto& r:rows.lift){const Id w=r.first;if(Rim(s,w))continue;Column c;c.wall=w;c.top=c.lid=r.second;
    const V x=Of(NodePoint(s,w)),y=Of(NodePoint(s,r.second));c.dir=Unit(y-x);
    for(const auto& f:PhysicalOf(s,StarOf(s,w),true))if(Has(f.key,w)){const auto it=layerOfMarker.find(f.marker);if(it!=layerOfMarker.end())c.walls.insert(it->second);}
    c.lidDistance=INFINITY;for(unsigned short wl:c.walls)c.lidDistance=std::min(c.lidDistance,WallDistanceTo(s,wl,y));
    if(forceStop.count(w)){c.stopped=true;c.stop="forced (test)";}
    G.columns.push_back(c);}
  for(const auto& r:rows.copies)for(size_t j=0;j<r.second.ids.size();++j){if(Rim(s,r.first)||!s.vertex_cells.count(r.second.ids[j]))continue;
    Column c;c.wall=r.first;c.top=c.lid=r.second.ids[j];c.dir=r.second.dirs[j];
    for(const auto& f:PhysicalOf(s,StarOf(s,r.first),true))if(Has(f.key,r.first)){const auto it=layerOfMarker.find(f.marker);if(it!=layerOfMarker.end())c.walls.insert(it->second);}
    c.lidDistance=INFINITY;for(unsigned short wl:c.walls)c.lidDistance=std::min(c.lidDistance,WallDistanceTo(s,wl,Of(NodePoint(s,c.top))));
    G.columns.push_back(c);}
  std::set<Id> stackCells=ColumnCells(s,rows);for(Id id:FanCells(s,rows))stackCells.insert(id);
  auto below=[&](Id id){return stackCells.count(id)||G.layerKeys.count(CellKey(s.mesh.at(id).cell));};
  // DC-16: stop texts are bounded (an exception text can be megabytes) and the census keys keep the reason class
  auto stop=[&](Column& c,const std::string& why,long double height){c.stopped=true;c.stop=why.substr(0,160);c.stopHeight=height;++G.stops[why.substr(0,why.find(' '))];
    ++G.stop_reasons[swbi_dc2::Category(why).substr(0,48)];};
  // DC-15: identities ClearAbove never touches (lids, apices, copies, stack vertices and tops)
  std::set<Id> fixed;for(const auto& r:rows.lift)fixed.insert(r.second);for(const auto& r:rows.apex)fixed.insert(r.second);
  for(const auto& r:rows.copies)for(Id c:r.second.ids)fixed.insert(c);for(const auto& c:G.columns){fixed.insert(c.top);fixed.insert(c.lid);}
  std::set<Id> lidIds,apexIds;for(const auto& r:rows.lift)lidIds.insert(r.second);for(const auto& r:rows.apex)apexIds.insert(r.second);
  dc16::roomStats=dc16::RoomStats{};
  auto apply=[&](const LTx& tx,const std::map<Id,size_t>& owner) {
    Apply(s,tx.ids,tx.fresh);for(const auto& k:tx.keys)G.layerKeys.insert(k);for(const auto& l:tx.lifted)fixed.insert(l.second);
    for(const auto& l:tx.lifted){auto& c=G.columns[owner.at(l.first)];c.stack.push_back(l.second);c.top=l.second;G.current[l.second]=l.second;++G.lifted;}
    ++G.transactions;G.max_old=std::max(G.max_old,tx.old_cells);G.max_fresh=std::max(G.max_fresh,tx.fresh_cells);};
  for(unsigned k=0;k<48;++k) {
    if(Seconds(t0)>=seconds){G.outcome="limit";break;}if(G.work>=maximum_work){G.outcome="limit:work";break;}BoundCaches(s);
    std::vector<size_t> active;for(size_t i=0;i<G.columns.size();++i)if(!G.columns[i].stopped&&G.columns[i].stack.size()==k)active.push_back(i);
    if(active.empty())break;
    std::map<Id,V> plan;std::map<Id,size_t> owner;
    // DC-16 memory diagnosis: resident-set deltas of the planning calls (largest ones with their column) and a guard
    const auto tp=Clock::now();const long rss0=dc16::RssKB();std::array<long,3> grown{0,0,0};std::vector<std::pair<long,std::string>> worst;
    auto note=[&](int which,long before,const Column& c,const std::string& what){const long d=dc16::RssKB()-before;grown[which]+=d;
      if(d>=65536){std::ostringstream w;const V x=Of(NodePoint(s,c.top));w<<(which==0?"NextLayer":which==1?"TangentialSize":"GeometryLength")<<" +"<<d<<" kB at top "<<c.top
        <<" ("<<double(x.x)<<","<<double(x.y)<<","<<double(x.z)<<") stack "<<c.stack.size()<<" "<<what;worst.push_back({d,w.str()});}};
    for(size_t i:active) {
      if(Seconds(t0)>=seconds){G.outcome="limit:time";break;}
      auto& c=G.columns[i];if(!s.vertex_cells.count(c.top)){stop(c,"s3 top vanished",INFINITY);continue;}
      if(dc16::RssKB()>dc16::rssCeilingKB){G.outcome="limit:memory";break;}
      long r=dc16::RssKB();
      Step st;try{st=NextLayer(s,c);}catch(const std::exception& e){st.ok=false;st.stop=std::string("s3 law: ")+e.what();}
      note(0,r,c,st.ok?"ok":st.stop);
      if(!st.ok){stop(c,st.stop,st.stop.rfind("s1",0)==0?st.d:INFINITY);continue;}
      r=dc16::RssKB();long double tangential=INFINITY;try{tangential=TangentialSize(s,st.u,c.dir);}catch(const std::exception&){}
      note(1,r,c,"");
      if(st.next-st.d>=tangential){stop(c,"s2 isotropy",st.d);continue;}
      r=dc16::RssKB();long double l=-1;try{l=GeometryLength(s,NodePoint(s,c.top),Pt(st.u));}catch(const std::exception&){l=-1;}
      note(2,r,c,"");
      if(l<0){stop(c,"s3 unmeasurable column segment",INFINITY);continue;}
      if(l<.6L||l>1.5L){stop(c,std::string("s1 metric ")+(l<.6L?"coarser":"finer")+" than the law",st.d);continue;}
      plan[c.top]=st.u;owner[c.top]=i;
    }
    std::sort(worst.rbegin(),worst.rend());
    std::cout<<"DC16 G plan "<<k<<" active="<<active.size()<<" planned="<<plan.size()<<" seconds="<<Seconds(tp)<<" rss_kB "<<rss0<<"->"<<dc16::RssKB()
             <<" grown_kB NextLayer="<<grown[0]<<" Tangential="<<grown[1]<<" Geometry="<<grown[2]<<std::endl;
    for(size_t w=0;w<worst.size()&&w<6;++w)std::cout<<"DC16 G plan worst "<<worst[w].second<<std::endl;
    if(G.outcome=="limit:memory" || G.outcome=="limit:time")break;
    if(plan.empty())break;
    // DC-16: room for the layer (deformation of the free volume above the active front, one-unit commit)
    if(dc16::roomStep)dc16::MakeRoom(s,below,fixed,plan,log);  // review 2: commit failures propagate (event fails)
    std::vector<Id> tops;for(const auto& p:plan)tops.push_back(p.first);
    std::set<Id> done,refusedNow;
    auto aboveOf=[&](Id t){std::set<Id> out;for(Id id:StarOf(s,t))if(!below(id))out.insert(id);return out;};
    // a planned top is pending while its column still has k layers and this top (a lifted top is a stack vertex)
    auto pending=[&](Id t){const auto o=owner.find(t);if(o==owner.end())return false;const auto& c=G.columns[o->second];return !c.stopped&&c.stack.size()==k&&c.top==t;};
    auto attempt=[&](const std::set<Id>& T0)->bool{
      std::set<Id> T;for(Id t:T0)if(pending(t))T.insert(t);if(T.empty())return true;  // lifted meanwhile (joint transaction)
      std::map<Id,V> targets;for(Id t:T)targets[t]=plan.at(t);
      LTx tx;bool jointTried=false;
      for(unsigned round=0;;++round){
        if(G.work>=maximum_work || Seconds(t0)>=seconds){G.outcome=G.work>=maximum_work?"limit:work":"limit:time";return false;}
        ++G.work;
        try{tx=LayerTransaction(s,below,targets);}catch(const std::exception& e){tx.ok=false;tx.cls="exception";tx.witness=e.what();}
        if(tx.ok){if(round)++G.clear_retry_accepted;break;}
        if(tx.cls!="clearance"||round>=3||Seconds(t0)>=seconds)break;
        for(const auto& b:tx.blockers){const Id x=b.first;
          if(fixed.count(x))++G.fixed_blockers[plan.count(x)?(targets.count(x)?"planned top (same transaction)":"planned top (other transaction)"):lidIds.count(x)?"lid":apexIds.count(x)?"apex":"stack or stopped top"];}
        // DC-16 (plan review 1): tops that must advance together are lifted in one joint transaction
        if(!jointTried){std::set<Id> joint;for(const auto& b:tx.blockers)if(plan.count(b.first)&&!targets.count(b.first)&&pending(b.first))joint.insert(b.first);
          if(!joint.empty()&&targets.size()+joint.size()<=16){for(Id x:joint)targets[x]=plan.at(x);jointTried=true;++G.joint_retries;continue;}}
        // DC-15: clear free blockers above the planned tops and retry (local fallback)
        if(!growthClearing)break;
        size_t cleared=0;
        for(const auto& b:tx.blockers){const Id t=b.second;if(!plan.count(t))continue;const auto& c=G.columns[owner.at(t)];
          const V top=Of(NodePoint(s,t)),u=plan.at(t);cleared+=ClearAbove(s,G,below,fixed,b.first,c.dir,u,Norm(u-top));}
        if(!cleared)break;++G.clear_retries;}
      if(tx.ok){apply(tx,owner);for(const auto& t:targets)done.insert(t.first);if(jointTried)++G.joint_accepted;
        ++G.accepted[targets.size()>1?"patch":"single"];(targets.size()>1?G.patch_tx:G.single_tx)++;return true;}
      ++G.refused;++G.refusals[tx.cls];G.last=tx.cls+": "+tx.witness;log<<"DC12 G refuse |T|="<<targets.size()<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";return false;};
    for(Id seed:tops) {
      if(done.count(seed))continue;if(G.work>=maximum_work || Seconds(t0)>=seconds)break;
      std::set<Id> T{seed},cav=aboveOf(seed);
      for(bool grew=true;grew&&T.size()<8;) {grew=false;
        std::set<Id> cand;for(Id t:T)for(Id id:StarOf(s,t))for(auto n:s.mesh.at(id).cell.v)if(plan.count(n.id)&&!done.count(n.id)&&!T.count(n.id))cand.insert(n.id);
        for(Id x:cand){auto more=cav;for(Id id:aboveOf(x))more.insert(id);if(more.size()<=MaximumCavityCells){T.insert(x);cav=more;grew=true;break;}}}
      for(Id t:T)done.insert(t);
      if(attempt(T))continue;
      if(T.size()>1){for(Id t:T)if(!attempt({t}))refusedNow.insert(t);}else refusedNow.insert(seed);
    }
    for(Id t:refusedNow){if(G.work>=maximum_work || Seconds(t0)>=seconds)break;auto& c=G.columns[owner.at(t)];if(c.stopped||c.stack.size()!=k)continue;
      if(++c.refusals>=2||!attempt({t})){if(c.stack.size()==k)stop(c,"s3 refused transactions ("+G.last.substr(0,G.last.find(':'))+")",INFINITY);}}
    ++G.layers;
    log<<"DC12 G layer "<<k<<" planned="<<plan.size()<<" lifted_total="<<G.lifted<<" refused_total="<<G.refused<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC16 G layer "<<k<<" planned="<<plan.size()<<" lifted_total="<<G.lifted<<" refused_total="<<G.refused<<" room_needy="<<dc16::roomStats.needy
             <<" room_moved="<<dc16::roomStats.moved<<" t="<<Seconds(t0)<<" "<<dc16::MemoryLine(s)<<std::endl;
  }
  for(auto& c:G.columns)if(!c.stopped&&c.stack.size()>=48){c.stopped=true;c.stop="s4 layer cap";++G.stops["s4"];}
  // DC-16 memory diagnosis (plan review 8): live structures vs allocator retention around one malloc_trim
  std::cout<<"DC16 G end before_trim "<<dc16::MemoryLine(s)<<std::endl;malloc_trim(0);std::cout<<"DC16 G end after_trim "<<dc16::MemoryLine(s)<<std::endl;
  if(G.outcome.empty())G.outcome="complete";G.seconds=Seconds(t0);return G;
}

// ---- census (A2) and first-row audit (A3) --------------------------------------------------------------------------
struct Bands { std::vector<long double> edges{0,3,25,50,100,200,800,INFINITY};std::vector<size_t> count; };
inline Bands BandCensus(State& s) {
  Bands b;b.count.assign(b.edges.size()-1,0);std::set<Id> seen;
  for(const auto& row:s.mesh)for(auto n:row.second.cell.v){if(!seen.insert(n.id).second)continue;const auto d=MinWallDistance(s,Of(n.p))/s.h0;
    for(size_t i=0;i+1<b.edges.size();++i)if(d>=b.edges[i]&&d<b.edges[i+1]){++b.count[i];break;}}
  return b;
}
inline std::string JsonBands(const Bands& b){std::ostringstream o;o<<"{";for(size_t i=0;i<b.count.size();++i)o<<(i?",":"")<<"\""<<double(b.edges[i])<<"-"<<(std::isfinite(b.edges[i+1])?std::to_string(int(b.edges[i+1])):"inf")<<"\":"<<b.count[i];o<<"}";return o.str();}
struct LayerCensus { size_t eligible=0,pass=0,missing=0,target_failed=0;std::vector<long> deviation;double fraction=0; };
// A2 over the post-R2 population (non-rim wall vertices after R2, independent of construction success): target = law
// interfaces in [3 h0, 25 h0] of the owning wall (NextLayer's rule at the lid; the smallest h0 without a column), from the
// lid distance (nominal h0 (1+g) without a lid), cut at the owner's thickness and at a metric stop height (s1/s2); count =
// the column's stack vertices still in the mesh (relabels followed) at owner-wall distance in the band, 0 without a column.
struct WallOf { std::set<unsigned short> walls;Point wall; };
inline std::map<Id,WallOf> EligibleAfterR2(State& s) {
  std::map<Id,WallOf> out;
  for(Id w:WallVertices(s)){if(Rim(s,w))continue;WallOf x;x.wall=NodePoint(s,w);
    for(const auto& f:PhysicalOf(s,StarOf(s,w),true))if(Has(f.key,w)){const auto it=layerOfMarker.find(f.marker);if(it!=layerOfMarker.end())x.walls.insert(it->second);}
    out.emplace(w,x);}
  return out;
}
// A2 (rev 5): target = interfaces of the target's wall-normal law (unit target-metric steps along the column, the rule G
// follows) with owner-wall distance in [3 h0, 25 h0], marched from the lid (a column-less vertex: two unit steps from the
// wall, the nominal lid), ended only by the owner's thickness or isotropy (s1/s2); owner = NextLayer's rule at the start
// point. A failed target march is a failure, never a pass.
inline LayerCensus CountLayers(State& s,const Growth& G,const std::map<Id,WallOf>& eligible) {
  LayerCensus out;std::map<Id,const Column*> byWall;for(const auto& c:G.columns)byWall.emplace(c.wall,&c);
  for(const auto& e:eligible) {
    ++out.eligible;const auto f=byWall.find(e.first);if(e.second.walls.empty()){++out.target_failed;out.deviation.push_back(-99);continue;}
    long target=0;unsigned short owner=*e.second.walls.begin();bool failed=false;
    try{
      V dir;V x=Of(e.second.wall);int skip=0;
      if(f!=byWall.end()){dir=f->second->dir;if(s.vertex_cells.count(f->second->lid))x=Of(NodePoint(s,f->second->lid));else skip=2;}
      else{const auto D=DirectionAt(s,e.first);if(!D.ok)throw std::runtime_error("no direction");dir=D.d;skip=2;}
      for(int k=0;k<skip;++k){const auto lam=UnitStep(s,x,dir);if(!(lam>0))throw std::runtime_error("no unit step");x=x+lam*dir;}
      long double best=INFINITY,d=0;for(unsigned short w:e.second.walls){const auto dw=WallDistanceTo(s,w,x),row=FirstHeightOf(s,w)+(GrowthOf(s,w)-1)*dw;if(row<best){best=row;owner=w;d=dw;}}
      const long double h0=FirstHeightOf(s,owner),thick=ThicknessOf(s,owner);
      for(int k=0;k<64;++k){const auto lam=UnitStep(s,x,dir);if(!(lam>0))throw std::runtime_error("no unit step");const V y=x+lam*dir;const auto dn=WallDistanceTo(s,owner,y);
        if(dn>thick||dn>25*h0*(1+1e-9L))break;if(dn-d>=TangentialSize(s,y,dir))break;if(dn>=3*h0)++target;x=y;d=dn;}
    }catch(const std::exception&){failed=true;}
    if(failed){++out.target_failed;out.deviation.push_back(-99);if(f==byWall.end())++out.missing;continue;}
    const long double h0=FirstHeightOf(s,owner);long count=0;
    if(f!=byWall.end())for(Id x:f->second->stack){const auto cur=G.current.find(x);const Id now=cur==G.current.end()?x:cur->second;
      if(!s.vertex_cells.count(now))continue;const auto dist=WallDistanceTo(s,owner,Of(NodePoint(s,now)));if(dist>=3*h0&&dist<=25*h0)++count;}
    else ++out.missing;
    out.deviation.push_back(count-target);if(std::abs(count-target)<=2)++out.pass;
  }
  out.fraction=out.eligible?double(out.pass)/out.eligible:0;return out;
}
inline std::map<Face,std::array<Id,4>> FirstChildren(State& s){std::map<Face,std::array<Id,4>> out;for(const auto& wf:WallFaces(s))out[wf.second]=CellKey(s.mesh.at(wf.first).cell);return out;}
struct Audit { size_t faces=0,unchanged=0,certified=0,violations=0,removed=0,created=0;std::vector<std::string> witnesses; };
inline Audit FirstRowAudit(State& s,const std::map<Face,std::array<Id,4>>& before,const std::set<Face>& certified) {
  Audit a;const auto now=FirstChildren(s);a.faces=before.size();
  for(const auto& f:before){const auto n=now.find(f.first);
    if(n==now.end()){if(certified.count(f.first))++a.removed;else{++a.violations;if(a.witnesses.size()<8)a.witnesses.push_back("removed "+std::to_string(f.first[0])+","+std::to_string(f.first[1])+","+std::to_string(f.first[2]));}continue;}
    if(n->second==f.second)++a.unchanged;else if(certified.count(f.first))++a.certified;
    else{++a.violations;if(a.witnesses.size()<8)a.witnesses.push_back(std::to_string(f.first[0])+","+std::to_string(f.first[1])+","+std::to_string(f.first[2]));}}
  for(const auto& n:now)if(!before.count(n.first)){if(certified.count(n.first))++a.created;else{++a.violations;if(a.witnesses.size()<8)a.witnesses.push_back("created "+std::to_string(n.first[0])+","+std::to_string(n.first[1])+","+std::to_string(n.first[2]));}}
  return a;
}

// ---- (3) certified first-child route for wall faces without a registered column -------------------------------------
struct FirstRowStats { size_t tried=0,inserted=0,relocated=0,refused=0,registrations_moved=0;std::map<std::string,size_t> why; };
// After an apex relocation (old identity -> new identity) every face registered with the old apex follows it.
inline void MoveRegistrations(Rows10& rows,Id from,Id to,std::set<Face>& certified,FirstRowStats* st=nullptr) {
  for(auto& r:rows.apex)if(r.second==from){r.second=to;certified.insert(r.first);if(st)++st->registrations_moved;}
}
// DC-15 (code review 2): a face is registered as a column only when all three of its vertices are lifted (the column
// routes and the continuation sidecar assume lifts); a certified first child without lifts stays unregistered.
inline bool ColumnEligible(const Rows& rows,const Face& key){return std::all_of(key.begin(),key.end(),[&](Id x){return rows.lift.count(x)>0;});}
inline size_t StaleRegistrations(const State& s,const Rows10& rows){size_t n=0;for(const auto& r:rows.apex)n+=!s.vertex_cells.count(r.second);return n;}
// (iii) ApexRelocate: the existing apex v of the face's first child (interior or on carriers, never a wall or lid vertex)
// moved by production MoveVertex to the best ConeColumn-grid candidate bound to v's carriers; Admit12 Repair + D.2.
inline bool ApexRelocate(State& s,Rows10& rows,Id cell,const Face& key,FirstRowStats& st,std::set<Face>& certified) {
  Scope scope("V12 ApexRelocate");
  const Cell c=s.mesh.at(cell).cell;Id v=UINT64_MAX;for(auto n:c.v)if(!Has(key,n.id))v=n.id;
  if(v==UINT64_MAX){++st.why["relocate: no apex"];return false;}
  for(const auto& r:rows.lift)if(r.second==v){++st.why["relocate: lid vertex"];return false;}
  const auto star=StarOf(s,v);if(star.empty()||star.size()>MaximumCavityCells){++st.why["relocate: star exceeds 64"];return false;}
  std::vector<Id> ids(star.begin(),star.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception&){++st.why["relocate: patch"];return false;}
  std::set<Id> carriers;
  for(const auto& f:PhysicalOf(s,star,true))if(Has(f.key,v)){++st.why["relocate: wall vertex"];return false;}
  for(const auto& f:PhysicalOf(s,star,false))if(Has(f.key,v))carriers.insert(f.facet);
  auto priv=s.reference->PrivateParameters(old);const PlanarReference& ref=priv?*priv:*s.reference;
  Face out{};for(auto f:CellFaces(c))if(Sorted(f)==Sorted(key))out=f;
  const V a=Of(NodePoint(s,out[0])),b=Of(NodePoint(s,out[1])),cc=Of(NodePoint(s,out[2]));const V n=-1.L*Outward(a,b,cc);const long double h0=FaceH0(s,key);
  const std::array<std::array<long double,3>,7> feet{{{1.L/3,1.L/3,1.L/3},{.6L,.2L,.2L},{.2L,.6L,.2L},{.2L,.2L,.6L},{.45L,.45L,.1L},{.45L,.1L,.45L},{.1L,.45L,.45L}}};
  Patch chosen;bool found=false;
  for(long double eta:{1.L,.92L,1.08L})for(const auto& w:feet) {
    Node node{s.next_node,Pt(w[0]*a+w[1]*b+w[2]*cc+(eta*h0)*n)};
    try{if(!carriers.empty())node=Constrain(s,priv.get(),node,carriers,carriers);}catch(const std::exception&){continue;}
    Patch fresh;std::string why;bool ok=false;try{ok=MoveVertex(old,v,node,s.next_cell,ref,fresh,why);if(ok)ref.ExportParameters(fresh);}catch(const std::exception&){ok=false;}
    if(!ok)continue;{std::string d2;if(!D2Ok(s,old,fresh,d2))continue;}
    const auto d=Admit12(s,old,fresh,FirstRowKind(s,old));if(!d.ok)continue;
    chosen=std::move(fresh);found=true;break;
  }
  if(!found){++st.why["relocate: no admissible candidate"];return false;}
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
  const Id moved=s.next_node;Apply(s,ids,chosen);
  MoveRegistrations(rows,v,moved,certified,&st);if(ColumnEligible(rows,key))rows.apex[key]=moved;else rows.apex.erase(key);certified.insert(key);++st.relocated;return true;
}
inline bool FirstRowInsert(State& s,Rows10& rows,Id cell,const Face& key,FirstRowStats& st,std::set<Face>& certified) {
  Scope scope("V12 FirstRowInsert");++st.tried;
  Patch old;try{old=PatchOf(s,{cell});}catch(const std::exception&){++st.refused;++st.why["patch"];return false;}
  std::set<Id> before;for(const auto& c:old.cells)for(auto n:c.v)before.insert(n.id);
  unsigned walls=0;for(const auto& sf:old.surface)walls+=s.walls.count(s.reference->Get(sf.facet).marker);
  auto admit=[&](Patch& fresh)->bool{bool added=false;for(const auto& c:fresh.cells)for(auto n:c.v)added=added||!before.count(n.id);
    if(!added)return false;  // retained child: not an insertion
    std::string d2;if(!D2Ok(s,old,fresh,d2)){++st.why["insert: D.2"];return false;}
    const auto d=Admit12(s,old,fresh,Kind::Layer);if(!d.ok){++st.why["insert: "+Key(d.reason)];return false;}return true;};
  Patch fresh;FirstHeightReport report;std::string why;bool ok=false;
  try{if(walls==1&&InsertFirstWallPoint(old,s.next_node,s.next_cell,s.heights,*s.reference,fresh,report,why))ok=admit(fresh);}catch(const std::exception& e){why=e.what();ok=false;}
  if(!ok) {
    std::vector<Point> backgrounds{Centroid(old.cells.front())};Patch chosen;
    for(const auto& f:old.surface){const auto h=s.heights.find(s.reference->Get(f.facet).marker);if(h==s.heights.end())continue;
      std::array<V,3> q;for(unsigned k=0;k<3;++k)for(auto n:old.cells.front().v)if(n.id==f.v[k])q[k]=Of(n.p);
      const V nn=Unit(Cross(q[1]-q[0],q[2]-q[0]));const V cc=(1.L/3)*(q[0]+q[1]+q[2]);
      for(double factor:{1.25,1.5,2.})backgrounds.push_back(Pt(cc-(factor*h->second)*nn));}
    for(const auto& b:backgrounds){Patch candidate;std::string reason;
      try{if(!ConstructFirstLayer(old,{s.next_node,b},s.next_node+1,s.next_cell,s.heights,*s.reference,candidate,report,reason)){why=reason;continue;}}catch(const std::exception& e){why=e.what();continue;}
      if(!admit(candidate))continue;chosen=std::move(candidate);ok=true;break;}
    if(ok)fresh=std::move(chosen);
  }
  if(ok) {
    Id apex=UINT64_MAX;for(const auto& c:fresh.cells){unsigned k=0;Id other=UINT64_MAX;for(auto n:c.v){if(Has(key,n.id))++k;else other=n.id;}if(k==3&&!before.count(other))apex=other;}
    for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
    Apply(s,{cell},fresh);if(apex!=UINT64_MAX&&ColumnEligible(rows,key))rows.apex[key]=apex;certified.insert(key);++st.inserted;return true;
  }
  ++st.why["insert: "+Key(why)];
  if(ApexRelocate(s,rows,cell,key,st,certified))return true;
  ++st.refused;return false;
}
// ApexMove with every registration of the old apex following the new one (shared apices) and the faces certified.
inline bool ApexMove12(State& s,Rows10& rows,const Face& key,VStage& g,std::string& why,std::set<Face>& certified) {
  const auto it=rows.apex.find(key);if(it==rows.apex.end()){why="not rowed";return false;}const Id before=it->second;
  // an apex on a carrier (non-wall physical face) cannot be re-coned: carrier-constrained relocation instead
  bool carrier=false;for(const auto& f:PhysicalOf(s,StarOf(s,before),false))carrier=carrier||Has(f.key,before);
  if(carrier){Id cell=UINT64_MAX;if(s.wall_indexed){const auto w=s.wall_index.find(key);if(w!=s.wall_index.end())cell=w->second;}else for(const auto& wf:WallFaces(s))if(wf.second==key)cell=wf.first;if(cell==UINT64_MAX){why="ApexRelocate: wall face gone";return false;}
    FirstRowStats st;++g.work;if(ApexRelocate(s,rows,cell,key,st,certified))return true;
    why="ApexRelocate: "+(st.why.empty()?std::string("refused"):st.why.rbegin()->first);++g.rejected;return false;}
  std::set<Face> touched;for(const auto& f:PhysicalOf(s,StarOf(s,before),true))touched.insert(f.key);
  if(!ApexMove(s,rows,key,g,why))return false;
  for(const auto& f:touched)certified.insert(f);
  const Id now=rows.apex.at(key);if(now!=before){rows.apex[key]=before;MoveRegistrations(rows,before,now,certified);}certified.insert(key);return true;
}

// ---- (4) V12 ------------------------------------------------------------------------------------------------------------
struct VStage12 : VStage {
  size_t refine_attempts=0,r2_retries=0,r2_new_rowed=0;FirstRowStats first;std::vector<OptimizeReport> opt;std::vector<SweepRow> per_sweep;
  std::map<std::string,size_t> acceptedBy;double refine_seconds=0,optimize_seconds=0,first_seconds=0,r2_seconds=0;size_t halves_guard=0,stale_skips=0;
};
inline VStage12 PhaseV12(State& s,Rows10& rows,Growth& G,size_t work,double seconds,size_t maxSweeps,std::set<Face>& certified,std::ostream& log) {
  Scope scope("PhaseV12");
  VStage12 g;std::set<Id> apexIds;Begin(s,g);const auto t0=Clock::now();
  auto remaining=[&]{return seconds-Seconds(t0);};
  auto limited=[&]{return g.work>=work||Seconds(t0)>=seconds;};
  struct Reset{~Reset(){splitPolicy.requested=-1;splitPolicy.relocated=nullptr;splitPolicy.relocatedCount=0;}} resetPolicy;
  std::set<std::vector<Id>> rejected;
  auto keyOf=[&](Id tag,const std::vector<Id>& head,const std::vector<Id>& ids){std::vector<Id> k{tag};k.insert(k.end(),head.begin(),head.end());k.push_back(UINT64_MAX);k.insert(k.end(),ids.begin(),ids.end());std::sort(k.begin()+head.size()+2,k.end());return k;};
  // a first child (the cell on a wall face) may change only through the certified first-row routes (A3)
  auto firstChildKept=[&](const Patch& old,const Patch& fresh){
    for(const auto& sf:old.surface){if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;const auto key=Sorted(sf.v);std::array<Id,4> a{},b{};bool fa=false,fb=false;
      for(const auto& c:old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3){a=CellKey(c);fa=true;}}
      for(const auto& c:fresh.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3){b=CellKey(c);fb=true;}}
      if(fa!=fb||a!=b)return false;}
    return true;};
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    if(op!="wall"&&!firstChildKept(old,fresh)){++g.work;++g.rejected;++g.reasons[op+" [6] changes a first child outside the certified routes"];return false;}
    ++g.work;const auto d=Admit12(s,old,fresh,kind);
    if(!d.ok){++g.rejected;++g.reasons[op+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=op+": "+d.reason;return false;}
    // only the production wall reconstructions (variant "wall": height-certified first-row rebuilds) may change first
    // children here; every other refinement must leave them unchanged (the A3 audit counts any other change)
    if(op=="wall"){std::set<Face> before,after;for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))before.insert(Sorted(sf.v));
     for(const auto& sf:fresh.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))after.insert(Sorted(sf.v));
     for(const auto& f:before)certified.insert(f);for(const auto& f:after)certified.insert(f);}
    Apply(s,ids,fresh);++g.accepted;++g.acceptedBy[op];return true;};
  ColumnIndex ci;
  auto build=[&]{const bool fresh=ci.Build(s,rows);if(!rows.fan.empty())for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;
    if(fresh){apexIds.clear();for(const auto& r:ci.apexFace)apexIds.insert(r.first);}splitPolicy.relocated=&apexIds;};
  auto apexMove=[&](const Face& key,std::string& why)->bool{const bool ok=ApexMove12(s,rows,key,g,why,certified);if(ok)++g.acceptedBy["ApexMove"];return ok;};
  auto columnSplit=[&](Edge w,std::string& why)->bool{
    long double lw=INFINITY,ll=INFINITY;try{lw=Len(s,w[0],w[1]);}catch(const std::exception&){}
    const auto la=rows.lift.find(w[0]),lb=rows.lift.find(w[1]);
    if(la!=rows.lift.end()&&lb!=rows.lift.end()){try{ll=Len(s,std::min(la->second,lb->second),std::max(la->second,lb->second));}catch(const std::exception&){}}else ll=0;
    const long double l=std::max(lw,ll);if(l<2*HalfGuard){++g.halves_guard;why="ColumnSplit: halves guard (split edges < 1.4)";return false;}
    splitPolicy.requested=l;struct Clear{~Clear(){splitPolicy.requested=-1;}} clear;
    std::set<Face> faces;for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))faces.insert(r.first);
    const bool ok=ColumnSplit(s,rows,w,g,why,rejected);
    if(ok){for(const auto& f:faces)certified.insert(f);for(const auto& wf:WallFaces(s))if(Has(wf.second,w[0])||Has(wf.second,w[1]))certified.insert(wf.second);++g.acceptedBy["ColumnSplit"];}
    return ok;};
  auto ridgeEdge=[&](Edge w){if(rows.copies.count(w[0])||rows.copies.count(w[1]))return true;
    for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))for(Id x:r.first)if(rows.copies.count(x))return true;return false;};
  std::map<Face,unsigned> firstTried;
  for(;;++g.sweeps) {
    if(limited()){g.outcome="limit";break;}
    if(g.sweeps>=maxSweeps){g.outcome="sweep cap";break;}
    const double R=remaining();bool changed=false;const auto sweepStart=Clock::now();
    // (a) R2 retry (sweeps 0 and 1) and the certified first-child route
    if(g.sweeps<2){const auto t=Clock::now();size_t rowed0=0;for(const auto& wf:WallFaces(s))rowed0+=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});
      const auto apex0=rows.apex;
      const auto r2=PhaseR2(s,static_cast<Rows&>(rows),std::min(60.,.1*R),log);++g.r2_retries;
      for(const auto& r:rows.apex){const auto o=apex0.find(r.first);if(o==apex0.end()||o->second!=r.second)certified.insert(r.first);}
      size_t rowed1=0;for(const auto& wf:WallFaces(s))rowed1+=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});
      g.r2_new_rowed+=rowed1>rowed0?rowed1-rowed0:0;changed|=r2.transactions>0;g.r2_seconds+=Seconds(t);}
    {const auto t=Clock::now();
     for(const auto& wf:WallFaces(s)){if(limited()||Seconds(t)>=.1*R)break;if(!s.mesh.count(wf.first))continue;
       const auto a=rows.apex.find(wf.second);if(a!=rows.apex.end()&&s.vertex_cells.count(a->second))continue;
       const bool hOk=HeightOk(s,wf.first);if(hOk)continue;
       if(firstTried[wf.second]++>=2)continue;
       if(FirstRowInsert(s,rows,wf.first,wf.second,g.first,certified))changed=true;}
     g.first_seconds+=Seconds(t);}
    build();
    // (b) bounded refinement of long edges (wall-first, longest first), at most 40 % of R and 1024 attempts per sweep
    {const auto t=Clock::now();const double slice=.4*R;size_t attempts=0;g.column_demand=0;g.demand.clear();
     auto sliceOver=[&]{return limited()||Seconds(t)>=slice||attempts>=1024;};
     auto touchesColumn=[&](const std::vector<Id>& ids){for(Id id:ids)if(ci.cells.count(id))return true;return false;};
     std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;
     for(const auto& row:s.mesh){if(sliceOver())break;const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
       const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
       long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l>MaximumMetricEdge){const int k=EdgeClass(s,e);longs.emplace_back(k==1||k==2?0:k==3?1:2,-l,e);}}}
     std::sort(longs.begin(),longs.end());std::set<Face> movedThisSweep;
     for(const auto& row:longs) {
       if(sliceOver())break;const Edge e=std::get<2>(row);const auto star=Star(s,e);if(star.empty())continue;
       if(touchesColumn(star)) {
         ++attempts;auto base=[&](Id x)->Id{const auto u=ci.unlift.find(x);if(u!=ci.unlift.end())return u->second;return rows.lift.count(x)?x:UINT64_MAX;};
         const Id b0=base(e[0]),b1=base(e[1]);bool done=false;std::string why;
         Face apexKey{};bool apex=false;for(Id x:e){const auto f=ci.apexFace.find(x);if(f!=ci.apexFace.end()){apexKey=f->second;apex=true;}}
         if(apex){if(!movedThisSweep.count(apexKey)){movedThisSweep.insert(apexKey);const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
             if(rejected.count(k)){++g.cache_skips;why="cached rejection (unchanged cavity)";}else{done=apexMove(apexKey,why);if(!done)rejected.insert(k);}}
           if(!done){long double best=-1;Edge w{0,0};for(int i=0;i<3;++i){const Id x=apexKey[i],y=apexKey[(i+1)%3];long double l=0;try{l=Len(s,x,y);}catch(const std::exception&){l=INFINITY;}if(l>best){best=l;w={std::min(x,y),std::max(x,y)};}}
             if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}}
         else if(b0!=UINT64_MAX&&b1!=UINT64_MAX&&b0!=b1){const Edge w{std::min(b0,b1),std::max(b0,b1)};if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}
         else why="column demand without a column route";
         if(done){changed=true;build();}else{++g.column_demand;++g.demand[Key(why)];g.last="column demand: "+why;}
         continue;
       }
       const auto key=keyOf(1,{e[0],e[1]},star);if(rejected.count(key)){++g.cache_skips;continue;}
       long double now=-std::get<1>(row);try{now=Len(s,e[0],e[1]);}catch(const std::exception&){now=INFINITY;}
       if(!(now>MaximumMetricEdge)){++g.stale_skips;continue;}
       if(now<2*HalfGuard){++g.halves_guard;continue;}
       ++attempts;++g.refine_attempts;splitPolicy.requested=now;bool taken=false;size_t vi=0;
       for(auto& variant:RefineVariantsOf(s,e,g)) {
         const size_t index=vi++;splitPolicy.relocatedCount=0;if(sliceOver())break;std::vector<Id> ids;Patch old,fresh;std::string reason;bool built=false;
         try{built=variant.second(ids,old,fresh,reason);}catch(const std::exception& x){reason=x.what();built=false;}
         if(built&&variant.first=="wall"&&(index==1||index==2)){std::set<Id> before;for(const auto& c:old.cells)for(auto n:c.v)before.insert(n.id);
           std::set<Id> moved;for(const auto& c:fresh.cells)for(auto n:c.v)if(n.id>=s.next_node+1&&!before.count(n.id))moved.insert(n.id);splitPolicy.relocatedCount=moved.size();}
         if(!built){++g.work;++g.rejected;++g.reasons["Refine build: "+Key(reason)];continue;}
         if(touchesColumn(ids)){++g.rejected;++g.reasons["Refine: protected column"];continue;}
         // the production builders return the fresh skin without the old boundary parameters: carry them and export the
         // reference's (DC-11 did this only for its "+repair" variant), so the facet check sees bound boundary points
         for(const auto& pr:old.parameters)fresh.parameters.emplace(pr.first,pr.second);
         try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
         if(attempt(variant.first,Kind::Refine,ids,old,fresh)){taken=true;break;}
       }
       splitPolicy.relocatedCount=0;splitPolicy.requested=-1;
       if(!taken)rejected.insert(key);
       changed|=taken;if(taken)build();
     }
     g.refine_seconds+=Seconds(t);}
    // (d) mesh-wide optimization with a reserved share of the sweep budget
    if(!limited()){const auto t=Clock::now();
      OptimizeOptions opt;opt.maximum_sweeps=8;opt.maximum_seconds=std::max(1.,std::min(remaining(),std::max(.4*R,.5*remaining())));
      opt.cache_entries=size_t(1)<<19;
      for(const auto& row:s.mesh)if(WallChild(s,row.first)){opt.frozen_cells.insert(row.first);for(auto n:row.second.cell.v)opt.frozen_vertices.insert(n.id);}
      for(Id id:FanCells(s,rows)){opt.frozen_cells.insert(id);for(auto n:s.mesh.at(id).cell.v)opt.frozen_vertices.insert(n.id);}
      for(const auto& r:rows.lift)opt.frozen_vertices.insert(r.second);for(const auto& r:rows.copies)for(Id c:r.second.ids)opt.frozen_vertices.insert(c);
      for(const auto& r:rows.apex)opt.frozen_vertices.insert(r.second);
      const RegionMetric target([&s](Point p){return s.field->Query(p).target;},[&s](Point a,Point b){return GeometryLength(s,a,b);});
      OptimizeReport rep;Id nn=s.next_node,nc=s.next_cell;bool moved=false;
      {Scope sc("V12 OptimizeMetricMesh");try{moved=OptimizeMetricMesh(s.mesh,target,nn,nc,opt,rep);}catch(const std::exception& e){rep.stop=std::string("exception: ")+e.what();}}
      s.next_node=std::max(s.next_node,nn);s.next_cell=std::max(s.next_cell,nc);Index(s);
      for(auto& c:G.current){const auto r=rep.relabel.find(c.second);if(r!=rep.relabel.end())c.second=r->second;}
      g.opt.push_back(rep);changed|=moved;g.optimize_seconds+=Seconds(t);
      log<<"DC12 V optimize sweep "<<g.sweeps<<" bad "<<rep.bad_before<<"->"<<rep.bad_after<<" min "<<rep.minimum_before<<"->"<<rep.minimum_after<<" O1="<<rep.edge_removals
         <<" O2="<<rep.face_swaps<<" O3="<<rep.collapses<<" O4="<<rep.moves<<" stop="<<rep.stop<<" t="<<rep.seconds<<"\n";}
    const auto gates=Final(s);
    log<<"DC12 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" unmeasurable="<<gates.unmeasurable<<" work="<<g.work<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC12 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" h="<<gates.height_failures<<" t="<<Seconds(t0)<<std::endl;
    g.per_sweep.push_back({gates.cells,gates.long_edges,gates.bad_shape,gates.height_failures,g.work,Seconds(t0)});
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
inline std::string JsonOpt(const OptimizeReport& r){std::ostringstream o;o<<std::setprecision(8)<<"{\"sweeps\":"<<r.sweeps<<",\"trials\":"<<r.trials<<",\"O1_edge_removals\":"<<r.edge_removals<<",\"O2_face_swaps\":"<<r.face_swaps
  <<",\"O3_collapses\":"<<r.collapses<<",\"O4_moves\":"<<r.moves<<",\"O1L_length_removals\":"<<r.length_removals<<",\"long_edges_seen\":"<<r.long_edges_seen<<",\"O5_boundary_flips\":"<<r.boundary_flips<<",\"O6_boundary_collapses\":"<<r.boundary_collapses<<",\"rejected\":{\"orientation\":"<<r.rejected_orientation<<",\"interface\":"<<r.rejected_interface<<",\"quality\":"<<r.rejected_quality
  <<",\"edge\":"<<r.rejected_edge<<",\"frozen\":"<<r.rejected_frozen<<",\"validity\":"<<r.rejected_validity<<"},\"bad\":["<<r.bad_before<<","<<r.bad_after<<"],\"min_score\":["<<r.minimum_before<<","<<r.minimum_after
  <<"],\"point_queries\":"<<r.point_queries<<",\"edge_integrations\":"<<r.edge_integrations<<",\"edge_estimates\":"<<r.edge_surrogates<<",\"seconds\":"<<r.seconds<<",\"stop\":"<<junction_wp0::Json(r.stop)<<"}";return o.str();}
}  // namespace dc12

namespace dc12 {
// Every DC-12 policy switched on for the scope of one case, restored on exit (earlier harnesses keep their defaults).
struct Policies {
  RefineAcceptance acceptance=GetRefineAcceptance();ShapeAdmission admission=GetShapeAdmission();
  bool hadSector=sectorFirst;
  Policies(){splitPolicy=SplitPolicy{};splitPolicy.rule=1;SetRefineAcceptance(RefineAcceptance::SplitInsertion);SetShapeAdmission(ShapeAdmission::ValidityOnly);
    shapePolicy.validity=true;admitOverride=Admit12;rimByRole=true;artificialMarkers.clear();sectorFirst=false;}
  ~Policies(){SetRefineAcceptance(acceptance);SetShapeAdmission(admission);splitPolicy=SplitPolicy{};shapePolicy=ShapePolicy{};admitOverride=nullptr;
    rimByRole=false;artificialMarkers.clear();reflectiveByRole=false;reflectiveMarkers.clear();directionField=nullptr;directionFieldUses=0;sectorFirst=hadSector;}
};
inline std::string JsonGrowth(State& s,const Growth& G) {
  std::ostringstream o;o<<std::setprecision(8)<<"{\"outcome\":\""<<G.outcome<<"\",\"seconds\":"<<G.seconds<<",\"columns\":"<<G.columns.size()<<",\"layers_run\":"<<G.layers
   <<",\"lifted_points\":"<<G.lifted<<",\"transactions\":"<<G.transactions<<",\"patch_transactions\":"<<G.patch_tx<<",\"single_transactions\":"<<G.single_tx
   <<",\"refused\":"<<G.refused<<",\"max_old_cells\":"<<G.max_old<<",\"max_fresh_cells\":"<<G.max_fresh<<",\"layer_cells\":"<<G.layerKeys.size()<<",\"stops\":{";
  {bool k=false;for(const auto& r:G.stops){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}o<<"},\"refusals\":{";
  {bool k=false;for(const auto& r:G.refusals){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  std::map<size_t,size_t> hist;for(const auto& c:G.columns)++hist[c.stack.size()];o<<"},\"layers_per_column\":{";
  {bool k=false;for(const auto& h:hist){o<<(k?",":"")<<"\""<<h.first<<"\":"<<h.second;k=true;}}
  o<<"},\"clear\":{\"moves\":"<<G.clear_moves<<",\"collapses\":"<<G.clear_collapses<<",\"retries\":"<<G.clear_retries<<",\"retries_accepted\":"<<G.clear_retry_accepted<<",\"refusals\":{";
  {bool k=false;for(const auto& r:G.clear_refusals){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  o<<"}},\"stop_reasons\":{";{bool k=false;for(const auto& r:G.stop_reasons){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  o<<"},\"joint\":{\"retries\":"<<G.joint_retries<<",\"accepted\":"<<G.joint_accepted<<"},\"fixed_blockers\":{";
  {bool k=false;for(const auto& r:G.fixed_blockers){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  o<<"},\"room\":"<<dc16::JsonRoom(dc16::roomStats)<<",\"last_refusal\":"<<junction_wp0::Json(G.last.substr(0,300))<<"}";(void)s;return o.str();
}
inline std::string JsonLayers(const LayerCensus& L) {
  std::vector<long> d=L.deviation;std::sort(d.begin(),d.end());auto at=[&](double f)->long{return d.empty()?0:d[size_t(f*(d.size()-1))];};
  std::ostringstream o;o<<std::setprecision(6)<<"{\"eligible\":"<<L.eligible<<",\"within_2\":"<<L.pass<<",\"fraction\":"<<L.fraction<<",\"without_column\":"<<L.missing<<",\"target_failed\":"<<L.target_failed
   <<",\"deviation_p10_median_p90\":["<<at(.1)<<","<<at(.5)<<","<<at(.9)<<"],\"A2_pass\":"<<(L.fraction>=.9)<<"}";return o.str();
}
inline std::string JsonAudit(const Audit& a){std::ostringstream o;o<<"{\"faces_after_R2\":"<<a.faces<<",\"unchanged\":"<<a.unchanged<<",\"certified_rebuilt\":"<<a.certified<<",\"violations\":"<<a.violations
  <<",\"faces_removed_by_certified_splits\":"<<a.removed<<",\"faces_created\":"<<a.created<<",\"witnesses\":[";for(size_t i=0;i<a.witnesses.size();++i)o<<(i?",":"")<<"\""<<a.witnesses[i]<<"\"";o<<"]}";return o.str();}
}  // namespace dc12
