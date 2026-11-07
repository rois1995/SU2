"""One owned full same-seed two-event attempt; preserve launch and terminal identity."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,tempfile,time
R=Path(__file__).resolve().parent;repo=R.parents[3];attempt=R/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k'
receipt=R/'m6_254_condition_launch.json';assert not receipt.exists() and not attempt.exists()
assert json.loads((R/'metric_target254_conditioned_v1/terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'condition254_mpi6_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'condition254_qualification.json').read_text())['actual_same_seed_target']['all_flow_fields_and_coordinates_bit_identical']
for name in ['build254_source_pins.json','metric_target254_conditioned_v1/pins.json']:
 pins=json.loads((R/name).read_text());assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in pins.items())
mem={k:int(v.split()[0])*1024 for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines()) if k in ['MemAvailable','SwapFree']}
assert mem['MemAvailable']>16*1024**3
scratch=Path(tempfile.mkdtemp(prefix='m6-conditioned254-',dir=repo/'integration_evidence/build-integrated-v2/compile-tmp'))
env=os.environ.copy()
for key in list(env):
 if key.startswith('SU2_NATIVE_'):env.pop(key)
env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt',TMPDIR=str(repo/'integration_evidence/build-integrated-v2/compile-tmp'),OMPI_MCA_orte_tmpdir_base=str(scratch))
argv=[sys.executable,str(R/'run_m6.py'),'build254',attempt.name,'2',str(R/'m6_inputs/euler_original_force_seed_v1'),'--inspect-rejected']
state={'state':'running','started_at':time.time(),'runner_wrapper':{'pid':os.getpid(),'start_ticks':int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19])},'attempt':str(attempt),'argv':argv,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'sensors':['MACH'],'loadavg':Path('/proc/loadavg').read_text(),'mem':mem,'disk_free_bytes':shutil.disk_usage(repo).free,'mpi_scratch':str(scratch),'launcher_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'scope':'Full same-seed MACH-only L1/50k TWO-event M6;strict unchanged gates/native5400/outer14400;bounded raw refusal capture,no routine VTU export.'}
receipt.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state),flush=True)
code=subprocess.call(argv,env=env)
state.update(state='terminal',exit_code=code,finished_at=time.time());receipt.write_text(json.dumps(state,indent=2)+'\n');raise SystemExit(code)
