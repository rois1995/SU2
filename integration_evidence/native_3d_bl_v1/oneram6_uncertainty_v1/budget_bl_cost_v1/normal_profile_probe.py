#!/usr/bin/env python3
"""Independent one-wall/isotropic-endpoint profile experiment; no production policy selection."""
import csv,json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE));from diagnose import statistics
sys.path.insert(0,str(HERE.parent));import run

def main():
 results=[]
 for label in ('global_correction_seed947_v3','global_correction_seed3557_v1'):
  folder=HERE/label/'np4';diagnosis=json.loads((folder/'diagnosis.json').read_text());count=diagnosis['samples']
  rows=[]
  for name,digest in diagnosis['rows_sha256'].items():assert run.sha(folder/name)==digest;rows.extend(csv.DictReader((folder/name).open()))
  rows.sort(key=lambda r:int(r['row']));assert all(r['wall']=='0' for r in rows) and len({r['row'] for r in rows})==len(rows),'One-wall diagnostic only'
  prep=json.loads((HERE.parent/label/'preparation.json').read_text());query=HERE.parent/label/'queries.bin';assert run.sha(query)==prep['queries_sha256']
  ipdf=np.fromfile(query,dtype='<f8',offset=16).reshape(-1,4)[:,3]
  indices=np.array([int(r['row']) for r in rows]);inv=ipdf[indices]
  d=np.array([float(r['distance']) for r in rows]);hn=np.array([float(r['hn']) for r in rows]);weight=np.array([float(r['weight']) for r in rows])
  tangent=np.array([[float(r['ht_min']),float(r['ht_max'])] for r in rows]);h0=diagnosis['first_height'];g=diagnosis['normal_growth']
  core=1e-4;background=core**1.5
  def cost(normal):
   spacing=np.column_stack([normal,tangent]);logs=-weight[:,None]*np.log(spacing)+(1-weight[:,None])*.5*np.log(core)
   return (np.exp(np.maximum(logs,.5*np.log(core)).sum(axis=1))-background)*inv
  old=np.array([float(r['coarsest_weighted_marginal']) for r in rows]);same=cost(hn)
  error=float(np.max(abs(same-old)/np.maximum(1,abs(old))));assert error<5e-6,error
  experiments=[]
  for protected in (h0,5*h0,10*h0,50*h0):
   anchor=max(h0,2*(h0+(g-1)*protected)/(g+1))
   for maximum_growth in (1.5,2.,5.,10.,1e6):
    slope0=2*(g-1)/(g+1);slope1=2*(1-2/(maximum_growth+1))
    trials=[]
    for parameter in np.linspace(0,1,9):
     normal=hn if parameter==0 else np.where(d<=protected,hn,np.maximum(hn,anchor+(slope0+parameter*(slope1-slope0))*(d-protected)))
     assert np.all(normal>=hn*(1-1e-15)) and np.array_equal(normal[d<=protected],hn[d<=protected])
     trials.append(dict(parameter=float(parameter),**statistics(cost(normal),count)))
    experiments.append(dict(protected_distance=protected,maximum_outer_growth=maximum_growth,trials=trials))
  results.append(dict(seed_label=label,maximum_baseline_density_drift=error,experiments=experiments,
    source_diagnosis_sha256=run.sha(folder/'diagnosis.json')))
 report=dict(scope='One faceted wall, constant isotropic HMAX100 endpoint only. Reconstruct eigenvalues from native normal/tangent spacing, preserve original fade/support; anchored outer-normal growth. No original anisotropic-sensor normalization, interacting-wall, construction or automatic selection qualification.',
  background_density=1e-6,background_integral_excluded=True,heuristic_uncertainty=True,proven_bound=False,production_changed=False,
  maximum_growth_1e6='Diagnostic near-limit of the geometric-row slope, not an acceptable physical accuracy choice.',results=results,helper_sha256=run.sha(Path(__file__)))
 (HERE/'normal_profile_probe.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
 for result in results:
  print(result['seed_label'],'baseline drift',result['maximum_baseline_density_drift'])
  for e in result['experiments']:
   if e['maximum_outer_growth'] in (2.,10.,1e6):print(e['protected_distance'],e['maximum_outer_growth'],e['trials'][-1])
if __name__=='__main__':main()
