"""Summarize bounded actual candidates; preserve partial/unknown exact measurements."""
from pathlib import Path
import json,hashlib,struct
R=Path(__file__).resolve().parent
case=R/'cases/curved441_surface_cavity_correction_20261112T134017_1450957'
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
terminal=json.loads((case/'terminal_handles_actual.json').read_text())
assert terminal['all_owned_handles_terminal'] and terminal['wrapper_verified_terminal']
pins=json.loads((case/'pins.json').read_text())
for path,digest in pins.items():
 with Path(path).open('rb') as stream:assert hashlib.file_digest(stream,'sha256').hexdigest()==digest,path
records=[json.loads(line) for line in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
assert records
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
source=R/'cases/curved441_ramp5_saved_target_surface_mpi6_20261112T132736_1419501'
old=hashes(sorted(source.glob('stage_surface_rank*_owned.bin')));new=hashes([case/'correction_unchanged_owned.bin']);assert old==new
summary={'case':str(case),'status':json.loads((case/'status.json').read_text()),'owned_records_byte_identical':len(new),'oriented_interface_refusals':0,'raw_control_characters_normalized':False,'strict_JSON_records':len(records),'all_case_pins_verified':len(pins),'all_actual_handles_terminal':True,'stage_height':6.4e-5,'final_requested_height':2e-6,'target_checkpoint':'441','same_field_as437':False,'goal_complete':False,'operators':[x for x in records if x.get('stage')=='operator_preflight'],'discovery':[x for x in records if x.get('stage')=='correction_discovery'],'exact_edge_audits':[x for x in records if x.get('stage')=='full_target_edge_audit'],'completed_exact_edges':[x for x in records if x.get('stage')=='completed_exact_edge'],'production_capability_integrated':False,'MPI_qualified':False,'reference_equivalent':False}
for row in summary['operators']:
 if row['private_feasible']:
  assert all(row[k] for k in ('constructed','complete_removed_stars','final_support_reserved','passing_heights_preserved','provisional_height','geometry_guidance_pass','metric_admission','parameter_preflight','contact_protocol','contact_pass'))
  assert not row['published'] and not row['mesh_accepted']
summary['private_feasible']=[x for x in summary['operators'] if x['private_feasible']]
summary['limits']=['Native bounded composition, not reference-equivalent search or pyAMG runtime comparison.','Serial final-support reservation/contact only; no distributed publication or cavity-growth qualification.','q/J are observational on private interpolation, not post-transport published mesh certification.','Partial exact edges stay unknown; SurfacePlanning admission is not a complete composed-target final certificate.']
summary['output_payloads']={}
for name in ('actual_bl_cavity_planning.jsonl','correction_unchanged_owned.bin'):
 path=case/name
 with path.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
 summary['output_payloads'][name]=dict(bytes=path.stat().st_size,sha256=digest)
summary['assessment_script_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
(R/'cavity_surface441_assessment.json').write_text(json.dumps(summary,indent=2)+'\n')
print({'records':len(records),'owned_records_byte_identical':len(new),'private_feasible':sum(x.get('private_feasible',False) for x in summary['operators']),'completed_exact_edges':len(summary['completed_exact_edges']),'audits_complete':sum(x['complete'] for x in summary['exact_edge_audits'])})
