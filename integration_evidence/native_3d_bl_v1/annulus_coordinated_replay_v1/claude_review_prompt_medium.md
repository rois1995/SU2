Please review this connected native tetrahedral remeshing change for correctness. The user explicitly authorized consultation using Opus5.5 with high effort. Read-only review: do not edit files, start builds/tests, launch agents or contact other services. A one-worker build is currently running; source files must remain unchanged. You may read connected source using Read/Grep/Glob.

Goal: coordinate an inserted surface point and complete-star surface/interior vertices to escape the saved finite annular BL construction blockage, preserving all geometry, first-height, positive-volume, finite-hole, quality/full-edge acceptance and MPI versioned/atomic semantics. Original sensor-only P1 donors and actual-query geometric BL composition are immutable. General symmetry/junction/2D/exterior requirements remain open; this is not a completeness review.

Independent saved-patch experiment: 50->54tets with same connectivity split. On-edge insertion+2interior points still q=.1803/J=.0402. Off-edge+2interior points q=.20193/J=.05049, changed-edge squared excess decreases77.2%; adding two smooth symmetry vertices gives q=.201996/J=.050491 and86.97% reduction. Exact positive volume, unchanged artificial interface, complete moved stars, finite physical face coverage and internal/external no-overlap all pass. Actual native construction still awaits current build. Same-field complete204-tet witness exists but is not a native lifecycle.

Focus on concrete bugs or overlooked necessary coordination. In particular: using existing CoupledMetricDirection inside PlaceSplitPatch, trial state/ID consistency, unchanged interface, correct physical-vertex admission via complete-star metadata and the original full acceptance gate, and loss/line-search consistency. Avoid proposing broad frameworks or additional local fallbacks. Return concise ranked findings with exact file/line references and one short assessment of this strategy. Flag uncertainty; do not assume a successful build or accepted mesh.

Current reviewed diff follows:
diff --git a/Common/include/adaptation/CNativeBoundary3D.hpp b/Common/include/adaptation/CNativeBoundary3D.hpp
index a7cd187c8a..e17b4be0ba 100644
--- a/Common/include/adaptation/CNativeBoundary3D.hpp
+++ b/Common/include/adaptation/CNativeBoundary3D.hpp
@@ -192,8 +192,8 @@ bool ReconstructSplitPatch(const Patch& old,Edge edge,Node point,Node interior,c
                            std::string& reason,KernelStats* stats = nullptr,
                            const std::map<int,double>& heights = {});
 class FrozenField;
-/*! Tentative bounded cover of split cells by non-endpoint vertices absent from the local physical skin.
- * Optional costs contain only globally admitted free candidates; missing entries are unavailable.
+/*! Tentative bounded cover of split cells by non-endpoint vertices; physical vertices are excluded by default.
+ * Optional costs contain only globally admitted movable candidates, including smooth physical vertices; missing entries are unavailable.
  * Minimum total cost within eight apices; at most511 binary search states. Complete stars remain caller duties.
  * Empty means no bounded cover. */
 std::vector<Id> SplitPatchApices(const Patch& patch,Edge edge,const std::map<Id,size_t>* costs=nullptr);
@@ -205,10 +205,10 @@ struct JointSplitReport {
   double initial_loss=0, best_loss=0, best_shape_loss=0;
   Patch best;  // Geometric/height-valid private proposal, including failed shape; never an accepted mesh.
 };
-/*! Bounded actual-field-directed placement of one split and up to eight free complete-star apices.
- * Surface points retain their chart; interior points have three spatial freedoms. All trials reconstruct
- * from the original patch; artificial interfaces and first-height constraints
- * remain fixed. Proposal scores use midpoint lengths only; success requires full composed Refine
+/*! Bounded actual-field-directed placement of one split and up to eight complete-star surface/interior vertices.
+ * Surface points retain their finite authority; the split may leave a same-chart edge. Coupled shape derivatives
+ * coordinate all admitted freedoms. All trials reconstruct from the original patch; artificial interfaces
+ * and first-height constraints remain fixed. Proposal scores use midpoint lengths only; success requires full composed Refine
  * validation. Placement reserves half of the shared trial bound for optional point-preserving private
  * reconnection; only the split and admitted apices may move, using their reserved identities.
  * Output is unchanged on failure; distributed admission and external clearance are caller duties. */
diff --git a/Common/src/adaptation/CNativeBoundary3D.cpp b/Common/src/adaptation/CNativeBoundary3D.cpp
index 5b8f2cb113..81340a996d 100644
--- a/Common/src/adaptation/CNativeBoundary3D.cpp
+++ b/Common/src/adaptation/CNativeBoundary3D.cpp
@@ -1443,7 +1443,7 @@ std::vector<Id> SplitPatchApices(const Patch& patch,Edge edge,const std::map<Id,
   if(pairs.empty())return {};
   std::map<Id,uint64_t> covered;std::map<Id,size_t> weight;
   for(size_t i=0;i<pairs.size();++i)for(Id id:pairs[i]) {
-    if(fixed.count(id) || (costs && (!costs->count(id) || !costs->at(id))))continue;
+    if((!costs && fixed.count(id)) || (costs && (!costs->count(id) || !costs->at(id))))continue;
     covered[id]|=uint64_t(1)<<i;weight[id]=costs?costs->at(id):1;
   }
   const uint64_t all=pairs.size()==64?UINT64_MAX:(uint64_t(1)<<pairs.size())-1;
@@ -1464,56 +1464,29 @@ std::vector<Id> SplitPatchApices(const Patch& patch,Edge edge,const std::map<Id,
   };
   visit(0,0);return best;
 }
+namespace {
+std::map<Id,Vector> CoupledMetricDirection(const Patch& patch,FrozenField& target,
+    const std::map<int,double>& heights,const PlanarReference& reference,const std::set<Id>& fixed,
+    const std::set<Id>& active,ConnectivityRepairReport& report);
+}
 bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std::pair<Id,Id>>& apices,Id first,
                        FrozenField& target,const PlanarReference& reference,const std::map<int,double>& heights,
                        Patch& fresh,JointSplitReport& report,std::string& reason,size_t maximum_trials) {
   report={};
   try {
     Bounds(old);Endpoints(edge);
-    if(apices.size()>8)throw std::invalid_argument("Native3D joint placement exceeds its apex bound.");
+    if(apices.size()>8)throw std::invalid_argument("Native3D joint placement exceeds its vertex bound.");
     if(!maximum_trials || maximum_trials>256)throw std::invalid_argument("Native3D joint placement exceeds its trial bound.");
     const auto nodes=Nodes(old.cells);const auto skin=SkinMap(Incidence(old.cells).Boundary());
-    CheckSurface(old,skin,reference,nullptr);
-    std::set<Id> selected,reserved{point.id};
+    const auto physical=CheckSurface(old,skin,reference,nullptr);
+    std::set<Id> selected,reserved{point.id},interface;
+    for(const auto& f:skin)if(!physical.count(f.first))interface.insert(f.second.begin(),f.second.end());
     for(const auto& apex:apices) {
       if(!selected.insert(apex.first).second || !reserved.insert(apex.second).second || nodes.count(apex.second))
-        throw std::invalid_argument("Native3D joint placement repeats an apex or reserved identity.");
+        throw std::invalid_argument("Native3D joint placement repeats a vertex or reserved identity.");
       nodes.at(apex.first);
-      for(const auto& f:skin)if(Has(f.second,apex.first))
-        throw std::invalid_argument("Native3D joint placement needs complete free apex stars.");
-    }
-    const Point a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
-    auto tangent=Delta(b,a);const auto square=Dot(tangent,tangent),length=Norm(tangent);
-    if(!(length>0))throw std::invalid_argument("Native3D joint placement has a degenerate edge.");
-    Vector normal{};bool boundary=false;
-    for(const auto& f:old.surface)if(Has(f.v,edge[0]) && Has(f.v,edge[1])) {
-      boundary=true;normal=Cross(Delta(nodes.at(f.v[1]).p,nodes.at(f.v[0]).p),Delta(nodes.at(f.v[2]).p,nodes.at(f.v[0]).p));break;
-    }
-    if(!boundary) {
-      unsigned k=0;for(unsigned i=1;i<3;++i)if(std::abs(tangent[i])<std::abs(tangent[k]))k=i;
-      Vector seed{};seed[k]=1;normal=Cross(tangent,seed);
-    }
-    const auto n=Norm(normal);if(!(n>0))throw std::invalid_argument("Native3D joint placement has a degenerate frame.");
-    for(unsigned i=0;i<3;++i){tangent[i]/=length;normal[i]/=n;}
-    const std::array<Vector,3> basis{normal,tangent,Cross(normal,tangent)};
-    std::vector<std::array<long double,3>> steps(apices.size());
-    for(size_t k=0;k<apices.size();++k) {
-      const auto origin=nodes.at(apices[k].first).p;const auto metric=target.Query(origin).target;
-      for(unsigned i=0;i<3;++i) {
-        const Point end{double(origin.x+basis[i][0]),double(origin.y+basis[i][1]),double(origin.z+basis[i][2])};
-        const auto norm=MetricNorm(origin,end,metric);
-        if(!(norm>0))throw std::invalid_argument("Native3D joint placement has an invalid directional metric.");
-        steps[k][i]=.25L/norm;
-      }
-    }
-    const size_t point_dofs=boundary?1:3;std::array<long double,2> transverse{};
-    if(!boundary)for(unsigned i=0;i<2;++i) {
-      const auto d=basis[2*i];const Point end{double(point.p.x+d[0]),double(point.p.y+d[1]),double(point.p.z+d[2])};
-      const auto norm=MetricNorm(point.p,end,target.Query(point.p).target);
-      if(!(norm>0))throw std::invalid_argument("Native3D joint placement has an invalid insertion metric.");
-      transverse[i]=.25L/norm;
+      if(interface.count(apex.first))throw std::invalid_argument("Native3D joint placement needs complete movable vertex stars.");
     }
-    std::vector<long double> best(point_dofs+3*apices.size(),0);best[0]=Dot(Delta(point.p,a),Delta(b,a))/square;
     double best_loss=std::numeric_limits<double>::infinity();
     const size_t placement_limit=(maximum_trials+1)/2;
     auto score=[&](const Patch& proposal) {
@@ -1530,24 +1503,9 @@ bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std
       }
       return std::make_pair(shape,100*shape+excess);
     };
-    auto trial=[&](const std::vector<long double>& state) {
+    auto trial=[&](Node inserted,const std::vector<std::pair<Id,Node>>& moves) {
       if(report.trials>=placement_limit)return false;
-      ++report.trials;
-      if(!(state[0]>0 && state[0]<1)){++report.geometry_rejected;return false;}
-      Node inserted=point;inserted.p={double((1-state[0])*a.x+state[0]*b.x),double((1-state[0])*a.y+state[0]*b.y),double((1-state[0])*a.z+state[0]*b.z)};
-      if(!boundary) {
-        Vector offset{};for(unsigned i=0;i<2;++i)for(unsigned k=0;k<3;++k)offset[k]+=state[1+i]*transverse[i]*basis[2*i][k];
-        inserted.p={double(inserted.p.x+offset[0]),double(inserted.p.y+offset[1]),double(inserted.p.z+offset[2])};
-      }
-      std::vector<std::pair<Id,Node>> moves;
-      for(size_t i=0;i<apices.size();++i) {
-        const auto origin=nodes.at(apices[i].first).p;Vector displacement{};
-        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)displacement[k]+=state[point_dofs+3*i+j]*basis[j][k]*steps[i][j];
-        Node moved{apices[i].second,{double(origin.x+displacement[0]),double(origin.y+displacement[1]),double(origin.z+displacement[2])}};
-        moved=ConstrainMovePoint(old,apices[i].first,moved,heights,reference);
-        if(moved.p.x!=origin.x || moved.p.y!=origin.y || moved.p.z!=origin.z)moves.emplace_back(apices[i].first,moved);
-      }
-      Patch proposal;
+      ++report.trials;Patch proposal;
       if(!SplitSurfaceStrip(old,edge,inserted,moves,first,reference,proposal,reason)) {
         ++report.geometry_rejected;return false;
       }
@@ -1556,35 +1514,48 @@ bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std
       const auto measured=score(proposal);const double shape=measured.first,loss=measured.second;
       if(report.trials==1)report.initial_loss=loss;
       if(!(loss<best_loss))return false;
-      best_loss=loss;report.best_loss=loss;report.best_shape_loss=shape;report.best=proposal;best=state;
+      best_loss=loss;report.best_loss=loss;report.best_shape_loss=shape;report.best=std::move(proposal);
       if(shape==0) {
         PatchMetric metric;
-        if(ValidateMetricChange(old.cells,proposal.cells,MetricChange::Refine,target,metric,reason)) {
-          fresh=std::move(proposal);return true;
+        if(ValidateMetricChange(old.cells,report.best.cells,MetricChange::Refine,target,metric,reason)) {
+          fresh=report.best;return true;
         }
         ++report.metric_rejected;
       }
       return false;
     };
-    if(trial(best)){reason.clear();return true;}
-    // Paired fraction/normal directions cross a blockage that either independent displacement can worsen.
-    std::vector<std::vector<int>> directions;
-    for(size_t i=0;i<best.size();++i){std::vector<int> d(best.size(),0);d[i]=1;directions.push_back(std::move(d));}
-    for(size_t i=0;i<apices.size();++i)for(int sign:{-1,1}) {
-      std::vector<int> d(best.size(),0);d[0]=1;d[point_dofs+3*i]=sign;directions.push_back(std::move(d));
-    }
-    if(!apices.empty())for(int split:{-1,0,1}) {
-      std::vector<int> d(best.size(),0);d[0]=split;for(size_t i=0;i<apices.size();++i)d[point_dofs+3*i]=1;
-      directions.push_back(std::move(d));
-    }
-    long double step=1;
-    while(report.trials<placement_limit && step>=1.L/128) {
-      const auto start=best_loss;
-      for(const auto& direction:directions)for(int sign:{-1,1}) {
-        auto state=best;for(size_t i=0;i<state.size();++i)state[i]+=sign*step*direction[i]*(i==0?.25L:1.L);
-        if(trial(state)){reason.clear();return true;}
+    if(trial(point,{})){reason.clear();return true;}
+    ConnectivityRepairReport solve;
+    // Solve for the admitted surface and volume points together; rebuild from the original complete stars.
+    while(!report.best.cells.empty() && report.best_shape_loss>0 && report.trials<placement_limit) {
+      const auto current=Nodes(report.best.cells);std::set<Id> movable{point.id},fixed,active;
+      for(const auto& apex:apices)movable.insert(current.count(apex.second)?apex.second:apex.first);
+      for(const auto& n:current)if(!movable.count(n.first) || interface.count(n.first))fixed.insert(n.first);
+      for(const auto& c:report.best.cells)active.insert(c.id);
+      const auto direction=CoupledMetricDirection(report.best,target,heights,reference,fixed,active,solve);
+      report.linearizations=solve.linearizations;report.metric_probes=solve.metric_probes;report.linear_iterations=solve.linear_iterations;
+      if(direction.empty())break;
+      const auto previous=best_loss;
+      for(double fraction=1;fraction>std::numeric_limits<double>::epsilon() && report.trials<placement_limit;fraction*=.5) {
+        auto displaced=[&](Id id) {
+          Node n=current.at(id);const auto d=direction.find(id);
+          if(d!=direction.end()) {
+            n.p={double(n.p.x+fraction*d->second[0]),double(n.p.y+fraction*d->second[1]),double(n.p.z+fraction*d->second[2])};
+            n=ConstrainMovePoint(report.best,id,n,heights,reference);
+          }
+          return n;
+        };
+        Node inserted=displaced(point.id);std::vector<std::pair<Id,Node>> moves;
+        for(const auto& apex:apices) {
+          auto moved=displaced(current.count(apex.second)?apex.second:apex.first);const auto origin=nodes.at(apex.first).p;
+          moved.id=apex.second;
+          if(moved.p.x!=origin.x || moved.p.y!=origin.y || moved.p.z!=origin.z)moves.emplace_back(apex.first,moved);
+        }
+        ++report.solve_trials;
+        if(trial(inserted,moves)){report.moves+=moves.size()+1;reason.clear();return true;}
+        if(best_loss<previous){report.moves+=moves.size()+1;break;}
       }
-      if(!(best_loss<start))step/=2;
+      if(!(best_loss<previous))break;
     }
     if(!report.best.cells.empty() && report.best_shape_loss>0 && report.trials<maximum_trials) {
       const auto present=Nodes(report.best.cells);std::set<Id> movable{point.id};
@@ -1593,9 +1564,9 @@ bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std
       const bool progress=RepairMetricConnectivity(report.best,target,reference,heights,first,repaired,connectivity,reason,
           maximum_trials-report.trials,false,&movable);
       report.trials+=connectivity.trials;report.connectivity_trials=connectivity.trials;
-      report.flips=connectivity.flips;report.moves=connectivity.moves;
-      report.linearizations=connectivity.linearizations;report.metric_probes=connectivity.metric_probes;
-      report.linear_iterations=connectivity.linear_iterations;report.solve_trials=connectivity.solve_trials;
+      report.flips=connectivity.flips;report.moves+=connectivity.moves;
+      report.linearizations+=connectivity.linearizations;report.metric_probes+=connectivity.metric_probes;
+      report.linear_iterations+=connectivity.linear_iterations;report.solve_trials+=connectivity.solve_trials;
       report.blocked_interior_vertex=connectivity.blocked_interior_vertex;
       if(progress) {
         for(const auto& apex:apices) {
diff --git a/Common/src/adaptation/CNativeEngine3D.cpp b/Common/src/adaptation/CNativeEngine3D.cpp
index 37cb5b0e33..c0c0fbf199 100644
--- a/Common/src/adaptation/CNativeEngine3D.cpp
+++ b/Common/src/adaptation/CNativeEngine3D.cpp
@@ -2282,15 +2282,20 @@ EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorC
         }
         if(strip && approved){cavity_inserted=true;joint=false;}
       }
+      auto movable_star=[&](const Imported& star,Id vertex) {
+        std::set<Id> components;
+        for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
+          if(row.physical[f].present && row.cell.v[f].id!=vertex)components.insert(geometry.Canonical(row.physical[f].facet));
+        return components.size()<=1;  // Smooth physical vertices share the same complete-star admission as interior points.
+      };
       bool search=joint && composition && flat_edge && !choice.geometric && wallApices.empty() && !approved;
       std::vector<Id> apices;
       if(world.maximum(search)) {
         std::vector<Id> candidates;std::vector<Feature> features;
         if(search)try {
-          std::set<Id> physical,vertices;
-          for(const auto& f:old.surface)physical.insert(f.v.begin(),f.v.end());
+          std::set<Id> vertices;
           for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)
-            for(auto n:c.v)if(n.id!=choice.edge[0] && n.id!=choice.edge[1] && !physical.count(n.id))vertices.insert(n.id);
+            for(auto n:c.v)if(n.id!=choice.edge[0] && n.id!=choice.edge[1])vertices.insert(n.id);
           candidates.assign(vertices.begin(),vertices.end());for(Id id:candidates)features.push_back({{{id,0,0}},1});
         }catch(const std::exception& e){failure.Set(2,0,e.what());}
         if(!(ready=vote(failure)))return finish();
@@ -2307,10 +2312,8 @@ EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorC
                 if(star.reason=="Native 3D dependency star exceeds budget.")continue;
                 throw std::runtime_error(star.reason);
               }
-              ++stats.placement_classified_vertices;bool physical=false;
-              for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
-                physical=physical || (row.physical[f].present && row.cell.v[f].id!=candidates[i]);
-              if(!physical)costs.emplace(candidates[i],star.cells.size());
+              ++stats.placement_classified_vertices;
+              if(movable_star(star,candidates[i]))costs.emplace(candidates[i],star.cells.size());
             }
             apices=SplitPatchApices(old,choice.edge,&costs);
             if(costs.count(initial_placement_obstruction)) {
@@ -2352,10 +2355,8 @@ EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorC
               if(star.reason=="Native 3D dependency star exceeds budget.")expand=false;
               else throw std::runtime_error(star.reason);
             } else {
-              ++stats.placement_classified_vertices;bool physical=false;
-              for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
-                physical=physical || (row.physical[f].present && row.cell.v[f].id!=placement_obstruction);
-              expand=!physical;
+              ++stats.placement_classified_vertices;
+              expand=movable_star(star,placement_obstruction);
             }
             if(expand){apices.push_back(placement_obstruction);++stats.placement_dependency_expansions;}
           }catch(const std::exception& e){failure.Set(2,0,e.what());}

Existing coupled direction helper, reused unchanged:
std::map<Id,Vector> CoupledMetricDirection(const Patch& patch,FrozenField& target,
    const std::map<int,double>& heights,const PlanarReference& reference,const std::set<Id>& fixed,
    const std::set<Id>& active,ConnectivityRepairReport& report) {
  struct Frame {
    size_t first=0,count=0,rank=0;std::array<Vector,3> basis{},constraint{};std::vector<FootPlane> feet;Tensor metric;
  };
  struct Row { std::array<std::pair<size_t,long double>,12> entry{};size_t count=0;long double residual=0; };
  const auto nodes=Nodes(patch.cells);std::map<Id,Frame> frames;size_t count=0;long double span=0;
  for(const auto& row:nodes)span=std::max(span,Norm(Delta(row.second.p,nodes.begin()->second.p)));
  if(!(span>0))return {};
  std::map<Id,std::vector<FootPlane>> feet;
  for(const auto& face:patch.surface)if(heights.count(reference.Get(face.facet).marker)) {
    FootPlane plane;for(unsigned j=0;j<3;++j)plane.p[j]=nodes.at(face.v[j]).p;
    plane.normal=Cross(Delta(plane.p[1],plane.p[0]),Delta(plane.p[2],plane.p[0]));plane.area=Norm(plane.normal);
    for(auto& x:plane.normal)x/=plane.area;
    for(const auto& c:patch.cells) {
      unsigned matches=0;Id apex=0;for(auto n:c.v){matches+=Has(face.v,n.id);if(!Has(face.v,n.id))apex=n.id;}
      if(matches==3)feet[apex].push_back(plane);
    }
  }
  auto metricDot=[](Tensor m,Vector a,Vector b) {
    return a[0]*(m.xx*b[0]+m.xy*b[1]+m.xz*b[2])+a[1]*(m.xy*b[0]+m.yy*b[1]+m.yz*b[2])+a[2]*(m.xz*b[0]+m.yz*b[1]+m.zz*b[2]);
  };
  for(const auto& row:nodes)if(!fixed.count(row.first)) {
    Frame frame;frame.first=count;std::array<Vector,3> tangent{};const auto origin=row.second.p;
    for(unsigned axis=0;axis<3;++axis) {
      auto p=origin;if(axis==0)p.x=double(p.x+span);else if(axis==1)p.y=double(p.y+span);else p.z=double(p.z+span);
      auto v=Delta(ConstrainMovePoint(patch,row.first,{row.first,p},heights,reference).p,origin);
      for(unsigned pass=0;pass<2;++pass)for(size_t k=0;k<frame.count;++k) {
        const auto q=Dot(v,tangent[k]);for(unsigned j=0;j<3;++j)v[j]-=q*tangent[k][j];
      }
      const auto norm=Norm(v);if(norm<=256*std::numeric_limits<double>::epsilon()*span)continue;
      for(auto& x:v)x/=norm;tangent[frame.count++]=v;
    }
    if(!frame.count)continue;
    for(unsigned axis=0;axis<3 && frame.rank<3-frame.count;++axis) {
      Vector normal{};normal[axis]=1;
      for(unsigned pass=0;pass<2;++pass) {
        for(size_t k=0;k<frame.count;++k){const auto q=Dot(normal,tangent[k]);for(unsigned a=0;a<3;++a)normal[a]-=q*tangent[k][a];}
        for(size_t k=0;k<frame.rank;++k){const auto q=Dot(normal,frame.constraint[k]);for(unsigned a=0;a<3;++a)normal[a]-=q*frame.constraint[k][a];}
      }
      const auto norm=Norm(normal);if(norm>256*std::numeric_limits<double>::epsilon()){for(auto& x:normal)x/=norm;frame.constraint[frame.rank++]=normal;}
    }
    if(frame.rank!=3-frame.count)throw std::invalid_argument("Unresolved wall tangent complement.");
    frame.feet=feet[row.first];const auto metric=target.Query(origin).target;frame.metric=metric;
    for(size_t k=0;k<frame.count;++k) {
      auto v=tangent[k];for(unsigned pass=0;pass<2;++pass)for(size_t j=0;j<k;++j) {
        const auto q=metricDot(metric,v,frame.basis[j]);for(unsigned a=0;a<3;++a)v[a]-=q*frame.basis[j][a];
      }
      const auto norm=std::sqrt(metricDot(metric,v,v));if(!(norm>0) || !std::isfinite(norm))throw std::invalid_argument("Unresolved constrained metric solve frame.");
      for(auto& x:v)x/=norm;frame.basis[k]=v;
    }
    count+=frame.count;frames.emplace(row.first,frame);
  }
  if(!count)return {};
  if(count>12*MaximumReplacementCells)throw std::invalid_argument("Native3D metric solve exceeds bounded variables.");
  ++report.linearizations;std::vector<Row> rows;rows.reserve(2*patch.cells.size());
  for(const auto& cell:patch.cells)if(active.count(cell.id) && std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return frames.count(n.id);})) {
    const auto center=Centroid(cell);const auto metric=target.Query(center).target;
    const auto volume=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p);
    long double altitude=INFINITY;
    for(unsigned opposite=0;opposite<4;++opposite) {
      std::array<Point,3> f{};unsigned at=0;for(unsigned k=0;k<4;++k)if(k!=opposite)f[at++]=cell.v[k].p;
      altitude=std::min(altitude,volume/Norm(Cross(Delta(f[1],f[0]),Delta(f[2],f[0]))));
    }
    std::array<Tensor,3> spatial{};
    for(unsigned axis=0;axis<3;++axis) {
      const auto value=axis==0?center.x:axis==1?center.y:center.z;
      const auto diagonal=axis==0?metric.xx:axis==1?metric.yy:metric.zz;
      const auto step=std::min(altitude/16,std::cbrt(std::numeric_limits<double>::epsilon())/std::sqrt(static_cast<long double>(diagonal)));
      if(!(step>8*std::numeric_limits<double>::epsilon()*std::max(span,std::abs(static_cast<long double>(value)))))
        throw std::invalid_argument("Native3D metric sensitivity step is not representable.");
      auto plus=center,minus=center;double high=double(value+step),low=double(value-step);
      if(axis==0){plus.x=high;minus.x=low;}else if(axis==1){plus.y=high;minus.y=low;}else{plus.z=high;minus.z=low;}
      ++report.metric_probes;const auto a=target.Query(plus).target;
      ++report.metric_probes;const auto b=target.Query(minus).target;
      const long double inverse=1/(static_cast<long double>(high)-low);
      spatial[axis]={double((a.xx-b.xx)*inverse),double((a.xy-b.xy)*inverse),double((a.xz-b.xz)*inverse),double((a.yy-b.yy)*inverse),double((a.yz-b.yz)*inverse),double((a.zz-b.zz)*inverse)};
    }
    const auto linear=LinearizeMetricShape({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},metric,&spatial);
    for(unsigned r=0;r<2;++r)if(linear.residual[r]>0) {
      Row row;row.residual=linear.residual[r];
      for(unsigned k=0;k<4;++k) {
        const auto found=frames.find(cell.v[k].id);if(found==frames.end())continue;const auto& frame=found->second;
        const auto& d=linear.derivative[r][k];const Vector gradient{d[0],d[1],d[2]};
        for(size_t j=0;j<frame.count;++j)row.entry[row.count++]={frame.first+j,Dot(gradient,frame.basis[j])};
      }
      rows.push_back(row);
    }
  }
  std::vector<long double> diagonal(count),gradient(count),step(count),residual(count),z(count),direction(count),product(count);
  for(const auto& row:rows)for(size_t k=0;k<row.count;++k) {
    const auto e=row.entry[k];diagonal[e.first]+=e.second*e.second;gradient[e.first]+=e.second*row.residual;
  }
  const auto maximum=*std::max_element(diagonal.begin(),diagonal.end());if(!(maximum>0))return {};
  auto regularizer=[&](size_t i){return 1e-3L*(diagonal[i]+maximum*1e-12L);};
  auto dot=[](const auto& a,const auto& b){long double result=0;for(size_t i=0;i<a.size();++i)result+=a[i]*b[i];return result;};
  for(size_t i=0;i<count;++i){residual[i]=-gradient[i];z[i]=residual[i]/(diagonal[i]+regularizer(i));direction[i]=z[i];}
  auto rz=dot(residual,z);const auto initial=dot(residual,residual);if(!(initial>0))return {};
  for(size_t iteration=0;iteration<count;++iteration) {
    for(size_t i=0;i<count;++i)product[i]=regularizer(i)*direction[i];
    for(const auto& row:rows) {
      long double value=0;for(size_t k=0;k<row.count;++k)value+=row.entry[k].second*direction[row.entry[k].first];
      for(size_t k=0;k<row.count;++k)product[row.entry[k].first]+=row.entry[k].second*value;
    }
    const auto denominator=dot(direction,product);if(!(denominator>0))break;
    const auto alpha=rz/denominator;++report.linear_iterations;
    for(size_t i=0;i<count;++i){step[i]+=alpha*direction[i];residual[i]-=alpha*product[i];}
    if(dot(residual,residual)<=1e-16L*initial)break;
    for(size_t i=0;i<count;++i)z[i]=residual[i]/(diagonal[i]+regularizer(i));
    const auto next=dot(residual,z);if(!(next>0))break;const auto beta=next/rz;rz=next;
    for(size_t i=0;i<count;++i)direction[i]=z[i]+beta*direction[i];
  }
  if(!(dot(step,gradient)<0))return {};
  long double largest=0;
  for(const auto& row:frames){long double norm=0;for(size_t k=0;k<row.second.count;++k)norm+=step[row.second.first+k]*step[row.second.first+k];largest=std::max(largest,std::sqrt(norm));}
  const auto scale=largest>.5L?.5L/largest:1.L;std::map<Id,Vector> movement;
  for(const auto& row:frames) {
    Vector delta{};for(size_t k=0;k<row.second.count;++k)for(unsigned a=0;a<3;++a)delta[a]+=scale*step[row.second.first+k]*row.second.basis[k][a];
    if(!std::isfinite(Norm(delta)))throw std::invalid_argument("Nonfinite coupled metric step.");
    if(!row.second.feet.empty()) {
      const auto origin=nodes.at(row.first).p;
      const auto fitted=FitNormalFeet({double(origin.x+delta[0]),double(origin.y+delta[1]),double(origin.z+delta[2])},row.second.constraint,row.second.rank,row.second.feet);
      delta=Delta(fitted,origin);
    }
    if(Norm(delta)>0)movement.emplace(row.first,delta);
  }
  largest=0;for(const auto& row:movement)largest=std::max(largest,std::sqrt(metricDot(frames.at(row.first).metric,row.second,row.second)));
  if(largest>.5L)for(auto& row:movement)for(auto& x:row.second)x*=.5L/largest;
  return movement;
}
Existing projection/complete-star policy helpers:
Node ProjectSplitPoint(Edge edge,Node point,const std::map<Id,Node>& nodes,const Physical& physical,
                       const PlanarReference& reference,KernelStats* stats,bool cavity=false) {
  const auto a=nodes.at(edge[0]),b=nodes.at(edge[1]);
    bool curved_edge=false;
    for(const auto& face:physical)if(Has(face.second.v,edge[0]) && Has(face.second.v,edge[1])) {
      const auto geometry=reference.Analytic().find(face.second.facet);
      curved_edge=curved_edge || reference.Affine().count(face.second.facet) || (geometry!=reference.Analytic().end() && geometry->second.kind!=AnalyticSurface::Kind::Disk);
    }
    const Point midpoint{double((static_cast<long double>(a.p.x)+b.p.x)/2),double((static_cast<long double>(a.p.y)+b.p.y)/2),double((static_cast<long double>(a.p.z)+b.p.z)/2)};
    const bool rounded_midpoint=curved_edge && point.p.x==midpoint.x && point.p.y==midpoint.y && point.p.z==midpoint.z;
    bool same_chart=false,curved_feature=false;
    if(cavity) {
      Id component=UINT64_MAX;unsigned incident=0;same_chart=true;
      for(const auto& f:physical)if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1])) {
        const auto next=reference.Canonical(f.second.facet);
        if(component==UINT64_MAX)component=next;else same_chart=same_chart && component==next;
        ++incident;
      }
      same_chart=same_chart && incident==2;
      curved_feature=curved_edge && incident==2 && !same_chart;
    }
    // Same-chart fans certify their border; a curved feature instead certifies its finite projected parameter.
    if (nodes.count(point.id) || (!rounded_midpoint && !same_chart && !curved_feature && !OnEdge(a.p, b.p, point.p, stats)) ||
        (point.p.x == a.p.x && point.p.y == a.p.y && point.p.z == a.p.z) ||
        (point.p.x == b.p.x && point.p.y == b.p.y && point.p.z == b.p.z))
      throw std::invalid_argument("Native 3D split point is not a distinct exact on-edge point.");
    for(unsigned pass=0;pass<2;++pass)for(const auto& f:physical)
      if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1]) && reference.Declared(f.second.facet) &&
         (reference.Affine().count(f.second.facet) || reference.Analytic().at(f.second.facet).kind==AnalyticSurface::Kind::Disk)==(pass==0))
        point.p=reference.Project(f.second.facet,point.p,stats);
    for (const auto& f : physical)if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1])) {
      if(!reference.Contains(f.second.facet,point.p,stats))
        throw std::invalid_argument("Native3D projected split leaves an incident physical feature.");
      const auto geometry=reference.Analytic().find(f.second.facet);
      if(curved_feature && geometry!=reference.Analytic().end() && geometry->second.kind!=AnalyticSurface::Kind::Disk &&
         !reference.OnProjectedEdge(f.second.facet,a.p,b.p,point.p,stats))
        throw std::invalid_argument("Native3D projected split leaves the finite curved feature parameter.");
    }
  return point;
}
Node ProjectMovePoint(Id vertex,Node point,const Skin& skin,const Physical& physical,
                      const PlanarReference& reference,KernelStats* stats) {
  for(unsigned pass=0;pass<2;++pass)for(const auto& face:physical)
    if(Has(face.second.v,vertex) && reference.Declared(face.second.facet) &&
       (reference.Affine().count(face.second.facet) || reference.Analytic().at(face.second.facet).kind==AnalyticSurface::Kind::Disk)==(pass==0))
      point.p=reference.Project(face.second.facet,point.p,stats);
  for(const auto& face:skin)if(Has(face.second,vertex)) {
    const auto marked=physical.find(face.first);
    if(marked==physical.end())throw std::invalid_argument("Native 3D incomplete movement dependency at interface node "+std::to_string(vertex)+".");
    if(!reference.Contains(marked->second.facet,point.p,stats))
      throw std::invalid_argument("Native 3D movement leaves incident reference facet "+std::to_string(marked->second.facet)+".");
  }
  return point;
}

You have all core context above. Tools are disabled for this retry. Return at most600words; focus on concrete correctness issues, not broad new architecture. The prior CLI review timed out with no report.
