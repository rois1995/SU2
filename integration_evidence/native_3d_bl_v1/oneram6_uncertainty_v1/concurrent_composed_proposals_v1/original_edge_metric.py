#!/usr/bin/env python3
"""Inspect the original witness's sensor demand and conservative wall-support separation."""
import hashlib,json,sys
from fractions import Fraction
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
source=HERE/'frozen_m6_np4';folder=HERE/'original_edge_witness';w=json.loads((folder/'witness.json').read_text());mesh=read_su2(source/'Coarse.su2')
dtype=np.dtype([('id','<u8'),('coordinates','<f8',(3,)),('sensor','<f8',(3,3))]);data=np.fromfile(source/'selected_sensor.bin',dtype=dtype,offset=8)
assert np.array_equal(data['id'],np.arange(len(mesh.P))) and np.array_equal(data['coordinates'],mesh.P)
a,b=w['edge'];direction=mesh.P[b].astype(np.longdouble)-mesh.P[a];ma=data['sensor'][a].astype(np.longdouble);mb=data['sensor'][b].astype(np.longdouble)
A=direction@ma@direction;B=direction@mb@direction;assert A>0 and B>0
length=float((np.longdouble(2)/3)*(A+B+np.sqrt(A*B))/(np.sqrt(A)+np.sqrt(B)))
lo=mesh.P[w['original_nodes']].min(axis=0);hi=mesh.P[w['original_nodes']].max(axis=0)
# Parse only the named existing layer tuples; no case-specific wall name or solver rule.
config={}
for line in (source/'run.cfg').read_text().splitlines():
 line=line.split('%',1)[0]
 if '=' in line:
  k,v=line.split('=',1);config[k.strip()]=v.strip()
markers=[x.strip() for x in config['ADAP_BL_MARKER'].strip('() ').split(',')]
thickness=[float(x) for x in config['ADAP_BL_THICKNESS'].strip('() ').split(',')];assert len(markers)==len(thickness)
wall_rows=[]
for name,T in zip(markers,thickness):
 points=mesh.P[np.unique(mesh.M[name])];wall_lo,wall_hi=points.min(axis=0),points.max(axis=0)
 gaps=[max(Fraction(float(wall_lo[k]))-Fraction(float(hi[k])),Fraction(float(lo[k]))-Fraction(float(wall_hi[k])),Fraction(0)) for k in range(3)]
 wall_rows.append({'marker':name,'thickness':T,'wall_min':wall_lo.tolist(),'wall_max':wall_hi.tolist(),
  'coordinate_gaps':list(map(float,gaps)),'entire_witness_box_beyond_thickness':any(x>Fraction(T) for x in gaps)})
result={'scope':'Original input witness; not proof of late cavity equivalence or metric feasibility',
 'wall_box_separation':wall_rows,'all_configured_walls_separated':all(x['entire_witness_box_beyond_thickness'] for x in wall_rows),
 'sensor_only_original_edge_metric_length':length,'edge_limit':1.8,'edge_exceeds_limit':length>1.8,
 'node_sensor_eigenvalues':{str(i):np.linalg.eigvalsh(data['sensor'][i]).tolist() for i in w['original_nodes']},
 'witness_sha256':hashlib.sha256((folder/'witness.json').read_bytes()).hexdigest(),
 'sensor_sha256':hashlib.sha256((source/'selected_sensor.bin').read_bytes()).hexdigest(),
 'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(folder/'metric_diagnosis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
