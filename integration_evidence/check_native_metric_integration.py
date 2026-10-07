from pathlib import Path
import hashlib,json,re
root=Path(__file__).resolve().parent.parent;e=root/'integration_evidence'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
build=json.loads((e/'native_metric_integration_build_v4/evidence.json').read_text())
pipeline=json.loads((e/'native_metric_integration_v4/post_frozen_v1.json').read_text())
assert pipeline['phase']=='terminal_pass' and len(pipeline['stages'])==9
assert all(s['exit_code']==0 for s in pipeline['stages'])
assert sha(Path(build['archived_binary']))==build['binary_sha256']
assert all(sha(root/p)==h for p,h in build['source_sha256'].items())
groups=[]
for name in ['native_metric_core_v3','native_metric_output_mpi_v3','native_metric_adapter_v4']:
 p=e/name/'evidence.json';m=json.loads(p.read_text())
 assert [r['ranks'] for r in m['runs']]==[1,2,4]
 assert all(r['verified'] and r['exit_code']==0 for r in m['runs'])
 assert m['binary_sha256']==build['test_driver_sha256'] and sha(Path(m['archived_binary']))==m['binary_sha256']
 assert all(sha(root/p)==h for p,h in m['source_sha256'].items())
 counts={}
 for r in m['runs']:
  successes=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',Path(r['log']).read_text(errors='replace'))
  assert len(successes)==r['ranks'];counts[str(r['ranks'])]=successes
 groups.append({'name':name,'filter':m['filter'],'runs':m['runs'],'assertions_and_cases_per_rank':counts,'source_files_rechecked':len(m['source_sha256']),'evidence_sha256':sha(p)})
record={'scope':'Final reconciled metric app v4 MPI4 cross-grid lifecycles and independent inspections/metric audits. Timings are single workstation observations, not a speedup study. Native scope remains static-polyline primal 2D triangles; no converged-force, native3D/unsteady/CAD claim.','production_source_files_rechecked':len(build['source_sha256']),'solver_sha256':build['binary_sha256'],'test_driver_sha256':build['test_driver_sha256'],'MPI_groups':groups,'cases':[],'sha256':{}}
for kind,name,baseline,seedhash in [('Euler','metricmerge_euler_cross_seed_v1','nativefix_euler_rans_seed_v14','c5adf8b1c155db0692e6221a25a51ff376d70e681a46369266b5685955644712'),('RANS','metricmerge_rans_cross_seed_v2','nativefix_rans_euler_seed_v23','940d8aed5d9ee0d6dc0a9f5b43a3973e7c76ed5d9123188c6e81d048cf6b6d26')]:
 d=e/'rae2822_transonic_v1'/name;old=d.with_name(baseline);run=json.loads((d/'run_evidence.json').read_text());inspection=json.loads((d/'inspection.json').read_text())
 assert run['phase']=='terminal' and run['solver_exit']==0 and run['ranks']==4 and run['binary_sha256']==build['binary_sha256']
 assert sha(d/'input.su2')==seedhash and (d/'input.su2').read_bytes()==(old/'input.su2').read_bytes()
 cfg=(d/'run.cfg').read_text();assert 'ADAP_HESSIAN_NOISE= 0' in cfg;assert 'ADAP_SURFACE= YES' in cfg and 'ADAP_TRANSFER= CONSERVATIVE' in cfg
 assert ('ADAP_BL_' not in cfg) if kind=='Euler' else 'ADAP_BL_FIRST_HEIGHT= ( 1e-5 )' in cfg
 assert [x['cycle'] for x in inspection['cycles']]==[0,1,2]
 for x in inspection['cycles']:
  assert all(g[0]=='PASS' for g in x['validity'].values())
  assert x['exact_restart_mesh_pairing'] and all(x['all_vtu_scalar_pairing'].values())
  assert x['minimum_density']>0 and x['minimum_internal_energy']>0 and x['pressure_consistency_relative_error']<1e-9
  assert all(y['all_declared_features_exactly_retained'] and abs(y['covered_arc_over_period']-1)<1e-10 and y['max_corresponding_parameter_deviation']<=1e-6 for y in x['reference'])
 assert len(set(x['boundary_edges']['AIRFOIL'] for x in inspection['cycles']))==3
 if kind=='Euler':
  targets=json.loads((d/'independent_metric_audit.json').read_text())['cycles']
  for x in targets:assert x['all_exact_positive'] and x['manifold_oriented_edges'] and x['physical_equals_exposed'] and not x['bad_cells']
 else:
  targets=[json.loads((d/f'independent_composite_metric_{c:05d}.json').read_text()) for c in (1,2)]
  for x in targets:assert not x['bad_quality_cells'] and not x['bad_length_edges']
 log=(d/'solver.log').read_text(errors='replace')
 residuals=re.findall(r'residual cells=(\d+), height faces=(\d+), reference faces=(\d+)',log)
 assert residuals==[('0','0','0')]*2
 assert len(re.findall(r'Mesh replaced in [\deE.+-]+ s \(solution transfer [\deE.+-]+ s\)',log))==2
 recovery=re.findall(r'States not admissible after the projection[^\n]*: (\d+); recovered by blending (\d+) patch',log)
 assert recovery==[('0','0')]*2,recovery
 timings=[]
 for fields in re.findall(r'^\|\s*(\d+)\|([^\n]+)',log,re.M):
  values=[s.strip() for s in fields[1].split('|')]
  if len(values)==6 and values[-1]=='':
   try:row={'cycle':int(fields[0]),'points':int(values[1]),'iterations':int(values[2]),'solve_seconds':float(values[3]),'adapt_seconds':None if values[4]=='-' else float(values[4])}
   except ValueError:continue
   timings.append(row)
 assert [x['cycle'] for x in timings]==[0,1,2],timings
 cycles=[]
 for c,target in zip((1,2),targets):
  x=inspection['cycles'][c];h=x['maximum_relative_first_height_error']
  if kind=='RANS':assert h is not None and h<=1e-8
  else:assert h is None
  assert target['min_quality']>=.18-1e-8 and target['max_simpson_length']<=1.8+1e-8
  mesh=d/f'mesh_adap_{c:05d}.su2'
  cycles.append({'cycle':c,'points':x['points'],'triangles':x['triangles'],'min_quality':target['min_quality'],'max_metric_length':target['max_simpson_length'],'first_height_error':h,'boundary_edges':x['boundary_edges'],'density_residual':x['last_density_residual'],'density_criterion_met':x['density_criterion_met'],'reference_deviation':max(y['max_corresponding_parameter_deviation'] for y in x['reference']),'mesh_sha256':sha(mesh),'historical_control_mesh_sha256':sha(old/mesh.name)})
 record['cases'].append({'kind':kind,'case':str(d),'solver_exit':0,'elapsed_seconds':run['elapsed_seconds'],'timings':timings,'solve_seconds':sum(x['solve_seconds'] for x in timings),'adapt_seconds':sum(x['adapt_seconds'] or 0 for x in timings),'maximum_ranks':4,'cycles':cycles,'transfer_seconds':[float(x) for x in re.findall(r'solution transfer ([\deE.+-]+) s',log)],'inadmissible_states_and_recovery_patches_per_transfer':recovery})
 for p in [*[d/name for name in ('independent_metric_audit.json',) if kind=='Euler'],*[d/f'independent_composite_metric_{c:05d}.json' for c in (1,2) if kind=='RANS'],d/'run.cfg',d/'input.su2',d/'run_evidence.json',d/'inspection.json',d/'solver.log',d/'mesh_and_mach.png',d/'surface_cp.png']:
  assert p.is_file();record['sha256'][str(p.relative_to(root))]=sha(p)
 for x in inspection['input_files_sha256']:
  assert sha(Path(x))==inspection['input_files_sha256'][x]
record['sha256']['integration_evidence/native_metric_integration_build_v4/evidence.json']=sha(e/'native_metric_integration_build_v4/evidence.json')
# Older Hessian campaigns cover unchanged recovery sources, not the later BL guard.
hessian_paths=['Common/include/linear_algebra/blas_structure.hpp','Common/include/CConfig.hpp','Common/src/CConfig.cpp','SU2_CFD/include/gradients/computeGradientsLeastSquares.hpp','SU2_CFD/include/gradients/computeHessians.hpp','SU2_CFD/include/gradients/computeHessiansQuadratic.hpp','SU2_CFD/src/solvers/CSolver.cpp']
record['Hessian_groups']=[]
for name in ('native_metric_hessian_v3','native_metric_hessian_mpi_v2'):
 p=e/name/'evidence.json';m=json.loads(p.read_text())
 assert sha(Path(m['archived_binary']))==m['binary_sha256']
 for path in hessian_paths:assert sha(root/path)==m['source_sha256'][path],path
 assert all(r['verified'] and r['exit_code']==0 for r in m['runs'])
 record['Hessian_groups'].append({'name':name,'ranks':[r['ranks'] for r in m['runs']],'unchanged_sources':hessian_paths,'evidence_sha256':sha(p)})
field=json.loads((e/'native_metric_integration_v4/frozen_field_audit.json').read_text())
assert field['noise']==0 and len(field['cases'])==4
assert sha(e/'audit_native_metric_integration.py')==field['auditor_sha256']
for inputs in field['inputs'].values():
 for path,h in inputs.items():assert sha(Path(path))==h
for key,x in field['cases'].items():
 assert x['sensor_nodal']['directed_samples_above_1_plus_1e_5']==0
 assert x['minimum_generalized_sensor_domination_ratio']>=1-1e-6
 seed,method=key.split('/')
 for n,v in x['mpi'].items():
  assert v['maximum_relative_sensor_difference']<1e-9
  case=e/'native_metric_integration_v4/frozen'/seed/(method+'_np'+n);run=json.loads((case/'run_evidence.json').read_text())
  assert run['solver_exit']==0 and run['binary_sha256']==build['binary_sha256']
  assert sha(case/'run.cfg')==run['config_sha256'] and sha(case/'input.su2')==run['mesh_sha256']
  cfg=(case/'run.cfg').read_text();assert re.search(r'^ITER\s*=\s*0\s*$',cfg,re.M) and re.search(r'^ADAP_HESSIAN_NOISE\s*=\s*0\s*$',cfg,re.M)
  assert sha(case/'fields.csv')==v['fields_sha256'] and sha(case/'solver.log')==v['log_sha256']
record['frozen_audit_scope']=field['scope'];record['composed_gradation_certified']=False
quad=json.loads((e/'native_metric_integration_v4/quadrature_audit_1.25.json').read_text())
assert sha(e/'audit_native_metric_quadrature.py')==quad['auditor_sha256']
assert max(abs(v) for x in quad['cases'].values() for v in x['relative_to_reported_target'].values())<2e-4
floor=json.loads((e/'native_metric_integration_v4/minimum_complexity.json').read_text())
assert floor['reported_minimum_composed_complexity']<10000 and all(floor['below_measured_floor'])
record['minimum_composed_complexity_probe']=floor
receipt=json.loads((e/'rae2822_transonic_v1/metricmerge_rans_cross_seed_v2/budget_change.json').read_text());assert not receipt['prepared_only'];record['RANS_budget_change']=receipt
# Incoming findings and rejection evidence are retained byte-for-byte.
import subprocess
incoming='b14a14ea1dec771834d7d691fca27614605df39d'
for rel in ['MESH_ADAPTATION_FINDINGS.md',*[f'integration_evidence/{name}' for name in ('metric_robustness_v2_validation.json','metric_robustness_v3_validation.json','metric_robustness_rejected_schur_v3.json','check_rae2822_metric_constraints.py')]]:
 assert (root/rel).read_bytes()==subprocess.check_output(['git','show',incoming+':'+rel],cwd=root)
 record['sha256'][rel]=sha(root/rel)
p=e/'metric_branch_v3_reference/inventory.json';inventory=json.loads(p.read_text());assert len(inventory['files'])==57
for rel,v in inventory['files'].items():
 raw=p.parent/rel;assert raw.stat().st_size==v['bytes'] and sha(raw)==v['sha256']
record['preserved_raw_incoming_files']=len(inventory['files'])
for name in ['frozen_field_audit.json','quadrature_audit_1.25.json','minimum_complexity.json','coarse_reference_preparation.json','prepared_cases.json','post_frozen_v1.json','final_build_freshness.log']:
 p=e/'native_metric_integration_v4'/name;record['sha256'][str(p.relative_to(root))]=sha(p)
out=e/'native_metric_integration_v4/completion_audit.json'
if out.exists():
 assert json.loads(out.read_text())==json.loads(json.dumps(record)), 'Stored completion audit differs from current evidence'
else:
 out.write_text(json.dumps(record,indent=2)+'\n')
print('Final case, MPI-group, current-source and artifact audit PASS:',out)
