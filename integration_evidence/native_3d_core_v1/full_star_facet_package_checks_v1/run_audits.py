"""Execute saved first-event equivalence/VTU and full-filter accounting checks sequentially."""
from pathlib import Path
import json,os,sys
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where.parent/'native_cluster_3d_core_v1'))
from run import command
case=where/'full_star_facet_n8_n1_m1_v1'
cpu=json.loads((where/'full_star_facet_restart_cgns_n2_n4_m2_v1/host_gate.json').read_text())['affinity'][0]
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
stages=[];status='FAIL';reason=''
try:
 for script,args,dest,label in [(case/'audit_first_cycle_equivalence.py',[],case,'first_cycle_equivalence'),
                                (case/'export_first_cycle.py',[case],case,'first_cycle_export'),
                                (folder/'collect.py',[],folder,'accounting')]:
  row=command(['taskset','-c',str(cpu),'nice','-n','19',sys.executable,str(script),*map(str,args)],dest,label,env,180)
  stages.append(row)
  assert row['exit_code']==0 and not row.get('timeout'),label
 status='PASS'
except Exception as e:reason=str(e)
(folder/'audit_execution.json').write_text(json.dumps(dict(status=status,reason=reason,stages=stages,
 scope='Sequential one-CPU first-event certificate equivalence, current state transfer/VTU export and actual full-filter counter accounting. Whole n8 campaign remains FAIL.'),indent=2)+'\n')
print(status,reason)
raise SystemExit(status!='PASS')
