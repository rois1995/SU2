"""Execute scoped failed-case transfer/equivalence and certificate/phase controls sequentially."""
from pathlib import Path
import json,os,sys
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where.parent/'native_cluster_3d_core_v1'))
from run import command
case=where/'query_routing_n8_n1_m1_v1'
cpu=json.loads((where/'query_routing_restart_cgns_n2_n4_m2_v1/host_gate.json').read_text())['affinity'][0]
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
stages=[];status='FAIL';reason=''
try:
 tasks=[(where/'audit_query_routing_equivalence.py',[case,where/'full_star_facet_n8_n1_m1_rounds1024_v1','--partial','--work-reference',where/'full_star_facet_n8_n1_m1_v1'],case,'first_cycle_equivalence'),
        (case/'export_first_cycle.py',[case],case,'first_cycle_export'),
        (folder/'check_equivalence_corruption.py',[],folder,'certificate_corruption'),
        (folder/'collect.py',[],folder,'accounting')]
 for script,args,dest,label in tasks:
  row=command(['taskset','-c',str(cpu),'nice','-n','19',sys.executable,str(script),*map(str,args)],dest,label,env,180)
  stages.append(row)
  assert row['exit_code']==0 and not row.get('timeout'),label
 status='PASS'
except Exception as e:reason=str(e)
(folder/'audit_execution.json').write_text(json.dumps(dict(status=status,reason=reason,stages=stages,
 scope='Sequential one-CPU first-event certificate/transfer equivalence, current VTU export, fresh negative controls and actual routing-counter accounting.512campaign stays FAIL;1024campaign is separately PASS.'),indent=2)+'\n')
print(status,reason)
raise SystemExit(status!='PASS')
