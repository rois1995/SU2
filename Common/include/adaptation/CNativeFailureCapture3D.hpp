/*!
 * \file CNativeFailureCapture3D.hpp
 * \brief Explicitly installed, bounded rank-local rejected-mesh inspection snapshot.
 */
#pragma once
#include "CNativeEngine3D.hpp"
#include "CNativeBoundaryParameters3D.hpp"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace SU2Native3D {
/*! No automatic installation: a selected diagnostic run supplies this through SetStageObserver.
 * Raw owned shards allow offline complete cross-rank stars without a runtime volume gather.
 * Geometry/metric values are inspected, never recomputed or used for admission here. */
class FailureCapture {
 public:
  static constexpr size_t FileLimit=size_t(128)<<20;
  static constexpr size_t ScratchBytes=size_t(1)<<20;
  FailureCapture(std::string prefix,int rank,int ranks,uint64_t event,
                 std::vector<std::string> marker_names={},size_t limit=FileLimit)
      :prefix(std::move(prefix)),rank(rank),ranks(ranks),event(event),marker_names(std::move(marker_names)),
       limit(std::min(limit,FileLimit)) {}
  void Observe(const StageProgressView& view)noexcept {
    if(saved || view.valid || !view.stage || std::string(view.stage)!="exit")return;
    saved=true;
    try {complete=Save(view);}
    catch(const std::exception& e){std::fprintf(stderr,"NATIVE3D_FAILURE_EXPORT rank=%d complete=0 reason=%s\n",rank,e.what());}
    catch(...){std::fprintf(stderr,"NATIVE3D_FAILURE_EXPORT rank=%d complete=0 reason=unknown_diagnostic_failure\n",rank);}
  }
  bool Complete()const{return complete;}
  std::string ManifestPath()const{return prefix+"_rank"+std::to_string(rank)+"_manifest.json";}

 private:
  struct File {
    FILE* file=nullptr;size_t bytes=0,limit=0;
    File(const std::string& path,size_t cap):limit(cap) {
      file=std::fopen(path.c_str(),"wbx");
      if(!file)throw std::runtime_error("Cannot exclusively create diagnostic file: "+path);
    }
    ~File(){if(file)std::fclose(file);}
    File(const File&)=delete;File& operator=(const File&)=delete;
    bool Fits(size_t count)const{return bytes<=limit && count<=limit-bytes;}
    bool Write(const char* data,size_t count) {
      if(!Fits(count))return false;
      if(std::fwrite(data,1,count,file)!=count)throw std::runtime_error("Diagnostic write failed.");
      bytes+=count;return true;
    }
    bool Write(const std::string& value){return Write(value.data(),value.size());}
    void Close(){if(file){const auto status=std::fclose(file);file=nullptr;if(status)throw std::runtime_error("Diagnostic close failed.");}}
  };
  std::string prefix;int rank,ranks;uint64_t event;std::vector<std::string> marker_names;
  size_t limit;bool saved=false,complete=false;
  static void Text(std::ostream& out,const std::string& value) {
    constexpr char hex[]="0123456789abcdef";out<<'"';
    for(unsigned char c:value) {
      if(c=='"'||c=='\\')out<<'\\'<<char(c);
      else if(c<32)out<<"\\u00"<<hex[c>>4]<<hex[c&15];
      else out<<char(c);
    }
    out<<'"';
  }
  static void Number(std::ostream& out,long double value,bool known=true) {
    if(known && std::isfinite(value))out<<value;else out<<"null";
  }
  static void PointRow(std::ostream& out,Point p){out<<'[';Number(out,p.x);out<<',';Number(out,p.y);out<<',';Number(out,p.z);out<<']';}
  static bool Bad(const CellDiagnostic& row) {
    return row.cache_valid&&(row.q_bad||row.j_bad||row.geometry_bad||row.size_bad||row.height_bad||row.height_unresolved);
  }
  // v2 compact JSON: id,version,flags,q,J,geometry,edges,repairs,bindings; no values are omitted.
  static std::string Metrics(const OwnedCell& owned,const CellDiagnostic& d,const PlanarReference& geometry) {
    const bool flags[]={d.cache_valid,d.q_measured,d.j_measured,d.geometry_measured,d.q_bad,d.j_bad,
                        d.geometry_bad,d.size_bad,d.height_bad,d.height_unresolved,d.surface,d.near_feature,d.feature_known};
    unsigned mask=0;for(unsigned i=0;i<13;++i)if(flags[i])mask|=1u<<i;
    std::ostringstream out;out<<std::setprecision(17)<<std::boolalpha;
    out<<'['<<owned.cell.id<<','<<owned.version<<','<<mask<<',';
    Number(out,d.q,d.cache_valid&&d.q_measured);out<<',';Number(out,d.J,d.cache_valid&&d.j_measured);
    out<<',';Number(out,d.geometry_excess_l2,d.cache_valid&&d.geometry_measured);out<<",[";
    size_t edge=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j,++edge) {
      if(edge)out<<',';
      const Edge endpoints=d.cache_valid?d.edges[edge]:Edge{owned.cell.v[i].id,owned.cell.v[j].id};
      out<<'['<<endpoints[0]<<','<<endpoints[1]<<',';Number(out,d.lengths[edge],d.cache_valid);out<<']';
    }
    out<<"],[";
    for(size_t i=0;i<d.repair_count;++i){if(i)out<<',';const auto& r=d.repairs[i];out<<"[[";
      for(unsigned j=0;j<r.feature.count;++j){if(j)out<<',';out<<r.feature.vertices[j];}
      out<<"],"<<r.blocked<<','<<r.epoch<<']';}
    out<<"],[";
    for(size_t i=0;i<4;++i) {
      if(i)out<<',';const auto& node=owned.cell.v[i];const auto* point=geometry.Parameter(node.p);
      if(!point){out<<"null";continue;}
      const auto& b=point->binding;unsigned dimension=b.dimension;
      if(geometry.Mobility() && geometry.Parameters())dimension=geometry.Parameters()->Geometry().MobilityDimension(b,*geometry.Mobility());
      out<<'['<<node.id<<','<<b.feature<<','<<b.version<<','<<b.dimension<<',';
      Number(out,b.u);out<<',';Number(out,b.v);out<<','<<dimension<<']';
    }
    out<<"]]\n";return out.str();
  }
  static std::string FacetRow(const Facet& facet,const PlanarReference& geometry) {
    std::ostringstream out;out<<std::setprecision(17)<<"{\"kind\":\"facet\",\"id\":"<<facet.id
      <<",\"marker\":"<<facet.marker<<",\"nodes\":[";
    for(unsigned i=0;i<3;++i){if(i)out<<',';const auto& n=facet.v[i];out<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<']';}
    out<<"],\"body\":";const auto* policy=geometry.Mobility();
    auto identity=[&](const auto* values){if(values){const auto p=values->find(facet.id);if(p!=values->end()){out<<p->second;return;}}out<<"null";};
    identity(policy?&policy->bodies:nullptr);out<<",\"sheet\":";identity(policy?&policy->sheets:nullptr);
    const auto* state=geometry.Parameters();const auto* authority=state?&state->Geometry():nullptr;
    out<<",\"mapping_error\":";
    if(authority){const auto found=authority->Errors().find(facet.id);Number(out,found==authority->Errors().end()?0:found->second);}
    else out<<'0';
    if(authority){const auto curve=authority->Curves().find(facet.id);if(curve!=authority->Curves().end()) {
      out<<",\"controls\":[";for(size_t i=0;i<curve->second.size();++i){if(i)out<<',';PointRow(out,curve->second[i]);}out<<']';}}
    const auto analytic=geometry.Analytic().find(facet.id);
    if(analytic!=geometry.Analytic().end()) {
      const auto& a=analytic->second;out<<",\"analytic\":{\"kind\":"<<unsigned(a.kind)<<",\"origin\":";PointRow(out,a.origin);
      out<<",\"extent\":";PointRow(out,a.extent);out<<",\"axis\":";PointRow(out,a.axis);out<<",\"tolerance\":";Number(out,a.tolerance);out<<'}';
    }
    const auto affine=geometry.Affine().find(facet.id);
    if(affine!=geometry.Affine().end()) {
      const auto& a=affine->second;out<<",\"affine\":{\"identity\":"<<a.identity<<",\"version\":"<<a.version<<",\"dimension\":"<<a.dimension<<",\"origin\":";
      PointRow(out,a.origin);out<<",\"normal\":";PointRow(out,a.normal);out<<",\"maximum_error\":";Number(out,a.maximum_error);out<<'}';
    }
    out<<"}\n";return out.str();
  }
  bool Save(const StageProgressView& view) {
    if(view.stage_observer_bytes<ScratchBytes)throw std::length_error("Failure capture scratch was not admitted.");
    uint16_t endian=1;if(*reinterpret_cast<unsigned char*>(&endian)!=1)throw std::runtime_error("Failure capture requires little-endian records.");
    RecordStream layout;OwnedCell dummy;layout(dummy);
    if(layout.Size()!=416)throw std::runtime_error("Unsupported failure owned-record layout.");
    const std::string stem=prefix+"_rank"+std::to_string(rank);
    const std::string owned_path=stem+"_owned.bin",metrics_path=stem+"_metrics.compact.jsonl",reference_path=stem+"_reference.jsonl",rejections_path=stem+"_rejections.jsonl";
    for(const auto& path:{ManifestPath(),owned_path,metrics_path,reference_path,rejections_path})
      if(std::ifstream(path).good())throw std::runtime_error("Refusing to overwrite failure capture: "+path);
    File manifest(ManifestPath(),FileLimit),owned(owned_path,limit),metrics(metrics_path,limit),reference(reference_path,limit),rejections(rejections_path,limit);
    uint64_t written=0;if(!owned.Write(reinterpret_cast<const char*>(&written),sizeof written))throw std::length_error("Failure capture cap is below its header.");
    std::vector<char> bytes;bytes.reserve(1024);bool capped=false,known=true;
    for(unsigned pass=0;pass<2&&!capped;++pass)for(const auto& item:view.working) {
      CellDiagnostic diagnostic;const bool cached=view.cell_diagnostic&&view.cell_diagnostic(item.first,diagnostic);
      if(!cached)diagnostic={};if(Bad(diagnostic)!=(pass==0))continue;
      auto row=item.second;bytes.clear();RecordStream encoded(bytes);encoded(row);
      const auto text=Metrics(row,diagnostic,view.geometry);
      if(text.size()>(size_t(64)<<10))throw std::length_error("Failure metric record exceeds streaming scratch.");
      if(!owned.Fits(bytes.size())||!metrics.Fits(text.size())){capped=true;break;}
      owned.Write(bytes.data(),bytes.size());metrics.Write(text);++written;
      known=known&&cached&&diagnostic.q_measured&&diagnostic.j_measured&&diagnostic.geometry_measured;
    }
    if(std::fseek(owned.file,0,SEEK_SET)||std::fwrite(&written,1,sizeof written,owned.file)!=sizeof written)
      throw std::runtime_error("Cannot finalize failure owned row count.");
    owned.Close();metrics.Close();bool reference_complete=true,rejections_complete=true;
    for(const auto& row:view.geometry.Facets())if(!reference.Write(FacetRow(row.second,view.geometry))){reference_complete=false;break;}
    const auto* policy=view.geometry.Mobility();
    if(policy&&reference_complete)for(const auto& row:policy->features) {
      std::ostringstream out;out<<"{\"kind\":\"feature\",\"edge\":["<<row.first[0]<<','<<row.first[1]<<"],\"feature\":"<<row.second<<"}\n";
      if(!reference.Write(out.str())){reference_complete=false;break;}
    }
    if(policy&&reference_complete)for(Id id:policy->corners)if(!reference.Write("{\"kind\":\"corner\",\"id\":"+std::to_string(id)+"}\n")){reference_complete=false;break;}
    reference.Close();
    if(view.rejections)for(const auto& row:*view.rejections) {
      std::ostringstream out;out<<std::setprecision(17)<<std::boolalpha<<"{\"sequence\":"<<row.sequence<<",\"round\":"<<row.round<<",\"feature\":[";
      for(unsigned i=0;i<row.feature.count;++i){if(i)out<<',';out<<row.feature.vertices[i];}
      out<<"],\"old_cells\":[";for(size_t i=0;i<row.old_count;++i){if(i)out<<',';out<<row.old_cells[i];}
      out<<"],\"old_cells_truncated\":"<<row.old_cells_truncated<<",\"reason_truncated\":"<<row.reason_truncated<<",\"reason\":";Text(out,row.reason.data());
      out<<",\"shape_measured\":"<<row.shape_measured<<",\"q\":";Number(out,row.q,row.shape_measured);out<<",\"J\":";Number(out,row.J,row.shape_measured);
      out<<",\"failure\":"<<unsigned(row.failure)<<",\"scope\":\"historical_not_final_state_reason\"}\n";
      if(!rejections.Write(out.str())){rejections_complete=false;break;}
    }
    rejections.Close();const bool volume_complete=!capped&&written==view.working.size();
    const bool all=volume_complete&&reference_complete&&rejections_complete;
    auto basename=[](const std::string& path){const auto slash=path.find_last_of("/\\");return slash==std::string::npos?path:path.substr(slash+1);};
    std::ostringstream out;out<<std::setprecision(17)<<std::boolalpha<<"{\"schema\":\"native_failure3d_v2\",\"rank\":"<<rank<<",\"ranks\":"<<ranks<<",\"event\":"<<event
      <<",\"metrics_encoding\":\"compact-array-v1\",\"private_diagnostic\":true,\"final_mesh_accepted\":false,\"complete_shard\":"<<all
      <<",\"owned_complete\":"<<volume_complete<<",\"metrics_complete\":"<<(volume_complete&&known)
      <<",\"reference_complete\":"<<reference_complete<<",\"rejections_complete\":"<<rejections_complete
      <<",\"working_rows\":"<<view.working.size()<<",\"written_rows\":"<<written<<",\"owned_record_bytes\":416,\"byte_order\":\"little\",\"cap_bytes\":"<<limit
      <<",\"composition_serialized\":false,\"edge_breakpoints_serialized\":false,\"reason\":";Text(out,view.reason);
    out<<",\"cache_consistent\":"<<view.progress.cache_consistent<<",\"census_complete\":"<<view.progress.census_complete<<",\"shape_complete\":"<<view.progress.shape_complete
       <<",\"geometry_complete\":"<<view.progress.geometry_complete<<",\"round\":"<<view.round
       <<",\"progress\":{\"owned\":"<<view.progress.owned_cells<<",\"measured\":"<<view.progress.measured_cells
       <<",\"q_bad\":"<<view.progress.q_bad_cells<<",\"j_bad\":"<<view.progress.j_bad_cells
       <<",\"geometry_bad\":"<<view.progress.geometry_bad_cells<<",\"size_bad\":"<<view.progress.size_bad_cells
       <<"},\"files\":{\"owned\":";Text(out,basename(owned_path));
    out<<",\"metrics\":";Text(out,basename(metrics_path));out<<",\"reference\":";Text(out,basename(reference_path));out<<",\"rejections\":";Text(out,basename(rejections_path));out<<'}';
    out<<",\"marker_names\":[";for(size_t i=0;i<marker_names.size();++i){if(i)out<<',';Text(out,marker_names[i]);}out<<']';
    out<<",\"geometry_excess_definition\":\"sqrt(sum(max(physical_face_error-surface_tolerance,0)^2))\",\"policies\":{\"surface_tolerance\":";Number(out,view.surface_tolerance);
    out<<",\"q_min\":"<<MinimumMeanRatio<<",\"j_min\":"<<MinimumScaledJacobian<<",\"max_edge\":"<<MaximumMetricEdge<<",\"height_tolerance\":";Number(out,view.wall_heights.tolerance);
    out<<",\"wall_heights\":[";bool comma=false;for(const auto& row:view.wall_heights){if(comma)out<<',';comma=true;out<<'['<<row.first<<',';Number(out,row.second);out<<']';}
    out<<"],\"region_walls\":[";comma=false;for(const auto& row:view.region_walls){if(comma)out<<',';comma=true;out<<'['<<row.first<<',';Number(out,row.second.height);out<<',';Number(out,row.second.growth);out<<',';Number(out,row.second.thickness);out<<']';}
    out<<"]}}\n";if(!manifest.Write(out.str()))throw std::length_error("Failure manifest exceeds its cap.");manifest.Close();
    std::fprintf(stdout,"NATIVE3D_FAILURE_EXPORT rank=%d complete=%d rows=%llu manifest=%s\n",rank,int(all),static_cast<unsigned long long>(written),ManifestPath().c_str());
    return all;
  }
};
}  // namespace SU2Native3D
