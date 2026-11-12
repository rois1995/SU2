"""Summarize bounded actual candidates; preserve partial/unknown exact measurements."""
from pathlib import Path
import json,hashlib,struct
R=Path(__file__).resolve().parent
case=R/'cases/curved437_cavity_correction_20261112T111648_1071306'
text=(case/'actual_bl_cavity_planning.jsonl').read_text();decoder=json.JSONDecoder(strict=False);position=0;records=[]
while position<len(text):
 while position<len(text) and text[position].isspace():position+=1
 if position==len(text):break
 row,position=decoder.raw_decode(text,position);records.append(row)
(case/'normalized_records.jsonl').write_text(''.join(json.dumps(x)+'\n' for x in records))
assert all(x.get('reason')!='Native 3D cavity oriented interface changed.' for x in records)
def hashes(paths):
 out={}
 for path in paths:
  with path.open('rb') as f:
   n=struct.unpack('<Q',f.read(8))[0]
   for _ in range(n):
    row=f.read(416);assert len(row)==416;identity=struct.unpack_from('<Q',row,8)[0];assert identity not in out;out[identity]=hashlib.sha256(row).hexdigest()
   assert not f.read()
 return out
source=R/'cases/curved434_saved_target_surface_mpi6_20261112T095336_826002'
old=hashes(sorted(source.glob('stage_surface_rank*_owned.bin')));new=hashes([case/'correction_unchanged_owned.bin']);assert old==new
summary={'case':str(case),'status':json.loads((case/'status.json').read_text()),'owned_records_byte_identical':len(new),'oriented_interface_refusals':0,'raw_control_characters_normalized':True,'operators':[x for x in records if x.get('stage')=='operator_preflight'],'discovery':[x for x in records if x.get('stage')=='correction_discovery'],'exact_edge_audits':[x for x in records if x.get('stage')=='full_target_edge_audit'],'completed_exact_edges':[x for x in records if x.get('stage')=='completed_exact_edge'],'production_capability_integrated':False,'MPI_qualified':False,'reference_equivalent':False}
(R/'cavity_correction437_assessment.json').write_text(json.dumps(summary,indent=2)+'\n')
print({'records':len(records),'owned_records_byte_identical':len(new),'private_feasible':sum(x.get('private_feasible',False) for x in summary['operators']),'completed_exact_edges':len(summary['completed_exact_edges']),'audits_complete':sum(x['complete'] for x in summary['exact_edge_audits'])})
