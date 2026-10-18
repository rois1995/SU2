#!/usr/bin/env python3
"""Derive an existing frozen-volume diagnostic with a streaming terminal-state observer."""
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent.parent
s=(BASE/'context.cpp').read_text().replace('../../../../','../../../../../../')
old='     control.rejected_trial=[this]';assert s.count(old)==1
new='''     control.rejected_state=[](const Owned& cells,const std::string&) {
       require(cells.size()<=2000000,"Rejected-state diagnostic row bound");
       std::ofstream out("rejected_working_rank"+std::to_string(CPassiveComm::Rank())+".bin",std::ios::binary);
       const char magic[8]={'N','A','T','O','W','N','1',0};const uint64_t count=cells.size();
       out.write(magic,8);out.write(reinterpret_cast<const char*>(&count),8);
       auto put=[&](const auto& x){out.write(reinterpret_cast<const char*>(&x),sizeof(x));};
       for(const auto& row:cells){const auto& c=row.second;require(row.first==c.cell.id,"Changed owned cell identity");put(c.cell.id);put(c.version);
         for(auto n:c.cell.v){put(n.id);put(n.p.x);put(n.p.y);put(n.p.z);}
         for(auto f:c.physical){const uint8_t present=f.present;put(present);put(f.facet);}}
       out.close();require(bool(out),"Cannot preserve rejected working state");
     };
     if(!CPassiveComm::Rank()){
       std::ofstream names("marker_names.dat");for(const auto& name:state.marker_names)names<<std::quoted(name)<<'\\n';require(bool(names),"Cannot retain marker names");
       std::ofstream facets("facet_markers.dat");for(const auto& row:state.original->Facets())facets<<row.first<<' '<<row.second.marker<<'\\n';require(bool(facets),"Cannot retain facet markers");
     }
'''+old
s=s.replace(old,new);(HERE/'context.cpp').write_text(s)
