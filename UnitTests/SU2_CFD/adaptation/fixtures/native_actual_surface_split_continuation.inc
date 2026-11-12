// Private copy only; complete stars are reserved anew before each bounded replacement.
const auto acceptedParameters=current.live_parameters->Payload();
junction_wp0::State trial;
trial.mesh=current.mesh;
const auto held=transfer_memory::Add(retained,transfer_memory::Mul(trial.mesh.size(),6*(sizeof(DonorCell)+640)));
REQUIRE(held<ceiling);
trial.live_parameters=BoundaryParameterState::FromPayload(current.live_parameters->Authority(),acceptedParameters,ceiling,held);
trial.reference=current.reference->WithParameterState(trial.live_parameters)->WithAtlasReservation(budget);
CavityMetricSource trialSource(world,trial.mesh,true);
struct ShapeRestore {ShapeAdmission old=GetShapeAdmission();~ShapeRestore(){SetShapeAdmission(old);}} restore;
SetShapeAdmission(ShapeAdmission::ValidityOnly);
Id nextNode=0,nextCell=0;
for(const auto& row:trial.mesh){nextCell=std::max(nextCell,row.first);for(auto n:row.second.cell.v)nextNode=std::max(nextNode,n.id);}
++nextNode;++nextCell;
std::vector<Edge> edges{{4218,8070},{4218,18565},{4219,8070},{4219,18565}};
long long netQ=0,netJ=0,netGeometry=0;unsigned committed=0;
for(unsigned round=0;round<4;++round) {
  const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();const auto work=budget.work;
  Directory directory(world,false,trial.live_parameters.get());
  REQUIRE(directory.Build(trial.mesh,reason,ceiling,held));
  budget.caller_bytes=transfer_memory::Add(held,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
  // The retained nose diagonal connects the perimeter stars even though it is not split.
  std::vector<Feature> features{{{4218,4219,0},2}};for(auto e:edges)features.push_back({{e[0],e[1],0},2});
  auto dependencies=directory.Lookup(features,ceiling,held);REQUIRE(dependencies.valid);
  auto support=directory.Fetch(dependencies,trial.mesh,ceiling,held);REQUIRE(support.valid);
  if(support.cells.size()>MaximumCavityCells) {
    log<<"{\"stage\":\"continuation_limit\",\"round\":"<<round<<",\"old_cells\":"<<support.cells.size()<<",\"reason\":\"complete support exceeds unchanged cavity limit\"}"<<std::endl;break;
  }
  REQUIRE(directory.Reserve(support,true,reason,ceiling,held));
  std::vector<Id> ids;std::map<Id,Node> nodes;
  for(const auto& row:support.cells){ids.push_back(row.cell.id);for(auto n:row.cell.v)nodes.emplace(n.id,n);}
  const auto old=junction_wp0::PatchOf(trial,ids);Patch fresh=old;bool built=true;std::string why;
  std::vector<Edge> children;unsigned completed=0;
  for(auto e:edges) {
    const auto a=nodes.at(e[0]).p,b=nodes.at(e[1]).p;const Id inserted=nextNode+completed;
    const Node point{inserted,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
    Patch next;if(!SplitSurfaceStrip(fresh,e,point,{},nextCell,*trial.reference,next,why)){built=false;break;}
    fresh=std::move(next);++completed;
    for(Id endpoint:e){Edge child{endpoint,inserted};std::sort(child.begin(),child.end());children.push_back(child);}
  }
  bool published=false,geometry=false,metric=false,parameters=false,protocol=false,contact=true,preserved=false,heightMeasured=false;
  long double before=-1,after=-1;size_t oldQ=0,newQ=0,oldJ=0,newJ=0,oldBad=0,newBad=0;
  double oldMaximum=-1,newMaximum=-1;std::string metricReason,parameterReason,contactReason,heightReason;
  std::vector<OwnedCell> transported;
  if(built) {
    REQUIRE(fresh.cells.size()<=MaximumReplacementCells);
    std::set<Id> identities;for(const auto& cell:fresh.cells){REQUIRE(identities.insert(cell.id).second);REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);OwnedCell row;row.cell=cell;transported.push_back(row);}
    PublishMetrics(support.cells,transported);
    auto privateContext=trial.reference->PrivateParameters(fresh);const auto& freshGeometry=privateContext?*privateContext:*trial.reference;
    const auto a=GeometryGuidanceExcess(old,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    const auto b=GeometryGuidanceExcess(fresh,freshGeometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));before=a.excess;after=b.excess;
    geometry=a.complete&&b.complete&&before>0&&after<before*(1-1e-12L);
    preserved=PreservePassingFirstHeights(old,fresh,heights,*trial.reference,heightReason);
    FirstHeightReport height;std::string firstHeightReason;const bool strict=CheckFirstHeight(fresh,heights,freshGeometry,height,firstHeightReason);
    heightMeasured=strict||(height.measured&&!height.reference_clearance_failures&&!height.missing_cells);
    parameters=trial.live_parameters->CheckReplacement(old,fresh,parameterReason,ceiling,budget.caller_bytes,&budget);
    protocol=directory.Clearance(trial.mesh,old,fresh,contact,contactReason,ceiling,held);
    auto measure=[&](const Patch& patch,const std::vector<OwnedCell>& rows,const PlanarReference& g,size_t& q,size_t& j,size_t& bad) {
      auto donors=trialSource.ImportCellsLocal(patch.cells,rows,ceiling,budget.caller_bytes);REQUIRE(donors.valid);
      const auto extensions=AssociatedExtensions(donors.cells,g);FrozenField core(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
      const auto compose=policy.Composition();std::map<Id,Point> positions;
      for(const auto& cell:patch.cells){for(auto n:cell.v)positions.emplace(n.id,n.p);const auto x=Centroid(cell);const auto tensor=compose(x,core.Sensor(x));
        const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);q+=m.mean_ratio<MinimumMeanRatio;j+=m.minimum_scaled_jacobian<MinimumScaledJacobian;}
      const auto incidence=Incidence(patch.cells);std::set<Id> failing;
      for(const auto& face:patch.surface)if(g.SurfaceDeviation(face.facet,{positions.at(face.v[0]),positions.at(face.v[1]),positions.at(face.v[2])},true)>SU2_TYPE::GetValue(config.GetAdap_Hausd()))
        for(Id id:incidence.FaceCells(face.v))failing.insert(id);
      bad=failing.size();
      const BoundaryMetric guidance=[&](Node n,const PlanarReference& ref,Tensor tensor){return policy.BoundaryGuidance(n,ref,tensor,reference->marker_names);};
      return MeasureSurfacePlanning(patch,core,g,heights,guidance);
    };
    const auto oldMetric=measure(old,support.cells,*trial.reference,oldQ,oldJ,oldBad);
    const auto newMetric=measure(fresh,transported,freshGeometry,newQ,newJ,newBad);
    oldMaximum=oldMetric.maximum_length;newMaximum=newMetric.maximum_length;MetricDiagnosis diagnosis;
    metric=ValidateMetricChange(old.cells.size(),oldMetric,fresh.cells.size(),newMetric,MetricChange::GeometryRefine,metricReason,&diagnosis);
    if(geometry&&metric&&parameters&&protocol&&contact&&preserved&&heightMeasured) {
      std::map<Face,Id> physical;for(const auto& f:fresh.surface)physical.emplace(junction_wp0::Sorted(f.v),f.facet);
      for(auto& row:transported){const auto faces=junction_wp0::CellFaces(row.cell);for(unsigned k=0;k<4;++k){const auto f=physical.find(junction_wp0::Sorted(faces[k]));if(f!=physical.end())row.physical[k]={true,f->second};}}
      const auto edits=BoundaryParameterState::Changes(old,fresh,trial.live_parameters->Revision());
      REQUIRE(directory.Publish(trial.mesh,dependencies.cells,transported,true,why,ceiling,held,edits,fresh.coverage,&budget));published=true;
      trial.reference=trial.reference->WithParameterState(trial.live_parameters)->WithAtlasReservation(budget);
      for(const auto& row:transported){REQUIRE(trial.mesh.count(row.cell.id));for(unsigned k=0;k<4;++k)REQUIRE(sameTensor(trial.mesh.at(row.cell.id).sensor[k],row.sensor[k]));}
      std::vector<Id> publishedIds;for(const auto& row:transported)publishedIds.push_back(row.cell.id);
      const auto publishedPatch=junction_wp0::PatchOf(trial,publishedIds);
      size_t publishedQ=0,publishedJ=0,publishedBad=0;
      const auto publishedMetric=measure(publishedPatch,transported,*trial.reference,publishedQ,publishedJ,publishedBad);
      REQUIRE(publishedQ==newQ);REQUIRE(publishedJ==newJ);REQUIRE(publishedBad==newBad);
      REQUIRE(publishedMetric.maximum_length==newMetric.maximum_length);REQUIRE(publishedMetric.edges.size()==newMetric.edges.size());
      for(const auto& e:newMetric.edges){REQUIRE(publishedMetric.edges.count(e.first));REQUIRE(publishedMetric.edges.at(e.first).length==e.second.length);}
      const auto publishedGeometry=GeometryGuidanceExcess(publishedPatch,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
      REQUIRE(publishedGeometry.complete);REQUIRE(publishedGeometry.excess==after);
      FirstHeightReport publishedHeight;std::string publishedHeightReason;
      REQUIRE(CheckFirstHeight(publishedPatch,heights,*trial.reference,publishedHeight,publishedHeightReason)==strict);
      REQUIRE(publishedHeight.measured==height.measured);REQUIRE(publishedHeight.reference_clearance_failures==height.reference_clearance_failures);
      REQUIRE(publishedHeight.missing_cells==height.missing_cells);
      netQ+=long(newQ)-long(oldQ);netJ+=long(newJ)-long(oldJ);netGeometry+=long(newBad)-long(oldBad);++committed;
    }
  }
  log<<"{\"stage\":\"split_continuation\",\"round\":"<<round<<",\"requested_splits\":"<<edges.size()<<",\"completed_splits\":"<<completed
     <<",\"constructed\":"<<built<<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<(built?fresh.cells.size():0)
     <<",\"old_geometry_excess\":"<<before<<",\"new_geometry_excess\":"<<after<<",\"geometry_progress\":"<<geometry
     <<",\"metric_admission\":"<<metric<<",\"parameters\":"<<parameters<<",\"contact_protocol\":"<<protocol<<",\"contact\":"<<contact
     <<",\"preserved_heights\":"<<preserved<<",\"provisional_height\":"<<heightMeasured<<",\"old_q_bad\":"<<oldQ<<",\"new_q_bad\":"<<newQ
     <<",\"old_J_bad\":"<<oldJ<<",\"new_J_bad\":"<<newJ<<",\"old_geometry_bad\":"<<oldBad<<",\"new_geometry_bad\":"<<newBad
     <<",\"old_planning_max\":"<<oldMaximum<<",\"new_planning_max\":"<<newMaximum<<",\"net_q_bad\":"<<netQ<<",\"net_J_bad\":"<<netJ<<",\"net_geometry_bad\":"<<netGeometry
     <<",\"private_published\":"<<published<<",\"reason\":"<<junction_wp0::Json(why)<<",\"metric_reason\":"<<junction_wp0::Json(metricReason)
     <<",\"parameter_reason\":"<<junction_wp0::Json(parameterReason)<<",\"contact_reason\":"<<junction_wp0::Json(contactReason)<<",\"height_reason\":"<<junction_wp0::Json(heightReason)
     <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC
     <<",\"geometry_work\":"<<budget.work-work<<",\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
  if(!published)break;
  nextNode+=completed;nextCell+=MaximumReplacementCells;edges=std::move(children);
}
REQUIRE(committed>0);REQUIRE(current.live_parameters->Payload()==acceptedParameters);
NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
