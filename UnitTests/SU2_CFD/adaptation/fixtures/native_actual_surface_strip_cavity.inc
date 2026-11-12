std::set<Id> faceVertices;for(auto edge:selected)faceVertices.insert(edge.begin(),edge.end());
auto sourceFace=std::find_if(previous.surface.begin(),previous.surface.end(),[&](const SurfaceFace& face){return std::set<Id>(face.v.begin(),face.v.end())==faceVertices;});
REQUIRE(sourceFace!=previous.surface.end());
auto face=sourceFace->v;std::rotate(face.begin(),face.begin()+(candidateIndex%3),face.end());
const Id a=face[0],b=face[1],c=face[2];
const auto incident=Incidence(previous.cells).FaceCells(sourceFace->v);REQUIRE(incident.size()==1);
const auto owner=std::find_if(previous.cells.begin(),previous.cells.end(),[&](const Cell& cell){return cell.id==incident[0];});REQUIRE(owner!=previous.cells.end());
Node apex;for(auto node:owner->v)if(!faceVertices.count(node.id))apex=node;
unsigned walls=0;for(auto f:previous.surface)if(std::all_of(f.v.begin(),f.v.end(),[&](Id id){return std::any_of(owner->v.begin(),owner->v.end(),[&](Node n){return n.id==id;});}))++walls;
if(walls!=1){constructed=false;rejection="Directional strip needs a single-wall source tetrahedron.";}
const Id start=nextNode+completed,first=nextCell+(candidateIndex+1)*MaximumReplacementCells;
const Id e=start,f=start+1,g=start+2,h=start+3,i=start+4,j=start+5;
const std::array<Edge,6> cuts{{{a,c},{b,c},{a,e},{b,f},{e,c},{f,c}}};
for(auto edge:cuts)if(constructed) {
  std::sort(edge.begin(),edge.end());std::map<Id,Node> currentNodes;for(const auto& cell:candidate.cells)for(auto n:cell.v)currentNodes.emplace(n.id,n);
  const auto x=currentNodes.at(edge[0]).p,y=currentNodes.at(edge[1]).p;
  Node point{start+splits,{double(((long double)x.x+y.x)/2),double(((long double)x.y+y.y)/2),double(((long double)x.z+y.z)/2)}};
  Patch next;if(!SplitSurfaceStrip(candidate,edge,point,{},first,*trial.reference,next,rejection)){constructed=false;break;}
  candidate=std::move(next);++splits;
}
if(constructed) {
  const std::set<Id> surfaceNodes{a,b,c,e,f,g,h,i,j};std::map<Id,Node> currentNodes;
  for(const auto& cell:candidate.cells)for(auto n:cell.v)currentNodes.emplace(n.id,n);
  const auto incidence=Incidence(candidate.cells);std::set<Id> retired;std::set<Face> retiredFaces;
  for(const auto& wall:candidate.surface)if(std::all_of(wall.v.begin(),wall.v.end(),[&](Id id){return surfaceNodes.count(id);})) {
    retiredFaces.insert(junction_wp0::Sorted(wall.v));const auto cells=incidence.FaceCells(wall.v);
    if(cells.size()!=1){constructed=false;rejection="Directional strip lost its unique wall owner.";break;}
    retired.insert(cells[0]);
  }
  if(retired.size()!=7||retiredFaces.size()!=7){constructed=false;rejection="Directional strip descendants do not form the expected complete face disk.";}
  for(const auto& cell:candidate.cells)if(retired.count(cell.id)) {
    if(!std::any_of(cell.v.begin(),cell.v.end(),[&](Node node){return node.id==apex.id;})){constructed=false;rejection="Directional strip descendants lack one retained common apex.";}
  }
  if(constructed) {
    auto context=trial.reference->PrivateParameters(candidate);REQUIRE(context);
    Patch rebuilt=candidate;
    rebuilt.cells.erase(std::remove_if(rebuilt.cells.begin(),rebuilt.cells.end(),[&](const Cell& cell){return retired.count(cell.id);}),rebuilt.cells.end());
    rebuilt.surface.erase(std::remove_if(rebuilt.surface.begin(),rebuilt.surface.end(),[&](const SurfaceFace& wall){return retiredFaces.count(junction_wp0::Sorted(wall.v));}),rebuilt.surface.end());
    const std::array<Face,7> bands{{{a,b,g},{g,b,h},{g,h,e},{e,h,f},{e,f,i},{i,f,j},{i,j,c}}};
    for(auto wall:bands) {
      Cell cell;cell.v={apex,currentNodes.at(wall[0]),currentNodes.at(wall[1]),currentNodes.at(wall[2])};
      if(!(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0)){constructed=false;rejection="Directional wall triangle is not visible from retained fluid apex.";break;}
      if(!context->ContainsFace(sourceFace->facet,{currentNodes.at(wall[0]).p,currentNodes.at(wall[1]).p,currentNodes.at(wall[2]).p})){constructed=false;rejection="Directional wall triangle lacks current geometry association/orientation.";break;}
      rebuilt.cells.push_back(cell);rebuilt.surface.push_back({wall,sourceFace->facet});
    }
    auto artificial=[](const Patch& patch){std::set<Face> physical;for(auto f:patch.surface)physical.insert(junction_wp0::Sorted(f.v));
      std::map<Face,Face> result;for(auto f:Incidence(patch.cells).Boundary())if(!physical.count(junction_wp0::Sorted(f))){std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());result.emplace(junction_wp0::Sorted(f),f);}return result;};
    if(constructed) {
      for(size_t k=0;k<rebuilt.cells.size();++k)rebuilt.cells[k].id=first+k;
      if(artificial(previous)!=artificial(rebuilt)){constructed=false;rejection="Directional strip changes oriented artificial interface.";}
      std::set<Face> keptFaces;std::set<Edge> keptEdges;
      auto skin=artificial(previous);for(auto wall:rebuilt.surface){std::rotate(wall.v.begin(),std::min_element(wall.v.begin(),wall.v.end()),wall.v.end());keptFaces.insert(wall.v);
        if(!skin.emplace(junction_wp0::Sorted(wall.v),wall.v).second){constructed=false;rejection="Directional strip repeats a physical/interface face.";}
        for(unsigned k=0;k<3;++k){Edge edge{wall.v[k],wall.v[(k+1)%3]};std::sort(edge.begin(),edge.end());keptEdges.insert(edge);}}
      for(auto it=rebuilt.coverage.faces.begin();it!=rebuilt.coverage.faces.end();)if(!keptFaces.count(it->first))it=rebuilt.coverage.faces.erase(it);else ++it;
      for(auto it=rebuilt.coverage.traces.begin();it!=rebuilt.coverage.traces.end();)if(!keptEdges.count(it->first))it=rebuilt.coverage.traces.erase(it);else ++it;
      std::vector<Face> expected;for(auto row:skin)expected.push_back(row.second);
      if(constructed&&!ValidatePrescribedInterface(previous.cells,rebuilt.cells,expected,rejection))constructed=false;
      if(constructed)candidate=std::move(rebuilt);
    }
  }
}
log<<"{\"stage\":\"strip_construction\",\"trial\":"<<candidateIndex<<",\"facet\":"<<sourceFace->facet<<",\"base\":["<<a<<','<<b<<"],\"opposite\":"<<c
   <<",\"apex\":["<<apex.id<<','<<apex.p.x<<','<<apex.p.y<<','<<apex.p.z<<"],\"new_points\":"<<splits<<",\"constructed\":"<<constructed<<",\"reason\":"<<junction_wp0::Json(rejection)<<"}"<<std::endl;
