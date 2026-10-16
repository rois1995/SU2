#!/usr/bin/env python3
"""Qualify the revised scalar fan against independently converged original-field integrals."""
import json
from pathlib import Path
import numpy as np
from run import HERE,OLD,invoke,sha
summary=[]
for cell in (88,157,329):
 order=72 if cell==157 else 128
 references=[HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell)/('order%d_apex%d'%(order,a))/'result.json' for a in (0,3)]
 values=[json.loads(p.read_text())['integral'] for p in references];reference=float(np.mean(np.array(values,dtype=np.longdouble)))
 for depth in (0,2,4):
  e=invoke(HERE/'pilot_fan_v1'/('cell%d_depth%d'%(cell,depth)),
           {'SU2_NATIVE_BL_COMPLEXITY':'8192','SU2_NATIVE_BL_SUBDIVISIONS':str(depth),'SU2_NATIVE_PILOT_DRIVER_SHA':sha(Path(__file__).resolve())},
           OLD/('cell%d'%cell)/'replay.dat')['estimate']
  e.update(cell=cell,depth=depth,reference=reference,reference_sha256=[sha(p) for p in references],
           actual_error=abs(e['value']-reference),relative_error=abs(e['value']/reference-1))
  if e['indicator'] is not None:e['indicator_to_observed_error']=e['indicator']/max(e['actual_error'],1e-30)
  summary.append(e)
(HERE/'pilot_fan_summary.json').write_text(json.dumps(summary,indent=2)+'\n')
