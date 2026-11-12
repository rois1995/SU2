REQUIRE(selected.size()==3);
auto splitContext=trial.reference->PrivateParameters(candidate);REQUIRE(splitContext);
splitExcess=GeometryGuidanceExcess(candidate,*splitContext,SU2_TYPE::GetValue(config.GetAdap_Hausd())).excess;
const Id midpoint=nextNode+completed,other1=midpoint+1,other2=midpoint+2;
std::set<Id> originalVertices;for(auto e:selected)originalVertices.insert(e.begin(),e.end());REQUIRE(originalVertices.size()==3);
Id opposite=UINT64_MAX;for(Id id:originalVertices)if(id!=selected[0][0]&&id!=selected[0][1])opposite=id;REQUIRE(opposite!=UINT64_MAX);
// Every generated point belongs only to the globally complete original split support.
for(const auto& row:trial.mesh)for(auto n:row.second.cell.v)REQUIRE((n.id!=midpoint&&n.id!=other1&&n.id!=other2));
auto has=[](const Cell& cell,Id a,Id b){bool first=false,second=false;for(auto n:cell.v){first|=n.id==a;second|=n.id==b;}return first&&second;};
std::set<Id> localIds;Patch local;local.parameters=candidate.parameters;local.coverage=candidate.coverage;
for(const auto& cell:candidate.cells)if(has(cell,midpoint,opposite)||has(cell,other1,other2)){localIds.insert(cell.id);local.cells.push_back(cell);}
const auto index=Incidence(local.cells);for(const auto& face:candidate.surface)if(index.FaceCells(face.v).size()==1)local.surface.push_back(face);
auto trim=[](Patch& patch) {
  std::set<Face> faces;std::set<Edge> edges;std::set<Id> vertices;
  for(auto face:patch.surface){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());faces.insert(face.v);
    vertices.insert(face.v.begin(),face.v.end());for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);}}
  for(auto i=patch.coverage.faces.begin();i!=patch.coverage.faces.end();)if(!faces.count(i->first))i=patch.coverage.faces.erase(i);else ++i;
  for(auto i=patch.coverage.traces.begin();i!=patch.coverage.traces.end();)if(!edges.count(i->first))i=patch.coverage.traces.erase(i);else ++i;
  for(auto i=patch.parameters.begin();i!=patch.parameters.end();)if(!vertices.count(i->first))i=patch.parameters.erase(i);else ++i;
};
trim(local);constructed=false;
const Id first=nextCell+(candidateIndex+1)*MaximumReplacementCells;
auto merge=[&](const Patch& replacement,std::string& why) {
  Patch merged=candidate;
  merged.cells.erase(std::remove_if(merged.cells.begin(),merged.cells.end(),[&](const Cell& c){return localIds.count(c.id);}),merged.cells.end());
  merged.cells.insert(merged.cells.end(),replacement.cells.begin(),replacement.cells.end());
  std::set<Face> oldPhysical;for(auto face:local.surface)oldPhysical.insert(junction_wp0::Sorted(face.v));
  merged.surface.erase(std::remove_if(merged.surface.begin(),merged.surface.end(),[&](const SurfaceFace& face){return oldPhysical.count(junction_wp0::Sorted(face.v));}),merged.surface.end());
  merged.surface.insert(merged.surface.end(),replacement.surface.begin(),replacement.surface.end());
  for(const auto& p:replacement.parameters)merged.parameters.insert_or_assign(p.first,p.second);
  for(const auto& p:replacement.coverage.faces)merged.coverage.faces.insert_or_assign(p.first,p.second);
  for(const auto& p:replacement.coverage.traces)merged.coverage.traces.insert_or_assign(p.first,p.second);
  trim(merged);for(size_t k=0;k<merged.cells.size();++k)merged.cells[k].id=first+k;
  auto artificial=[](const Patch& patch){std::set<Face> physical;for(auto f:patch.surface)physical.insert(junction_wp0::Sorted(f.v));
    std::map<Face,Face> result;for(auto f:Incidence(patch.cells).Boundary())if(!physical.count(junction_wp0::Sorted(f))){
      std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());result.emplace(junction_wp0::Sorted(f),f);}return result;};
  if(artificial(candidate)!=artificial(merged)){why="Direct diagonal replacement changes oriented artificial interface.";return false;}
  auto skin=artificial(candidate);for(auto f:merged.surface){std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());if(!skin.emplace(junction_wp0::Sorted(f.v),f.v).second){why="Diagonal replacement repeats a boundary face.";return false;}}
  std::vector<Face> expected;for(auto f:skin)expected.push_back(f.second);
  const bool closed=ValidatePrescribedInterface(candidate.cells,merged.cells,expected,why,nullptr,MaximumReplacementCells);
  if(!closed)return false;
  candidate=std::move(merged);constructed=diagonalChanged=true;return true;
};
if(local.cells.empty()||local.cells.size()>MaximumCavityCells)rejection="Complete generated diagonal star exceeds existing cavity bound.";
else for(unsigned attempt=0;attempt<3&&!diagonalChanged;++attempt) {
  const Id apex=attempt==1?other1:other2;Patch exposed,shelled;std::string why;
  const bool connected=attempt==0?(exposed=local,true):Reconnect(local,apex,first+MaximumReplacementCells,*splitContext,exposed,why);
  bool removed=false;
  if(connected) {
    auto exposedContext=splitContext->PrivateParameters(exposed);const auto& geometry=exposedContext?*exposedContext:*splitContext;
    for(const auto& cell:exposed.cells) {
      unsigned count=0;for(const auto& face:exposed.surface)if(std::find(face.v.begin(),face.v.end(),midpoint)!=face.v.end()&&
          std::find(face.v.begin(),face.v.end(),opposite)!=face.v.end()&&
          std::all_of(face.v.begin(),face.v.end(),[&](Id id){return std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return n.id==id;});}))++count;
      if(count==2&&ShellBoundaryCell(exposed,cell.id,first+2*MaximumReplacementCells,geometry,shelled,why)){removed=true;break;}
    }
    if(!removed&&why.empty())why="No positive boundary cap exposes the requested inherited diagonal.";
  }
  log<<"{\"stage\":\"diagonal_construction\",\"trial\":"<<candidateIndex<<",\"attempt\":"<<attempt<<",\"apex\":"<<(attempt?apex:0)
     <<",\"local_cells\":"<<local.cells.size()<<",\"reconnected\":"<<connected<<",\"shelled\":"<<removed<<",\"reason\":"<<junction_wp0::Json(why)<<"}"<<std::endl;
  if(!removed){rejection=why;continue;}
  if(merge(shelled,why))rejection.clear();else rejection=why;
}

// A direct coupled two-tet edit may change the wall without an admissible intermediate cap.
if(!diagonalChanged&&local.cells.size()==2&&local.surface.size()==2) {
  std::string why;std::map<Id,Node> nodes;for(const auto& cell:local.cells)for(auto n:cell.v)nodes.emplace(n.id,n);
  std::set<Id> shared;for(auto n:local.cells[0].v)if(std::any_of(local.cells[1].v.begin(),local.cells[1].v.end(),[&](Node m){return m.id==n.id;}))shared.insert(n.id);
  shared.erase(midpoint);shared.erase(opposite);bool valid=shared.size()==1;
  Patch direct;direct.parameters=local.parameters;direct.coverage=local.coverage;
  auto direction=[](Face face,Id a,Id b){for(unsigned k=0;k<3;++k){if(face[k]==a&&face[(k+1)%3]==b)return 1;if(face[k]==b&&face[(k+1)%3]==a)return -1;}return 0;};
  const std::array<Face,2> faces{{{midpoint,other1,other2},{other1,opposite,other2}}};
  const std::array<Edge,2> retained{{{midpoint,other1},{other1,opposite}}};
  for(unsigned k=0;k<2&&valid;++k) {
    auto face=faces[k];const auto edge=retained[k];int oldDirection=0;for(auto f:local.surface)oldDirection+=direction(f.v,edge[0],edge[1]);
    if(std::abs(oldDirection)!=1){valid=false;why="Boundary quad lacks a unique retained oriented edge.";break;}
    if(direction(face,edge[0],edge[1])!=oldDirection)std::swap(face[1],face[2]);
    Cell cell;cell.id=first+k;cell.v={nodes.at(*shared.begin()),nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])};
    if(!(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0)){valid=false;why="New wall triangle is not visible from the retained common apex.";break;}
    if(!splitContext->ContainsFace(local.surface[0].facet,{nodes.at(face[0]).p,nodes.at(face[1]).p,nodes.at(face[2]).p})){valid=false;why="Direct wall triangle lacks current entity association/orientation.";break;}
    direct.cells.push_back(cell);direct.surface.push_back({face,local.surface[0].facet});
  }
  if(valid){trim(direct);valid=merge(direct,why);}
  if(valid)rejection.clear();
  if(!valid){if(why.empty())why="Direct diagonal replacement lacks one retained common apex.";rejection=why;}
  log<<"{\"stage\":\"diagonal_direct\",\"trial\":"<<candidateIndex<<",\"local_cells\":"<<local.cells.size()<<",\"constructed\":"<<valid<<",\"reason\":"<<junction_wp0::Json(why)<<"}"<<std::endl;
}
