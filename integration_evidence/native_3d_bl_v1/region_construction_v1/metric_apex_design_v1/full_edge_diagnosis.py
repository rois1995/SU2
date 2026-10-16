"""Check whether full curved-edge lengths, rather than wall-apex shape, force sampling."""
import json,math,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField,integrate
from construct import sha
start=time.monotonic();source=HERE.parent/'native_front_constant_v1';field=CylinderField(source,'core_construction_rejected.json');rows=[]
for count in (36,42):
 maximum=0.;worst=None;examples=[]
 for i in range(count):
  a=2*math.pi*i/count;b=2*math.pi*(i+1)/count
  a0=np.array([math.cos(a),math.sin(a),0.]);a1=a0+np.array([0,0,.2]);b0=np.array([math.cos(b),math.sin(b),0.]);b1=b0+np.array([0,0,.2])
  value0,_,_=integrate(field,a0,b1);value1,_,_=integrate(field,b0,a1)
  # The sampler can choose either diagonal, so use the better one on each patch.
  length=min(value0,value1)
  if length>maximum:maximum=length;worst=i
  examples.append([value0,value1])
 rows.append({'angular_intervals':count,'maximum_best_diagonal_full_length':maximum,'worst_patch':worst,'diagonals':examples})
report={'status':'PASS_DIAGNOSIS_NO_APEX_CHANGE_JUSTIFIED','rows':rows,'helper_sha256':sha(Path(__file__)),'field_helper_sha256':sha(HERE.parent/'audit_full_metric.py'),'elapsed_seconds':time.monotonic()-start,
        'scope':'Independent full metric integrals of both surface diagonals around the entire source-cylinder wall. Existing apex shapes already pass at36 intervals, but full curved diagonal lengths do not; metric-nearest apex placement does not remove that edge requirement. No production change or complete mesh claim.'}
assert rows[0]['maximum_best_diagonal_full_length']>1.8 and rows[1]['maximum_best_diagonal_full_length']<=1.8
(HERE/'full_edge_diagnosis.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],[(r['angular_intervals'],r['maximum_best_diagonal_full_length']) for r in rows],flush=True)
