// Bounded composition of existing splits; all old dependencies precede reservation.
const auto acceptedParameters=current.live_parameters->Payload();
Directory directory(world,false,current.live_parameters.get());REQUIRE(directory.Build(current.mesh,reason,ceiling,retained));
budget.caller_bytes=transfer_memory::Add(retained,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
struct ShapeRestore {ShapeAdmission old=GetShapeAdmission();~ShapeRestore(){SetShapeAdmission(old);}} restore;
SetShapeAdmission(ShapeAdmission::ValidityOnly);
std::vector<std::pair<std::string,Edge>> contextsToTest{{"recorded_edge_r5",{7907,7908}},{"r4_adjacent_candidate",{4303,23829}},
    {"smooth_control",{6032,6070}},{"leading_edge_control",{4218,4219}}};
Id nextNode=0,nextCell=0;for(const auto& row:current.mesh){nextCell=std::max(nextCell,row.first);for(auto n:row.second.cell.v)nextNode=std::max(nextNode,n.id);}
++nextNode;++nextCell;
for(const auto& entry:contextsToTest) {
  const auto edge=entry.second;auto dep=directory.Lookup({Feature{{edge[0],edge[1],0},2}},ceiling,retained);REQUIRE(dep.valid);
  auto imported=directory.Fetch(dep,current.mesh,ceiling,retained);REQUIRE(imported.valid);
  std::vector<Id> seedIds;for(const auto& row:imported.cells)seedIds.push_back(row.cell.id);
  const auto seed=junction_wp0::PatchOf(current,seedIds);
  std::vector<std::pair<std::string,std::vector<Edge>>> alternatives;
  std::set<Edge> perimeter;
  for(const auto& face:seed.surface)if(heights.count(current.reference->Get(face.facet).marker)&&
      std::find(face.v.begin(),face.v.end(),edge[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge[1])!=face.v.end()) {
    std::vector<Edge> neighbors;
    for(unsigned k=0;k<3;++k){Edge other{face.v[k],face.v[(k+1)%3]};std::sort(other.begin(),other.end());if(other!=edge){neighbors.push_back(other);perimeter.insert(other);}}
    for(auto other:neighbors)alternatives.push_back({"pair_"+std::to_string(face.facet)+"_"+std::to_string(other[0])+"_"+std::to_string(other[1]),{edge,other}});
    alternatives.push_back({"one_parent_"+std::to_string(face.facet),{edge,neighbors[0],neighbors[1]}});
  }
  REQUIRE(perimeter.size()<=4);REQUIRE_FALSE(perimeter.empty());
  alternatives.push_back({"perimeters_only",std::vector<Edge>(perimeter.begin(),perimeter.end())});
  std::vector<Edge> all{edge};all.insert(all.end(),perimeter.begin(),perimeter.end());alternatives.push_back({"both_parent_perimeters",all});
  for(const auto& trial:alternatives) {
    const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();const auto work=budget.work;
    // Mandatory seed cells stay represented even when the unhelpful selected diagonal is not split.
    std::vector<Feature> features{{{edge[0],edge[1],0},2}};for(auto e:trial.second)features.push_back({{e[0],e[1],0},2});
    auto dependencies=directory.Lookup(features,ceiling,retained);REQUIRE(dependencies.valid);
    auto support=directory.Fetch(dependencies,current.mesh,ceiling,retained);REQUIRE(support.valid);
    REQUIRE(support.cells.size()<=MaximumCavityCells);REQUIRE(directory.Reserve(support,true,reason,ceiling,retained));
    std::vector<Id> ids;std::map<Id,Node> nodes;for(const auto& row:support.cells){ids.push_back(row.cell.id);for(auto n:row.cell.v)nodes.emplace(n.id,n);}
    const auto old=junction_wp0::PatchOf(current,ids);Patch fresh=old;std::string why;bool built=true;unsigned completed=0;
    for(auto e:trial.second) {
      const auto a=nodes.at(e[0]).p,b=nodes.at(e[1]).p;
      const Node point{nextNode+completed,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
      Patch next;if(!SplitSurfaceStrip(fresh,e,point,{},nextCell,*current.reference,next,why)){built=false;break;}
      fresh=std::move(next);++completed;
    }
    bool geometry=false,metric=false,parameters=false,protocol=false,contact=true,preserved=false,heightMeasured=false;
    long double before=-1,after=-1;size_t oldQ=0,newQ=0,oldJ=0,newJ=0,oldGeometryBad=0,newGeometryBad=0;
    double oldMaximum=-1,newMaximum=-1;bool measured=false;std::string metricReason,parameterReason,contactReason,heightReason;
    if(built) {
      for(const auto& c:fresh.cells)REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
      const auto a=GeometryGuidanceExcess(old,*current.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
      const auto b=GeometryGuidanceExcess(fresh,*current.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));before=a.excess;after=b.excess;
      geometry=a.complete&&b.complete&&before>0&&after<before*(1-1e-12L);
      preserved=PreservePassingFirstHeights(old,fresh,heights,*current.reference,heightReason);
      auto freshContext=current.reference->PrivateParameters(fresh);const auto& freshGeometry=freshContext?*freshContext:*current.reference;
      FirstHeightReport height;std::string firstHeightReason;const bool strict=CheckFirstHeight(fresh,heights,freshGeometry,height,firstHeightReason);
      heightMeasured=strict||(height.measured&&!height.reference_clearance_failures&&!height.missing_cells);
      parameters=current.live_parameters->CheckReplacement(old,fresh,parameterReason,ceiling,budget.caller_bytes,&budget);
      protocol=directory.Clearance(current.mesh,old,fresh,contact,contactReason,ceiling,retained);
      auto measure=[&](const Patch& patch,size_t& qBad,size_t& jBad,size_t& geometryBad) {
        auto context=current.reference->PrivateParameters(patch);const auto& g=context?*context:*current.reference;
        auto donors=source.ImportCellsLocal(patch.cells,support.cells,ceiling,budget.caller_bytes);REQUIRE(donors.valid);
        const auto extensions=AssociatedExtensions(donors.cells,g);FrozenField core(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
        const auto compose=policy.Composition();std::map<Id,Point> positions;for(const auto& cell:patch.cells) {
          for(auto n:cell.v)positions.emplace(n.id,n.p);const auto x=Centroid(cell);const auto tensor=compose(x,core.Sensor(x));
          const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);
          qBad+=m.mean_ratio<MinimumMeanRatio;jBad+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
        }
        std::set<Id> bad;
        for(const auto& face:patch.surface)if(g.SurfaceDeviation(face.facet,{positions.at(face.v[0]),positions.at(face.v[1]),positions.at(face.v[2])},true)>SU2_TYPE::GetValue(config.GetAdap_Hausd()))
          for(Id cell:Incidence(patch.cells).FaceCells(face.v))bad.insert(cell);
        geometryBad=bad.size();
        const BoundaryMetric boundary=[&](Node n,const PlanarReference& ref,Tensor tensor){return policy.BoundaryGuidance(n,ref,tensor,reference->marker_names);};
        return MeasureSurfacePlanning(patch,core,g,heights,boundary);
      };
      auto oldMetric=measure(old,oldQ,oldJ,oldGeometryBad);auto freshMetric=measure(fresh,newQ,newJ,newGeometryBad);measured=true;
      oldMaximum=oldMetric.maximum_length;newMaximum=freshMetric.maximum_length;MetricDiagnosis diagnosis;
      metric=ValidateMetricChange(old.cells.size(),oldMetric,fresh.cells.size(),freshMetric,MetricChange::GeometryRefine,metricReason,&diagnosis);
    }
    const bool useful=built&&geometry&&metric&&parameters&&protocol&&contact&&preserved&&heightMeasured;
    log<<"{\"stage\":\"compound_split\",\"context\":"<<std::quoted(entry.first)<<",\"trial\":"<<std::quoted(trial.first)<<",\"edges\":[";
    for(size_t k=0;k<trial.second.size();++k){if(k)log<<',';log<<'['<<trial.second[k][0]<<','<<trial.second[k][1]<<']';}
    log<<"],\"constructed\":"<<built<<",\"completed_splits\":"<<completed<<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<(built?fresh.cells.size():0)
       <<",\"old_geometry_excess\":"<<before<<",\"new_geometry_excess\":"<<after<<",\"geometry_progress\":"<<geometry
       <<",\"metric_admission\":"<<metric<<",\"parameters\":"<<parameters<<",\"contact_protocol\":"<<protocol<<",\"contact\":"<<contact
       <<",\"preserved_heights\":"<<preserved<<",\"provisional_height\":"<<heightMeasured<<",\"complete_private_useful\":"<<useful
       <<",\"centroid_measured\":"<<measured<<",\"old_q_bad\":"<<oldQ<<",\"new_q_bad\":"<<newQ<<",\"old_J_bad\":"<<oldJ<<",\"new_J_bad\":"<<newJ
       <<",\"old_geometry_bad\":"<<oldGeometryBad<<",\"new_geometry_bad\":"<<newGeometryBad<<",\"old_planning_max\":"<<oldMaximum<<",\"new_planning_max\":"<<newMaximum
       <<",\"reason\":"<<junction_wp0::Json(why)<<",\"metric_reason\":"<<junction_wp0::Json(metricReason)<<",\"parameter_reason\":"<<junction_wp0::Json(parameterReason)
       <<",\"contact_reason\":"<<junction_wp0::Json(contactReason)<<",\"height_reason\":"<<junction_wp0::Json(heightReason)
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC
       <<",\"geometry_work\":"<<budget.work-work<<",\"published\":false,\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
    REQUIRE(current.live_parameters->Payload()==acceptedParameters);
    nextNode+=MaximumReplacementCells+2;nextCell+=MaximumReplacementCells;
  }
}
NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
