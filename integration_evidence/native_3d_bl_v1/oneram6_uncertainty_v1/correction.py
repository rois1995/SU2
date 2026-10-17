#!/usr/bin/env python3
"""Compare full-density and BL-correction quadrature on the SAME original cells; no CFD."""
import json
from pathlib import Path
import numpy as np
from run import HERE,OLD,invoke,sha

def main():
 folder=HERE/'correction_comparison_v1';folder.mkdir(exist_ok=True)
 report={'scope':'Three original M6 fields, identical absolute allocation and geometry kernel; no full-field or lifecycle claim',
         'absolute_allocation':.001,'maximum_density_queries_per_estimate':8192,'cells':[]}
 for cell in (88,157,329):
  case=folder/('cell%d'%cell);fixture=OLD/('cell%d'%cell)/'replay.dat'
  r=invoke(case,{'SU2_NATIVE_BL_CORRECTION':'1','HELPER_SHA256':sha(Path(__file__))},fixture)
  result=json.loads((case/'correction.json').read_text());assert len(result['runs'])==4
  for offset in (0,2):
   a,b=result['runs'][offset:offset+2]
   for key in ('value','indicator','evaluations'):assert a[key]==b[key],(cell,key,a,b)
   assert a['warm']==False and b['warm']==True and a['decomposed']==(offset==2)
  # Independently integrate sensor-only P1 with higher-order tensor-Gauss; no wall/adaptive rule reused.
  lines=fixture.read_text().splitlines();sensors=np.array([[float(x) for x in line.split()] for line in lines[-4:]]).reshape(4,3,3)
  coords=np.array([[float(x) for x in line.split()] for line in lines[-9:-5]])
  vol=abs(np.linalg.det((coords[1:]-coords[0]).T))/6
  x,w=np.polynomial.legendre.leggauss(12);x=(x+1)/2;w=w/2
  u,v,z=np.meshgrid(x,x,x,indexing='ij');wu,wv,wz=np.meshgrid(w,w,w,indexing='ij')
  beta=np.stack([1-u,u*(1-v),u*v*(1-z),u*v*z],axis=-1)
  tensors=np.einsum('...k,kij->...ij',beta,sensors);sign,ld=np.linalg.slogdet(tensors);assert np.all(sign>0)
  sensor_ref=float(np.sum(6*vol*u*u*v*wu*wv*wz*np.exp(ld/2),dtype=np.longdouble))
  background_error=abs(result['background']-sensor_ref)
  refs=HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell);order=72 if cell==157 else 128
  ref=float(np.mean([json.loads((refs/('order%d_apex%d'%(order,a))/'result.json').read_text())['integral'] for a in (0,3)]))
  for row in result['runs']:
   row['absolute_reference_error']=abs(row['value']-ref);row['relative_reference_error']=abs(row['value']/ref-1)
  result.update(cell=cell,reference=ref,sensor_reference_order12=sensor_ref,background_absolute_error=background_error,
                provenance_sha256=r['provenance_sha256'],output_sha256=sha(case/'correction.json'))
  report['cells'].append(result);print('CORRECTION',cell,json.dumps(result),flush=True)
 (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
