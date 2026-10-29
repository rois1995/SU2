// Geometry-only proposal on a final private capture, not a replay of an earlier transaction.
#include "../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
using namespace SU2Native3D;
using namespace SU2NativeGeometry;
static std::vector<char> Read(const char* path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open input");
  const auto size=in.tellg();if(size<0 || size>std::streamoff(size_t(128)<<20))throw std::length_error("Input exceeds capture ceiling");
  in.seekg(0);std::vector<char> out(size_t(size),0);if(!in.read(out.data(),size))throw std::runtime_error("Cannot read input");return out;
}
int main(int argc,char** argv) try {
  if(argc!=9 && argc!=10)throw std::invalid_argument("usage: replay authority payload owned idA idB max_work tolerance scratch_bytes [split|collapse]; collapse removes idA into idB");
  const std::string operation=argc==10?argv[9]:"split";if(operation!="split" && operation!="collapse")throw std::invalid_argument("Invalid operation");
  const auto number=[](const char* p){size_t at=0;if(!*p || *p=='-')throw std::invalid_argument("Invalid integer");const auto n=std::stoull(p,&at);if(p[at] || n>SIZE_MAX)throw std::invalid_argument("Invalid integer");return n;};
  Edge edge{number(argv[4]),number(argv[5])};if(edge[0]==edge[1])throw std::invalid_argument("Degenerate edge");const Id removed=edge[0],retained=edge[1];if(operation=="split")std::sort(edge.begin(),edge.end());
  const auto refbytes=Read(argv[1]);auto archive=DecodeReference(std::string(refbytes.begin(),refbytes.end()),ReferenceStorageLimit3D);
  if(!archive.original || !archive.original->Parameters())throw std::runtime_error("Authority lacks parameters");
  const auto payload=Read(argv[2]);auto state=BoundaryParameterState::FromPayload(archive.original->Parameters()->Authority(),payload,ReferenceStorageLimit3D,0);
  {
    AtlasBudget probe{3,size_t(1)<<20,4096};if(!probe.ChargeWork(2) || probe.ChargeWork(2) || !probe.work_exhausted || probe.work!=2 || probe.ChargeWork(0))
      throw std::runtime_error("Sticky work denial self-check failed.");
    AtlasBudget ample{size_t(1)<<20,size_t(1)<<24,4096};ExactRatio one,zero,out;std::string why;
    if(!ExactInteger(1,one,ample,why) || !ExactInteger(0,zero,ample,why) || !ExactInteger(7,out,ample,why))throw std::runtime_error("Exact setup failed.");
    const auto saved=out;if(ExactDivide(one,zero,out,ample,why) || ample.work_exhausted || out.numerator!=saved.numerator || out.denominator!=saved.denominator || out.negative!=saved.negative)
      throw std::runtime_error("Invalid arithmetic self-check failed.");
    NativeMappedGeometryBackend3D backend(state->Authority(),state->Mobility());GeometryProjectionScope scope;GeometryProjection result;
    result.binding.feature=12345;const auto previous=result.binding;std::string reason;
    if(backend.Project({std::numeric_limits<double>::quiet_NaN(),0,0},scope,result,ample,reason) || ample.work_exhausted || ample.work>=ample.maximum_work || result.binding.feature!=previous.feature)
      throw std::runtime_error("Invalid projection self-check failed.");
    std::cout<<"budget_self_checks=pass\n";
  }
  auto reference=PlanarReference::WithParameters(archive.original,state);
  const auto bytes=Read(argv[3]);RecordStream input(bytes);uint64_t count=0;input(count);
  if(count>4096)throw std::length_error("Capture exceeds bounded replay");
  Patch old;Id maxcell=0,maxnode=0;bool have_a=false,have_b=false;Point a{},b{};std::set<Id> ids;
  for(uint64_t i=0;i<count;++i){OwnedCell row;input(row);if(!ids.insert(row.cell.id).second)throw std::runtime_error("Duplicate cell");
    maxcell=std::max(maxcell,row.cell.id);bool first=false,second=false;
    for(const auto& n:row.cell.v){maxnode=std::max(maxnode,n.id);if(n.id==edge[0]){first=true;a=n.p;have_a=true;}if(n.id==edge[1]){second=true;b=n.p;have_b=true;}}
    if(!first || (operation=="split" && !second))continue;old.cells.push_back(row.cell);
    const auto& v=row.cell.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned k=0;k<4;++k)if(row.physical[k].present)old.surface.push_back({faces[k],row.physical[k].facet});
  }
  if(!input.End()||!have_a||!have_b||old.cells.empty())throw std::runtime_error("Invalid or missing captured edge star");
  if(maxnode==UINT64_MAX || maxcell>UINT64_MAX-4096)throw std::overflow_error("Fresh identity unavailable");
  size_t end=0;const double tolerance=std::stod(argv[7],&end);if(argv[7][end] || !std::isfinite(tolerance) || tolerance<=0)throw std::invalid_argument("Invalid tolerance");
  const size_t scratch=number(argv[8]);if(!scratch || scratch>SIZE_MAX/8)throw std::invalid_argument("Invalid scratch");
  size_t low=0,high=scratch*8;while(low<high){const size_t bits=low+(high-low+1)/2;
    if(transfer_memory::Mul(2,CurvedAtlasWorkspaceBytes(bits))<=scratch/2)low=bits;else high=bits-1;}
  if(!low)throw std::length_error("No admitted integer workspace");
  const size_t caller=transfer_memory::Add(transfer_memory::Bytes(refbytes),transfer_memory::Bytes(payload),transfer_memory::Bytes(bytes),
      state->ResidentBytes(),state->Geometry().ResidentBytes(),transfer_memory::Bytes(old.cells),transfer_memory::Bytes(old.surface),
      BoundaryCoverageBytes(old.coverage));
  const size_t ceiling=size_t(128)<<20;if(ceiling==SIZE_MAX)throw std::length_error("Admission overflow");
  AtlasBudget budget{size_t(number(argv[6])),ceiling,low,0,caller,0};
  std::cout<<"scratch_bytes="<<scratch<<" caller_bytes="<<caller<<" integer_bits="<<low<<'\n';
  auto scoped=reference->WithAtlasReservation(budget);scoped->ExportParameters(old);
  const Node midpoint{maxnode+1,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
  auto quality=[](const char* label,const Patch& patch){
    double minq=INFINITY,minj=INFINITY,minscore=INFINITY,maxedge=0;const Tensor tensor{4,0,0,4,0,4};
    for(const auto& cell:patch.cells){const auto& v=cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
      minq=std::min(minq,m.mean_ratio);minj=std::min(minj,m.minimum_scaled_jacobian);
      minscore=std::min(minscore,std::min(m.mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian));maxedge=std::max(maxedge,m.maximum_edge);
      std::cout<<"quality="<<label<<" cell="<<cell.id<<" q="<<m.mean_ratio<<" J="<<m.minimum_scaled_jacobian<<" maxedge="<<m.maximum_edge<<'\n';}
    std::cout<<"quality_summary="<<label<<" cells="<<patch.cells.size()<<" minq="<<minq<<" minJ="<<minj<<" minscore="<<minscore<<" maxedge="<<maxedge<<'\n';};
  std::cout<<std::setprecision(17)<<"geometry_entity="<<state->EntityGeometry()<<" geometry_version="<<state->Geometry().Version()<<" geometry_tolerance="<<tolerance<<" old_faces="<<old.surface.size()<<" parameter_count="<<old.parameters.size()<<" ceiling_bytes="<<ceiling<<'\n';
  quality("old",old);
  const auto work_before=budget.work;SetShapeAdmission(ShapeAdmission::ValidityOnly);Patch fresh;std::string reason;
  const bool valid=operation=="split" ? SplitEdge(old,edge,midpoint,maxcell+1,*scoped,fresh,reason) :
      CollapseVertex(old,removed,retained,maxcell+1,*scoped,fresh,reason);
  std::cerr<<"operator="<<operation<<" removed_or_edge_a="<<removed<<" retained_or_edge_b="<<retained<<" valid="<<valid<<" old_cells="<<old.cells.size()<<" new_cells="<<fresh.cells.size()<<" work="<<budget.work<<" work_exhausted="<<budget.work_exhausted<<" reason="<<reason<<std::endl;
  const auto work_after=budget.work;
  std::cout<<"scope=final_private_state_proposal old_cells="<<old.cells.size()<<" new_cells="<<fresh.cells.size()<<" new_faces="<<fresh.surface.size()<<" valid="<<valid<<" reason="<<reason<<" work_before_op="<<work_before<<" work_after_op="<<work_after<<" work_exhausted="<<budget.work_exhausted<<" peak="<<budget.peak_bytes<<'\n';
  if(valid)quality("fresh",fresh);
  return 0;
} catch(const std::exception& e){std::cerr<<"replay_error="<<e.what()<<'\n';return 1;}
