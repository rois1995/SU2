#!/usr/bin/env python3
"""Independent finite-cube geometry/P1/profile/edge/current+BDF audit; no CFD-accuracy claim."""
import importlib.util,json,math,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
spec=importlib.util.spec_from_file_location('policy_audit',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def check(folder):
 reports=[]
 for path in sorted(folder.glob('bl_budget_*_profile.json')):
  meta=json.loads(path.read_text());stem=path.name[:-len('_profile.json')];source=folder/(stem+'.json');data=json.loads(source.read_text())
  original,_=audit.base.sources(data);theta=meta['final_sensor_eigenvalue'];parameter=meta['parameter']
  residual=max(abs(float(m[k])-([theta,0,0,theta,0,theta][k])) for cell in original for m in cell['metrics'] for k in range(6))
  assert residual<2e-12,'Oracle requires the declared spatially constant ORIGINAL sensor'
  def eigen(z):
   z=np.asarray(z);h=np.maximum(.1,(.1+.2*z)/1.1);slope=2*.2/2.2+parameter*(2*9/11-2*.2/2.2)
   if parameter:h=np.where(z<=.1,h,np.maximum(h,(.1+.2*.1)/1.1+slope*(z-.1)))
   u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);core=math.log(theta)
   return tuple(np.exp(np.maximum(core,w*value+(1-w)*core)) for value in (math.log(.5),math.log(1.5),-2*np.log(h)))
  def metric(_original,p):
   low,high,normal=(float(x) for x in eigen(float(p[2])));t=(low+high)/2;b=(low-high)/2
   return audit.packed(np.array([[t,b,0],[b,t,0],[0,0,normal]]))
  edge_spreads=[]
  def edge(_original,a,b):
   delta=np.array([float(y-x) for x,y in zip(a,b)]);za,zb=float(a[2]),float(b[2]);cuts=[0.,1.]
   if za!=zb:cuts += [(z-za)/(zb-za) for z in (.05,.1,.27,.3) if 0<(z-za)/(zb-za)<1]
   values=[]
   for order in (32,64):
    x,w=np.polynomial.legendre.leggauss(order);total=0.
    for lower,upper in zip(sorted(cuts),sorted(cuts)[1:]):
     t=(lower+upper)/2+(upper-lower)*x/2;lo,hi,no=eigen(za+t*(zb-za))
     speed=np.sqrt((lo+hi)*.5*(delta[0]**2+delta[1]**2)+(lo-hi)*delta[0]*delta[1]+no*delta[2]**2)
     total+=(upper-lower)/2*float(w@speed)
    values.append(total)
   edge_spreads.append(abs(values[0]-values[1]));assert edge_spreads[-1]<1e-8
   return values[-1]
  exported=folder/(stem+'_independent_mesh.json');exported.write_bytes(source.read_bytes())
  geometry,_=audit.base.frame(exported,True,metric_query=metric,edge_query=edge)
  assert geometry['volume']=='1'
  references=[]
  for order in (32,64,96):
   x,w=np.polynomial.legendre.leggauss(order);total=0.
   for a,b in zip((0.,.05,.1,.27,.3),(.05,.1,.27,.3,1.)):
    z=(a+b)/2+(b-a)*x/2;lo,hi,no=eigen(z);total+=(b-a)/2*float(w@np.sqrt(lo*hi*no))
   references.append(total)
  assert max(references)-min(references)<1e-9
  states=json.loads((folder/(stem+'_states.json')).read_text());before=audit.integrals(data,states['donor'],True);after=audit.integrals(data,states['accepted'])
  defect=max(abs(a-b)/max(1.,abs(a)) for a,b in zip(before,after));assert defect<1e-10
  transport=max(abs(float(x)-y) for row in states['accepted'] for x,y in zip(row['metric'],[theta,0,0,theta,0,theta]));assert transport<2e-12
  projection=json.loads((folder/(stem+'_projection.json')).read_text());assert projection['time_levels']==2
  projection_defect=max(abs(a-b)/max(1.,abs(s)) for a,b,s in zip(projection['new_total'],projection['target_total'],projection['scale']));assert projection_defect<1e-10
  reloads=list(folder.glob(stem+'_written_*_reload_target.json'));assert len(reloads)==2,'SU2 and CGNS round trips required'
  assert all(audit.signature(json.loads(p.read_text()))==audit.signature(data) for p in reloads)
  restart=json.loads((folder/(stem+'_restart_states.json')).read_text());expected={tuple(r['point']):r['states'] for r in restart['donor']}
  assert len(restart['accepted'])==len(expected)
  restart_defect=max(abs(a-b)/max(1.,abs(a)) for r in restart['accepted'] for a,b in zip(expected[tuple(r['point'])],r['states']));assert restart_defect<1e-10
  for level in (1,2):
   _,fields,precision=audit.read_restart(folder/(stem+'_restart_'+str(level).zfill(5)+'.dat'))
   assert precision=='Float64' and np.isfinite(fields['Nu_Tilde']).all() and (fields['Nu_Tilde']>=0).all()
  nodes={n[0]:tuple(map(audit.F,n[1:])) for n in data['nodes']};heights=[]
  for face in data['surface']:
   if any(nodes[n][2]!=0 for n in face['nodes']):continue
   cells=[c for c in data['cells'] if set(face['nodes']).issubset(c['nodes'])];assert len(cells)==1
   apex=nodes[next(n for n in cells[0]['nodes'] if n not in face['nodes'])];heights.append(abs(float(apex[2])/.1-1))
  assert heights and max(heights)<1e-8
  meta.update(independent_geometry_and_full_metric=geometry,independent_reference=references[-1],reference_order_spread=max(references)-min(references),
   observed_estimate_error=abs(meta['mass']-references[-1]),indicator_covers_observed_error=abs(meta['mass']-references[-1])<=meta['indicator'],
   reference_budget_discrepancy=references[-1]/meta['budget']-1,flow_BDF_integral_defect=defect,projection_defect=projection_defect,
   transported_sensor_residual=transport,first_height_error=max(heights),maximum_edge_reference_order_spread=max(edge_spreads,default=0),
   source_sensor_constant_residual=residual,SU2_and_CGNS_reload=True,restart_flow_BDF_defect=restart_defect,SA_restart_fields=True,
   proven_bound=False,independent_helper_sha256=audit.digest(Path(__file__)),
   house_auditor_sha256=audit.digest(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'))
  reports.append(meta);print('AUDITED',stem,'lambda',parameter,'cells',geometry['cells'],'reference',references[-1],flush=True)
 assert len(reports)==2,'Both declared workflow cases must finish'
 (folder/'independent_enabled.json').write_text(json.dumps(reports,indent=2,allow_nan=False)+'\n');return reports
if __name__=='__main__':check(Path(sys.argv[1]).resolve())
