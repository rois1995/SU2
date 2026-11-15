"""Reuse the completed P6c physical gates on profile logs, without another solver run."""
from pathlib import Path
import importlib.util,json,hashlib,sys,time
HERE=Path(__file__).resolve().parent;B=HERE/'profile/run_B';P6=HERE.parent/'p6c_20261115'
spec=importlib.util.spec_from_file_location('physical_gate',P6/'validate_completed_workflow.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
s=json.loads((B/'status.json').read_text());assert s['state']=='terminal' and s['exit_code']==0 and s['all_owned_handles_terminal'] and s['inputs_unchanged'] and not s['profiler_failed'] and s['source_pins_before_after_equal']
# Log placement only: the profiler isolates native stdout/stderr by rank instead of mpirun output-filename.
for rank in range(6):
 target=(B/f'profile/rank{rank}.program.log').resolve();out=B/f'rank_output/1/rank.{rank}/stdout';out.parent.mkdir(parents=True,exist_ok=True)
 if out.exists():assert out.is_symlink() and out.resolve()==target
 else:out.symlink_to(target)
m.self_check();result=m.validator()(B)
result.update(assessment_unix=time.time(),scope='Independent physical/numerical gates applied to the single requested profile B; no new numerical run and no isolated restart claim.',log_adapter='rank_output/1/rank.N/stdout symlinks to immutable completed profile/rankN.program.log',validator_sha256=hashlib.sha256((P6/'validate_completed_workflow.py').read_bytes()).hexdigest(),frozen_controller_sha256=hashlib.sha256((P6/'run_full_workflow.py').read_bytes()).hexdigest())
(B/'independent_workflow_receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'workflow_accepted':result['workflow_accepted'],'errors':result['errors'],'events':[{'event':e['event'],'seconds':e['whole_event_seconds_max_rank'],'counts':e['actual_mesh_counts']} for e in result['events']]}),flush=True)
raise SystemExit(not result['workflow_accepted'])
