// Recheck an already-published control with independent immutable before/after parameter states.
#include "native_actual_surface_trial_context.inc"
const std::string baselineRoot=std::getenv("SU2_NATIVE_BL_SAVED_PATCH");
junction_wp0::State baseline;
{
  const auto bytes=TransactionRead(baselineRoot+"/trajectory_working_owned.bin");RecordStream stream(bytes);
  const auto rows=TransactionVector<OwnedCell>(stream,1000000);REQUIRE(stream.End());
  REQUIRE(transfer_memory::Add(held,transfer_memory::Bytes(bytes),transfer_memory::Bytes(rows),transfer_memory::Mul(rows.size(),512),size_t(256)<<20)<ceiling);
  for(auto row:rows)REQUIRE(baseline.mesh.emplace(row.cell.id,std::move(row)).second);
}
budget.caller_bytes=transfer_memory::Add(held,OwnedStorageBytes(baseline.mesh),size_t(256)<<20);REQUIRE(budget.caller_bytes<ceiling);
const auto payload=TransactionRead(baselineRoot+"/trajectory_working_parameters.bin");
baseline.live_parameters=BoundaryParameterState::FromPayload(current.live_parameters->Authority(),payload,ceiling,budget.caller_bytes);
baseline.reference=reference->original->WithParameterState(baseline.live_parameters)->WithAtlasReservation(budget);
std::vector<Id> removed,added;std::vector<OwnedCell> beforeRows,afterRows;
for(const auto& row:baseline.mesh)if(!current.mesh.count(row.first)){removed.push_back(row.first);beforeRows.push_back(row.second);}
for(const auto& row:current.mesh)if(!baseline.mesh.count(row.first)){added.push_back(row.first);afterRows.push_back(row.second);}
REQUIRE((removed.size()==10&&added.size()==8));
const auto old=junction_wp0::PatchOf(baseline,removed),fresh=junction_wp0::PatchOf(current,added);
std::set<Id> beforeVertices,afterVertices;for(auto c:old.cells)for(auto n:c.v)beforeVertices.insert(n.id);for(auto c:fresh.cells)for(auto n:c.v)afterVertices.insert(n.id);
std::set<Id> removedVertices;for(Id id:beforeVertices)if(!afterVertices.count(id))removedVertices.insert(id);
REQUIRE(removedVertices.size()==1);std::set<Id> expected(removed.begin(),removed.end());
for(const auto& row:baseline.mesh)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return removedVertices.count(n.id);}))REQUIRE(expected.count(row.first));
for(auto c:fresh.cells)REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
size_t q0=0,q1=0,j0=0,j1=0,g0=0,g1=0;
auto before=measure(old,beforeRows,*baseline.reference,q0,j0,g0),after=measure(fresh,afterRows,*current.reference,q1,j1,g1);
auto neutral=[&](PatchMetric& metric,const Patch& patch,const PlanarReference& g) {
  std::set<Edge> edges;for(auto f:patch.surface)if(heights.count(g.Get(f.facet).marker))for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);}
  metric.maximum_length=0;for(auto& e:metric.edges){if(!edges.count(e.first))e.second.length=1;metric.maximum_length=std::max(metric.maximum_length,e.second.length);}
};
neutral(before,old,*baseline.reference);neutral(after,fresh,*current.reference);std::string why;
REQUIRE(ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),after,MetricChange::Coarsen,why));
const auto oldGeometry=GeometryGuidanceExcess(old,*baseline.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
const auto newGeometry=GeometryGuidanceExcess(fresh,*current.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
REQUIRE(oldGeometry.complete);REQUIRE(newGeometry.complete);REQUIRE(newGeometry.excess==0);
REQUIRE(PreservePassingFirstHeights(old,fresh,heights,*baseline.reference,why));
FirstHeightReport h;const bool strict=CheckFirstHeight(fresh,heights,*current.reference,h,why);
REQUIRE((strict||(h.measured&&!h.reference_clearance_failures&&!h.missing_cells)));
REQUIRE(baseline.live_parameters->CheckReplacement(old,fresh,why,ceiling,budget.caller_bytes,&budget));
REQUIRE(baseline.live_parameters->Payload()==payload);REQUIRE(current.live_parameters->Payload()==acceptedParameters);
log<<"{\"stage\":\"saved_publication_recheck\",\"constructor_calls\":0,\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<fresh.cells.size()
   <<",\"old_q_bad\":"<<q0<<",\"new_q_bad\":"<<q1<<",\"old_J_bad\":"<<j0<<",\"new_J_bad\":"<<j1<<",\"old_geometry_bad\":"<<g0<<",\"new_geometry_bad\":"<<g1
   <<",\"old_geometry_excess\":"<<oldGeometry.excess<<",\"new_geometry_excess\":"<<newGeometry.excess<<",\"strict_height\":"<<strict<<",\"provisional_height\":true,\"independent_parameter_epochs\":true,\"existing_state_reused\":true,\"mesh_accepted\":false}"<<std::endl;
NativeStageCapture::WriteOwned("rechecked_owned.bin",current.mesh);
NativeStageCapture::CheckPayloadBound(current.live_parameters->PayloadBytes(),NativeStageCapture::limit);
NativeStageCapture::WriteBytes("rechecked_parameters.bin",current.live_parameters->Payload());
