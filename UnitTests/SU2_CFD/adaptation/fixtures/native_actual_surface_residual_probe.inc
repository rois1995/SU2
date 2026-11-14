if(residualProbe) {
  std::map<Id,Node> positions;for(const auto& c:publishedPatch.cells)for(auto n:c.v)positions.emplace(n.id,n);
  std::map<Edge,long double> demanded;std::vector<std::vector<Edge>> faceGroups;bool cleanControl=false;std::map<std::vector<Edge>,double> groupBounds;
  for(const auto& face:publishedPatch.surface) {
    std::array<Point,3> points;std::array<SU2NativeGeometry::SurfaceBinding,3> bindings;
    bool direct=true;const auto& authority=trial.live_parameters->Geometry();
    for(unsigned k=0;k<3;++k){points[k]=positions.at(face.v[k]).p;const auto* p=trial.reference->Parameter(points[k]);REQUIRE(p);bindings[k]=p->binding;direct=direct&&authority.Supports(face.facet,bindings[k]);}
    const double bound=trial.reference->SurfaceDeviation(face.facet, points, SU2NativeGeometry::SurfaceQueryNeed::Magnitude(), true);
    log<<"{\"stage\":\"residual_face\",\"facet\":"<<face.facet<<",\"bound\":"<<bound<<",\"direct\":"<<direct<<",\"vertices\":[";
    for(unsigned k=0;k<3;++k){if(k)log<<',';const auto p=points[k];const auto b=bindings[k];log<<'['<<face.v[k]<<','<<p.x<<','<<p.y<<','<<p.z<<','<<b.dimension<<','<<b.feature<<','<<b.u<<','<<b.v<<','<<b.version<<']';}
    log<<"],\"planning_lengths\":[";std::vector<Edge> group;
    for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());group.push_back(e);if(k)log<<',';log<<newMetric.edges.at(e).length;
      if(bound>SU2_TYPE::GetValue(config.GetAdap_Hausd()))demanded[e]=std::max(demanded[e],(long double)bound);}
    auto groupKey=group;std::sort(groupKey.begin(),groupKey.end());groupBounds.emplace(groupKey,bound);
    log<<"],\"reference_controls\":[";const auto curve=authority.Curves().find(face.facet);
    if(curve!=authority.Curves().end())for(unsigned k=0;k<10;++k){if(k)log<<',';const auto p=curve->second[k];log<<'['<<p.x<<','<<p.y<<','<<p.z<<']';}
    log<<"],\"final_certificate\":false}"<<std::endl;
    if(diagonalProbe&&!cleanControl&&bound<=SU2_TYPE::GetValue(config.GetAdap_Hausd())){faceGroups.push_back(group);cleanControl=true;}
    if(bound>SU2_TYPE::GetValue(config.GetAdap_Hausd())) {
      faceGroups.push_back(group);
      const auto entity=trial.live_parameters->Entity(face.facet);
      for(unsigned sample=0;sample<4;++sample) {
        Point point;
        if(sample<3){const auto a=points[sample],b=points[(sample+1)%3];point={double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};}
        else point={double(.25L*points[0].x+.25L*points[1].x+.5L*points[2].x),double(.25L*points[0].y+.25L*points[1].y+.5L*points[2].y),double(.25L*points[0].z+.25L*points[1].z+.5L*points[2].z)};
        double scale=1;for(auto p:points)scale=std::max({scale,std::abs(p.x),std::abs(p.y),std::abs(p.z)});
        // Cover rounding of the dyadic convex combination before asserting an outside witness.
        const double roundoff=std::nextafter(16*std::numeric_limits<double>::epsilon()*scale,INFINITY);
        size_t attempted=0;bool charged=false;const auto stamp=std::chrono::steady_clock::now();
        try {
          const auto closest=authority.ClosestCurved(face.facet,point,1e-6,
              std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work),
              budget.maximum_bytes-budget.caller_bytes,[&](Id id){return trial.live_parameters->Entity(id)==entity;},&attempted,&budget);
          charged=true;REQUIRE(budget.ChargeWork(attempted));
          log<<"{\"stage\":\"residual_witness\",\"facet\":"<<face.facet<<",\"face\":["<<face.v[0]<<','<<face.v[1]<<','<<face.v[2]<<"],\"sample\":"<<sample
             <<",\"lower\":"<<closest.lower<<",\"upper\":"<<closest.upper<<",\"roundoff\":"<<roundoff<<",\"outside_witness\":"<<(closest.lower>SU2_TYPE::GetValue(config.GetAdap_Hausd())+roundoff)
             <<",\"work\":"<<attempted<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-stamp).count()<<",\"face_acceptance_certificate\":false}"<<std::endl;
        } catch(const std::exception& e) {
          if(!charged)REQUIRE(budget.ChargeWork(attempted));
          log<<"{\"stage\":\"residual_witness_unresolved\",\"facet\":"<<face.facet<<",\"sample\":"<<sample<<",\"reason\":"<<junction_wp0::Json(e.what())<<",\"work\":"<<attempted<<",\"outside_witness\":false}"<<std::endl;
        }
      }
    }
  }
  if(patchTrajectory) {
    const auto& seedPatch=publishedPatch;
    const std::vector<Feature> seedQueries{{{4218,0,0},1},{{4219,0,0},1}};
    const Id trajectoryNode=nextNode+completed,trajectoryCell=nextCell+MaximumReplacementCells;
    unsigned trajectoryRemaining=24;
#include "native_actual_surface_patch_trajectory.inc"
  } else {
  std::vector<std::pair<long double,Edge>> ordered;for(auto e:demanded)ordered.push_back({-e.second,e.first});std::sort(ordered.begin(),ordered.end());
  std::vector<std::vector<Edge>> candidates;std::set<std::vector<Edge>> seen;
  auto add=[&](std::vector<Edge> e){std::sort(e.begin(),e.end());if(candidates.size()<24&&seen.insert(e).second)candidates.push_back(std::move(e));};
  // Singles were measured in458; cover complete-face alternatives first under the SAME trial cap.
  for(auto group:faceGroups)add(group);
  if(!diagonalProbe){for(auto group:faceGroups)for(unsigned k=0;k<3;++k)add({group[k],group[(k+1)%3]});
    for(auto e:ordered)add({e.second});}
  if(stripProbe) {
    std::vector<std::pair<double,std::vector<Edge>>> ranked;std::vector<Edge> control;
    for(auto group:faceGroups){std::sort(group.begin(),group.end());const auto bound=groupBounds.at(group);
      if(bound>SU2_TYPE::GetValue(config.GetAdap_Hausd()))ranked.push_back({-bound,group});else if(control.empty())control=group;}
    std::sort(ranked.begin(),ranked.end());REQUIRE(ranked.size()>=4);REQUIRE_FALSE(control.empty());candidates.clear();
    for(unsigned k=0;k<4;++k)for(unsigned base=0;base<3;++base)candidates.push_back(ranked[k].second);
    for(unsigned base=0;base<3;++base)candidates.push_back(control);
  }
  REQUIRE_FALSE(candidates.empty());
  const auto committedParameters=trial.live_parameters->Payload();unsigned candidateIndex=0;
  for(const auto& selected:candidates) {
    const auto candidateStart=std::chrono::steady_clock::now();const auto candidateCpu=std::clock();const auto candidateWork=budget.work;
    std::vector<Feature> queries;for(auto e:selected)queries.push_back({{e[0],e[1],0},2});
    const auto deps=directory.Lookup(queries,ceiling,held);REQUIRE(deps.valid);
    const auto rows=directory.Fetch(deps,trial.mesh,ceiling,held);REQUIRE(rows.valid);
    REQUIRE(rows.cells.size()<=MaximumCavityCells);REQUIRE(directory.Reserve(rows,true,reason,ceiling,held));
    std::vector<Id> cellIds;std::map<Id,Node> localNodes;for(const auto& row:rows.cells){cellIds.push_back(row.cell.id);for(auto n:row.cell.v)localNodes.emplace(n.id,n);}
    const auto previous=junction_wp0::PatchOf(trial,cellIds);Patch candidate=previous;bool constructed=true;std::string rejection;
    unsigned splits=0;GeometrySplitPrediction prediction;
    if(selected.size()==1)prediction=PredictGeometrySplit(selected[0],{localNodes.at(selected[0][0]).p,localNodes.at(selected[0][1]).p},*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    if(stripProbe) {
#include "native_actual_surface_strip_cavity.inc"
    } else for(auto e:selected) {
      const auto a=localNodes.at(e[0]).p,b=localNodes.at(e[1]).p;
      Node point{nextNode+completed+splits,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
      Patch replacement;if(!SplitSurfaceStrip(candidate,e,point,{},nextCell+(candidateIndex+1)*MaximumReplacementCells,*trial.reference,replacement,rejection)){constructed=false;break;}
      candidate=std::move(replacement);++splits;
    }
    const bool splitConstructed=constructed;bool diagonalChanged=false;long double splitExcess=-1;
    if(diagonalProbe&&!stripProbe&&constructed) {
#include "native_actual_surface_red_reconnect.inc"
    }
    bool admissible=false,geometryProgress=false,metricOK=false,parameterOK=false,contactOK=true,contactProtocol=false,heightOK=false,preserveOK=false;
    size_t q0=0,q1=0,j0=0,j1=0,bad0=0,bad1=0;long double score0=-1,score1=-1;double length0=-1,length1=-1;
    std::string metricWhy,parameterWhy,contactWhy,heightWhy;
    if(constructed) {
      REQUIRE(candidate.cells.size()<=MaximumReplacementCells);std::vector<OwnedCell> metricRows;
      for(const auto& c:candidate.cells){REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);OwnedCell row;row.cell=c;metricRows.push_back(row);}
      PublishMetrics(rows.cells,metricRows);
      auto context=trial.reference->PrivateParameters(candidate);const auto& g=context?*context:*trial.reference;
      const auto a=GeometryGuidanceExcess(previous,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
      const auto b=GeometryGuidanceExcess(candidate,g,SU2_TYPE::GetValue(config.GetAdap_Hausd()));score0=a.excess;score1=b.excess;
      geometryProgress=a.complete&&b.complete&&score0>0&&score1<score0*(1-1e-12L);
      preserveOK=PreservePassingFirstHeights(previous,candidate,heights,*trial.reference,heightWhy);
      FirstHeightReport h;const bool strictHeight=CheckFirstHeight(candidate,heights,g,h,heightWhy);heightOK=strictHeight||(h.measured&&!h.reference_clearance_failures&&!h.missing_cells);
      parameterOK=trial.live_parameters->CheckReplacement(previous,candidate,parameterWhy,ceiling,budget.caller_bytes,&budget);
      contactProtocol=directory.Clearance(trial.mesh,previous,candidate,contactOK,contactWhy,ceiling,held);
      const auto oldPlan=measure(previous,rows.cells,*trial.reference,q0,j0,bad0);
      const auto newPlan=measure(candidate,metricRows,g,q1,j1,bad1);length0=oldPlan.maximum_length;length1=newPlan.maximum_length;
      if(stripProbe)for(unsigned side=0;side<2;++side) {
        const auto& patch=side?candidate:previous;const auto& plan=side?newPlan:oldPlan;std::set<Edge> wallEdges;
        for(auto face:patch.surface)if(heights.count(g.Get(face.facet).marker))for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());wallEdges.insert(e);}
        for(auto e:wallEdges){const auto& m=plan.edges.at(e);log<<"{\"stage\":\"strip_edge\",\"trial\":"<<candidateIndex<<",\"side\":"<<std::quoted(side?"fresh":"old")
          <<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"a\":["<<m.a.x<<','<<m.a.y<<','<<m.a.z<<"],\"b\":["<<m.b.x<<','<<m.b.y<<','<<m.b.z<<"],\"length\":"<<m.length<<"}"<<std::endl;}
      }
      metricOK=ValidateMetricChange(previous.cells.size(),oldPlan,candidate.cells.size(),newPlan,MetricChange::GeometryRefine,metricWhy);
      admissible=geometryProgress&&preserveOK&&heightOK&&parameterOK&&contactProtocol&&contactOK&&metricOK;
    }
    log<<"{\"stage\":\"residual_candidate\",\"trial\":"<<candidateIndex<<",\"edges\":[";
    for(size_t k=0;k<selected.size();++k){if(k)log<<',';log<<'['<<selected[k][0]<<','<<selected[k][1]<<']';}
    log<<"],\"strip_mode\":"<<stripProbe<<",\"base_choice\":"<<(candidateIndex%3)<<",\"new_points\":"<<splits<<",\"split_constructed\":"<<splitConstructed<<",\"diagonal_changed\":"<<diagonalChanged<<",\"split_excess\":"<<splitExcess<<",\"constructed\":"<<constructed<<",\"old_cells\":"<<previous.cells.size()<<",\"new_cells\":"<<(constructed?candidate.cells.size():0)
       <<",\"old_geometry_excess\":"<<score0<<",\"new_geometry_excess\":"<<score1<<",\"old_geometry_bad\":"<<bad0<<",\"new_geometry_bad\":"<<bad1
       <<",\"old_q_bad\":"<<q0<<",\"new_q_bad\":"<<q1<<",\"old_J_bad\":"<<j0<<",\"new_J_bad\":"<<j1<<",\"old_planning_max\":"<<length0<<",\"new_planning_max\":"<<length1
       <<",\"admissible\":"<<admissible<<",\"geometry_progress\":"<<geometryProgress<<",\"metric\":"<<metricOK<<",\"parameters\":"<<parameterOK
       <<",\"contact_protocol\":"<<contactProtocol<<",\"contact\":"<<contactOK<<",\"preserved_heights\":"<<preserveOK<<",\"provisional_height\":"<<heightOK
       <<",\"prediction_complete\":"<<prediction.complete<<",\"prediction_before\":"<<prediction.before<<",\"prediction_after\":"<<prediction.after
       <<",\"reason\":"<<junction_wp0::Json(rejection)<<",\"metric_reason\":"<<junction_wp0::Json(metricWhy)<<",\"parameter_reason\":"<<junction_wp0::Json(parameterWhy)
       <<",\"contact_reason\":"<<junction_wp0::Json(contactWhy)<<",\"height_reason\":"<<junction_wp0::Json(heightWhy)
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-candidateStart).count()<<",\"cpu_seconds\":"<<double(std::clock()-candidateCpu)/CLOCKS_PER_SEC
       <<",\"geometry_work\":"<<budget.work-candidateWork<<",\"published\":false,\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
    REQUIRE(trial.live_parameters->Payload()==committedParameters);++candidateIndex;
  }
  }
}
