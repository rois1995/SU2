#!/usr/bin/env python3
"""Derive the actual frozen M6 edge-star engine replay from the saved measurement helper."""
from pathlib import Path
HERE=Path(__file__).resolve().parent;old=HERE.parent/'cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1'
s=(old/'replay.cpp').read_text().replace('../../../../../../../','../../../../')
s=s.replace('#include <fstream>','#include "../../../../Common/include/adaptation/CNativeEngine3D.hpp"\n#include <fstream>')
a=s.index('  FrozenField field(');b=s.index('int main(',a)
s=s[:a]+'''  World world;Owned accepted;
  auto key=[](Face f){std::sort(f.begin(),f.end());return f;};std::map<Face,Id> physical;
  for(auto f:old.surface)physical.emplace(key(f.v),f.facet);
  for(size_t i=0;i<old.cells.size();++i)if(i%world.size==unsigned(world.rank)){
    OwnedCell row;row.cell=old.cells[i];row.version=17;
    const auto& v=row.cell.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned j=0;j<4;++j){auto f=physical.find(key(faces[j]));if(f!=physical.end())row.physical[j]={true,f->second};}
    accepted.emplace(row.cell.id,row);
  }
  FrozenField sensor(donors);const auto a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
  require(sensor.TargetEdgeLength(a,b)<MaximumMetricEdge,"Sensor-only negative control unexpectedly violates the edge bound");
  std::vector<DonorCell> original;for(size_t i=0;i<donors.size();++i)if(i%world.size==unsigned(world.rank))original.push_back(donors[i]);
  auto snapshot=[&](){std::vector<char> bytes;RecordStream stream(bytes);for(auto row:accepted)stream(row.second);return bytes;};
  const auto before=snapshot();EngineOptions options;options.maximum_rounds=32;options.maximum_work=4096;options.maximum_seconds=20;options.timings=true;options.edge_breakpoints=bands;
  for(unsigned short k=0;k<config.GetnAdap_BL();++k){const auto& layer=config.GetAdap_BL(k);auto it=std::find(state.marker_names.begin(),state.marker_names.end(),layer.marker);require(it!=state.marker_names.end(),"Missing wall marker");const int id=int(it-state.marker_names.begin());options.wall_heights[id]=SU2_TYPE::GetValue(layer.firstHeight);options.region_walls[id]={SU2_TYPE::GetValue(layer.firstHeight),SU2_TYPE::GetValue(layer.growth),SU2_TYPE::GetValue(layer.thickness)};}
  const auto start=world.seconds();auto result=Refine(world,accepted,original,*state.original,compose,options);const auto elapsed=world.seconds()-start;
  require(snapshot()==before,"Early refusal mutated the accepted input");require(!result.valid,"Incompatible exact geometry was accepted");
  auto witness=result.reason.substr(0,result.reason.find(" Last local rejection:"));
  std::ofstream out("result_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)<<"{\\"rank\\":"<<world.rank<<",\\"ranks\\":"<<world.size<<",\\"seconds\\":"<<elapsed<<",\\"stop\\":"<<std::quoted(EngineStopName(result.stats.stop))<<",\\"rounds\\":"<<result.stats.rounds<<",\\"work\\":"<<result.stats.global_work<<",\\"accepted_edits\\":"<<result.stats.accepted<<",\\"immutable_input\\":true,\\"reason\\":"<<std::quoted(result.reason)<<"}\\n";out.close();require(bool(out),"Cannot save refusal receipt");
  require(witness.find("Native3D exact-facet resolution conflict")!=std::string::npos,"Did not reach the expected geometry diagnosis");
  uint64_t hash=1469598103934665603ull;for(unsigned char c:witness){hash^=c;hash*=1099511628211ull;}
  require(CPassiveComm::Allreduce(hash,CPassiveComm::Op::MIN,world.comm)==CPassiveComm::Allreduce(hash,CPassiveComm::Op::MAX,world.comm),"Ranks disagree on the elected witness");return 0;
}
'''+s[b:]
(HERE/'probe.cpp').write_text(s)
for name in ['working.dat','edge.dat','serial.cfg','donors.dat']:(HERE/name).write_bytes((old/name).read_bytes())
