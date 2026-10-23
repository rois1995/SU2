// DC-12: TEST-ONLY [NativeFirstRowDC12] harness (OPTIMIZE_PLAN rev 3): build-then-optimize. It reuses DC-11 (option-A
// target, W10, R1, R2 single lift, ColumnSplit/ApexMove, halves guard, rejection cache, final gates, export, re-audit) and
// adds, as one connected step: (1) validity-only construction (production ShapeAdmission::ValidityOnly, dc9 shapePolicy for
// the R2/D.2/ConeColumn first-child checks, Admit12 for every other admission: every fresh cell q > 0 and J >= 1e-3, height
// T, facet/marker/L-C, caps, the SplitPolicy rule-1 progress rules); (2) the BL growth-layer operator G (column lifts at the
// production BL law heights d' = h0 + g d of the column's own wall); (3) the certified first-child route FirstRowInsert and
// the R2 retry; (4) V12 = bounded refinement + ApexMove + the production mesh-wide optimizer OptimizeMetricMesh. Final gates
// are unchanged and absolute. Nothing here is production code except the optimizer it calls.
#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
namespace dc12 {
using namespace dc11;
bool Valid(const CellShape& sh){return sh.q>0&&sh.J>=ValiditySliverJacobian;}
std::string Fmt(double x){std::ostringstream o;o<<std::setprecision(6)<<x;return o.str();}

// ---- (1) Admit12: Admit8 with every shape floor replaced by validity --------------------------------------------------
Decision Admit12(State& s,const Patch& old,const Patch& fresh,Kind kind) {
  Scope scope("Admit12");
  Decision base;base.old_cells=old.cells.size();base.fresh_cells=fresh.cells.size();
  if(old.cells.empty()||fresh.cells.empty()||old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){base.category=5;base.reason="replacement size";return base;}
  std::string why;if(!HeightT(s,fresh,why)){base.category=4;base.reason=why;return base;}
  PatchMetric after,before;
  try{auto reuse=ReuseOf(s,old.cells,&fresh.cells);
    after=MeasureMetricCells(fresh.cells,*s.field,nullptr,&reuse,false);StoreLengths(s,after);
    for(const auto& e:after.edges)reuse.insert_or_assign(e.first,e.second);
    before=MeasureMetricCells(old.cells,*s.field,nullptr,&reuse,false);StoreLengths(s,before);}
  catch(const std::exception& e){base.category=5;base.reason=e.what();return base;}
  base.q_old=before.minimum_mean_ratio;base.J_old=before.minimum_scaled_jacobian;base.q_new=after.minimum_mean_ratio;base.J_new=after.minimum_scaled_jacobian;
  for(const auto& c:fresh.cells){const auto sh=ShapeOf(s,c);
    if(IsFirstChild(s,fresh,c)){base.first_q=std::min(base.first_q,sh.q);base.first_J=std::min(base.first_J,sh.J);}
    else{base.other_q=std::min(base.other_q,sh.q);base.other_J=std::min(base.other_J,sh.J);}}
  auto shared=[](const std::map<Edge,EdgeMeasure>& others,const std::pair<const Edge,EdgeMeasure>& e){
    const auto o=others.find(e.first);return o!=others.end()&&SamePoint(o->second.a,e.second.a)&&SamePoint(o->second.b,e.second.b);};
  for(const auto& e:before.edges)if(!shared(after.edges,e))base.removed+=layer2d::ExcessOf(e.second.length);
  for(const auto& e:after.edges)if(!shared(before.edges,e))base.added+=layer2d::ExcessOf(e.second.length);
  const bool size_progress=base.removed>0&&base.added<base.removed*(1-1e-12L);
  const bool quality_progress=(base.q_new>base.q_old+1e-7&&base.J_new>=base.J_old*(1-1e-12))||(base.J_new>base.J_old+1e-7&&base.q_new>=base.q_old*(1-1e-12));
  Decision d=base;
  if(!Valid({base.first_q,base.first_J})){d.category=2;d.reason="validity first child: q="+std::to_string(base.first_q)+" J="+std::to_string(base.first_J);return d;}
  if(!Valid({base.other_q,base.other_J})){d.category=2;d.reason="validity: q="+std::to_string(base.other_q)+" J="+std::to_string(base.other_J);return d;}
  if(kind==Kind::Refine||kind==Kind::Layer) {  // SplitPolicy rule 1 (the 2D split rule), as Admit8
    const bool removes=RemovesVertex(old.cells,fresh.cells,splitPolicy.relocated,splitPolicy.relocatedCount);
    if(kind==Kind::Layer){if(!removes){++splitPolicy.layer_free;d.ok=true;return d;}++splitPolicy.layer_checked;
      if(base.added<=base.removed){d.ok=true;return d;}d.category=3;d.reason="layer: removes a vertex and adds frozen excess";return d;}
    if(removes){++splitPolicy.vertex_removal_checks;if(size_progress){d.ok=true;return d;}
      ++splitPolicy.vertex_removal_refusals;d.category=3;d.reason="refine: removes a vertex without strict changed-edge excess reduction";return d;}
    long double longest=splitPolicy.requested;
    if(longest<0){longest=0;for(const auto& e:before.edges)if(!shared(after.edges,e))longest=std::max(longest,e.second.length);}
    if(longest>(long double)SplitInsertionLength){++splitPolicy.size_insertions;d.ok=true;return d;}
    ++splitPolicy.nonsize_checks;bool interiorOk=base.added==0;
    if(interiorOk){std::set<Edge> in;try{in=InteriorEdges(fresh.cells);}catch(const std::exception&){interiorOk=false;}
      for(const auto& e:after.edges)if(in.count(e.first)&&e.second.length>MaximumMetricEdge)interiorOk=false;}
    if(interiorOk&&base.q_new>base.q_old+1e-7){d.ok=true;return d;}
    ++splitPolicy.nonsize_refusals;d.category=3;d.reason="refine: non-size insertion adds an oversized interior edge or improves no shape";return d;
  }
  if(kind==Kind::Repair){if(size_progress||(quality_progress&&base.added<=base.removed)){d.ok=true;return d;}d.category=3;d.reason="repair: neither size nor quality progress";return d;}
  d.category=3;d.reason="unknown kind";return d;
}

// ---- wall policy of a column (production layers, per wall) -------------------------------------------------------------
std::map<int,unsigned short> layerOfMarker;
void MapLayers(State& s){layerOfMarker.clear();for(int m:s.walls)for(unsigned short w=0;w<s.layers->GetnWall();++w)if(s.layers->GetLayer(w).marker==s.markers.at(m))layerOfMarker[m]=w;}
long double WallDistanceTo(State& s,unsigned short w,V x){const su2double X[3]={double(x.x),double(x.y),double(x.z)};return SU2_TYPE::GetValue(s.layers->Evaluate(w,X,s.core).distance);}
long double MinWallDistance(State& s,V x){long double d=INFINITY;for(unsigned short w=0;w<s.layers->GetnWall();++w)d=std::min(d,WallDistanceTo(s,w,x));return d;}
long double FirstHeightOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).firstHeight);}
long double GrowthOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).growth);}
long double ThicknessOf(State& s,unsigned short w){return SU2_TYPE::GetValue(s.layers->GetLayer(w).thickness);}
// Per-marker first heights from the matching production wall policy (OPTIMIZE_PLAN sect. 3); s.h0 = their minimum.
void ApplyWallPolicies(State& s) {
  MapLayers(s);long double low=INFINITY;
  for(const auto& m:layerOfMarker){const auto h=FirstHeightOf(s,m.second);s.heights[m.first]=double(h);low=std::min(low,h);}
  if(std::isfinite(low))s.h0=double(low);s.heights.tolerance=HeightTolerance;
}

// ---- column directions (OPTIMIZE_PLAN rev 5): boundary tangency n' = n - (n.s) s, smooth blend, visibility ---------
struct DirField { std::map<Id,V> d;size_t vertices=0,constrained=0,lines=0,blended=0,fallback=0,witness=0,visibility_reduced=0,kept=0,low_margin=0;
  long double min_margin=INFINITY,max_turn_deg=0;std::vector<std::string> witnesses; };
long double SmoothStep(long double u){u=std::clamp(u,0.L,1.L);return u*u*(3-2*u);}
DirField ComputeDirectionField(State& s,long double margin=.2L) {
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
std::string JsonDirections(const DirField& F){std::ostringstream o;o<<std::setprecision(6)<<"{\"wall_vertices\":"<<F.vertices<<",\"constrained_plane\":"<<F.constrained-F.lines<<",\"constrained_line\":"<<F.lines
  <<",\"blended\":"<<F.blended<<",\"blend_kept_unconstrained\":"<<F.kept<<",\"visibility_reduced\":"<<F.visibility_reduced<<",\"fallback_mirrored_ring\":"<<F.fallback<<",\"witnesses\":"<<F.witness
  <<",\"below_margin_0.2\":"<<F.low_margin<<",\"min_visibility\":"<<double(F.min_margin)<<",\"max_turn_deg\":"<<double(F.max_turn_deg)<<",\"uses_in_DirectionAt\":"<<directionFieldUses<<",\"witness_list\":[";
  for(size_t i=0;i<F.witnesses.size();++i)o<<(i?",":"")<<junction_wp0::Json(F.witnesses[i]);o<<"]}";return o.str();}

// ---- (2) BL growth layers ---------------------------------------------------------------------------------------------
struct Column { Id wall=0,top=0,lid=0;V dir;std::set<unsigned short> walls;std::vector<Id> stack;bool stopped=false;std::string stop;
  unsigned refusals=0;long double stopHeight=INFINITY,lidDistance=0;int owner=-1; };
struct Growth {
  std::vector<Column> columns;std::set<std::array<Id,4>> layerKeys;std::map<std::string,size_t> stops,refusals,accepted;
  size_t transactions=0,patch_tx=0,single_tx=0,lifted=0,refused=0,layers=0,max_old=0,max_fresh=0;double seconds=0;std::string outcome,last;
  std::map<Id,Id> current;  // stack vertex identity -> its identity now (optimizer moves)
  std::set<Id> forceStop;   // tests: wall vertices whose column never grows
};
struct Step { bool ok=false;std::string stop;V u;long double d=0,next=0;int wall=-1; };
// Target metric density along the unit direction d at x, and the distance along d from x0 at which the target metric
// length reaches `units` (trapezoid on point tensors, substeps of 1/8 of the local size, linear inverse in the last one).
long double Density(State& s,V x,V d){const Tensor m=s.field->Query(Pt(x)).target;
  return std::sqrt(d.x*(m.xx*d.x+m.xy*d.y+m.xz*d.z)+d.y*(m.xy*d.x+m.yy*d.y+m.yz*d.z)+d.z*(m.xz*d.x+m.yz*d.y+m.zz*d.z));}
long double UnitStep(State& s,V x0,V d,long double units=1) {
  long double lam=0,L=0,r0=Density(s,x0,d);
  for(int i=0;i<400;++i){if(!(r0>0))return -1;const long double h=.125L/r0,r1=Density(s,x0+(lam+h)*d,d),inc=h*(r0+r1)/2;
    if(L+inc>=units)return lam+h*(units-L)/inc;L+=inc;lam+=h;r0=r1;}
  return -1;
}
// Next layer of a column (OPTIMIZE_PLAN sect. 2, rev 5): one unit step of the target metric along the column, i.e. the row
// height follows the target's wall-normal size hn(d) (the production law h0 g^k, or its configured ADAP_BL_BUDGET outer
// profile); owner = the wall with the smallest next-row height h0 + (g-1) d at its own distance (thickness, reports).
Step NextLayer(State& s,const Column& c) {
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
long double TangentialSize(State& s,V x,V d) {
  const Tensor m=s.field->Query(Pt(x)).target;V e1=Cross(d,{1,0,0});if(Norm(e1)<.5L)e1=Cross(d,{0,1,0});e1=Unit(e1);const V e2=Unit(Cross(d,e1));
  auto q=[&](V a,V b){return a.x*(m.xx*b.x+m.xy*b.y+m.xz*b.z)+a.y*(m.xy*b.x+m.yy*b.y+m.yz*b.z)+a.z*(m.xz*b.x+m.yz*b.y+m.zz*b.z);};
  const long double a=q(e1,e1),b=q(e1,e2),c=q(e2,e2),lmax=(a+c)/2+std::sqrt(((a-c)/2)*((a-c)/2)+b*b);
  return lmax>0?1/std::sqrt(lmax):INFINITY;
}
struct LTx { bool ok=false;std::string cls,witness;std::vector<Id> ids;Patch fresh;std::map<Id,Id> lifted;std::vector<std::array<Id,4>> keys;size_t old_cells=0,fresh_cells=0; };
// One layer transaction for the tops of T: the cells of their stars above the stack are re-attached to the new points
// (R2 lift mechanics), every top face of the stack touching T is filled by the sequential decomposition in increasing id
// order of its lifted vertices; carrier faces, carrier loops, interface, D.2 and validity are checked as for R2.
LTx LayerTransaction(State& s,const std::function<bool(Id)>& below,const std::map<Id,V>& targets) {
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
  for(const auto& c:old.cells){Cell m=c;for(auto& n:m.v){const auto f=L.find(n.id);if(f!=L.end())n=nodes.at(f->second);}
    if(!Positive(m.v)){tx.cls="clearance";tx.witness="moved cell "+std::to_string(c.id)+" inverts (the volume above is too close)";return tx;}fresh.push_back(m);}
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
  // D.2: a wall face of the cavity keeps a passing first child (height T and validity)
  for(const auto& sf:out.surface){if(!s.walls.count(markerOf(sf.facet)))continue;const auto key=Sorted(sf.v);
    Patch before,after;for(const auto& c:old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)before.cells.push_back(c);}
    for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)after.cells.push_back(c);}
    before.surface=after.surface={sf};before.parameters=old.parameters;after.parameters=out.parameters;
    std::string w0,w1;const bool was=before.cells.size()==1&&HeightT(s,before,w0)&&Valid(Shape(s,before.cells.front()));
    if(was&&!(after.cells.size()==1&&HeightT(s,after,w1)&&Valid(Shape(s,after.cells.front())))){tx.cls="invariant";tx.witness="a wall face loses its passing first child";return tx;}}
  for(const auto& c:fresh)if(!Valid(Shape(s,c))){const auto sh=Shape(s,c);tx.cls="validity";tx.witness="fresh cell q="+Fmt(sh.q)+" J="+Fmt(sh.J);return tx;}
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
// G: layer index by layer index; per layer, bounded wall patches of adjacent planned tops (<= 8 tops, above cavity <= 64),
// each member alone on failure, one retry of every refused column at the same index; stop rules s1-s4 (OPTIMIZE_PLAN 2).
Growth PhaseG(State& s,Rows10& rows,double seconds,std::ostream& log,const std::set<Id>& forceStop={}) {
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
  auto stop=[&](Column& c,const std::string& why,long double height){c.stopped=true;c.stop=why;c.stopHeight=height;++G.stops[why.substr(0,why.find(' '))];};
  auto apply=[&](const LTx& tx,const std::map<Id,size_t>& owner) {
    Apply(s,tx.ids,tx.fresh);for(const auto& k:tx.keys)G.layerKeys.insert(k);
    for(const auto& l:tx.lifted){auto& c=G.columns[owner.at(l.first)];c.stack.push_back(l.second);c.top=l.second;G.current[l.second]=l.second;++G.lifted;}
    ++G.transactions;G.max_old=std::max(G.max_old,tx.old_cells);G.max_fresh=std::max(G.max_fresh,tx.fresh_cells);};
  for(unsigned k=0;k<48;++k) {
    if(Seconds(t0)>=seconds){G.outcome="limit";break;}
    std::vector<size_t> active;for(size_t i=0;i<G.columns.size();++i)if(!G.columns[i].stopped&&G.columns[i].stack.size()==k)active.push_back(i);
    if(active.empty())break;
    std::map<Id,V> plan;std::map<Id,size_t> owner;
    for(size_t i:active) {
      auto& c=G.columns[i];if(!s.vertex_cells.count(c.top)){stop(c,"s3 top vanished",INFINITY);continue;}
      Step st;try{st=NextLayer(s,c);}catch(const std::exception& e){st.ok=false;st.stop=std::string("s3 law: ")+e.what();}
      if(!st.ok){stop(c,st.stop,st.stop.rfind("s1",0)==0?st.d:INFINITY);continue;}
      long double tangential=INFINITY;try{tangential=TangentialSize(s,st.u,c.dir);}catch(const std::exception&){}
      if(st.next-st.d>=tangential){stop(c,"s2 isotropy",st.d);continue;}
      long double l=-1;try{l=GeometryLength(s,NodePoint(s,c.top),Pt(st.u));}catch(const std::exception&){l=-1;}
      if(l<0){stop(c,"s3 unmeasurable column segment",INFINITY);continue;}
      if(l<.6L||l>1.5L){stop(c,std::string("s1 metric ")+(l<.6L?"coarser":"finer")+" than the law",st.d);continue;}
      plan[c.top]=st.u;owner[c.top]=i;
    }
    if(plan.empty())break;
    std::vector<Id> tops;for(const auto& p:plan)tops.push_back(p.first);
    std::set<Id> done,refusedNow;
    auto aboveOf=[&](Id t){std::set<Id> out;for(Id id:StarOf(s,t))if(!below(id))out.insert(id);return out;};
    auto attempt=[&](const std::set<Id>& T)->bool{
      std::map<Id,V> targets;for(Id t:T)targets[t]=plan.at(t);
      LTx tx;try{tx=LayerTransaction(s,below,targets);}catch(const std::exception& e){tx.ok=false;tx.cls="exception";tx.witness=e.what();}
      if(tx.ok){apply(tx,owner);++G.accepted[T.size()>1?"patch":"single"];(T.size()>1?G.patch_tx:G.single_tx)++;return true;}
      ++G.refused;++G.refusals[tx.cls];G.last=tx.cls+": "+tx.witness;log<<"DC12 G refuse |T|="<<T.size()<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";return false;};
    for(Id seed:tops) {
      if(done.count(seed))continue;if(Seconds(t0)>=seconds)break;
      std::set<Id> T{seed},cav=aboveOf(seed);
      for(bool grew=true;grew&&T.size()<8;) {grew=false;
        std::set<Id> cand;for(Id t:T)for(Id id:StarOf(s,t))for(auto n:s.mesh.at(id).cell.v)if(plan.count(n.id)&&!done.count(n.id)&&!T.count(n.id))cand.insert(n.id);
        for(Id x:cand){auto more=cav;for(Id id:aboveOf(x))more.insert(id);if(more.size()<=MaximumCavityCells){T.insert(x);cav=more;grew=true;break;}}}
      for(Id t:T)done.insert(t);
      if(attempt(T))continue;
      if(T.size()>1){for(Id t:T)if(!attempt({t}))refusedNow.insert(t);}else refusedNow.insert(seed);
    }
    for(Id t:refusedNow){auto& c=G.columns[owner.at(t)];if(c.stopped||c.stack.size()!=k)continue;
      if(++c.refusals>=2||!attempt({t})){if(c.stack.size()==k)stop(c,"s3 refused transactions ("+G.last.substr(0,G.last.find(':'))+")",INFINITY);}}
    ++G.layers;
    log<<"DC12 G layer "<<k<<" planned="<<plan.size()<<" lifted_total="<<G.lifted<<" refused_total="<<G.refused<<" t="<<Seconds(t0)<<"\n";
  }
  for(auto& c:G.columns)if(!c.stopped&&c.stack.size()>=48){c.stopped=true;c.stop="s4 layer cap";++G.stops["s4"];}
  if(G.outcome.empty())G.outcome="complete";G.seconds=Seconds(t0);return G;
}

// ---- census (A2) and first-row audit (A3) --------------------------------------------------------------------------
struct Bands { std::vector<long double> edges{0,3,25,50,100,200,800,INFINITY};std::vector<size_t> count; };
Bands BandCensus(State& s) {
  Bands b;b.count.assign(b.edges.size()-1,0);std::set<Id> seen;
  for(const auto& row:s.mesh)for(auto n:row.second.cell.v){if(!seen.insert(n.id).second)continue;const auto d=MinWallDistance(s,Of(n.p))/s.h0;
    for(size_t i=0;i+1<b.edges.size();++i)if(d>=b.edges[i]&&d<b.edges[i+1]){++b.count[i];break;}}
  return b;
}
std::string JsonBands(const Bands& b){std::ostringstream o;o<<"{";for(size_t i=0;i<b.count.size();++i)o<<(i?",":"")<<"\""<<double(b.edges[i])<<"-"<<(std::isfinite(b.edges[i+1])?std::to_string(int(b.edges[i+1])):"inf")<<"\":"<<b.count[i];o<<"}";return o.str();}
struct LayerCensus { size_t eligible=0,pass=0,missing=0,target_failed=0;std::vector<long> deviation;double fraction=0; };
// A2 over the post-R2 population (non-rim wall vertices after R2, independent of construction success): target = law
// interfaces in [3 h0, 25 h0] of the owning wall (NextLayer's rule at the lid; the smallest h0 without a column), from the
// lid distance (nominal h0 (1+g) without a lid), cut at the owner's thickness and at a metric stop height (s1/s2); count =
// the column's stack vertices still in the mesh (relabels followed) at owner-wall distance in the band, 0 without a column.
struct WallOf { std::set<unsigned short> walls;Point wall; };
std::map<Id,WallOf> EligibleAfterR2(State& s) {
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
LayerCensus CountLayers(State& s,const Growth& G,const std::map<Id,WallOf>& eligible) {
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
std::map<Face,std::array<Id,4>> FirstChildren(State& s){std::map<Face,std::array<Id,4>> out;for(const auto& wf:WallFaces(s))out[wf.second]=CellKey(s.mesh.at(wf.first).cell);return out;}
struct Audit { size_t faces=0,unchanged=0,certified=0,violations=0,removed=0,created=0;std::vector<std::string> witnesses; };
Audit FirstRowAudit(State& s,const std::map<Face,std::array<Id,4>>& before,const std::set<Face>& certified) {
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
void MoveRegistrations(Rows10& rows,Id from,Id to,std::set<Face>& certified,FirstRowStats* st=nullptr) {
  for(auto& r:rows.apex)if(r.second==from){r.second=to;certified.insert(r.first);if(st)++st->registrations_moved;}
}
size_t StaleRegistrations(const State& s,const Rows10& rows){size_t n=0;for(const auto& r:rows.apex)n+=!s.vertex_cells.count(r.second);return n;}
// (iii) ApexRelocate: the existing apex v of the face's first child (interior or on carriers, never a wall or lid vertex)
// moved by production MoveVertex to the best ConeColumn-grid candidate bound to v's carriers; Admit12 Repair + D.2.
bool ApexRelocate(State& s,Rows10& rows,Id cell,const Face& key,FirstRowStats& st,std::set<Face>& certified) {
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
  double best=-1;Patch chosen;
  for(long double eta:{1.L,.92L,1.08L})for(const auto& w:feet) {
    Node node{s.next_node,Pt(w[0]*a+w[1]*b+w[2]*cc+(eta*h0)*n)};
    try{if(!carriers.empty())node=Constrain(s,priv.get(),node,carriers,carriers);}catch(const std::exception&){continue;}
    Patch fresh;std::string why;bool ok=false;try{ok=MoveVertex(old,v,node,s.next_cell,ref,fresh,why);if(ok)ref.ExportParameters(fresh);}catch(const std::exception&){ok=false;}
    if(!ok)continue;{std::string d2;if(!D2Ok(s,old,fresh,d2))continue;}
    const auto d=Admit12(s,old,fresh,Kind::Repair);if(!d.ok)continue;
    double score=1e300;for(const auto& x:fresh.cells)if(IsFirstChild(s,fresh,x)){const auto sh=Shape(s,x);score=std::min(score,std::min(sh.q/MinimumMeanRatio,sh.J/MinimumScaledJacobian));}
    if(score>best){best=score;chosen=std::move(fresh);}
  }
  if(best<0){++st.why["relocate: no admissible candidate"];return false;}
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
  const Id moved=s.next_node;Apply(s,ids,chosen);
  MoveRegistrations(rows,v,moved,certified,&st);rows.apex[key]=moved;certified.insert(key);++st.relocated;return true;
}
bool FirstRowInsert(State& s,Rows10& rows,Id cell,const Face& key,FirstRowStats& st,std::set<Face>& certified) {
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
    std::vector<Point> backgrounds{Centroid(old.cells.front())};double best=-1;Patch chosen;
    for(const auto& f:old.surface){const auto h=s.heights.find(s.reference->Get(f.facet).marker);if(h==s.heights.end())continue;
      std::array<V,3> q;for(unsigned k=0;k<3;++k)for(auto n:old.cells.front().v)if(n.id==f.v[k])q[k]=Of(n.p);
      const V nn=Unit(Cross(q[1]-q[0],q[2]-q[0]));const V cc=(1.L/3)*(q[0]+q[1]+q[2]);
      for(double factor:{1.25,1.5,2.})backgrounds.push_back(Pt(cc-(factor*h->second)*nn));}
    for(const auto& b:backgrounds){Patch candidate;std::string reason;
      try{if(!ConstructFirstLayer(old,{s.next_node,b},s.next_node+1,s.next_cell,s.heights,*s.reference,candidate,report,reason)){why=reason;continue;}}catch(const std::exception& e){why=e.what();continue;}
      if(!admit(candidate))continue;double q=-1;try{q=MetricShapeQuality(candidate.cells,*s.field);}catch(const std::exception&){}
      if(q>best){best=q;chosen=std::move(candidate);ok=true;}}
    if(ok)fresh=std::move(chosen);
  }
  if(ok) {
    Id apex=UINT64_MAX;for(const auto& c:fresh.cells){unsigned k=0;Id other=UINT64_MAX;for(auto n:c.v){if(Has(key,n.id))++k;else other=n.id;}if(k==3&&!before.count(other))apex=other;}
    for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
    Apply(s,{cell},fresh);if(apex!=UINT64_MAX)rows.apex[key]=apex;certified.insert(key);++st.inserted;return true;
  }
  ++st.why["insert: "+Key(why)];
  if(ApexRelocate(s,rows,cell,key,st,certified))return true;
  ++st.refused;return false;
}
// ApexMove with every registration of the old apex following the new one (shared apices) and the faces certified.
bool ApexMove12(State& s,Rows10& rows,const Face& key,VStage& g,std::string& why,std::set<Face>& certified) {
  const auto it=rows.apex.find(key);if(it==rows.apex.end()){why="not rowed";return false;}const Id before=it->second;
  // an apex on a carrier (non-wall physical face) cannot be re-coned: carrier-constrained relocation instead
  bool carrier=false;for(const auto& f:PhysicalOf(s,StarOf(s,before),false))carrier=carrier||Has(f.key,before);
  if(carrier){Id cell=UINT64_MAX;for(const auto& wf:WallFaces(s))if(wf.second==key)cell=wf.first;if(cell==UINT64_MAX){why="ApexRelocate: wall face gone";return false;}
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
VStage12 PhaseV12(State& s,Rows10& rows,Growth& G,size_t work,double seconds,size_t maxSweeps,std::set<Face>& certified,std::ostream& log) {
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
  auto build=[&]{ci.Build(s,rows);for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;
    apexIds.clear();for(const auto& r:ci.apexFace)apexIds.insert(r.first);splitPolicy.relocated=&apexIds;};
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
       double w=0;const bool hOk=HeightOk(s,wf.first,&w);if(hOk&&!Bad(MeshShape(s,wf.first)))continue;
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
    // (c) bad first-row column cells -> ApexMove (certified, Admit12 Repair through the override)
    {std::vector<std::pair<double,Id>> badColumn;for(const auto& row:s.mesh)if(ci.cells.count(row.first)){const auto sh=MeshShape(s,row.first);if(Bad(sh))badColumn.emplace_back(sh.q,row.first);}
     std::sort(badColumn.begin(),badColumn.end());std::set<Face> moved;
     for(const auto& b:badColumn){if(limited()||Seconds(sweepStart)>=.6*R)break;if(!s.mesh.count(b.second))continue;  // keep the optimizer's 40 % share
       Face apexKey{};bool found=false;for(auto n:s.mesh.at(b.second).cell.v){const auto f=ci.apexFace.find(n.id);if(f!=ci.apexFace.end()&&rows.apex.count(f->second)&&rows.apex.at(f->second)==n.id){apexKey=f->second;found=true;}}
       if(!found||!moved.insert(apexKey).second)continue;
       const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
       if(rejected.count(k))continue;std::string why;if(apexMove(apexKey,why)){changed=true;build();}else{rejected.insert(k);++g.demand["bad column cell: "+Key(why)];}}
    }
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
std::string JsonOpt(const OptimizeReport& r){std::ostringstream o;o<<std::setprecision(8)<<"{\"sweeps\":"<<r.sweeps<<",\"trials\":"<<r.trials<<",\"O1_edge_removals\":"<<r.edge_removals<<",\"O2_face_swaps\":"<<r.face_swaps
  <<",\"O3_collapses\":"<<r.collapses<<",\"O4_moves\":"<<r.moves<<",\"O1L_length_removals\":"<<r.length_removals<<",\"long_edges_seen\":"<<r.long_edges_seen<<",\"O5_boundary_flips\":"<<r.boundary_flips<<",\"O6_boundary_collapses\":"<<r.boundary_collapses<<",\"rejected\":{\"orientation\":"<<r.rejected_orientation<<",\"interface\":"<<r.rejected_interface<<",\"quality\":"<<r.rejected_quality
  <<",\"edge\":"<<r.rejected_edge<<",\"frozen\":"<<r.rejected_frozen<<",\"validity\":"<<r.rejected_validity<<"},\"bad\":["<<r.bad_before<<","<<r.bad_after<<"],\"min_score\":["<<r.minimum_before<<","<<r.minimum_after
  <<"],\"point_queries\":"<<r.point_queries<<",\"edge_integrations\":"<<r.edge_integrations<<",\"edge_estimates\":"<<r.edge_surrogates<<",\"seconds\":"<<r.seconds<<",\"stop\":"<<junction_wp0::Json(r.stop)<<"}";return o.str();}
}  // namespace dc12

namespace dc12 {
// Every DC-12 policy switched on for the scope of one case, restored on exit (earlier harnesses keep their defaults).
struct Policies {
  RefineAcceptance acceptance=GetRefineAcceptance();ShapeAdmission admission=GetShapeAdmission();
  bool hadSector=std::getenv("SU2_DC9_SECTOR_FIRST")!=nullptr;std::string sector=hadSector?std::getenv("SU2_DC9_SECTOR_FIRST"):"";
  Policies(){splitPolicy=SplitPolicy{};splitPolicy.rule=1;SetRefineAcceptance(RefineAcceptance::SplitInsertion);SetShapeAdmission(ShapeAdmission::ValidityOnly);
    shapePolicy.validity=true;admitOverride=Admit12;rimByRole=true;artificialMarkers.clear();unsetenv("SU2_DC9_SECTOR_FIRST");}
  ~Policies(){SetRefineAcceptance(acceptance);SetShapeAdmission(admission);splitPolicy=SplitPolicy{};shapePolicy=ShapePolicy{};admitOverride=nullptr;
    rimByRole=false;artificialMarkers.clear();reflectiveByRole=false;reflectiveMarkers.clear();directionField=nullptr;directionFieldUses=0;if(hadSector)setenv("SU2_DC9_SECTOR_FIRST",sector.c_str(),1);}
};
std::string JsonGrowth(State& s,const Growth& G) {
  std::ostringstream o;o<<std::setprecision(8)<<"{\"outcome\":\""<<G.outcome<<"\",\"seconds\":"<<G.seconds<<",\"columns\":"<<G.columns.size()<<",\"layers_run\":"<<G.layers
   <<",\"lifted_points\":"<<G.lifted<<",\"transactions\":"<<G.transactions<<",\"patch_transactions\":"<<G.patch_tx<<",\"single_transactions\":"<<G.single_tx
   <<",\"refused\":"<<G.refused<<",\"max_old_cells\":"<<G.max_old<<",\"max_fresh_cells\":"<<G.max_fresh<<",\"layer_cells\":"<<G.layerKeys.size()<<",\"stops\":{";
  {bool k=false;for(const auto& r:G.stops){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}o<<"},\"refusals\":{";
  {bool k=false;for(const auto& r:G.refusals){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  std::map<size_t,size_t> hist;for(const auto& c:G.columns)++hist[c.stack.size()];o<<"},\"layers_per_column\":{";
  {bool k=false;for(const auto& h:hist){o<<(k?",":"")<<"\""<<h.first<<"\":"<<h.second;k=true;}}
  o<<"},\"last_refusal\":"<<junction_wp0::Json(G.last.substr(0,300))<<"}";(void)s;return o.str();
}
std::string JsonLayers(const LayerCensus& L) {
  std::vector<long> d=L.deviation;std::sort(d.begin(),d.end());auto at=[&](double f)->long{return d.empty()?0:d[size_t(f*(d.size()-1))];};
  std::ostringstream o;o<<std::setprecision(6)<<"{\"eligible\":"<<L.eligible<<",\"within_2\":"<<L.pass<<",\"fraction\":"<<L.fraction<<",\"without_column\":"<<L.missing<<",\"target_failed\":"<<L.target_failed
   <<",\"deviation_p10_median_p90\":["<<at(.1)<<","<<at(.5)<<","<<at(.9)<<"],\"A2_pass\":"<<(L.fraction>=.9)<<"}";return o.str();
}
std::string JsonAudit(const Audit& a){std::ostringstream o;o<<"{\"faces_after_R2\":"<<a.faces<<",\"unchanged\":"<<a.unchanged<<",\"certified_rebuilt\":"<<a.certified<<",\"violations\":"<<a.violations
  <<",\"faces_removed_by_certified_splits\":"<<a.removed<<",\"faces_created\":"<<a.created<<",\"witnesses\":[";for(size_t i=0;i<a.witnesses.size();++i)o<<(i?",":"")<<"\""<<a.witnesses[i]<<"\"";o<<"]}";return o.str();}
}  // namespace dc12

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
// x=0, 2 artificial boundary, 3 second wall z=1 (topWall) and 4 second reflective carrier y=0 (seam); names from the spec only.
struct BoxSpec { int n=4;double h0=4e-3,g=1.2,thickness=.5,h0Top=6e-3,gTop=1.15,angle=0,shear=0,shearY=0;bool topWall=false,seam=false;
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
  s->markers={b.names[0],b.names[1],b.names[2],b.names[3],b.names[4]};
  auto plane=[&](const Face& f)->int{std::array<std::array<int,3>,3> q;for(int t=0;t<3;++t)q[t]=ijk.at(f[t]);
    auto all=[&](int axis,int val){return q[0][axis]==val&&q[1][axis]==val&&q[2][axis]==val;};
    if(all(2,0))return 0;if(all(0,0))return 1;if(all(2,n))return b.topWall?3:2;if(all(1,0))return b.seam?4:2;return 2;};
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
  for(const auto& f:s->facets)if(!s->walls.count(f.marker)){CBoundaryLayerMetric::FeatureFace p;p.reflective=f.marker==1||f.marker==4;
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
    r.s=SyntheticBox(b);ApplyWallPolicies(*r.s);DetectCreases(*r.s);size_t unbound=0;r.areas=MarkerAreas(*r.s,unbound);REQUIRE(unbound==0);
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
