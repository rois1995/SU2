"""Qualify actual complete stage0 target reconstruction; never imply BL/mesh suitability."""
from pathlib import Path
import hashlib,json,re,sys
R=Path(__file__).resolve().parent
producer=Path(sys.argv[1]).resolve();reload=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
status=json.loads((producer/'status.json').read_text());terminal=json.loads((producer/'terminal_handles_actual.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified'] and terminal['wrapper_verified_terminal']
files={}
for rank in range(6):
 for name in (f'target_rank{rank}.bin',f'target_witness_rank{rank}.txt'):
  p=producer/name;files[name]={'sha256':sha(p),'bytes':p.stat().st_size}
receipt={'scope':'Complete proposed32h0 stage0 target; same saved CFD/no advance. Fidelity separate from suitability.','producer':str(producer),'status':status,'terminal':terminal,'files':files,'total_bytes':sum(x['bytes'] for x in files.values()),'stage':{'N':5,'event':0,'effective_h0':6.4e-5,'final_h0':2e-6,'growth':1.2,'thickness':.01},'physical_target_suitability_qualified':False,'mesh_generated':False}
(R/'target_checkpoint441_producer.json').write_text(json.dumps(receipt,indent=2)+'\n')
if reload is None:print({'producer':str(producer),'bytes':receipt['total_bytes'],'fidelity_pending':True});raise SystemExit()
state=json.loads((reload/'status.json').read_text());end=json.loads((reload/'terminal_handles_actual.json').read_text())
assert state['state']=='terminal' and state['exit_code']==0 and state['pins_verified'] and end['wrapper_verified_terminal']
records=[];owned=exact=0;maximum=0;chord_equal=True
for rank in range(6):
 rows=[json.loads(x) for x in (reload/f'saved_target_reload_rank{rank}.jsonl').read_text().splitlines()]
 assert len(rows)==2;meta,chord=rows;assert meta['complete_checkpoint'] and not meta['cfd_advanced'] and not meta['metric_recomputed']
 owned+=meta['owned_queries'];exact+=meta['bit_identical'];maximum=max(maximum,meta['maximum_tensor_relative_difference'])
 expected=float((producer/f'target_witness_rank{rank}.txt').read_text());chord_equal=chord_equal and chord['length']==expected
 records.append({'rank':rank,'metadata':meta,'chord':chord})
assert owned==exact==61330 and maximum==0 and chord_equal
log=re.sub(r'\x1b\[[0-9;]*m','',(reload/'run.log').read_text());passes=re.findall(r'All tests passed\s*\((\d+) assertions in (\d+) test case',log);assert len(passes)==6
result={'scope':__doc__,'state':'checkpoint_fidelity_qualified','producer':str(producer),'reload':str(reload),'producer_receipt_sha256':sha(R/'target_checkpoint441_producer.json'),'status':state,'terminal':end,'pins_verified':True,'owned_nodes':owned,'owned_bit_identical':exact,'maximum_tensor_relative_difference':maximum,'chord_bit_identical':chord_equal,'passing_by_rank':passes,'records':records,'actual_target_checkpoint_qualified':True,'physical_target_suitability_qualified':False,'mesh_generation_qualified':False,'native_MPI_BL_generation_qualified':False,'CFD_advanced':False,'actual_adapted_disk_restart':False}
(R/'target_checkpoint441_qualification.json').write_text(json.dumps(result,indent=2)+'\n');print({'owned':owned,'exact':exact,'chord_bit_identical':chord_equal,'fidelity_qualified':True,'mesh_generation_qualified':False})
