from pathlib import Path
import json,time,hashlib,os,importlib.util,sys
P=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11').resolve();V=P/'B8/validation';CASE=V/'cases/B8_G2_558_R1_corrected';D=P/'end_B_preparation/R3_G2_controller_recovery';D.mkdir(exist_ok=True)
def load(path):
 s=importlib.util.spec_from_file_location('adopt_g2_wrapper',path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
wrapper=load(V/'run_g2_gq_prefix_558.py');runner=wrapper.load_runner();env,validation=runner['namespace']();source=runner['patched_controller']();suffix=source[source.index('        while True:'):source.index('\n\nif __name__ == "__main__":')];assert suffix.count('        while True:')==1
status=json.loads((CASE/'status.json').read_text());assert status['state']=='running';old= json.loads((V/'G2_R1_primary_launch_identity.json').read_text())['controller'];rows=env['read_processes']();assert not env['identity_alive'](old,rows);assert env['identity_alive'](status['identity'],rows)
class AdoptedProcess:
 def __init__(self,identity):self.identity=identity;self.pid=identity['pid']
 def poll(self):return None if env['identity_alive'](self.identity,env['read_processes']()) else 'terminal_exit_code_unavailable'
 def wait(self,timeout=10):
  end=time.monotonic()+timeout
  while self.poll() is None and time.monotonic()<end:time.sleep(.1)
  assert self.poll() is not None
  return None
seen={};peaks={}
for line in (CASE/'host_resources.jsonl').read_text().splitlines():
 row=json.loads(line)
 for process in row['owned_processes']:seen[(process['pid'],process['start_ticks'])]=process
 for key,value in row['rank_process_peak_hwm_bytes'].items():
  pid,ticks=map(int,key.split(':'));peaks[(pid,ticks)]=max(peaks.get((pid,ticks),0),value)
stat=Path('/proc/self/stat').read_text().rsplit(')',1)[1].split();identity=dict(pid=os.getpid(),start_ticks=int(stat[19]));pins={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in [Path(__file__).resolve(),V/'run_g2_gq_prefix_558.py',V/'validate_g2_gq.py',P/'build558/source_pins.json',P/'build558/build_status.json',CASE/'provenance.json']};record=dict(actual_host_proc=True,unix=time.time(),controller=identity,previous_controller=old,previous_controller_exec_session=2592,previous_controller_exec_exit_code=143,existing_solver_identity=status['identity'],same_numerical_run_not_restarted=True,original_status=status,reused_controller_suffix_sha256=hashlib.sha256(suffix.encode()).hexdigest(),pins=pins);(D/'adoption_launch_identity.json').write_text(json.dumps(record,indent=2)+'\n');(D/'status_before_adoption.json').write_text(json.dumps(status,indent=2)+'\n')
env.update(ADOPT_CASE=CASE,ADOPT_STATUS=status,ADOPT_SEEN=seen,ADOPT_PEAKS=peaks,AdoptedProcess=AdoptedProcess)
preamble='''def resume_existing_case():
    case=ADOPT_CASE; gate='G2'; full_mode=False; reference_case=None
    pins_before=verify_pins(); provenance=json.loads((case/'provenance.json').read_text()); input_hashes=provenance['input_hashes']
    baseline293=json.loads(REFERENCE293.read_text()); baseline494=baseline293['build494']
    started_unix=ADOPT_STATUS['started_unix']; start=time.monotonic()-(time.time()-started_unix)
    stop_reason=None; captured={}; construction={}; p6_collapse=None; p6_construction=None; deferred_milestones={}; gate_receipt=None; g2_receipt=None
    worker_peak_hwm=ADOPT_PEAKS; seen=ADOPT_SEEN; root_stamp=ADOPT_STATUS['identity']; proc=AdoptedProcess(root_stamp)
    with (case/'run.log').open('a') as log, (case/'host_resources.jsonl').open('a') as receipts:
'''
exec(compile(preamble+suffix,'<existing G2 controller suffix with adopted live process>','exec'),env)
try:env['resume_existing_case']()
except SystemExit as e:assert e.code in (None,0),e.code
# Unknown solver exit status is explicitly retained; prefix acceptance uses the unchanged gates.
assert json.loads((CASE/'status.json').read_text())['exit_code'] is None
r=validation.validate('G2',CASE);assert r['passed'];(D/'adoption_completed.json').write_text(json.dumps(dict(passed=True,unix=time.time(),same_numerical_run_not_restarted=True,solver_exit_code_unavailable=True,controller_suffix_unchanged=True,comparison_sha256=hashlib.sha256((CASE/'g2_build558_comparison.json').read_bytes()).hexdigest()),indent=2)+'\n');print('Adopted original G2 run completed and strict comparison passed.')
