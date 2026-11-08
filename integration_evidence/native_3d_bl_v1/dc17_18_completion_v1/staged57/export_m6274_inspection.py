"""Preserve one meaningful terminal mesh inspection using the existing exporter."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3];A=R/'m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k'
sys.path.insert(0,str(R));from run_cases import sample_tree
assert json.loads((A/'terminal_handles.json').read_text())['all_owned_handles_terminal']
status=A/'inspection_export_status.json';assert not status.exists() and not (A/'inspection').exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins=json.loads((A/'pins.json').read_text());ready=json.loads((A/'capture_ready.json').read_text());assert ready['owned_rows']==ready['expected_owned_rows']  # All current cached q/J are known; final whole-surface two-sided geometry remains unproven.
for p,item in ready['files'].items():assert sha(p)==item['sha256'];pins[p]=item['sha256']
for p in [A/'capture_ready.json',R/'inspect_native_failure3d.py',R/'export_m6254_inspection.py',A/'terminal_handles.json',A/'owned_stop_once.json',Path(__file__).resolve(),Path(sys.executable).resolve()]:pins[str(p)]=sha(p)
def check():
 changed=[p for p,h in pins.items() if sha(p)!=h];assert not changed,changed
check();(A/'inspection_export_pins.json').write_text(json.dumps(pins,indent=2)+'\n')
env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
argv=['nice','-n','19','timeout','1200',sys.executable,str(R/'inspect_native_failure3d.py'),str(A/'adapted_mesh_inspection_rank*_manifest.json'),'--output',str(A/'inspection')]
state={'state':'running','started_at':time.time(),'argv':argv,'helper_pid':os.getpid(),'scope':'Complete actual terminal topology with captured nodal metric,bad cells,cavities/reference;all cached q/J known and clean; full two-sided global geometry certification incomplete. STRICTLYREFUSED. No acceptance claim.','pins_count':len(pins),'load':Path('/proc/loadavg').read_text(),'mem_available':[s for s in Path('/proc/meminfo').read_text().splitlines() if s.startswith('MemAvailable')],'disk_free_bytes':shutil.disk_usage(REPO).free}
with (A/'inspection_export.log').open('x') as log,(A/'inspection_export_resources.jsonl').open('x') as resources:
 proc=subprocess.Popen(argv,cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);state['pid']=proc.pid;status.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state),flush=True)
 while proc.poll() is None:
  rows=sample_tree(proc.pid);resources.write(json.dumps({'elapsed':time.time()-state['started_at'],'rows':rows})+'\n');resources.flush();time.sleep(2)
 code=proc.wait()
check();state.update(state='terminal',exit_code=code,seconds=time.time()-state['started_at'],pins_verified=True);status.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state),flush=True);raise SystemExit(code)
