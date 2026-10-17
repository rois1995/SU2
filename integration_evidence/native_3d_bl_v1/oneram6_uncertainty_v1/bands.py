#!/usr/bin/env python3
"""Qualify scalar distance bands against strict controls and exact saved seed failures; no CFD."""
import json,sys
from pathlib import Path
import numpy as np
from run import HERE,OLD,invoke,sha,pins
sys.path.insert(0,str(HERE.parent/'oneram6_density_reference_v1'))
import reference

def fixed_reference(folder,fixture,order,apex):
 folder.mkdir(parents=True,exist_ok=True)
 identity={'producer':pins(),'helper_sha256':sha(Path(__file__)),'point_runner_sha256':sha(Path(reference.__file__)),
           'fixture_sha256':sha(fixture),'order':order,'apex':apex,'numpy':np.__version__}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Changed independent reference identity'
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 result=folder/'result.json'
 if result.exists():
  saved=json.loads(result.read_text());assert saved['provenance_sha256']==sha(proof)
  assert saved['call']['log_sha256']==sha(folder/'work/run.log')
  return saved
 x,w=np.polynomial.legendre.leggauss(order);x=(x+1)/2;w=w/2
 u,v,z=np.meshgrid(x,x,x,indexing='ij');wu,wv,wz=np.meshgrid(w,w,w,indexing='ij')
 canonical=np.stack([1-u,u*(1-v),u*v*(1-z),u*v*z],axis=-1).reshape(-1,4)
 beta=np.empty_like(canonical);beta[:,[apex]+[k for k in range(4) if k!=apex]]=canonical
 lines=fixture.read_text().splitlines();coords=np.array([[float(a) for a in line.split()] for line in lines[-9:-5]],dtype=np.longdouble)
 volume=abs(np.dot(coords[1]-coords[0],np.cross(coords[2]-coords[0],coords[3]-coords[0])))/6
 weights=(6*volume*u*u*v*wu*wv*wz).ravel()
 values,call=reference.invoke(folder/'work',fixture,beta)
 integral=float(np.sum(weights*values[:,4],dtype=np.longdouble))
 first=np.sum(weights[:,None]*values[:,:4],axis=0)/volume
 moments=max(float(abs(np.sum(weights,dtype=np.longdouble)/volume-1)),float(np.max(abs(first-.25))))
 assert moments<2e-10 and integral>0
 saved={'order':order,'apex':apex,'integral':integral,'points':order**3,'volume':float(volume),'moment_error':moments,
        'provenance_sha256':sha(proof),'call':call,'rigorous_bound':False}
 result.write_text(json.dumps(saved,indent=2)+'\n')
 for name in ('queries.f64','point_values.f64'):(folder/'work'/name).unlink()
 print('REFERENCE',fixture.name,order,apex,integral,flush=True)
 return saved

def main():
 folder=HERE/'distance_band_scalar_v1';folder.mkdir(exist_ok=True)
 tag={'HELPER_SHA256':sha(Path(__file__))}
 invoke(folder/'controls',tag,OLD/'cell329/replay.dat',test_filter='[NativeChordEnvelope],[NativeComplexityEstimate3D]')
 summary={'scope':'Distance-band scalar quadrature of unchanged original fields; no full-field normalization or accepted mesh',
          'original_cells':[],'new_seed_failures':[]}
 for cell in (88,157,329):
  case=folder/('cell%d'%cell);invoke(case,dict(tag,SU2_NATIVE_BL_CORRECTION='1'),OLD/('cell%d'%cell)/'replay.dat')
  data=json.loads((case/'correction.json').read_text());refs=HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell);order=72 if cell==157 else 128
  ref=sum(json.loads((refs/('order%d_apex%d'%(order,a))/'result.json').read_text())['integral'] for a in (0,3))/2
  for offset in (0,2):
   for key in ('value','indicator','evaluations'):assert data['runs'][offset][key]==data['runs'][offset+1][key]
  for row in data['runs']:row['absolute_reference_error']=abs(row['value']-ref)
  data.update(cell=cell,reference=ref,output_sha256=sha(case/'correction.json'));summary['original_cells'].append(data)
  print('BANDS',cell,json.dumps(data),flush=True)
 for cell in (622,2331):
  fixture=folder/'inputs'/('cell%d.dat'%cell)
  seed=invoke(folder/('cell%d_seed'%cell),dict(tag,SU2_NATIVE_BL_COMPLEXITY='8192',SU2_NATIVE_BL_ABSOLUTE='1e300'),fixture)['estimate']
  bounded=invoke(folder/('cell%d_bounded'%cell),dict(tag,SU2_NATIVE_BL_COMPLEXITY='8192'),fixture)['estimate']
  refs=[fixed_reference(folder/('cell%d_reference'%cell)/('order%d_apex%d'%(order,apex)),fixture,order,apex) for order in (24,48) for apex in (0,3)]
  finest=[r['integral'] for r in refs if r['order']==48];center=sum(finest)/2
  item={'cell':cell,'seed':seed,'bounded':bounded,'references':refs,'finest_chart_span_relative':abs(finest[0]-finest[1])/center,
        'last_order_changes_relative':[abs(refs[k+2]['integral']-refs[k]['integral'])/center for k in (0,1)],
        'reference_is_proven_bound':False}
  for estimate in (seed,bounded):estimate['difference_from_finest_mean']=abs(estimate['value']-center)
  summary['new_seed_failures'].append(item);print('NEW_CELL',json.dumps(item),flush=True)
 (folder/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
if __name__=='__main__':main()
