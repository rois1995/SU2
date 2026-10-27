#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
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
        <<",\"shape_measured\":"<<(e.metric.shape_measured?"true":"false")<<",\"q\":";
      if(e.metric.shape_measured)log<<e.metric.minimum_mean_ratio;else log<<"null";
      log<<",\"J\":";if(e.metric.shape_measured)log<<e.metric.minimum_scaled_jacobian;else log<<"null";
      log<<'}';
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
      <<"{\"shape_measured\":true,\"initial_split_exact\":"<<(valid==first.passed && PatchBytes(fresh)==PatchBytes(first.fresh))
      <<",\"first_failure_exact\":"<<(failure==metric->reason)<<",\"before_q\":"<<before.minimum_mean_ratio
      <<",\"before_J\":"<<before.minimum_scaled_jacobian<<",\"after_q\":"<<after.minimum_mean_ratio
      <<",\"after_J\":"<<after.minimum_scaled_jacobian<<",\"maximum_integrated_edge\":"<<double(after.maximum_length)
      <<",\"complete_sequence_replayed\":false,\"external_contact_certified\":false,\"accepted_repair\":false}\n";
    log.close();REQUIRE(bool(log));
  }
};

struct NativeStageCapture {
  static constexpr size_t limit=size_t(128)<<20;
  static constexpr size_t streaming_reserve=size_t(64)<<10;
  bool quality_saved=false,rejected_saved=false;
  size_t max_quality_windows=0,quality_windows=0;
  bool diagnostic_window_stop=false;

  static size_t ParseWindowLimit(const char* value) {
    if(!value || !*value)return 0;
    for(const auto* p=value;*p;++p)if(*p<'0'||*p>'9')throw std::invalid_argument("SU2_NATIVE_STAGE_CAPTURE_WINDOWS must be an unsigned integer.");
    size_t consumed=0;unsigned long parsed=0;
    try {parsed=std::stoul(value,&consumed);}
    catch(const std::exception&) {throw std::invalid_argument("SU2_NATIVE_STAGE_CAPTURE_WINDOWS must be between 1 and 64.");}
    if(consumed!=std::string(value).size() || parsed==0 || parsed>64)
      throw std::invalid_argument("SU2_NATIVE_STAGE_CAPTURE_WINDOWS must be between 1 and 64.");
    return static_cast<size_t>(parsed);
  }
  bool CountQualityWindow() {
    if(!max_quality_windows)return false;
    if(++quality_windows<max_quality_windows)return false;
    diagnostic_window_stop=true;return true;
  }

  template<class T> static void WriteRecord(std::ofstream& file,std::vector<char>& bytes,T value) {
    bytes.clear();RecordStream encoded(bytes);encoded(value);
    file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
    if(!file)throw std::runtime_error("Cannot stream native stage capture record.");
  }
  template<class Rows,class Project> static void WriteRows(const std::string& path,const Rows& rows,Project project) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);
    if(!file)throw std::runtime_error("Cannot open native stage capture shard.");
    std::vector<char> bytes;bytes.reserve(1024);WriteRecord(file,bytes,uint64_t(rows.size()));
    for(const auto& row:rows)WriteRecord(file,bytes,project(row));
    file.close();if(!file)throw std::runtime_error("Cannot close native stage capture shard.");
  }
  static void WriteOwned(const std::string& path,const Owned& rows) {
    WriteRows(path,rows,[](const auto& row){return row.second;});
  }
  static void WriteDonors(const std::string& path,const std::vector<DonorCell>& rows) {
    WriteRows(path,rows,[](const auto& row){return row;});
  }
  static void WriteBytes(const std::string& path,const std::vector<char>& bytes) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);
    if(!file)throw std::runtime_error("Cannot open native stage parameter payload.");
    file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
    file.close();if(!file)throw std::runtime_error("Cannot close native stage parameter payload.");
  }
  static void CheckPayloadBound(size_t payload_bytes,size_t reserved_bytes) {
    const size_t ceiling=std::min(limit,reserved_bytes);
    if(ceiling<streaming_reserve || payload_bytes>ceiling-streaming_reserve)
      throw std::length_error("Native stage boundary payload exceeds its admitted capture memory.");
  }
  static void Number(std::ostream& out,long double value) {
    if(std::isfinite(value))out<<value;else out<<"null";
  }
  static const char* Phase(EnginePhase phase) {return phase==EnginePhase::Construction?"construction":"improvement";}
  void Observe(const StageProgressView& view,int rank) {
    const std::string stage=view.stage?view.stage:"unknown";
    const bool stop_after_window=stage=="window"&&view.phase==EnginePhase::Improvement&&CountQualityWindow();
    const std::string reason=stop_after_window?"Native stage diagnostic observation window complete":view.reason;
    std::ofstream trace("stage_trace_rank"+std::to_string(rank)+".jsonl",std::ios::app);
    if(!trace)throw std::runtime_error("Cannot open native stage progress trace.");
    const auto& p=view.progress;const auto& s=view.stats;
    trace<<std::setprecision(17)<<"{\"rank\":"<<rank<<",\"stage\":"<<std::quoted(stage)<<",\"phase\":"
      <<std::quoted(Phase(view.phase))<<",\"round\":"<<view.round<<",\"valid\":"<<(view.valid?"true":"false")
      <<",\"complete\":"<<(view.complete?"true":"false")<<",\"reason\":"<<std::quoted(reason)<<",\"work\":{\"global\":"<<s.global_work
      <<",\"limit\":"<<s.work_limit<<",\"rounds\":"<<s.rounds<<",\"accepted\":"<<s.accepted
      <<",\"moved\":"<<s.moved<<",\"reconnected\":"<<s.reconnected<<",\"inserted\":"<<s.inserted
      <<",\"optimization_moves\":"<<s.optimization_moves<<",\"optimization_insertions\":"<<s.optimization_insertions
      <<",\"optimization_collapses\":"<<s.optimization_collapses<<",\"optimization_swaps\":"<<s.optimization_swaps
      <<",\"quality_progress\":"<<s.quality_progress<<",\"size_progress\":"<<s.size_progress
      <<",\"geometry_progress\":"<<s.geometry_progress<<",\"remaining_repairs\":"<<p.remaining_repairs
      <<"},\"elapsed_seconds\":";
    Number(trace,view.elapsed_seconds);
    trace<<",\"policies\":{\"surface_tolerance\":";
    Number(trace,view.surface_tolerance);
    trace<<",\"height_requested\":"<<(p.height_requested?"true":"false")<<",\"surface_error_requested\":"
      <<(p.surface_error_requested?"true":"false")<<",\"wall_height_count\":"<<view.wall_heights.size()
      <<",\"region_wall_count\":"<<view.region_walls.size()<<",\"composition_available\":"
      <<(bool(view.composition)?"true":"false")<<",\"edge_breakpoints_available\":"
      <<(bool(view.breakpoints)?"true":"false")<<"},\"progress\":{\"owned_cells\":"<<p.owned_cells
      <<",\"measured_cells\":"<<p.measured_cells<<",\"unmeasured_cells\":"<<p.unmeasured_cells
      <<",\"shape_measured_cells\":"<<p.shape_measured_cells<<",\"shape_unmeasured_cells\":"<<p.shape_unmeasured_cells
      <<",\"q_measured_cells\":"<<p.q_measured_cells<<",\"j_measured_cells\":"<<p.j_measured_cells
      <<",\"height_measured_cells\":"<<p.height_measured_cells<<",\"height_unmeasured_cells\":"<<p.height_unmeasured_cells
      <<",\"geometry_measured_cells\":"<<p.geometry_measured_cells<<",\"geometry_unmeasured_cells\":"<<p.geometry_unmeasured_cells
      <<",\"height_unresolved_cells\":"<<p.height_unresolved_cells
      <<",\"invalid_cells\":"<<p.invalid_cells<<",\"construction_invalid_cells\":"<<p.construction_invalid_cells
      <<",\"quality_invalid_cells\":"<<p.quality_invalid_cells<<",\"remaining_edges\":"<<p.remaining_edges
      <<",\"remaining_repairs\":"<<p.remaining_repairs
      <<",\"q_bad_cells\":"<<p.q_bad_cells<<",\"j_bad_cells\":"<<p.j_bad_cells<<",\"size_bad_cells\":"<<p.size_bad_cells
      <<",\"height_bad_cells\":"<<p.height_bad_cells<<",\"geometry_bad_cells\":"<<p.geometry_bad_cells
      <<",\"q_hinge_squared\":";
    Number(trace,p.q_hinge_squared);
    trace<<",\"j_hinge_squared\":";Number(trace,p.j_hinge_squared);
    trace<<",\"size_hinge_squared\":";Number(trace,p.size_hinge_squared);
    trace<<",\"height_hinge_squared\":";Number(trace,p.height_hinge_squared);
    trace<<",\"geometry_excess_squared\":";Number(trace,p.geometry_excess_squared);
    trace<<",\"cache_consistent\":"<<(p.cache_consistent?"true":"false")<<",\"census_complete\":"
      <<(p.census_complete?"true":"false")<<",\"shape_complete\":"
      <<(p.shape_complete?"true":"false")<<",\"height_complete\":"<<(p.height_complete?"true":"false")
      <<",\"geometry_complete\":"<<(p.geometry_complete?"true":"false")<<"}}\n";
    trace.close();if(!trace)throw std::runtime_error("Cannot close native stage progress trace.");
    if(stage=="quality_entry"&&!quality_saved){Save(view,rank,"quality_entry");quality_saved=true;}
    if(stage=="exit"&&!view.valid&&!rejected_saved){Save(view,rank,"rejected_exit");rejected_saved=true;}
    if(stop_after_window) {
      Save(view,rank,"observation_stop");
      throw std::runtime_error("Native stage diagnostic observation window complete");
    }
  }
  void Save(const StageProgressView& view,int rank,const std::string& label) {
    RecordStream owned_layout,donor_layout;OwnedCell owned_row;DonorCell donor_row;
    owned_layout(owned_row);donor_layout(donor_row);
    const std::string stem="stage_"+label+"_rank"+std::to_string(rank);
    const auto owned_path=stem+"_owned.bin",donors_path=stem+"_donors.bin",parameters_path=stem+"_parameters.bin";
    WriteOwned(owned_path,view.working);WriteDonors(donors_path,view.original);
    const auto* parameters=view.geometry.Parameters();size_t payload_bytes=0;bool has_parameters=parameters!=nullptr;
    if(parameters) {
      payload_bytes=parameters->PayloadBytes();
      CheckPayloadBound(payload_bytes,view.stage_observer_bytes);
      const auto bytes=parameters->Payload();
      if(bytes.size()!=payload_bytes)throw std::runtime_error("Native stage boundary payload size changed while capturing.");
      WriteBytes(parameters_path,bytes);
    }
    const auto manifest_path=stem+"_manifest.json",temporary_path=manifest_path+".tmp";
    std::ofstream out(temporary_path,std::ios::trunc);
    if(!out)throw std::runtime_error("Cannot open native stage shard manifest.");
    const auto& p=view.progress;const auto& s=view.stats;
    out<<std::setprecision(17)<<"{\"complete_shard\":true,\"private_capture\":true,\"rank\":"<<rank
       <<",\"stage\":"<<std::quoted(view.stage?view.stage:"unknown")<<",\"phase\":"<<std::quoted(Phase(view.phase))
       <<",\"round\":"<<view.round<<",\"source_valid\":"<<(view.valid?"true":"false")
       <<",\"reason\":"<<std::quoted(diagnostic_window_stop?"Native stage diagnostic observation window complete":view.reason)
       <<",\"diagnostic_complete\":"<<(view.complete?"true":"false")<<",\"final_mesh_accepted\":false"
       <<",\"diagnostic_observation_stop\":"<<(diagnostic_window_stop?"true":"false")
       <<",\"quality_windows_observed\":"<<quality_windows<<",\"quality_window_limit\":"<<max_quality_windows
       <<",\"exact_continuation\":false,\"composition_serialized\":false,\"edge_breakpoints_serialized\":false"
       <<",\"record_format\":1,\"owned_record_bytes\":"<<owned_layout.Size()<<",\"donor_record_bytes\":"<<donor_layout.Size()
       <<",\"working_rows\":"<<view.working.size()<<",\"donor_rows\":"<<view.original.size()
       <<",\"parameters_present\":"<<(has_parameters?"true":"false")<<",\"parameters_payload_bytes\":"<<payload_bytes
       <<",\"files\":{\"owned\":"<<std::quoted(owned_path)<<",\"donors\":"<<std::quoted(donors_path)
       <<",\"parameters\":";
    if(has_parameters)out<<std::quoted(parameters_path);else out<<"null";
    out<<"},\"progress\":{\"owned_cells\":"<<p.owned_cells<<",\"measured_cells\":"<<p.measured_cells
       <<",\"unmeasured_cells\":"<<p.unmeasured_cells<<",\"shape_measured_cells\":"<<p.shape_measured_cells
       <<",\"invalid_cells\":"<<p.invalid_cells<<",\"construction_invalid_cells\":"<<p.construction_invalid_cells
       <<",\"quality_invalid_cells\":"<<p.quality_invalid_cells<<",\"remaining_edges\":"<<p.remaining_edges
       <<",\"remaining_repairs\":"<<p.remaining_repairs
       <<",\"shape_unmeasured_cells\":"<<p.shape_unmeasured_cells<<",\"q_measured_cells\":"<<p.q_measured_cells
       <<",\"j_measured_cells\":"<<p.j_measured_cells<<",\"height_measured_cells\":"<<p.height_measured_cells
       <<",\"height_unmeasured_cells\":"<<p.height_unmeasured_cells<<",\"geometry_measured_cells\":"<<p.geometry_measured_cells
       <<",\"geometry_unmeasured_cells\":"<<p.geometry_unmeasured_cells<<",\"height_unresolved_cells\":"<<p.height_unresolved_cells
       <<",\"q_bad_cells\":"<<p.q_bad_cells<<",\"j_bad_cells\":"<<p.j_bad_cells<<",\"size_bad_cells\":"<<p.size_bad_cells
       <<",\"height_bad_cells\":"<<p.height_bad_cells<<",\"geometry_bad_cells\":"<<p.geometry_bad_cells
       <<",\"q_hinge_squared\":";Number(out,p.q_hinge_squared);
    out<<",\"j_hinge_squared\":";Number(out,p.j_hinge_squared);
    out<<",\"size_hinge_squared\":";Number(out,p.size_hinge_squared);
    out<<",\"height_hinge_squared\":";Number(out,p.height_hinge_squared);
    out<<",\"geometry_excess_squared\":";Number(out,p.geometry_excess_squared);
    out<<",\"height_requested\":"<<(p.height_requested?"true":"false")<<",\"surface_error_requested\":"
       <<(p.surface_error_requested?"true":"false")<<",\"census_complete\":"<<(p.census_complete?"true":"false")
       <<",\"shape_complete\":"<<(p.shape_complete?"true":"false")<<",\"height_complete\":"
       <<(p.height_complete?"true":"false")<<",\"geometry_complete\":"<<(p.geometry_complete?"true":"false")
       <<",\"cache_consistent\":"<<(p.cache_consistent?"true":"false")<<'}'
       <<",\"work\":{\"stop\":"<<std::quoted(EngineStopName(s.stop))<<",\"global\":"<<s.global_work
       <<",\"limit\":"<<s.work_limit<<",\"rounds\":"<<s.rounds<<",\"accepted\":"<<s.accepted
       <<",\"rejected\":"<<s.rejected<<",\"moved\":"<<s.moved<<",\"reconnected\":"<<s.reconnected
       <<",\"inserted\":"<<s.inserted<<",\"quality_progress\":"<<s.quality_progress
       <<",\"optimization_moves\":"<<s.optimization_moves<<",\"optimization_insertions\":"<<s.optimization_insertions
       <<",\"optimization_collapses\":"<<s.optimization_collapses<<",\"optimization_swaps\":"<<s.optimization_swaps
       <<",\"size_progress\":"<<s.size_progress<<",\"geometry_progress\":"<<s.geometry_progress<<'}'
       <<",\"policies\":{\"surface_tolerance\":";Number(out,view.surface_tolerance);
    out<<",\"maximum_seconds\":";Number(out,view.maximum_seconds);
    out<<",\"elapsed_seconds\":";Number(out,view.elapsed_seconds);
    out<<",\"maximum_rounds\":"<<view.maximum_rounds<<",\"maximum_work\":"<<view.maximum_work
       <<",\"memory_ceiling\":"<<view.memory_ceiling<<",\"caller_bytes\":"<<view.caller_bytes
       <<",\"stage_observer_bytes\":"<<view.stage_observer_bytes<<",\"wall_height_tolerance\":";
    Number(out,view.wall_heights.tolerance);
    out<<",\"wall_heights\":[";
    bool comma=false;for(const auto& row:view.wall_heights){if(comma)out<<',';comma=true;out<<'['<<row.first<<',';Number(out,row.second);out<<']';}
    out<<"],\"region_walls\":[";comma=false;
    for(const auto& row:view.region_walls){if(comma)out<<',';comma=true;out<<'['<<row.first<<',';Number(out,row.second.height);
      out<<',';Number(out,row.second.growth);out<<',';Number(out,row.second.thickness);out<<']';}
    out<<"],\"composition_serialized\":false,\"edge_breakpoints_serialized\":false}}\n";
    out.close();if(!out)throw std::runtime_error("Cannot close native stage shard manifest.");
    if(std::rename(temporary_path.c_str(),manifest_path.c_str())!=0)
      throw std::runtime_error("Cannot publish completed native stage shard manifest.");
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

TEST_CASE("Native stage capture streams complete owned and original donor rows", "[NativeStageCapture3D]") {
  const auto rank=CPassiveComm::Rank();
  const auto owned_path="native_stage_roundtrip_owned_rank"+std::to_string(rank)+".bin";
  const auto donor_path="native_stage_roundtrip_donors_rank"+std::to_string(rank)+".bin";
  const auto empty_owned_path="native_stage_roundtrip_empty_owned_rank"+std::to_string(rank)+".bin";
  const auto empty_donor_path="native_stage_roundtrip_empty_donors_rank"+std::to_string(rank)+".bin";
  Owned owned;OwnedCell row;row.cell.id=71;row.version=9;row.physical[2]={true,53};
  row.cell.v={Node{1,{.1,.2,.3}},Node{2,{1.1,.2,.3}},Node{3,{.1,1.2,.3}},Node{4,{.1,.2,1.3}}};
  REQUIRE(owned.emplace(row.cell.id,row).second);
  DonorCell donor;donor.cell=row.cell;donor.sensor={Tensor{1,2,3,4,5,6},Tensor{7,8,9,10,11,12},
      Tensor{13,14,15,16,17,18},Tensor{19,20,21,22,23,24}};donor.extension_component=61;donor.parent_cell=62;
  const std::vector<DonorCell> donors{donor};
  NativeStageCapture::WriteOwned(owned_path,owned);NativeStageCapture::WriteDonors(donor_path,donors);
  const auto owned_bytes=TransactionRead(owned_path);RecordStream owned_stream(owned_bytes);
  const auto decoded_owned=TransactionVector<OwnedCell>(owned_stream,4);
  REQUIRE(owned_stream.End());REQUIRE(decoded_owned.size()==1);
  CHECK(decoded_owned[0].cell.id==71);CHECK(decoded_owned[0].version==9);
  CHECK(decoded_owned[0].physical[2].present);CHECK(decoded_owned[0].physical[2].facet==53);
  CHECK(decoded_owned[0].cell.v[3].p.z==1.3);
  const auto donor_bytes=TransactionRead(donor_path);RecordStream donor_stream(donor_bytes);
  const auto decoded_donors=TransactionVector<DonorCell>(donor_stream,4);
  REQUIRE(donor_stream.End());REQUIRE(decoded_donors.size()==1);
  CHECK(decoded_donors[0].cell.id==71);CHECK(decoded_donors[0].extension_component==61);
  CHECK(decoded_donors[0].parent_cell==62);CHECK(decoded_donors[0].sensor[3].zz==24);
  const Owned empty_owned;const std::vector<DonorCell> empty_donors;
  NativeStageCapture::WriteOwned(empty_owned_path,empty_owned);NativeStageCapture::WriteDonors(empty_donor_path,empty_donors);
  const auto empty_owned_bytes=TransactionRead(empty_owned_path);RecordStream empty_owned_stream(empty_owned_bytes);
  CHECK(TransactionVector<OwnedCell>(empty_owned_stream,4).empty());CHECK(empty_owned_stream.End());
  const auto empty_donor_bytes=TransactionRead(empty_donor_path);RecordStream empty_donor_stream(empty_donor_bytes);
  CHECK(TransactionVector<DonorCell>(empty_donor_stream,4).empty());CHECK(empty_donor_stream.End());
  REQUIRE_NOTHROW(NativeStageCapture::CheckPayloadBound(0,NativeStageCapture::streaming_reserve));
  CHECK_THROWS_AS(NativeStageCapture::CheckPayloadBound(1,NativeStageCapture::streaming_reserve),std::length_error);
  std::remove(owned_path.c_str());std::remove(donor_path.c_str());
  std::remove(empty_owned_path.c_str());std::remove(empty_donor_path.c_str());
}

TEST_CASE("Native stage capture window stop is bounded and opt-in", "[NativeStageCapture3D]") {
  CHECK(NativeStageCapture::ParseWindowLimit(nullptr)==0);
  CHECK(NativeStageCapture::ParseWindowLimit("")==0);
  CHECK(NativeStageCapture::ParseWindowLimit("4")==4);
  CHECK_THROWS_AS(NativeStageCapture::ParseWindowLimit("0"),std::invalid_argument);
  CHECK_THROWS_AS(NativeStageCapture::ParseWindowLimit("65"),std::invalid_argument);
  CHECK_THROWS_AS(NativeStageCapture::ParseWindowLimit("4x"),std::invalid_argument);
  NativeStageCapture capture;CHECK_FALSE(capture.CountQualityWindow());CHECK(capture.quality_windows==0);
  capture.max_quality_windows=2;
  CHECK_FALSE(capture.CountQualityWindow());CHECK(capture.quality_windows==1);CHECK_FALSE(capture.diagnostic_window_stop);
  CHECK(capture.CountQualityWindow());CHECK(capture.quality_windows==2);CHECK(capture.diagnostic_window_stop);
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
      e.metric.shape_measured=std::isfinite(e.metric.minimum_mean_ratio)&&std::isfinite(e.metric.minimum_scaled_jacobian);
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
  const bool boundary_optimizer=std::getenv("SU2_NATIVE_TRANSACTION_BOUNDARY_OPTIMIZER");
  size_t constructions=0,metrics=0,reconnects=0;bool exact=true,placement_set=false;Tensor placement;
  const NativeTransactionCapture::Event* first=nullptr;
  if(!boundary_optimizer)for(size_t i=0;i<capture.events.size();++i) {
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
  std::ofstream strict("strict_replay.json");
  if(boundary_optimizer)strict<<"{\"construction_calls\":0,\"shape_failures\":0,\"calls_and_failure_reasons_exact\":null,\"historical_construction_replay_skipped\":true,\"growth_selection_policy_replayed\":false,\"MPI_peer_schedule_replayed\":false}\n";
  else strict<<"{\"construction_calls\":"<<constructions<<",\"shape_failures\":"<<metrics
    <<",\"calls_and_failure_reasons_exact\":"<<exact<<",\"historical_construction_replay_skipped\":false,\"growth_selection_policy_replayed\":false,\"MPI_peer_schedule_replayed\":false}\n";
  strict.close();REQUIRE(bool(strict));if(!boundary_optimizer){REQUIRE(exact);REQUIRE(first);REQUIRE(constructions==26);}
  size_t seed_event=1;
  if(const auto* seed=std::getenv("SU2_NATIVE_TRANSACTION_SEED_EVENT")) {
    seed_event=std::stoul(seed);REQUIRE(seed_event<capture.events.size());
  }
  if(boundary_optimizer)REQUIRE(seed_event==1);
  if(boundary_optimizer || std::getenv("SU2_NATIVE_TRANSACTION_SEED_EVENT")) {
    REQUIRE(seed_event<capture.events.size());first=&capture.events[seed_event];
  }
  if(boundary_optimizer || std::getenv("SU2_NATIVE_TRANSACTION_SEED_EVENT")) {
    REQUIRE(first);REQUIRE(first->passed);
    REQUIRE((first->stage=="SplitEdge" || first->stage=="ReconstructSplitPatch"));
  }
  std::set<Id> boundary_vertices,all_vertices;
  for(const auto& f:Incidence(first->fresh.cells).Boundary())boundary_vertices.insert(f.begin(),f.end());
  for(const auto& c:first->fresh.cells)for(auto n:c.v)all_vertices.insert(n.id);
  const auto interior_vertices=all_vertices.size()-boundary_vertices.size();
  // The live capture certifies the old footprint; every changed proposal still needs its own terminal checks.
  const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();
  std::cout<<"TRANSACTION_REPAIR_BEGIN"<<std::endl;
  FrozenField repair_field(std::move(source),{},false,components,{}, {},true,capture.donors.size());
  const auto initial=MeasureMetricCells(first->fresh.cells,repair_field);
  REQUIRE(initial.minimum_mean_ratio>0);REQUIRE(initial.minimum_scaled_jacobian>=ValiditySliverJacobian);
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  Patch repaired;ConnectivityRepairReport report;std::string reason;
  const auto cell_faces=[](const Cell& c)->std::array<Face,4> {
    const auto& v=c.v;return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
        {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
  };
  const bool parameter_search=std::getenv("SU2_NATIVE_TRANSACTION_PARAMETER_SEARCH");
  const bool surface_placement=parameter_search || std::getenv("SU2_NATIVE_TRANSACTION_SURFACE_PLACEMENT");
  JointSplitReport placement_report;bool changed=false;std::string method="connectivity";
  if(parameter_search) {
    REQUIRE(seed_event==1);std::map<Id,Point> points;
    for(const auto& c:first->old.cells)for(auto n:c.v)points.emplace(n.id,n.p);
    const auto a=points.at(edge[0]),b=points.at(edge[1]);double best=-1;
    const auto before_geometry=TransactionSurfaceExcess(first->old,*capture.reference,tolerance);
    std::ofstream scan("parameter_scan.json");scan<<std::setprecision(17)<<"[";
    for(unsigned step=1;step<16;++step) {
      const long double t=step/16.L;Node point{first->first_node,{double(a.x+t*(b.x-(long double)a.x)),
          double(a.y+t*(b.y-(long double)a.y)),double(a.z+t*(b.z-(long double)a.z))}};
      Patch trial;std::string why;++placement_report.trials;
      const bool built=SplitEdge(first->old,edge,point,first->first_cell,*capture.reference,trial,why);
      if(step>1)scan<<",";scan<<"{\"t\":"<<double(t)<<",\"built\":"<<built;
      if(built) {
        const auto m=MeasureMetricCells(trial.cells,repair_field);
        const auto geometry=TransactionSurfaceExcess(trial,*capture.reference,tolerance);
        const bool progress=before_geometry>0 && geometry<before_geometry*(1-1e-12L);
        const double score=std::min(m.minimum_mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian);
        scan<<",\"shape_measured\":true,\"q\":"<<m.minimum_mean_ratio<<",\"J\":"<<m.minimum_scaled_jacobian
          <<",\"maximum_edge\":"<<double(m.maximum_length)<<",\"geometry_progress\":"<<progress;
        if(progress && score>best){best=score;repaired=std::move(trial);changed=true;}
      }else {++placement_report.geometry_rejected;scan<<",\"shape_measured\":false,\"q\":null,\"J\":null,\"reason\":"<<std::quoted(why);}
      scan<<"}";
    }
    scan<<"]\n";scan.close();REQUIRE(bool(scan));method="edge_parameter_search";
  } else if(surface_placement) {
    REQUIRE(seed_event==1);
    PlaceSplitPatch(first->old,edge,first->point,{},first->first_cell,repair_field,*capture.reference,{},
        repaired,placement_report,reason,256);
    // The existing placer uses size-refinement progress internally; assess its best valid proposal for geometry below.
    if(!placement_report.best.cells.empty()){repaired=placement_report.best;changed=true;}
    method="surface_placement";
  } else if(!boundary_optimizer) changed=RepairMetricConnectivity(first->fresh,repair_field,*capture.reference,{},first->first_cell,
      repaired,report,reason,64,false);
  size_t optimizer_trials=0;OptimizeReport optimized;
  // A second bounded alternative starts from the same seed; no partial repair is published or chained.
  bool strict_shape=false;
  if(changed) {
    const auto m=MeasureMetricCells(repaired.cells,repair_field);
    strict_shape=m.minimum_mean_ratio>=MinimumMeanRatio && m.minimum_scaled_jacobian>=MinimumScaledJacobian;
  }
  if(!surface_placement && !strict_shape && report.trials<256) {
    Owned cells;std::map<Face,Id> physical;
    for(auto f:first->fresh.surface){std::sort(f.v.begin(),f.v.end());physical.emplace(f.v,f.facet);}
    for(auto cell:first->fresh.cells){OwnedCell row;row.cell=cell;const auto faces=cell_faces(cell);
      for(unsigned k=0;k<4;++k){auto f=faces[k];std::sort(f.begin(),f.end());const auto found=physical.find(f);
        if(found!=physical.end())row.physical[k]={true,found->second};}cells.emplace(cell.id,row);}
    auto parameters=capture.reference->PrivateParameters(first->fresh);
    OptimizeOptions options;options.maximum_work=256-report.trials;options.maximum_seconds=std::max(0.001,
        60-std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    options.boundary=false;options.collapses=false;options.smoothing=true;
    if(boundary_optimizer){options.geometry=parameters?parameters.get():capture.reference.get();options.surface_tolerance=tolerance;}
    Id node=first->first_node+2,cell=first->first_cell+MaximumReplacementCells;
    const bool improved=OptimizeMetricMesh(cells,RegionMetric(repair_field),node,cell,options,optimized);
    optimizer_trials=optimized.trials;
    if(improved && cells.size()<=MaximumReplacementCells && node-first->first_node<=162) {
      repaired=first->fresh;repaired.cells.clear();repaired.surface.clear();
      for(const auto& row:optimized.parameters)repaired.parameters.insert_or_assign(row.first,row.second);
      for(const auto& row:cells){auto c=row.second.cell;c.id=first->first_cell+repaired.cells.size();repaired.cells.push_back(c);
        const auto faces=cell_faces(c);for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)repaired.surface.push_back({faces[k],row.second.physical[k].facet});}
      changed=true;method=boundary_optimizer?"boundary_optimizer":"interior_optimizer";
    }
  }
  SetShapeAdmission(ShapeAdmission::Absolute);
  bool interface=false,geometry_progress=false,transaction=false,final_patch=false,contact=false;PatchMetric measured;
  if(changed) {
    NativeTransactionCapture::Write("repair_candidate.bin",NativeTransactionCapture::PatchBytes(repaired));
    if(surface_placement || boundary_optimizer) {
      std::set<Face> physical;
      for(auto f:first->old.surface){std::sort(f.v.begin(),f.v.end());physical.insert(f.v);}
      std::map<Face,Face> expected_by_key;
      for(auto f:Incidence(first->old.cells).Boundary()) {
        auto key=f;std::sort(key.begin(),key.end());if(!physical.count(key))expected_by_key.emplace(key,f);
      }
      for(auto f:repaired.surface) {
        std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());
        auto key=f.v;std::sort(key.begin(),key.end());expected_by_key.emplace(key,f.v);
      }
      std::vector<Face> expected;for(const auto& face:expected_by_key)expected.push_back(face.second);
      interface=ValidatePrescribedInterface(first->old.cells,repaired.cells,expected,reason,nullptr,MaximumReplacementCells);
    } else interface=ValidateFixedInterface(first->fresh.cells,repaired.cells,reason,nullptr,MaximumReplacementCells);
    const auto before_geometry=TransactionSurfaceExcess(first->old,*capture.reference,tolerance);
    geometry_progress=before_geometry>0 && TransactionSurfaceExcess(repaired,*capture.reference,tolerance)<before_geometry*(1-1e-12L);
    if(interface) {
      const auto before=MeasureMetricCells(first->old.cells,repair_field);
      measured=MeasureMetricCells(repaired.cells,repair_field);
      transaction=ValidateMetricChange(first->old.cells.size(),before,repaired.cells.size(),measured,MetricChange::GeometryRefine,reason);
      if(transaction)final_patch=ValidateMetricPatch(repaired.cells,repair_field,measured,reason);
    }
  }
  if(transaction && geometry_progress) {
    Owned current;
    for(int rank=0;rank<6;++rank) {
      const auto bytes=TransactionRead(root+"/transaction_rank"+std::to_string(rank)+"_working.bin");RecordStream stream(bytes);
      for(auto c:TransactionVector<OwnedCell>(stream,1000000))REQUIRE(current.emplace(c.cell.id,std::move(c)).second);REQUIRE(stream.End());
    }
    Directory directory(world,false,capture.reference->Parameters());REQUIRE(directory.Build(current,reason,size_t(12)<<30));
    contact=true;REQUIRE(directory.Clearance(current,first->old,repaired,contact,reason,size_t(12)<<30));
  }
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  const double cpu_seconds=double(std::clock()-cpu)/CLOCKS_PER_SEC;
  REQUIRE(report.trials+optimizer_trials+placement_report.trials<=256);const bool capped=seconds>60 || cpu_seconds>60;
  std::ofstream out("repair_replay.json");out<<std::setprecision(17)<<"{\"seed_event\":"<<seed_event<<",\"seed_interior_vertices\":"<<interior_vertices<<",\"changed\":"<<changed<<",\"trials\":"<<report.trials
    <<",\"optimizer_trials\":"<<optimizer_trials<<",\"placement_trials\":"<<placement_report.trials
    <<",\"placement_geometry_rejected\":"<<placement_report.geometry_rejected<<",\"optimizer_moves\":"<<optimized.moves
    <<",\"optimizer_stop\":"<<std::quoted(optimized.stop)<<",\"method\":"<<std::quoted(method)
    <<",\"flips\":"<<report.flips<<",\"moves\":"<<report.moves<<",\"removals\":"<<report.removals
    <<",\"prescribed_interface\":"<<interface<<",\"geometry_progress\":"<<geometry_progress<<",\"transaction_metric\":"<<transaction<<",\"final_patch_metric\":"<<final_patch
    <<",\"external_contact\":"<<contact<<",\"shape_measured\":"<<(changed && interface?"true":"false")
    <<",\"candidate_measured\":"<<(changed && interface)<<",\"q\":";
  if(changed && interface)out<<measured.minimum_mean_ratio;else out<<"null";
  out<<",\"J\":";if(changed && interface)out<<measured.minimum_scaled_jacobian;else out<<"null";
  out<<",\"maximum_edge\":";if(changed && interface)out<<double(measured.maximum_length);else out<<"null";
  out<<",\"seconds\":"<<seconds<<",\"cpu_seconds\":"<<cpu_seconds
    <<",\"time_cap_reached\":"<<capped<<",\"reason\":"<<std::quoted(reason)
    <<",\"accepted_local_repair\":"<<(transaction && interface && geometry_progress && contact && !capped)
    <<",\"seed_geometry_excess\":"<<double(TransactionSurfaceExcess(first->fresh,*capture.reference,tolerance))
    <<",\"candidate_geometry_excess\":";
  if(changed)out<<double(TransactionSurfaceExcess(repaired,*capture.reference,tolerance));else out<<"null";
  out<<",\"final_geometry\":"<<(changed && TransactionSurfaceExcess(repaired,*capture.reference,tolerance)==0)
    <<",\"full_mesh_accepted\":false}\n";out.close();REQUIRE(bool(out));
  if(boundary_optimizer) {
    REQUIRE(changed);CHECK(optimized.moves>0);CHECK(interface);CHECK(transaction);CHECK(final_patch);CHECK(contact);CHECK_FALSE(capped);
    // This is a provisional quality stage: remaining curvature demands must return to surface refinement.
    CHECK(TransactionSurfaceExcess(first->fresh,*capture.reference,tolerance)>0);
    CHECK(TransactionSurfaceExcess(repaired,*capture.reference,tolerance)>TransactionSurfaceExcess(first->fresh,*capture.reference,tolerance));
    CHECK(optimized.relabel.count(first->first_node)==1);
  }
}
