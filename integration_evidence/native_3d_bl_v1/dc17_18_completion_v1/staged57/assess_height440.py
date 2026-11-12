"""Bounded same-cavity height comparison; no generated-BL or accepted-mesh claim."""
from pathlib import Path
import hashlib,json,struct
R=Path(__file__).resolve().parent
case=R/'cases/curved440_cavity_height_continuation_20261112T120931_1211699'
assert json.loads((case/'status.json').read_text())['pins_verified']
assert json.loads((case/'terminal_handles_actual.json').read_text())['wrapper_verified_terminal']
raw=(case/'actual_bl_cavity_planning.jsonl').read_text();decoder=json.JSONDecoder(strict=False);position=0;records=[]
while position<len(raw):
 while position<len(raw) and raw[position].isspace():position+=1
 if position==len(raw):break
 record,position=decoder.raw_decode(raw,position);records.append(record)
(case/'normalized_records.jsonl').write_text(''.join(json.dumps(x)+'\n' for x in records))
patches=[x for x in records if x.get('stage')=='height_continuation_patch'];edges=[x for x in records if x.get('stage')=='height_continuation_edge'];tangent=[x for x in records if x.get('stage')=='height_continuation_tangential_edges']
assert len(patches)==60 and len(edges)==6 and len(tangent)==60
assert all(x['shape_measured'] and x['height_measured'] for x in patches)
final=[x for x in patches if x['k']==5];assert len(final)==10 and all(x['final_tensor_checked'] and x['final_tensor_relative_difference']==0 for x in final)
base={(x['context'],x['side']):{tuple(e[:2]):e[2] for e in x['edges']} for x in tangent if x['k']==0}
maximum_tangent_relative_difference=0;classification_changes=0
for row in tangent:
 for edge in row['edges']:
  old=base[(row['context'],row['side'])][tuple(edge[:2])]
  maximum_tangent_relative_difference=max(maximum_tangent_relative_difference,abs(edge[2]-old)/old)
  classification_changes+=sum((edge[2]<threshold)!=(old<threshold) for threshold in (.5,.9,1.6,1.8))
assert classification_changes==0
def hashes(paths):
 out={}
 for p in paths:
  with p.open('rb') as f:
   n=struct.unpack('<Q',f.read(8))[0]
   for _ in range(n):
    row=f.read(416);assert len(row)==416;identity=struct.unpack_from('<Q',row,8)[0];assert identity not in out;out[identity]=hashlib.sha256(row).hexdigest()
   assert not f.read()
 return out
original=R/'cases/curved434_saved_target_surface_mpi6_20261112T095336_826002'
a=hashes(sorted(original.glob('stage_surface_rank*_owned.bin')));b=hashes([case/'correction_unchanged_owned.bin']);assert a==b
# Reconstruct the exact in-memory diagnostic config for review; it was not a normalized target checkpoint.
text=''
for line in (case/'run.cfg').read_text().splitlines():
 if 'ADAP_SUBITER=' in line:line='ADAP_SUBITER= ( 7 )'
 assert 'ADAP_BL_H0_RAMP=' not in line
 text+=line+'\n'
text+='ADAP_BL_H0_RAMP= 5\n';cfg=case/'diagnostic_ramp5_config_reconstructed.cfg';cfg.write_text(text)
summary={'case':str(case),'status':json.loads((case/'status.json').read_text()),'owned_records_byte_identical':len(a),'records':len(records),'raw_JSON_control_characters_normalized':True,'N':5,'final_h0':2e-6,'growth':1.2,'thickness':.01,'fixed_current_core_and_tangential_scale':True,'normalized_for_each_height':False,'height_records':patches,'edge_records':edges,'tangential_records':tangent,'maximum_tangent_edge_relative_difference':maximum_tangent_relative_difference,'sampled_tangent_threshold_classification_changes':classification_changes,'final_stage_centroid_target_tensors_bit_identical_to_original':True,'strict_height_passes':sum(x['strict_height'] for x in patches),'constructed_height_stages':0,'actual_layer_progression_verified':False,'new_MPI_cavity_capability':False,'mesh_accepted':False,'reconstructed_diagnostic_config':{'path':str(cfg),'sha256':hashlib.sha256(cfg.read_bytes()).hexdigest()}}
(R/'height_continuation440_assessment.json').write_text(json.dumps(summary,indent=2)+'\n')
print({'records':len(records),'owned_unchanged':len(a),'strict_height_passes':summary['strict_height_passes'],'edge_complete':[x['complete'] for x in edges],'maximum_tangent_relative_difference':maximum_tangent_relative_difference,'final_target_identical':True})
