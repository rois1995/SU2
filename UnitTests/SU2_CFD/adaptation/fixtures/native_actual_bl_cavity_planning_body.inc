const auto* capture=std::getenv("SU2_NATIVE_BL_PLANNING_CAPTURE");REQUIRE(capture);
const std::string root=capture;constexpr size_t maxRows=1000000;
junction_wp0::State current;
{
  const auto bytes=TransactionRead(root+"/stage_surface_rank0_owned.bin");RecordStream input(bytes);
  auto rows=TransactionVector<OwnedCell>(input,maxRows);REQUIRE(input.End());
  REQUIRE(transfer_memory::Add(reference->original->ResidentBytes(),restored.family->ResidentBytes(),
      transfer_memory::Mul(rows.size(),6*(sizeof(DonorCell)+640)),size_t(256)<<20)<ceiling);
  for(auto& row:rows)REQUIRE(current.mesh.emplace(row.cell.id,std::move(row)).second);
}
const auto retained=transfer_memory::Add(ReferenceStorageBytes(*reference),restored.family->ResidentBytes(),
    transfer_memory::Mul(current.mesh.size(),6*(sizeof(DonorCell)+640)),size_t(256)<<20);
current.live_parameters=BoundaryParameterState::FromPayload(reference->original->Parameters()->Authority(),
    TransactionRead(root+"/stage_surface_rank0_parameters.bin"),ceiling,retained);
SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;budget.caller_bytes=retained;
budget.maximum_work=transfer_memory::Mul(current.mesh.size(),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
constexpr size_t atlasScratch=size_t(16)<<20;size_t low=0,high=atlasScratch*8;
while(low<high){const size_t bits=low+(high-low+1)/2;
  if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=atlasScratch/2)low=bits;else high=bits-1;}
REQUIRE(low>0);budget.maximum_integer_bits=low;
const auto begun=std::chrono::steady_clock::now();
budget.within_time=[&](){return std::chrono::duration<double>(std::chrono::steady_clock::now()-begun).count()<300;};
auto lease=policy.ReserveGeometry(budget);
current.reference=reference->original->WithParameterState(current.live_parameters)->WithAtlasReservation(budget);
std::vector<DonorCell> donors;
{
  const auto bytes=TransactionRead(root+"/stage_surface_rank0_donors.bin");RecordStream input(bytes);
  donors=TransactionVector<DonorCell>(input,maxRows);REQUIRE(input.End());
}
const auto donorCount=donors.size();REQUIRE(donorCount>0);
REQUIRE(transfer_memory::Add(retained,transfer_memory::Mul(donorCount,4*(sizeof(DonorCell)+640)))<ceiling);
const auto extensions=AssociatedExtensions(donors,*current.reference);
auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
FrozenField field(std::move(donors),{},true,extensions,{},identity,true,std::max(MaximumDonors,donorCount),true);
WallHeights heights;heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
for(unsigned short w=0;w<config.GetnAdap_BL();++w) {
  const auto& spec=config.GetAdap_BL(w);const auto marker=std::find(reference->marker_names.begin(),reference->marker_names.end(),spec.marker);
  REQUIRE(marker!=reference->marker_names.end());heights[int(marker-reference->marker_names.begin())]=SU2_TYPE::GetValue(spec.firstHeight);
}
auto star=[&](Id vertex) {std::vector<Id> ids;
  for(const auto& row:current.mesh)for(auto node:row.second.cell.v)if(node.id==vertex){ids.push_back(row.first);break;}
  REQUIRE_FALSE(ids.empty());REQUIRE(ids.size()<=MaximumCavityCells);return ids;
};
std::vector<std::pair<std::string,std::vector<Id>>> contexts;
contexts.push_back({"recorded_orientation_refusal",star(21292)});REQUIRE(contexts.back().second.size()==10);
std::set<Id> constrained=current.live_parameters->Mobility().corners;
for(const auto& edge:current.live_parameters->Mobility().features)constrained.insert(edge.first.begin(),edge.first.end());
Id smooth=0;long double nearest=INFINITY;
for(const auto& row:current.live_parameters->Records()) {
  const auto& p=row.second.point;if(constrained.count(row.first))continue;
  bool requested=false;current.live_parameters->Geometry().ForFacets(p.binding,[&](Id facet) {
    requested=requested||heights.count(reference->original->Get(facet).marker);
  });if(!requested)continue;
  const auto x=p.node.p.x-.7L,y=p.node.p.y-.5L,z=p.node.p.z-.04L;const auto distance=x*x+y*y+z*z;
  if(distance<nearest){nearest=distance;smooth=row.first;}
}
REQUIRE(smooth!=0);contexts.push_back({"unchanged_wall_control",star(smooth)});
for(const auto& row:current.mesh)if(std::none_of(row.second.physical.begin(),row.second.physical.end(),[](FacetBinding f){return f.present;})) {
  contexts.push_back({"unchanged_interior_control",{row.first}});break;
}
contexts.push_back({"leading_edge",star(4219)});
REQUIRE(contexts.size()==4);
std::ofstream log("actual_bl_cavity_planning.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
auto samePoint=[](Point a,Point b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
auto sameTensor=[](Tensor a,Tensor b){return a.xx==b.xx&&a.xy==b.xy&&a.xz==b.xz&&a.yy==b.yy&&a.yz==b.yz&&a.zz==b.zz;};
for(const auto& entry:contexts) {
  const auto patch=junction_wp0::PatchOf(current,entry.second);
  for(const auto& c:patch.cells)REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
  std::map<Id,std::vector<BoundaryMetricSample>> witness;std::map<Edge,EdgeMeasure> baseline;
  // ABBA retains shared query-cache effects in both directions; no geometry result is a mesh certificate.
  for(unsigned repeat=0;repeat<4;++repeat) {
    const bool bulk=repeat==1||repeat==2;size_t calls=0;double guidanceSeconds=0;
    const BoundaryMetric target=[&](Node node,const PlanarReference& geometry,Tensor sensor) {
      const auto t=std::chrono::steady_clock::now();auto result=policy.BoundaryGuidance(node,geometry,sensor,reference->marker_names);
      guidanceSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();++calls;
      const auto inserted=witness.emplace(node.id,result);
      if(!inserted.second) {
        REQUIRE(inserted.first->second.size()==result.size());
        for(size_t i=0;i<result.size();++i) {
          CHECK(sameTensor(inserted.first->second[i].tensor,result[i].tensor));
          CHECK(samePoint(inserted.first->second[i].normal,result[i].normal));
          CHECK(samePoint(inserted.first->second[i].strip_rule,result[i].strip_rule));
        }
      }
      return result;
    };
    const auto before=field.Statistics();const auto workBefore=budget.work;MetricStats counted;
    const auto t=std::chrono::steady_clock::now();std::map<Edge,EdgeMeasure> measured;
    if(bulk)measured=MeasureSurfacePlanning(patch,field,*current.reference,heights,target,&counted).edges;
    else for(const auto& cell:patch.cells) {
      Patch part;part.cells={cell};part.parameters=patch.parameters;part.coverage=patch.coverage;
      const auto faces=junction_wp0::CellFaces(cell);
      for(const auto& face:patch.surface)if(std::any_of(faces.begin(),faces.end(),[&](Face f){std::sort(f.begin(),f.end());auto other=face.v;std::sort(other.begin(),other.end());return f==other;}))part.surface.push_back(face);
      const auto result=MeasureSurfacePlanning(part,field,*current.reference,heights,target,&counted);
      for(const auto& edge:result.edges){const auto inserted=measured.emplace(edge);if(!inserted.second)CHECK(inserted.first->second.length==edge.second.length);}
    }
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
    if(!repeat)baseline=measured;
    REQUIRE(measured.size()==baseline.size());
    for(const auto& edge:measured) {
      REQUIRE(baseline.count(edge.first));const auto& old=baseline.at(edge.first);
      CHECK(edge.second.length==old.length);CHECK(samePoint(edge.second.a,old.a));CHECK(samePoint(edge.second.b,old.b));
    }
    log<<"{\"context\":"<<std::quoted(entry.first)<<",\"cells\":"<<patch.cells.size()<<",\"surface_faces\":"<<patch.surface.size()
       <<",\"mode\":"<<std::quoted(bulk?"complete_patch":"per_cell")<<",\"repeat\":"<<repeat<<",\"edges\":"<<measured.size()
       <<",\"guidance_calls\":"<<calls<<",\"unique_nodes\":"<<witness.size()<<",\"seconds\":"<<seconds
       <<",\"guidance_seconds_nested\":"<<guidanceSeconds<<",\"geometry_work\":"<<budget.work-workBefore
       <<",\"core_edge_evaluations\":"<<counted.edge_evaluations<<",\"field_requests\":"<<field.Statistics().requests-before.requests
       <<",\"exact_edge_and_sample_agreement\":true,\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
  }
}
