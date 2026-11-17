from pathlib import Path
import os,json,time,hashlib,sys
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation').resolve()
def read(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
action,label=sys.argv[1:3]
if action=='seal':
 r=read(v/f'n14_g3_{label}_gate.json');assert r['passed'];case=Path(r['case']);term=r['terminal'];assert term['all_owned_handles_terminal'];rows=term['owned_processes']+[dict(read(v/f'G3_{label}_primary_launch_identity.json')['controller'],name='controller')]
 for row in rows:
  try:
   f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();alive=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
  except FileNotFoundError:alive=False
  row['alive']=alive;assert not alive,row
 pins=read(case/'pins.json');extra=read(v/f'G3_{label}_primary_validation_pins.json');assert all(sha(p)==h for p,h in pins.items());assert all(sha(p)==h for p,h in extra.items());out=v/f'G3_{label}_primary_terminal_receipt.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),identities=rows,all_owned_handles_terminal=True,exec_session=int(sys.argv[3]),exec_exit_code=0,case_pins_verified=len(pins),extra_validation_pins_verified=len(extra),passed=True,accepted_splits=r['purpose']['accepted_splits']),indent=2)+'\n');print(out)
else:
 assert action=='launch' and label=='second'
 assert read(v/'G3_first_primary_terminal_receipt.json')['passed']
 host=v/'G3_second_actual_host_preflight.json';paths=[v/'run_g3_552.py',v/'run_focused552.py',v.parent/'build552_aliases.py',host,v.parent/'build552/source_pins.json',v.parent/'build552/build_status.json']
 pins={str(p):sha(p) for p in paths};out=v/f'G3_{label}_primary_validation_pins.json';assert not out.exists();out.write_text(json.dumps(pins,indent=2)+'\n');args=['/usr/bin/python3',str(v/'run_g3_552.py'),label,'--host-preflight',str(host)];f=Path('/proc/self/stat').read_text().rsplit(')',1)[1].split();out=v/f'G3_{label}_primary_launch_identity.json';assert not out.exists();out.write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),controller=dict(pid=os.getpid(),start_ticks=int(f[19])),argv=args),indent=2)+'\n');os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt');os.nice(19);os.execv(args[0],args)
