"""Read-only actual441 core allocation and exact selected-core export; the separate complete checkpoint carries the full target."""
from pathlib import Path
import hashlib,json,re,sys,time,struct
import numpy as np
R=Path(__file__).resolve().parent
sys.path.insert(0,str(R.parents[3]/'TestCases/adaptation/capability'))
import capcheck
A=Path(sys.argv[1]).resolve()
INPUTS=Path(sys.argv[2]).resolve()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 start=time.monotonic();status=json.loads((A/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 assert json.loads((A/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
 output=A/'core_allocation_assessment.json';assert not output.exists()
 inputs=INPUTS;provenance=json.loads((inputs/'provenance.json').read_text())
 pins={str(inputs/n):v['sha256'] for n,v in provenance['files'].items()}
 for p,h in pins.items():assert sha(p)==h,p
 mesh=capcheck.read_su2(inputs/'m6.su2');xyz,fields,_=capcheck.read_restart(inputs/'baseline_sa_seed.dat');assert np.array_equal(xyz,mesh.P)
 n=len(xyz);M=np.full((n,3,3),np.nan);mach=np.full(n,np.nan);seen=set();headers=[]
 for rank in range(6):
  p=A/f'saved_cfd_metric_rank{rank}.jsonl';pins[str(p)]=sha(p)
  with p.open() as stream:
   header=json.loads(next(stream));headers.append(header);count=0
   assert header['complete_checkpoint'] is True and header['cfd_advanced'] is False
   for line in stream:
    row=json.loads(line)
    if 'id' not in row:continue
    i=row['id'];assert 0<=i<n and i not in seen;seen.add(i);count+=1
    assert np.array_equal(np.asarray(row['x']),xyz[i]),i
    v=row['core_tensor'];assert len(v)==6
    M[i]=[[v[0],v[1],v[2]],[v[1],v[3],v[4]],[v[2],v[4],v[5]]];mach[i]=row['mach']
  assert count==header['owned_nodes']
 assert len(seen)==n and np.isfinite(M).all() and np.isfinite(mach).all()
 for h in headers:assert all(h[k]==headers[0][k] for k in ('estimated_complexity','indicator','core','tangential_scale','anchor','mach_error'))
 eigen=np.linalg.eigvalsh(M);assert (eigen>0).all()
 rho=fields['Density'];momentum2=sum(fields[k]**2 for k in ('Momentum_x','Momentum_y','Momentum_z'))
 pressure=.4*(fields['Energy']-.5*momentum2/rho);expected=np.sqrt(momentum2/(rho*1.4*pressure))
 err=float(np.max(np.abs(expected-mach)));assert err<1e-9
 weighted=np.sqrt(np.linalg.det(M))*capcheck.dual_volumes(mesh);assert np.isfinite(weighted).all() and (weighted>=0).all()
 total=float(weighted.sum());log=(A/'run.log').read_text();reported=float(re.search(r'Sensor-only donor complexity after gradation: ([0-9.eE+-]+)',log)[1]);assert abs(total-reported)<max(1e-4*total,1e-8)
 radius=np.linalg.norm(xyz,axis=1);regions={}
 for label,mask in [('radius_le_2',radius<=2),('radius_2_20',(radius>2)&(radius<=20)),('radius_gt_20',radius>20)]:
  regions[label]={'nodes':int(mask.sum()),'core_nodal_complexity':float(weighted[mask].sum()),'fraction':float(weighted[mask].sum()/total)}
 geometry=json.loads((R/'geometry369_assessment.json').read_text());samples=[]
 for sample in geometry['points']:
  i=sample['vertex'];assert np.linalg.norm(xyz[i]-sample['point'])<1e-12
  spacing=[]
  for d in sample['directions']:
   d=np.asarray(d);assert abs(d@d-1)<1e-10
   spacing.append(float(1/np.sqrt(d@M[i]@d)))
  samples.append({'context':sample['context'],'node':i,'point':sample['point'],'principal_curvatures_local':sample['curvatures'],'flow_core_spacing_in_principal_directions':spacing})
 # Existing NativeFrozenVolume3D scalar layout, exact double values; explicitly core-only.
 payload=A/'selected_core_native.bin';assert not payload.exists();dtype=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(9,))]);assert dtype.itemsize==104
 records=np.empty(n,dtype=dtype);records['id']=np.arange(n);records['x']=xyz;records['m']=M.reshape(n,9)
 with payload.open('xb') as f:f.write(struct.pack('<Q',n));records.tofile(f)
 assert payload.stat().st_size==8+104*n
 raw=np.memmap(payload,dtype=dtype,offset=8,shape=(n,),mode='r');assert np.array_equal(raw['m'],M.reshape(n,9)) and np.array_equal(raw['x'],xyz)
 report={'scope':__doc__,'status':status,'metadata':headers[0],'nodes':n,'tetrahedra':len(mesh.E),'mach_error_independent':err,'core_nodal_complexity':total,'core_allocation':regions,'core_eigenvalue_range':[float(eigen.min()),float(eigen.max())],'maximum_core_aspect_ratio':float(np.sqrt(eigen[:,-1]/eigen[:,0]).max()),'actual_surface_samples_flow_contribution_only':samples,'core_native_export':{'path':str(payload),'sha256':sha(payload),'layout':'Existing NativeFrozenVolume3D: uint64 count then globalID/xyz/9double tensor','complete_composed_checkpoint':False},'source_audit':'SA restart refreshes primitives before SetAuxVar_Adapt; runtime conserved values unchanged and Mach comparison passed on each rank.','limits':['Only flow/core contribution; no recreated full BL metric or physical geometry certification.','No constructed mesh, height/progression/coverage/transition or target reasonableness acceptance.','Nodal median-dual core allocation is not an additive decomposition of composed complexity.','Local curvature directions are reused from369 verified original-coordinate samples, not global certified curvature bounds.'],'input_output_sha256':pins,'helper_sha256':sha(__file__),'seconds':time.monotonic()-start}
 for p,h in pins.items():assert sha(p)==h,p
 output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print(json.dumps({k:report[k] for k in ('nodes','tetrahedra','core_nodal_complexity','core_allocation','mach_error_independent','seconds')}))
if __name__=='__main__':main()
