#!/usr/bin/env python3
"""Independent commuting-metric scalar integral; empirical diagnostics, never a confidence proof."""
import json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
folder=Path(sys.argv[1]).resolve();p=folder/'adaptive_complexity_control.json';data=json.loads(p.read_text())
assert data['unit_cube'] and data['sensor']==data['core']==2
h0,g,T=data['h0'],data['growth'],data['thickness'];full=.9*T;cut=h0/2
assert 0<cut<full<T<.5
plateau=cut/h0
body=plateau+(g+1)/(2*(g-1))*np.log((h0+(g-1)*full)/(h0+(g-1)*cut))
references=[]
for order in (64,128):
 x,w=np.polynomial.legendre.leggauss(order);d=full+(x+1)*(T-full)/2
 height=np.maximum(h0,2*(h0+(g-1)*d)/(g+1));u=(d-full)/(T-full);fade=1-u*u*(3-2*u)
 normal=np.exp(-2*fade*np.log(height)+(1-fade)*np.log(2))
 density=np.sqrt(np.maximum(2,normal));tail=(T-full)/2*float(w@density)
 references.append(float(2*np.sqrt(2)*(1-2*T)+4*(body+tail)))
assert abs(references[1]-references[0])<1e-10
trials=[]
for r in data['trials']:
 error=abs(r['value']-references[-1]);trials.append(dict(r,actual_reference_error=error,
  indicator_covers_observed_error=r['indicator'] is not None and error<=r['indicator']))
result={'reference':references[-1],'order_spread':abs(references[1]-references[0]),'trials':trials,
 'all_observed_indicators_cover':all(r['indicator_covers_observed_error'] for r in trials),'proven_bound':False,
 'input_sha256':run.sha(p),'helper_sha256':run.sha(Path(__file__)),
 'scope':'Unit cube, one complete planar wall at z=.5, sensor2I dominates tangent eigenvalues. Exact full-band integral and two positive Gaussian fade references. Does not calibrate general M6 tails or sequential stopping.'}
(folder/'independent_calibration.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print(json.dumps(result,indent=2))
