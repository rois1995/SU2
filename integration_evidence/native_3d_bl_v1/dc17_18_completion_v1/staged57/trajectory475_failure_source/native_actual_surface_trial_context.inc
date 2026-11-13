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
