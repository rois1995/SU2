// Diagnostic composition only. Discovery precedes the final full-support reservation; no publication.
auto prescribed=[&](const Patch& old) {
  std::map<Face,Face> expected;std::set<Face> physical;
  for(auto f:old.surface){std::sort(f.v.begin(),f.v.end());physical.insert(f.v);}
  for(auto f:Incidence(old.cells).Boundary()) {
    if(std::find(f.begin(),f.end(),vertex)!=f.end()) {
      auto key=f;std::sort(key.begin(),key.end());
      if(!physical.count(key))throw std::invalid_argument("Mandatory removed-vertex star is incomplete at the outer interface.");
      for(auto& id:f)if(id==vertex)id=neighbor;
      if(f[0]==f[1]||f[0]==f[2]||f[1]==f[2])continue;
    }
    std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());
    auto key=f;std::sort(key.begin(),key.end());
    if(!expected.emplace(key,f).second)throw std::invalid_argument("Contraction repeats a prescribed boundary face.");
  }
  std::vector<Face> result;for(const auto& f:expected)result.push_back(f.second);return result;
};
// Exercise the prescribed-interface contract on the existing primitive before judging compound failures.
{Patch control;std::string why;if(CollapseVertex(vertexPatch,vertex,neighbor,nextCell,*current.reference,control,why))
  REQUIRE(ValidatePrescribedInterface(vertexPatch.cells,control.cells,prescribed(vertexPatch),why));}
auto compound=[&](const Patch& old,Id apex,Patch& fresh,std::string& why) {
  return ReconnectCollapse(old,vertex,neighbor,apex,nextCell,*current.reference,fresh,why);
};
std::vector<std::pair<size_t,Id>> ordered;
for(const auto& n:nodes)if(n.first!=vertex&&n.first!=neighbor) {
  size_t unseen=0;for(const auto& f:Incidence(vertexPatch.cells).Boundary()) {
    if(std::find(f.begin(),f.end(),n.first)!=f.end())continue;
    unseen+=Orientation(n.second.p,nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p)<=0;
  }ordered.push_back({unseen,n.first});
}
std::sort(ordered.begin(),ordered.end());if(ordered.size()>2)ordered.resize(2);
ordered.insert(ordered.begin(),{0,neighbor});
for(const auto& a:ordered) {
  if(!budget.within_time())break;
  const Id apex=a.second;std::set<Id> mandatory(entry.second.begin(),entry.second.end()),region=mandatory;
  std::string growthReason;size_t absorbed=0;
  for(;;) {
    std::vector<Id> ids(region.begin(),region.end());const auto old=junction_wp0::PatchOf(current,ids);
    std::map<Id,Node> points;for(const auto& c:old.cells)for(auto n:c.v)points.emplace(n.id,n);
    Face blocked{};bool obstruction=false;std::set<Face> physical;
    for(auto f:old.surface){std::sort(f.v.begin(),f.v.end());physical.insert(f.v);}
    const auto visibleFaces=apex==neighbor?prescribed(old):Incidence(old.cells).Boundary();
    for(auto f:visibleFaces) {
      if(std::find(f.begin(),f.end(),apex)!=f.end())continue;
      if(Orientation(points.at(apex).p,points.at(f[0]).p,points.at(f[1]).p,points.at(f[2]).p)>0)continue;
      blocked=f;std::sort(blocked.begin(),blocked.end());obstruction=true;break;
    }
    if(!obstruction)break;
    if(physical.count(blocked)){growthReason="Physical boundary visibility obstruction; no outside-body absorption.";break;}
    if(region.size()==MaximumCavityCells||!budget.within_time()){growthReason="Existing cavity/time capacity exhausted during discovery.";break;}
    const auto extra=directory.Lookup({Feature{blocked,3}},ceiling,retained);
    if(!extra.valid){growthReason=extra.reason;break;}
    const size_t before=region.size();for(const auto& d:extra.cells)region.insert(d.cell);
    if(region.size()>MaximumCavityCells){growthReason="Correction exceeds existing cavity bound.";break;}
    if(region.size()==before){growthReason="Visibility obstruction has no additional current neighbor.";break;}
    absorbed+=region.size()-before;
  }
  if(region.size()>MaximumCavityCells)continue;
  std::vector<Id> grown(region.begin(),region.end());
  log<<"{\"context\":"<<std::quoted(entry.first)<<",\"stage\":\"correction_discovery\",\"apex\":"<<apex
     <<",\"mandatory_cells\":"<<mandatory.size()<<",\"grown_cells\":"<<region.size()<<",\"absorbed\":"<<absorbed<<",\"reason\":"<<junction_wp0::Json(growthReason)<<"}"<<std::endl;
  const auto op=std::string(apex==neighbor?"PrescribedCollapseCone_apex":"ReconnectCollapse_apex")+std::to_string(apex);
  auto construct=[&](const Patch& old,Patch& fresh,std::string& why) {
    if(apex!=neighbor)return compound(old,apex,fresh,why);
    if(!current.reference->ContractionAllowed(vertex,neighbor)){why="Contraction leaves its physical body or feature.";return false;}
    Patch proposal;proposal.parameters=old.parameters;proposal.parameters.erase(vertex);std::map<Id,Node> points;
    for(const auto& c:old.cells)for(auto n:c.v)points.emplace(n.id,n);
    const auto expected=prescribed(old);
    for(const auto& f:expected)if(std::find(f.begin(),f.end(),neighbor)==f.end()) {
      if(proposal.cells.size()==MaximumReplacementCells){why="Prescribed cone exceeds replacement budget.";return false;}
      Cell cell{nextCell+proposal.cells.size(),{points.at(neighbor),points.at(f[0]),points.at(f[1]),points.at(f[2])}};
      if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<=0){why="Prescribed collapsed-skin cone remains obstructed.";return false;}
      proposal.cells.push_back(cell);
    }
    for(auto f:old.surface) {
      for(auto& id:f.v)if(id==vertex)id=neighbor;
      if(f.v[0]==f.v[1]||f.v[0]==f.v[2]||f.v[1]==f.v[2])continue;
      std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());proposal.surface.push_back(f);
    }
    for(const auto& f:old.coverage.faces)if(std::find(f.first.begin(),f.first.end(),vertex)==f.first.end())proposal.coverage.faces.emplace(f);
    for(const auto& e:old.coverage.traces)if(e.first[0]!=vertex&&e.first[1]!=vertex)proposal.coverage.traces.emplace(e);
    if(!ValidatePrescribedInterface(old.cells,proposal.cells,expected,why))return false;
    current.reference->ExportParameters(proposal);fresh=std::move(proposal);return true;
  };
  test(op.c_str(),grown,MetricChange::Coarsen,construct);
  // Bounded reductions keep the entire removed-vertex star. The full interface validator judges every proposal.
  unsigned reductions=0;
  for(Id id:region)if(!mandatory.count(id)&&reductions++<2&&budget.within_time()) {
    auto reduced=region;reduced.erase(id);std::vector<Id> ids(reduced.begin(),reduced.end());
    const auto reducedOp=op+"_reduce"+std::to_string(id);
    test(reducedOp.c_str(),ids,MetricChange::Coarsen,construct);
  }
}
