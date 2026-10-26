#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include <chrono>
#include <ctime>
// Bounded test-only capture; borrowed engine objects never survive the callback.
namespace {
struct NativeTransactionCapture {
  struct Event {
    std::string stage,reason;
    bool passed;
    Patch old,fresh;
    std::vector<Dependency> dependencies;
    Node point;
    Id first_cell,first_node;
    MetricDiagnosis metric;
  };
  std::vector<OwnedCell> working;
  std::vector<Event> events;
  std::map<Id,DonorCell> donors;
  std::shared_ptr<const PlanarReference> reference;
  MetricComposition composition;
  MetricEdgeBreakpoints breakpoints;
  Feature feature{};
  uint64_t round=0;
  bool selected=false,begun=false,ended=false,truncated=false;
  size_t bytes=0;
  static constexpr size_t limit=size_t(128)<<20;
  bool Admit(size_t extra) {
    if(truncated || extra>limit-bytes){truncated=true;return false;}
    bytes+=extra;return true;
  }
  void Observe(const TransactionView& v) {
    if(std::string(v.stage)=="begin") {
      begun=true;selected=v.selected;feature=v.feature;round=v.round;
      const size_t geometry_bytes=v.geometry.Parameters()?2*v.geometry.Parameters()->ResidentBytes():v.geometry.ResidentBytes();
      if(!Admit(v.working.size()*sizeof(OwnedCell)+geometry_bytes))return;
      working.reserve(v.working.size());for(const auto& row:v.working)working.push_back(row.second);
      if(v.geometry.Parameters()) {
        const auto* p=v.geometry.Parameters();
        reference=v.geometry.WithParameterState(BoundaryParameterState::FromPayload(p->Authority(),p->Payload()));
      } else reference=std::make_shared<PlanarReference>(v.geometry);
      composition=v.composition;breakpoints=v.breakpoints;
    }
    if((!selected && std::string(v.stage)!="begin") || truncated)return;
    if(v.donors) {
      for(const auto& d:v.donors->cells)if(!donors.count(d.cell.id)) {
        if(!Admit(sizeof(DonorCell)+8*sizeof(void*)))return;
        donors.emplace(d.cell.id,d);
      }
      return;
    }
    if(events.size()==512){truncated=true;return;}
    const auto patch_bytes=[](const Patch& p) {return p.cells.size()*sizeof(Cell)+p.surface.size()*sizeof(SurfaceFace)+
        p.parameters.size()*(sizeof(BoundaryPoint)+8*sizeof(void*));};
    if(!Admit(sizeof(Event)+v.reason.size()+patch_bytes(v.old)+patch_bytes(v.fresh)+v.dependencies.cells.size()*sizeof(Dependency)))return;
    events.push_back({v.stage,v.reason,v.passed,v.old,v.fresh,v.dependencies.cells,v.point,v.first_cell,v.first_node,v.metric});
    ended=ended || std::string(v.stage)=="transaction_end";
  }
  template<class T> static void Vector(RecordStream& out,std::vector<T> values) {
    uint64_t n=values.size();out(n);for(auto& value:values)out(value);
  }
  static std::vector<char> PatchBytes(const Patch& patch) {
    std::vector<char> bytes;RecordStream out(bytes);Vector(out,patch.cells);Vector(out,patch.surface);
    std::vector<BoundaryPoint> points;for(const auto& row:patch.parameters)points.push_back(row.second);Vector(out,points);return bytes;
  }
  static void Write(const std::string& path,const std::vector<char>& bytes) {
    std::ofstream out(path,std::ios::binary);out.write(bytes.data(),bytes.size());out.close();REQUIRE(bool(out));
  }
  void Save(int rank) const {
    const auto prefix="transaction_rank"+std::to_string(rank);
    std::vector<char> data;RecordStream out(data);Vector(out,working);Write(prefix+"_working.bin",data);
    if(reference && reference->Parameters())Write(prefix+"_parameters.bin",reference->Parameters()->Payload());
    data.clear();std::vector<DonorCell> source;for(const auto& row:donors)source.push_back(row.second);
    RecordStream source_out(data);Vector(source_out,source);Write(prefix+"_donors.bin",data);
    std::ofstream log(prefix+".json");log<<std::setprecision(17)<<"{\"rank\":"<<rank<<",\"round\":"<<round
      <<",\"begun\":"<<begun<<",\"selected\":"<<selected<<",\"ended\":"<<ended<<",\"truncated\":"<<truncated
      <<",\"copied_bytes_estimate\":"<<bytes<<",\"working_cells\":"<<working.size()<<",\"donors\":"<<donors.size()
      <<",\"feature\":["<<feature.vertices[0]<<','<<feature.vertices[1]<<','<<feature.vertices[2]<<"],\"events\":[";
    for(size_t i=0;i<events.size();++i) {
      const auto& e=events[i];const auto stem=prefix+"_event"+std::to_string(i);
      Write(stem+"_old.bin",PatchBytes(e.old));Write(stem+"_fresh.bin",PatchBytes(e.fresh));
      data.clear();RecordStream dependencies(data);Vector(dependencies,e.dependencies);Write(stem+"_dependencies.bin",data);
      if(i)log<<',';log<<"{\"stage\":"<<std::quoted(e.stage)<<",\"passed\":"<<e.passed<<",\"reason\":"<<std::quoted(e.reason)
        <<",\"point\":["<<e.point.id<<','<<e.point.p.x<<','<<e.point.p.y<<','<<e.point.p.z<<"],\"first_cell\":"<<e.first_cell
        <<",\"first_node\":"<<e.first_node<<",\"metric_failure\":"<<unsigned(e.metric.failure)
        <<",\"q\":"<<e.metric.minimum_mean_ratio<<",\"J\":"<<e.metric.minimum_scaled_jacobian<<'}';
    }
    log<<"]}\n";log.close();REQUIRE(bool(log));
  }
  void CheckContext(World& world) const {
    Owned owned;for(const auto& row:working)owned.emplace(row.cell.id,row);
    Directory directory(world,false,reference->Parameters());std::string reason;
    REQUIRE(directory.Build(owned,reason,size_t(1)<<30));
    const auto deps=directory.Lookup(selected?std::vector<Feature>{feature}:std::vector<Feature>{},size_t(1)<<30);
    INFO(deps.reason);REQUIRE(deps.valid);
    const auto imported=directory.Fetch(deps,owned,size_t(1)<<30);INFO(imported.reason);REQUIRE(imported.valid);
    const bool reserved=directory.Reserve(imported,selected,reason,size_t(1)<<30);
    CHECK((!selected || reserved));
    Patch old,fresh;bool candidate=false;
    if(selected) {
      const auto first=std::find_if(events.begin(),events.end(),[](const Event& e){return e.stage=="SplitEdge";});
      REQUIRE(first!=events.end());old=first->old;fresh=first->fresh;candidate=first->passed;
      CHECK(deps.cells.size()==first->dependencies.size());
      for(const auto& dep:first->dependencies)CHECK(std::any_of(deps.cells.begin(),deps.cells.end(),[&](const Dependency& d){
        return d.cell==dep.cell && d.version==dep.version && d.owner==dep.owner;}));
    }
    REQUIRE(directory.Clearance(owned,old,fresh,candidate,reason,size_t(1)<<30));
    if(selected) {
      std::ofstream log("transaction_contact_rank"+std::to_string(world.rank)+".json");log
        <<"{\"dependency_versions_match\":true,\"single_candidate_reserved\":"<<reserved
        <<",\"initial_candidate_external_contact_passed\":"<<candidate<<",\"reason\":"<<std::quoted(reason)
        <<",\"accepted_repair\":false,\"original_peer_reservations_replayed\":false}\n";
      log.close();REQUIRE(bool(log));
    }
  }
  void ReplayFirst(int rank) const {
    if(!selected)return;
    REQUIRE_FALSE(truncated);REQUIRE(ended);REQUIRE(reference);
    const auto found=std::find_if(events.begin(),events.end(),[](const Event& e){return e.stage=="SplitEdge";});
    REQUIRE(found!=events.end());const auto& first=*found;
    Patch fresh;std::string reason;
    const bool valid=SplitEdge(first.old,{feature.vertices[0],feature.vertices[1]},first.point,first.first_cell,*reference,fresh,reason);
    CHECK(valid==first.passed);CHECK(PatchBytes(fresh)==PatchBytes(first.fresh));
    std::vector<DonorCell> source;for(const auto& row:donors)source.push_back(row.second);
    REQUIRE_FALSE(source.empty());
    const auto components=AssociatedExtensions(source,*reference);
    FrozenField field(std::move(source),composition,false,components,breakpoints,{},true,donors.size());
    const auto before=MeasureMetricCells(first.old.cells,field);
    const auto after=MeasureMetricCells(fresh.cells,field);
    const auto metric=std::find_if(found,events.end(),[](const Event& e){return e.stage=="metric";});
    REQUIRE(metric!=events.end());
    MetricDiagnosis diagnosis;PatchMetric failed;std::string failure;
    const auto prior=GetShapeAdmission();struct Restore {ShapeAdmission prior;~Restore(){SetShapeAdmission(prior);}} restore{prior};
    SetShapeAdmission(ShapeAdmission::Absolute);
    bool strict=true;
    for(const auto& cell:fresh.cells)try {MeasureMetricCells({cell},field,nullptr,nullptr,true,&diagnosis);}
      catch(const std::exception& e){strict=false;failure=e.what();break;}
    CHECK_FALSE(strict);CHECK(failure==metric->reason);
    CHECK(diagnosis.minimum_mean_ratio==metric->metric.minimum_mean_ratio);
    CHECK(diagnosis.minimum_scaled_jacobian==metric->metric.minimum_scaled_jacobian);
    std::ofstream log("transaction_replay_rank"+std::to_string(rank)+".json");log<<std::setprecision(17)
      <<"{\"initial_split_exact\":"<<(valid==first.passed && PatchBytes(fresh)==PatchBytes(first.fresh))
      <<",\"first_failure_exact\":"<<(failure==metric->reason)<<",\"before_q\":"<<before.minimum_mean_ratio
      <<",\"before_J\":"<<before.minimum_scaled_jacobian<<",\"after_q\":"<<after.minimum_mean_ratio
      <<",\"after_J\":"<<after.minimum_scaled_jacobian<<",\"maximum_integrated_edge\":"<<double(after.maximum_length)
      <<",\"complete_sequence_replayed\":false,\"external_contact_certified\":false,\"accepted_repair\":false}\n";
    log.close();REQUIRE(bool(log));
  }
};
}

namespace {
std::vector<char> TransactionRead(const std::string& path,size_t limit=size_t(128)<<20) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);REQUIRE(bool(input));
  const auto length=input.tellg();REQUIRE(length>=0);REQUIRE(size_t(length)<=limit);
  std::vector<char> bytes(static_cast<size_t>(length));input.seekg(0);input.read(bytes.data(),bytes.size());REQUIRE(bool(input));return bytes;
}
template<class T> std::vector<T> TransactionVector(RecordStream& stream,size_t limit) {
  uint64_t count=0;stream(count);REQUIRE(count<=limit);std::vector<T> result(count);
  for(auto& value:result)stream(value);return result;
}
Patch TransactionPatch(const std::string& path) {
  const auto bytes=TransactionRead(path);RecordStream stream(bytes);Patch result;
  result.cells=TransactionVector<Cell>(stream,MaximumReplacementCells);
  result.surface=TransactionVector<SurfaceFace>(stream,4*MaximumReplacementCells);
  for(auto p:TransactionVector<BoundaryPoint>(stream,4*MaximumReplacementCells))REQUIRE(result.parameters.emplace(p.node.id,p).second);
  REQUIRE(stream.End());return result;
}
std::string TransactionString(RecordStream& stream) {
  auto bytes=TransactionVector<unsigned char>(stream,8192);return {bytes.begin(),bytes.end()};
}
// Independent surface-error comparison; no production acceptance decision is bypassed.
long double TransactionSurfaceExcess(const Patch& p,const PlanarReference& base,double tolerance) {
  auto overlay=base.PrivateParameters(p);const auto& reference=overlay?*overlay:base;
  std::map<Id,Point> points;for(const auto& c:p.cells)for(auto n:c.v)points.emplace(n.id,n.p);
  long double sum=0;
  for(const auto& f:p.surface) {
    const std::array<Point,3> x{points.at(f.v[0]),points.at(f.v[1]),points.at(f.v[2])};
    const long double excess=std::max(0.,reference.SurfaceError(f.facet,x)-tolerance);
    const std::array<long double,3> a{x[1].x-(long double)x[0].x,x[1].y-(long double)x[0].y,x[1].z-(long double)x[0].z};
    const std::array<long double,3> b{x[2].x-(long double)x[0].x,x[2].y-(long double)x[0].y,x[2].z-(long double)x[0].z};
    long double area2=0;for(unsigned i=0;i<3;++i){const auto v=a[(i+1)%3]*b[(i+2)%3]-a[(i+2)%3]*b[(i+1)%3];area2+=v*v;}
    sum+=excess*excess*std::sqrt(area2);
  }
  return sum;
}
}

TEST_CASE("Saved native transaction construction replay", "[.][NativeTransactionReplay3D]") {
  World world;REQUIRE(world.size==1);
  const auto* folder=std::getenv("SU2_NATIVE_TRANSACTION_REPLAY");REQUIRE(folder);
  const auto* authority=std::getenv("SU2_NATIVE_TRANSACTION_REFERENCE");REQUIRE(authority);
  const auto* tolerance_text=std::getenv("SU2_NATIVE_TRANSACTION_HAUSD");REQUIRE(tolerance_text);
  const double tolerance=std::stod(tolerance_text);REQUIRE(tolerance>0);
  const std::string root=folder;NativeTransactionCapture capture;capture.selected=capture.begun=capture.ended=true;
  capture.feature={{{23754,24333,0}},2};
  const auto encoded=TransactionRead(authority);auto reference=DecodeReference({encoded.begin(),encoded.end()});
  REQUIRE(reference.original->Parameters());
  capture.reference=reference.original->WithParameterState(BoundaryParameterState::FromPayload(
      reference.original->Parameters()->Authority(),TransactionRead(root+"/transaction_rank2_parameters.bin")));
  {
    const auto bytes=TransactionRead(root+"/transaction_rank2_donors.bin");RecordStream stream(bytes);
    for(auto d:TransactionVector<DonorCell>(stream,65536))REQUIRE(capture.donors.emplace(d.cell.id,d).second);REQUIRE(stream.End());
  }
  {
    const auto bytes=TransactionRead(root+"/events.bin");RecordStream stream(bytes);uint64_t count=0;stream(count);REQUIRE(count<=512);
    for(size_t i=0;i<count;++i) {
      NativeTransactionCapture::Event e;uint64_t passed=0,failure=0;
      e.stage=TransactionString(stream);e.reason=TransactionString(stream);stream(passed,e.point,e.first_cell,e.first_node,failure,
          e.metric.minimum_mean_ratio,e.metric.minimum_scaled_jacobian);REQUIRE(passed<=1);e.passed=passed;
      REQUIRE(failure<=unsigned(MetricFailure::Progress));e.metric.failure=MetricFailure(failure);
      const auto stem=root+"/transaction_rank2_event"+std::to_string(i);
      e.old=TransactionPatch(stem+"_old.bin");e.fresh=TransactionPatch(stem+"_fresh.bin");capture.events.push_back(std::move(e));
    }
    REQUIRE(stream.End());
  }
  const auto prior=GetShapeAdmission();struct Restore {ShapeAdmission prior;~Restore(){SetShapeAdmission(prior);}} restore{prior};
  SetShapeAdmission(ShapeAdmission::Absolute);
  std::vector<DonorCell> source;for(const auto& d:capture.donors)source.push_back(d.second);
  const auto components=AssociatedExtensions(source,*capture.reference);
  FrozenField field(source,{},false,components,{}, {},true,source.size());
  const Edge edge{capture.feature.vertices[0],capture.feature.vertices[1]};
  size_t constructions=0,metrics=0,reconnects=0;bool exact=true,placement_set=false;Tensor placement;
  const NativeTransactionCapture::Event* first=nullptr;
  for(size_t i=0;i<capture.events.size();++i) {
    const auto& e=capture.events[i];INFO(i);INFO(e.stage);INFO(e.reason);
    Patch fresh;std::string reason;bool built=false,construction=true;
    if(e.stage=="SplitEdge") {
      first=&e;built=SplitEdge(e.old,edge,e.point,e.first_cell,*capture.reference,fresh,reason);
    } else if(e.stage=="ReconstructSplitPatch") {
      if(!placement_set){placement=field.Query(Centroid(e.old.cells.front())).target;placement_set=true;}
      Face obstruction;built=ReconstructSplitPatch(e.old,edge,e.point,{e.first_node+1,{}},placement,e.first_cell,
          *capture.reference,fresh,obstruction,reason);
    } else if(e.stage=="Reconnect") {
      std::set<Id> nodes;for(const auto& c:e.old.cells)for(auto n:c.v)nodes.insert(n.id);
      REQUIRE(reconnects<nodes.size());auto apex=nodes.begin();std::advance(apex,reconnects++);
      built=Reconnect(e.old,*apex,e.first_cell,*capture.reference,fresh,reason);
    } else construction=false;
    if(construction) {
      ++constructions;const bool same=built==e.passed && reason==e.reason &&
          (!built || NativeTransactionCapture::PatchBytes(fresh)==NativeTransactionCapture::PatchBytes(e.fresh));
      CHECK(same);exact=exact && same;
    } else if(e.stage=="metric" && e.metric.failure==MetricFailure::Shape) {
      MetricDiagnosis diagnosis;bool failed=false;
      for(const auto& cell:e.fresh.cells)try {MeasureMetricCells({cell},field,nullptr,nullptr,true,&diagnosis);}
        catch(const std::exception& error){failed=true;reason=error.what();break;}
      const bool same=failed && reason==e.reason && diagnosis.failure==e.metric.failure;
      CHECK(same);exact=exact && same;++metrics;
    } else if(e.stage=="geometry") {
      const auto before=TransactionSurfaceExcess(e.old,*capture.reference,tolerance);
      const auto after=TransactionSurfaceExcess(e.fresh,*capture.reference,tolerance);
      CHECK(after>before);exact=exact && after>before;
    }
  }
  std::ofstream strict("strict_replay.json");strict<<"{\"construction_calls\":"<<constructions<<",\"shape_failures\":"<<metrics
    <<",\"calls_and_failure_reasons_exact\":"<<exact<<",\"growth_selection_policy_replayed\":false,\"MPI_peer_schedule_replayed\":false}\n";
  strict.close();REQUIRE(bool(strict));REQUIRE(exact);REQUIRE(first);REQUIRE(constructions==26);
  // The live capture already certified first-cavity completeness, versions, reservation and exterior contact.
  // Here one bounded connectivity repair keeps that initial candidate's complete curved skin fixed.
  const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();
  std::cout<<"TRANSACTION_REPAIR_BEGIN"<<std::endl;
  FrozenField repair_field(std::move(source),{},false,components,{}, {},true,capture.donors.size());
  const auto initial=MeasureMetricCells(first->fresh.cells,repair_field);
  REQUIRE(initial.minimum_mean_ratio>0);REQUIRE(initial.minimum_scaled_jacobian>=ValiditySliverJacobian);
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  Patch repaired;ConnectivityRepairReport report;std::string reason;
  bool changed=RepairMetricConnectivity(first->fresh,repair_field,*capture.reference,{},first->first_cell,
      repaired,report,reason,64,false);
  size_t optimizer_trials=0;std::string method="connectivity";
  // A second bounded alternative starts from the same seed; no partial repair is published or chained.
  bool strict_shape=false;
  if(changed) {
    const auto m=MeasureMetricCells(repaired.cells,repair_field);
    strict_shape=m.minimum_mean_ratio>=MinimumMeanRatio && m.minimum_scaled_jacobian>=MinimumScaledJacobian;
  }
  if(!strict_shape && report.trials<256) {
    Owned cells;for(auto cell:first->fresh.cells){OwnedCell row;row.cell=cell;cells.emplace(cell.id,row);}
    OptimizeOptions options;options.maximum_work=256-report.trials;options.maximum_seconds=std::max(0.001,
        60-std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    options.boundary=false;options.collapses=false;options.smoothing=true;
    Id node=first->first_node+2,cell=first->first_cell+MaximumReplacementCells;
    OptimizeReport optimized;const bool improved=OptimizeMetricMesh(cells,RegionMetric(repair_field),node,cell,options,optimized);
    optimizer_trials=optimized.trials;
    if(improved && cells.size()<=MaximumReplacementCells && node-first->first_node<=162) {
      repaired=first->fresh;repaired.cells.clear();
      for(const auto& row:cells){auto c=row.second.cell;c.id=first->first_cell+repaired.cells.size();repaired.cells.push_back(c);}
      changed=true;method="interior_optimizer";
    }
  }
  SetShapeAdmission(ShapeAdmission::Absolute);
  bool interface=false,transaction=false,final_patch=false,contact=false;PatchMetric measured;
  if(changed) {
    interface=ValidateFixedInterface(first->fresh.cells,repaired.cells,reason,nullptr,MaximumReplacementCells);
    if(interface) {
      const auto before=MeasureMetricCells(first->old.cells,repair_field);
      measured=MeasureMetricCells(repaired.cells,repair_field);
      transaction=ValidateMetricChange(first->old.cells.size(),before,repaired.cells.size(),measured,MetricChange::GeometryRefine,reason);
      if(transaction)final_patch=ValidateMetricPatch(repaired.cells,repair_field,measured,reason);
    }
  }
  if(transaction) {
    Owned current;
    for(int rank=0;rank<6;++rank) {
      const auto bytes=TransactionRead(root+"/transaction_rank"+std::to_string(rank)+"_working.bin");RecordStream stream(bytes);
      for(auto c:TransactionVector<OwnedCell>(stream,1000000))REQUIRE(current.emplace(c.cell.id,std::move(c)).second);REQUIRE(stream.End());
    }
    Directory directory(world,false,capture.reference->Parameters());REQUIRE(directory.Build(current,reason,size_t(1)<<30));
    contact=true;REQUIRE(directory.Clearance(current,first->old,repaired,contact,reason,size_t(1)<<30));
  }
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  const double cpu_seconds=double(std::clock()-cpu)/CLOCKS_PER_SEC;
  REQUIRE(report.trials+optimizer_trials<=256);const bool capped=seconds>60 || cpu_seconds>60;
  std::ofstream out("repair_replay.json");out<<std::setprecision(17)<<"{\"changed\":"<<changed<<",\"trials\":"<<report.trials
    <<",\"optimizer_trials\":"<<optimizer_trials<<",\"method\":"<<std::quoted(method)
    <<",\"flips\":"<<report.flips<<",\"moves\":"<<report.moves<<",\"removals\":"<<report.removals
    <<",\"fixed_interface\":"<<interface<<",\"transaction_metric\":"<<transaction<<",\"final_patch_metric\":"<<final_patch
    <<",\"external_contact\":"<<contact<<",\"q\":"<<measured.minimum_mean_ratio<<",\"J\":"<<measured.minimum_scaled_jacobian
    <<",\"maximum_edge\":"<<double(measured.maximum_length)<<",\"seconds\":"<<seconds<<",\"cpu_seconds\":"<<cpu_seconds
    <<",\"time_cap_reached\":"<<capped<<",\"reason\":"<<std::quoted(reason)
    <<",\"accepted_local_repair\":"<<(transaction && interface && contact && !capped)
    <<",\"full_mesh_accepted\":false}\n";out.close();REQUIRE(bool(out));
}
