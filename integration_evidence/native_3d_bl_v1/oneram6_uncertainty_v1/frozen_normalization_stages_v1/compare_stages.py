"""Compare the SAME corrected original field on the two actual candidate partitions."""
import json,math,re
from pathlib import Path
import campaign as c
reports=[]
for folder in sorted(c.HERE.glob('attempt2_*runtime_np*')):
 result=folder/'independent_normalization.json'
 if not result.exists():continue
 log=(folder/'run.log').read_text()
 rows=re.findall(r'Native3D complexity normalization: status=(\S+) estimate=(\S+) uncertainty_indicator=(\S+) .*?trials=(\d+) log_scale=(\S+) frozen_core=(\S+)',log)
 for accepted in json.loads(result.read_text()):
  theta=accepted['final_sensor_eigenvalue'];reference=accepted['reference']
  candidates=[r for r in rows if abs(float(r[4])-math.log(theta))<1e-12]
  assert candidates,'Missing original-field root assessment'
  assert len({tuple(r) for r in candidates})==1,'Repeated root assessments differ'
  r=candidates[0];before=float(r[1]);uncertainty=float(r[2]);assert abs(float(r[5])-theta)<1e-12
  stem=accepted['geometry_and_full_field']['file'][:-len('_accepted_mesh.json')]
  data=json.loads((folder/(stem+'.json')).read_text());expected=[theta,0,0,theta,0,theta]
  assert all(max(abs(x-y) for x,y in zip(v[4],expected))<2e-13 for d in data['original'] for v in d['vertices'])
  before_error=abs(before-reference);after_error=abs(accepted['mass']-reference)
  reports.append(dict(case=folder.name,initial_scale=accepted['initial_scale'],same_original_beta_field=True,
   reference=reference,before=dict(value=before,indicator=uncertainty,absolute_error=before_error),
   after=dict(value=accepted['mass'],indicator=accepted['indicator'],absolute_error=after_error),
   indicator_ratio=accepted['indicator']/uncertainty,error_ratio=after_error/before_error,
   samples_per_assessment=8192,proven_bound=False,native_provenance=c.run.sha(folder/'provenance.json'),
   native_result=c.run.sha(folder/'result.json'),independent_audit=c.run.sha(result)))
assert len(reports)==8,'Both directions on MPI1,2 and4 (M2/M4) are required.'
c.save(c.HERE/'same_field_stage_comparison.json',dict(cases=reports,helper_sha256=c.run.sha(Path(__file__)),
 scope='Actual adapted partitions and one identical corrected original P1/BL field per pair; refinement/coarsening need not improve variance monotonically. These are finite empirical checks, not proven uncertainty bounds.'))
