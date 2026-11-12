// Query thresholds affect bound preparation/refinement only; final tolerance stays unchanged.
const auto acceptedParameters=current.live_parameters->Payload();
Directory directory(world,false,current.live_parameters.get());
REQUIRE(directory.Build(current.mesh,reason,ceiling,retained));
budget.caller_bytes=transfer_memory::Add(retained,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
struct ShapeRestore {ShapeAdmission old=GetShapeAdmission();~ShapeRestore(){SetShapeAdmission(old);}} restore;
SetShapeAdmission(ShapeAdmission::ValidityOnly);
std::vector<std::pair<std::string,Edge>> edges{{"recorded_edge_r5",{7907,7908}},{"r4_adjacent_candidate",{4303,23829}},
    {"smooth_control",{6032,6070}},{"leading_edge_control",{4218,4219}}};
Id nextNode=0,nextCell=0;
for(const auto& row:current.mesh){nextCell=std::max(nextCell,row.first);for(auto n:row.second.cell.v)nextNode=std::max(nextNode,n.id);}
++nextNode;++nextCell;
std::set<Id> exported;
for(const auto& entry:edges) {
  const auto edge=entry.second;
  auto dependencies=directory.Lookup({Feature{{edge[0],edge[1],0},2}},ceiling,retained);REQUIRE(dependencies.valid);
  auto imported=directory.Fetch(dependencies,current.mesh,ceiling,retained);REQUIRE(imported.valid);
  std::vector<Id> edgeIds;for(const auto& row:imported.cells)edgeIds.push_back(row.cell.id);
  const auto edgePatch=junction_wp0::PatchOf(current,edgeIds);std::vector<Feature> features{{{edge[0],edge[1],0},2}};
  std::map<Id,Node> coordinates;for(const auto& cell:edgePatch.cells)for(auto n:cell.v)coordinates.emplace(n.id,n);
  for(const auto& face:edgePatch.surface)if(heights.count(current.reference->Get(face.facet).marker)&&
      std::find(face.v.begin(),face.v.end(),edge[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge[1])!=face.v.end())
    for(const auto& cell:edgePatch.cells) {
      unsigned count=0;Id apex=0;for(auto n:cell.v)if(std::find(face.v.begin(),face.v.end(),n.id)!=face.v.end())++count;else apex=n.id;
      if(count==3)features.push_back({{apex,0,0},1});
    }
  const auto joined=directory.Lookup(features,ceiling,retained);REQUIRE(joined.valid);
  const auto complete=directory.Fetch(joined,current.mesh,ceiling,retained);REQUIRE(complete.valid);
  std::vector<Id> ids;for(const auto& row:complete.cells)ids.push_back(row.cell.id);
  const auto old=junction_wp0::PatchOf(current,ids);REQUIRE(old.cells.size()<=MaximumCavityCells);
  REQUIRE(directory.Reserve(complete,true,reason,ceiling,retained));
  const auto a=coordinates.at(edge[0]).p,b=coordinates.at(edge[1]).p;
  const Node point{nextNode,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
  Patch fresh;REQUIRE(SplitSurfaceStrip(old,edge,point,{},nextCell,*current.reference,fresh,reason));
  std::set<Face> oldFaces;for(auto face:old.surface){std::sort(face.v.begin(),face.v.end());oldFaces.insert(face.v);}
  for(unsigned side=0;side<2;++side) {
    const auto& patch=side?fresh:old;const char* label=side?"fresh":"old";
    auto privateGeometry=current.reference->PrivateParameters(patch);const auto& geometry=privateGeometry?*privateGeometry:*current.reference;
    const auto& state=*geometry.Parameters();const auto& g=state.Geometry();
    auto donors=source.ImportCellsLocal(patch.cells,complete.cells,ceiling,budget.caller_bytes);REQUIRE(donors.valid);
    const auto extensions=AssociatedExtensions(donors.cells,geometry);
    FrozenField field(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
    size_t qBad=0,jBad=0;std::map<Id,Node> nodes;const auto compose=policy.Composition();
    for(const auto& cell:patch.cells) {
      REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
      for(auto n:cell.v)nodes.emplace(n.id,n);
      const auto x=Centroid(cell);const auto m=compose(x,field.Sensor(x));
      const auto shape=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},m);
      qBad+=shape.mean_ratio<MinimumMeanRatio;jBad+=shape.minimum_scaled_jacobian<MinimumScaledJacobian;
      log<<"{\"stage\":\"cell\",\"context\":"<<std::quoted(entry.first)<<",\"side\":"<<std::quoted(label)<<",\"cell\":"<<cell.id
         <<",\"q\":"<<shape.mean_ratio<<",\"J\":"<<shape.minimum_scaled_jacobian<<",\"tensor\":["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<"],\"vertices\":[";
      for(unsigned k=0;k<4;++k){if(k)log<<',';const auto n=cell.v[k];log<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<']';}log<<"]}"<<std::endl;
    }
    const BoundaryMetric boundary=[&](Node n,const PlanarReference& ref,Tensor m){return policy.BoundaryGuidance(n,ref,m,reference->marker_names);};
    const auto planning=MeasureSurfacePlanning(patch,field,geometry,heights,boundary);
    const auto guidance=GeometryGuidanceExcess(patch,geometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    std::array<long double,4> scores{};const std::array<double,4> thresholds{1e-4,2e-4,5e-4,1e-3};
    long double strictScore=0;
    for(const auto& face:patch.surface) {
      std::array<Point,3> p;std::array<SU2NativeGeometry::SurfaceBinding,3> binding;bool direct=true;double affine=0;
      for(unsigned k=0;k<3;++k){p[k]=nodes.at(face.v[k]).p;const auto* record=geometry.Parameter(p[k]);REQUIRE(record);binding[k]=record->binding;
        direct=direct&&g.Supports(face.facet,binding[k]);
        const auto plane=geometry.Affine().find(face.facet);if(plane!=geometry.Affine().end())affine=std::max(affine,SU2NativeGeometry::AffineChart(plane->second).ErrorBound(p[k]));}
      const double production=geometry.SurfaceDeviation(face.facet,p,true);
      const auto error=std::max(0.L,(long double)production-1e-4);strictScore+=error*error;
      auto key=face.v;std::sort(key.begin(),key.end());
      const bool affected=side?!oldFaces.count(key):std::find(face.v.begin(),face.v.end(),edge[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge[1])!=face.v.end();
      if(!affected){for(auto& score:scores)score+=error*error;continue;}
      double directBound=INFINITY;std::vector<Id> support;
      if(direct){support={face.facet};directBound=g.SurfaceError(face.facet,binding,p);}
      else {
        Point lo=p[0],hi=p[0];const auto model=g.Realize(binding[0]);double radius=0;
        for(auto q:p){lo={std::min(lo.x,q.x),std::min(lo.y,q.y),std::min(lo.z,q.z)};hi={std::max(hi.x,q.x),std::max(hi.y,q.y),std::max(hi.z,q.z)};
          radius=std::max(radius,std::hypot(q.x-model.x,q.y-model.y,q.z-model.z));}
        radius=std::nextafter(radius+state.GeometryTolerance(),INFINITY);
        lo={std::nextafter(lo.x-radius,-INFINITY),std::nextafter(lo.y-radius,-INFINITY),std::nextafter(lo.z-radius,-INFINITY)};
        hi={std::nextafter(hi.x+radius,INFINITY),std::nextafter(hi.y+radius,INFINITY),std::nextafter(hi.z+radius,INFINITY)};
        support=g.NearbySurfaceFacets(face.facet,{lo,hi},&budget);const auto entity=state.Entity(face.facet);
        support.erase(std::remove_if(support.begin(),support.end(),[&](Id id){return state.Entity(id)!=entity;}),support.end());
      }
      REQUIRE_FALSE(support.empty());
      log<<"{\"stage\":\"face\",\"context\":"<<std::quoted(entry.first)<<",\"side\":"<<std::quoted(label)<<",\"facet\":"<<face.facet
         <<",\"direct\":"<<direct<<",\"production_bound\":"<<production<<",\"vertices\":[";
      for(unsigned k=0;k<3;++k){if(k)log<<',';log<<'['<<face.v[k]<<','<<p[k].x<<','<<p[k].y<<','<<p[k].z<<','<<binding[k].dimension<<','<<binding[k].feature<<','<<binding[k].u<<','<<binding[k].v<<','<<binding[k].version<<']';}
      log<<"],\"support\":[";for(size_t k=0;k<support.size();++k){if(k)log<<',';log<<support[k];}log<<"]}"<<std::endl;
      for(Id id:support)if(exported.insert(id).second) {
        const auto curve=g.Curves().find(id);log<<"{\"stage\":\"reference_piece\",\"context\":"<<std::quoted(entry.first)<<",\"facet\":"<<id<<",\"curved\":"<<(curve!=g.Curves().end())<<",\"controls\":[";
        if(curve!=g.Curves().end())for(unsigned k=0;k<10;++k){if(k)log<<',';auto q=curve->second[k];log<<'['<<q.x<<','<<q.y<<','<<q.z<<']';}log<<"]}"<<std::endl;
      }
      for(unsigned trial=0;trial<thresholds.size();++trial) {
        const auto work=budget.work;const auto start=std::chrono::steady_clock::now();
        const bool directAccepted=direct&&(std::max(affine,directBound)<=state.GeometryTolerance()||affine>state.GeometryTolerance());
        const double unionBound=directAccepted?production:g.BoundPhysicalSurface(support,{p},thresholds[trial],budget);
        const double bound=directAccepted?production:std::max(affine,std::min(directBound,unionBound));
        if(!trial)REQUIRE(bound==production);
        const auto excess=std::max(0.L,(long double)bound-1e-4);scores[trial]+=excess*excess;
        log<<"{\"stage\":\"face_bound\",\"context\":"<<std::quoted(entry.first)<<",\"side\":"<<std::quoted(label)<<",\"facet\":"<<face.facet
           <<",\"face\":["<<face.v[0]<<','<<face.v[1]<<','<<face.v[2]<<"],\"query_threshold\":"<<thresholds[trial]
           <<",\"strict_tolerance\":0.0001,\"bound\":"<<bound<<",\"geometry_work\":"<<budget.work-work
           <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<",\"final_certificate\":false}"<<std::endl;
      }
    }
    REQUIRE(strictScore==guidance.excess);REQUIRE(scores[0]==guidance.excess);
    log<<"{\"stage\":\"complete_patch\",\"context\":"<<std::quoted(entry.first)<<",\"side\":"<<std::quoted(label)<<",\"cells\":"<<patch.cells.size()
       <<",\"q_bad\":"<<qBad<<",\"J_bad\":"<<jBad<<",\"planning_max\":"<<planning.maximum_length<<",\"production_excess\":"<<guidance.excess<<",\"scores\":[";
    for(unsigned k=0;k<scores.size();++k){if(k)log<<',';log<<scores[k];}log<<"],\"published\":false,\"mesh_accepted\":false}"<<std::endl;
    REQUIRE(current.live_parameters->Payload()==acceptedParameters);
  }
  nextNode+=MaximumReplacementCells+2;nextCell+=MaximumReplacementCells;
}
NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
