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
diff --git a/UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp b/UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp
index bfbe507c42..b0b9fdb1de 100644
--- a/UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp
+++ b/UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp
@@ -1591,3 +1591,79 @@ TEST_CASE("Native exact planar annuli retain holes across complete face containm
   CHECK(clockwise.Contains(0,{2,.5,0}));CHECK_FALSE(clockwise.Contains(0,{2,2,0}));
   CHECK(clockwise.ContainsFace(0,{{{0,0,0},{2,.5,0},{4,0,0}}}));
 }
+
+TEST_CASE("Saved finite-surface blockage admits coordinated placement under the original field", "[.][NativeJointSurfaceReplay3D]") {
+  const char* path=std::getenv("SU2_NATIVE_JOINT_REPLAY");REQUIRE(path);
+  std::ifstream in(path);std::string magic;double core=0;Edge edge;Node point;size_t count=0;
+  in>>magic>>core>>edge[0]>>edge[1]>>point.id>>point.p.x>>point.p.y>>point.p.z;
+  REQUIRE(magic=="NATIVE_JOINT_REPLAY_V1");REQUIRE(core>0);
+  in>>count;REQUIRE(count<=8);std::vector<Id> movers(count);for(auto& id:movers)in>>id;
+  in>>count;REQUIRE(count<=64);std::vector<std::vector<Id>> groups(count);
+  for(auto& g:groups){size_t n=0;in>>n;REQUIRE(n<=128);g.resize(n);for(auto& id:g)in>>id;}
+  in>>count;REQUIRE(count<=128);std::vector<Facet> facets(count);
+  for(auto& f:facets){in>>f.id>>f.marker;for(auto& n:f.v)in>>n.id>>n.p.x>>n.p.y>>n.p.z;}
+  const PlanarReference reference(facets,groups);
+  auto read_patch=[&]() {
+    Patch p;size_t nc=0,nf=0;in>>nc>>nf;REQUIRE(nc<=256);REQUIRE(nf<=256);
+    p.cells.resize(nc);p.surface.resize(nf);
+    for(auto& c:p.cells){in>>c.id;for(auto& n:c.v)in>>n.id>>n.p.x>>n.p.y>>n.p.z;}
+    for(auto& f:p.surface)in>>f.facet>>f.v[0]>>f.v[1]>>f.v[2];
+    REQUIRE(bool(in));return p;
+  };
+  const auto old=read_patch(),original=read_patch(),oracle=read_patch();const auto saved=Connectivity(old);
+  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};
+  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};CBoundaryLayerMetric layers(3,{wall});
+  const MetricComposition compose=[&](Point p,Tensor sensor) {
+    CBoundaryLayerMetric::Tensor m;m.m[0][0]=sensor.xx;m.m[1][1]=sensor.yy;m.m[2][2]=sensor.zz;
+    m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;m.m[1][2]=m.m[2][1]=sensor.yz;
+    const su2double x[]={p.x,p.y,p.z};layers.ApplyPoint(x,m,core,nullptr,false);
+    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
+      SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
+  };
+  std::vector<DonorCell> donors;
+  for(auto c:original.cells){DonorCell d;d.cell=c;d.sensor.fill({core,0,0,core,0,core});donors.push_back(d);}
+  FrozenField target(donors,compose,false,{},[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
+  std::map<int,double> heights;
+  for(const auto& f:facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.1;
+  FirstHeightReport height;std::string reason;REQUIRE(CheckFirstHeight(old,heights,reference,height,reason));
+  std::map<Id,size_t> costs;for(auto id:movers)costs[id]=1;
+  const auto admitted=SplitPatchApices(old,edge,&costs);REQUIRE(!admitted.empty());
+  CHECK(std::any_of(admitted.begin(),admitted.end(),[&](Id id){
+    return std::any_of(old.surface.begin(),old.surface.end(),[&](const SurfaceFace& f){return std::find(f.v.begin(),f.v.end(),id)!=f.v.end();});
+  }));
+  std::vector<std::pair<Id,Id>> apices;
+  for(size_t i=0;i<movers.size();++i)apices.emplace_back(movers[i],point.id+1+i);
+  std::vector<std::pair<Id,Node>> oracle_moves;
+  for(const auto& apex:apices)oracle_moves.emplace_back(apex.first,Node{apex.second,Position(oracle,apex.first)});
+  Node oracle_point=point;
+  for(const auto& c:oracle.cells)for(auto n:c.v) {
+    bool present=false;for(const auto& d:old.cells)for(auto v:d.v)present=present || v.id==n.id;
+    if(!present)oracle_point.p=n.p;
+  }
+  Patch witnessed;REQUIRE(SplitSurfaceStrip(old,edge,oracle_point,oracle_moves,2000000,reference,witnessed,reason));
+  PatchMetric measured;MetricDiagnosis diagnosis;
+  INFO(reason);REQUIRE(ValidateMetricChange(old.cells,witnessed.cells,MetricChange::Refine,target,measured,reason,nullptr,nullptr,&diagnosis));
+  std::cout<<"JOINT_ORACLE q="<<diagnosis.minimum_mean_ratio<<" J="<<diagnosis.minimum_scaled_jacobian
+    <<" removed="<<double(diagnosis.removed_excess)<<" added="<<double(diagnosis.added_excess)<<std::endl;
+  Patch fresh;JointSplitReport report;
+  const bool accepted=PlaceSplitPatch(old,edge,point,apices,2000000,target,reference,heights,fresh,report,reason,256);
+  std::cout<<"JOINT_PLACEMENT accepted="<<accepted<<" trials="<<report.trials<<" geometry_rejected="<<report.geometry_rejected
+    <<" height_rejected="<<report.height_rejected<<" metric_rejected="<<report.metric_rejected<<" linearizations="<<report.linearizations
+    <<" solves="<<report.solve_trials<<" loss="<<report.best_loss<<" shape_loss="<<report.best_shape_loss<<" reason="<<reason<<std::endl;
+  if(!report.best.cells.empty())SaveCurved(report.best,reference,"native_joint_best");
+  INFO(reason);REQUIRE(accepted);CHECK(report.trials<=256);CHECK(report.linearizations>0);
+  REQUIRE(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,target,measured,reason,nullptr,nullptr,&diagnosis));
+  CHECK(CheckFirstHeight(fresh,heights,reference,height,reason));CHECK(Connectivity(old)==saved);
+  for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)CHECK(ConformingDisjoint(fresh.cells[i],fresh.cells[j]));
+  SaveCurved(fresh,reference,"native_joint_after");
+  auto sentinel=fresh;const auto unchanged=Connectivity(sentinel);
+  CHECK_FALSE(PlaceSplitPatch(old,edge,point,apices,2000000,target,reference,heights,sentinel,report,reason,0));
+  CHECK(Connectivity(sentinel)==unchanged);
+  auto incomplete=old;incomplete.cells.erase(std::find_if(incomplete.cells.begin(),incomplete.cells.end(),[&](const Cell& c){
+    return std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==movers.front();});
+  }));
+  const auto skin=Incidence(incomplete.cells).Boundary();
+  incomplete.surface.erase(std::remove_if(incomplete.surface.begin(),incomplete.surface.end(),[&](const SurfaceFace& f){return std::find(skin.begin(),skin.end(),f.v)==skin.end();}),incomplete.surface.end());
+  CHECK_FALSE(PlaceSplitPatch(incomplete,edge,point,apices,2000000,target,reference,heights,sentinel,report,reason,256));
+  CHECK(Connectivity(sentinel)==unchanged);CHECK(reason.find("complete movable vertex stars")!=std::string::npos);
+}
