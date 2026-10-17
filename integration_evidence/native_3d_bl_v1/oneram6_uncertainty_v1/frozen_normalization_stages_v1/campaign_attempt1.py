"""Resumable original-family/correction-stage checks; one process campaign, no CFD steps."""
import gzip,importlib.util,json,math,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
import run
ROOT=run.ROOT
old_pins=run.pins
SOURCES=[str(p.relative_to(ROOT)) for folder in ('Common/include/adaptation','SU2_CFD/include/adaptation') for p in (ROOT/folder).glob('CNative*3D.hpp')]
SOURCES += ['Common/src/adaptation/'+name for name in ('CNativeImport3D.cpp','CNativeRemesher3D.cpp','CNativePredicates3D.cpp')]
SOURCES += ['SU2_CFD/include/solvers/CSolver.hpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc',
 'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp',
 'integration_evidence/native_3d_core_v1/audit_coupled.py','integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py',str(Path(__file__).resolve().relative_to(ROOT))]
def pins():return dict(old_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
spec=importlib.util.spec_from_file_location('policy',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
def save(p,x):p.write_text(json.dumps(x,indent=2,allow_nan=False)+'\n')
def execute(name,ranks,filters,env=None):
 folder=HERE/name;folder.mkdir(parents=True,exist_ok=True)
 capsule=folder/'executed_source.json.gz';text=json.dumps({p:(ROOT/p).read_text() for p in sorted(set(SOURCES))},sort_keys=True)
 if capsule.exists():assert gzip.decompress(capsule.read_bytes()).decode()==text,'Changed executed source capsule'
 else:capsule.write_bytes(gzip.compress(text.encode(),mtime=0))
 print('CASE',folder,flush=True)
 run.invoke(folder,env or {},run.OLD/'cell329/replay.dat',ranks,test_filter=filters)
 return folder

def integral(data,order):
 """Independent volume Duffy quadrature of ORIGINAL P1 sensor only."""
 original,_=audit.base.sources(data);points,weights=np.polynomial.legendre.leggauss(order);points=(points+1)/2;weights=weights/2
 u,v,w=np.meshgrid(points,points,points,indexing='ij');a,b,c=np.meshgrid(weights,weights,weights,indexing='ij')
 lam=np.array([1-u,u*(1-v),u*v*(1-w),u*v*w]).reshape(4,-1).T
 jac=(a*b*c*u*u*v).ravel();total=0.
 for cell in original:
  p=np.array(cell['points'],dtype=float);m=np.array([audit.matrix(x) for x in cell['metrics']]);tensor=np.einsum('nk,kij->nij',lam,m)
  total+=abs(np.linalg.det((p[1:]-p[0]).T))*float(jac@np.sqrt(np.linalg.det(tensor)))
 return total

def original(folder):
 result=json.loads((folder/'original_family_result.json').read_text());data=json.loads((folder/'original_family_beta.json').read_text())
 refs=[integral(data,n) for n in (24,40)]
 assert abs(refs[0]-refs[1])<max(1.,abs(refs[-1]))*1e-7,refs
 error=abs(refs[-1]-result['corrected_mass']);assert error<=result['corrected_indicator'],(refs,result)
 result.update(independent_sensor_reference=refs[-1],reference_order_spread=abs(refs[0]-refs[1]),observed_absolute_error=error,proven_bound=False)
 save(folder/'independent_original_family.json',result);return result

def planar_reference(theta,order):
 assert theta>=1.5,'Use the general tensor oracle for smaller tangential demand.'
 x,w=np.polynomial.legendre.leggauss(order);total=0.
 for a,b in ((0.,.05),(.05,.27),(.27,.3),(.3,1.)):
  z=(a+b)/2+(b-a)*x/2;h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);fade=1-u*u*(3-2*u)
  density=theta**1.5*np.exp(fade*(-np.log(h)-.5*math.log(theta)))
  total+=(b-a)/2*float(w@density)
 return total

def runtime(folder):
 reports=[]
 for path in sorted(folder.glob('normalization_m*_a[0-9]*_normalization.json')):
  meta=json.loads(path.read_text());stem=path.name[:-len('_normalization.json')];mesh=folder/(stem+'.json');data=json.loads(mesh.read_text())
  audit_mesh=folder/(stem+'_accepted_mesh.json');payload=mesh.read_bytes()
  if audit_mesh.exists():assert audit_mesh.read_bytes()==payload
  else:audit_mesh.write_bytes(payload)
  # The house auditor exports beside its JSON; preserve the actual original input mesh.
  measured,_=audit.base.frame(audit_mesh,True,metric_query=audit.target,edge_query=audit.edge);assert measured['volume']=='1'
  original,_=audit.base.sources(data);theta=meta['final_sensor_eigenvalue'];refs=[planar_reference(theta,n) for n in (32,64,96)]
  assert max(refs)-min(refs)<1e-9;error=abs(meta['mass']-refs[-1]);assert error<=meta['indicator'],(meta,refs)
  states=json.loads((folder/(stem+'_states.json')).read_text());before=audit.integrals(data,states['donor'],True);after=audit.integrals(data,states['accepted'])
  defect=max(abs(a-b)/max(1.,abs(a)) for a,b in zip(before,after));assert defect<1e-10
  transport=max(abs(x-y) for row in states['accepted'] for x,y in zip(row['metric'],[theta,0,0,theta,0,theta]));assert transport<2e-13
  projection=json.loads((folder/(stem+'_projection.json')).read_text());assert projection['time_levels']==2
  projection_defect=max(abs(a-b)/max(1.,abs(s)) for a,b,s in zip(projection['new_total'],projection['target_total'],projection['scale']));assert projection_defect<1e-10
  restart=json.loads((folder/(stem+'_restart_states.json')).read_text());expected={tuple(r['point']):r['states'] for r in restart['donor']}
  assert len(restart['accepted'])==len(expected)
  restart_defect=max(abs(a-b)/max(1.,abs(a)) for r in restart['accepted'] for a,b in zip(expected[tuple(r['point'])],r['states']));assert restart_defect<1e-10
  for level,factor in ((2,1.),(1,.9375)):
   points,fields,precision=audit.read_restart(folder/(stem+'_restart_'+str(level).zfill(5)+'.dat'))
   scale=2*projection['donor_total'][5]*factor;assert scale>0 and precision=='Float64'
   assert np.isfinite(fields['Nu_Tilde']).all() and (fields['Nu_Tilde']>=0).all()
   assert float(np.max(abs(fields['Nu_Tilde']-scale*points[:,2])))/scale<2e-10
  reloads=list(folder.glob(stem+'_written_*_reload_target.json'));assert len(reloads)==2
  assert all(audit.signature(json.loads(p.read_text()))==audit.signature(data) for p in reloads)
  nodes={n[0]:tuple(map(audit.F,n[1:])) for n in data['nodes']};heights=[]
  for face in data['surface']:
   if any(nodes[n][2]!=0 for n in face['nodes']):continue
   cells=[c for c in data['cells'] if set(face['nodes']).issubset(c['nodes'])];assert len(cells)==1
   apex=nodes[next(n for n in cells[0]['nodes'] if n not in face['nodes'])];heights.append(abs(float(apex[2])/.1-1))
  assert heights and max(heights)<1e-8
  meta.update(geometry_and_full_field=measured,reference=refs[-1],reference_spread=max(refs)-min(refs),observed_mass_error=error,
   relative_budget_discrepancy=meta['mass']/20-1,reference_budget_discrepancy=refs[-1]/20-1,flow_BDF_integral_defect=defect,
   projection_all_field_defect=projection_defect,transport_error=transport,first_height_error=max(heights),SU2_and_CGNS_reload=True,restart_flow_defect=restart_defect,SA_restart_fields=True,proven_bound=False)
  reports.append(meta)
 assert len(reports)==2,'Both declared initialization-error directions must finish.'
 save(folder/'independent_normalization.json',reports);return reports

def main():
 report={}
 folder=execute('pilot_original_np1',1,'[NativeDensity3D],[NativeMetricFamily3D]');report['original_np1']=original(folder)
 folder=execute('pilot_runtime_np1',1,'[NativeNormalizationRuntime3D]',{'SU2_NATIVE_NORMALIZATION_WORKERS':'1'});report['runtime_np1']=runtime(folder)
 save(HERE/'pilot_summary.json',report)
 if '--pilot' in sys.argv:return
 for n in (2,4):
  folder=execute('original_np%d'%n,n,'[NativeMetricFamily3D]');report['original_np%d'%n]=original(folder)
  for key in ('corrected_mass','corrected_indicator','scale_after'):
   assert math.isclose(report['original_np1'][key],report['original_np%d'%n][key],rel_tol=2e-11,abs_tol=1e-12),key
 for n,m in ((2,2),(4,2),(4,4)):
  key='runtime_np%d_m%d'%(n,m);folder=execute(key,n,'[NativeNormalizationRuntime3D]',{'SU2_NATIVE_NORMALIZATION_WORKERS':str(m)});report[key]=runtime(folder)
 execute('affected_np4',4,'[NativeBridge3D],[NativeCurvedRuntime3D],[NativeReassessment3D]')
 save(HERE/'summary.json',report)
if __name__=='__main__':main()
