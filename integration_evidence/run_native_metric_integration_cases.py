"""Sequential independent frozen-field audits and actual cross-grid CFD cases."""
from pathlib import Path
import json,os,re,subprocess,sys,time
root=Path(__file__).resolve().parent.parent;os.chdir(root);e=root/'integration_evidence';d=e/'native_metric_integration_v4';state=d/'post_frozen_v1.json';assert not state.exists()
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1');record={'controller_pid':os.getpid(),'sequential':True,'stages':[]}
def save():state.write_text(json.dumps(record,indent=2)+'\n')
def run(name,args):
 start=time.monotonic();record.update(phase=name,command=args);save()
 with (d/(name+'.controller.log')).open('x') as log:
  child=subprocess.Popen([sys.executable,*args],env=env,stdout=log,stderr=subprocess.STDOUT);record['child_pid']=child.pid;save();code=child.wait()
 record.pop('child_pid',None);record['stages'].append({'name':name,'exit_code':code,'elapsed_seconds':time.monotonic()-start});save();print(name,'exit',code,flush=True)
 if code:record['phase']='terminal_failed';save();raise SystemExit(code)
assert json.loads((d/'campaign_frozen.json').read_text())['phase']=='terminal_pass'
run('frozen_field_audit',['integration_evidence/audit_native_metric_integration.py',str(d)])
run('higher_order_quadrature',['integration_evidence/audit_native_metric_quadrature.py',str(d),'1.25'])
p=d/'frozen/coarse/minimum_complexity_np4';p.mkdir();(p/'input.su2').symlink_to('../inputs/input.su2');cfg=(d/'frozen/coarse/weighted_least_squares_np4/run.cfg').read_text().replace('ADAP_COMPLEXITY= 60000','ADAP_COMPLEXITY= 1000');(p/'run.cfg').write_text(cfg)
run('minimum_complexity',['integration_evidence/run_rae2822_trial.py',str(p),'--ranks','4','--timeout','600','--build-evidence',str(e/'native_metric_integration_build_v4/evidence.json')])
log=(p/'solver.log').read_text();floor=float(re.search(r'Mesh complexity after native constraints: ([\d.e+\-]+)',log)[1]);assert 'outside the attainable bounds' in log and floor>1000
(d/'minimum_complexity.json').write_text(json.dumps({'reported_minimum_composed_complexity':floor,'probe_target':1000,'historical_requests':[4000,6000],'below_measured_floor':[x<floor for x in (4000,6000)],'frozen_case':str(p),'scope':'Configured bounds and original geometric BL; coarsest sensor endpoint. Not a CFD solution or exact minimum over arbitrary geometry/metric policies.'},indent=2)+'\n')
case_root=e/'rae2822_transonic_v1'
for kind,name in [('euler','metricmerge_euler_cross_seed_v1'),('rans','metricmerge_rans_cross_seed_v2')]:
 case=case_root/name
 if kind=='rans':
  assert floor<10000
  receipt=json.loads((case/'budget_change.json').read_text());receipt.update(prepared_only=False,measured_minimum_composed_complexity=floor,reason='Geometric BL is included in ADAP_COMPLEXITY. The measured floor is recorded; feasible composed budgets retain room for the flow sensor.');(case/'budget_change.json').write_text(json.dumps(receipt,indent=2)+'\n')
 run(kind+'_lifecycle',['integration_evidence/run_rae2822_trial.py',str(case),'--ranks','4','--timeout','1200','--build-evidence',str(e/'native_metric_integration_build_v4/evidence.json')])
 run(kind+'_inspection',['integration_evidence/inspect_rae2822_solution.py',str(case)])
 run(kind+'_metric',[str(case/'audit_metric_source.py')] if kind=='euler' else ['integration_evidence/audit_native_composite_rae.py',str(case)])
record['phase']='terminal_pass';save()
