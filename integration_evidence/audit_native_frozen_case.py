from pathlib import Path
import argparse,sys,json,hashlib,numpy as np
r=Path(__file__).resolve().parent.parent;e=r/'integration_evidence';sys.path.insert(0,str(e))
from audit_native_composite_rae import audit,capcheck,check_math
check_math()
from airfoil_reference_audit import reference_audit
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
parser=argparse.ArgumentParser(description='Independent grid and final-tensor audit of a frozen RAE case.')
parser.add_argument('case',type=Path);parser.add_argument('fixture',choices=('rae_euler_to_bl','rae_bl_to_euler'))
parser.add_argument('--report-name',default='independent_frozen_metric_audit.json')
parser.add_argument('--self-contained',action='store_true',help='Use the collected case config/input/raw sensor and original source flow')
args=parser.parse_args();name=args.fixture;case=args.case.resolve()
assert Path(args.report_name).name==args.report_name
actual,step,h = ('rae_rans_window200_np4_v1',199,1e-5) if name=='rae_euler_to_bl' else ('rae_euler_from_bl_window100_np4_v1',99,None)
orig=case if args.self_contained else e/'native_unsteady_performance_v1'/actual
n=int(json.loads((case/'run_evidence.json').read_text())['command'][2])
meshpath=case/'native_frozen_adapted.su2';output=case/args.report_name;assert not output.exists()
transported=sorted(case.glob('native_frozen_target_rank_*.csv'))
if transported:assert len(transported)==n
restart=case/'frozen_sensor_source_flow.vtu' if args.self_contained else orig/f'flow_{step:05d}.vtu'
row=audit(orig,1,orig/'input.su2',meshpath,restart,sensor_csv=case/'frozen_sensor.csv',transported=transported or None);assert not row['bad_quality_cells'] and not row['bad_length_edges'],row
mesh=capcheck.read_su2(meshpath);row['validity']=capcheck.check_validity(mesh);assert all(g[0]=='PASS' for g in row['validity'].values())
row['original_reference']=reference_audit(orig/'input.su2',meshpath)
assert all(ref['all_declared_features_exactly_retained'] and abs(ref['covered_arc_over_period']-1)<1e-10 and ref['max_corresponding_parameter_deviation']<=1e-6 for ref in row['original_reference'])
error=None
if h:
 incident={tuple(sorted(edge)):t for t in mesh.E for edge in (t[[0,1]],t[[1,2]],t[[2,0]])};errors=[]
 for edge in mesh.M['AIRFOIL']:
  a,b=mesh.P[edge];t=incident[tuple(sorted(edge))];v=mesh.P[next(i for i in t if i not in edge)]
  area2=abs((b[0]-a[0])*(v[1]-a[1])-(b[1]-a[1])*(v[0]-a[0]));errors.append(abs(area2/np.linalg.norm(b-a)/h-1))
 error=max(errors);assert error<=1e-8
expected=json.loads((case/'run_evidence.json').read_text())['inputs_sha256']['frozen_sensor.csv'] if args.self_contained else sha(e/'native_frozen_scaling_v1'/name/'frozen_sensor.csv')
assert sha(case/'frozen_sensor.csv')==expected
row.update(status=('FAIL' if transported and row['transported_metric']['status']!='PASS' else 'PASS'),max_relative_first_height_error=error,frozen_csv_sha256=sha(case/'frozen_sensor.csv'),checker_sha256=sha(Path(__file__)))
output.write_text(json.dumps(row,indent=2)+'\n')
if transported:assert row['transported_metric']['status']=='PASS',row['transported_metric']
print(name,n,'PASS',row['min_quality'],row['max_simpson_length'],error,flush=True)
