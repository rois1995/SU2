"""Counterfactual coarse partition versus actual accepted RANS partition, same original P1/BL."""
import csv,importlib.util,json,math,struct,sys
from pathlib import Path
import numpy as np
import campaign as c
run=c.run;ROOT=c.ROOT;BASE=c.BASE;folder=BASE/'accepted_partition_control'
source_json=BASE/'mpi4_m2/policy_m2_s0.json';fine_json=BASE/'mpi4_m2/reassessment_policy_m2_stage0.json'
assert json.loads((BASE/'mpi4_m2/result.json').read_text())['exit_code']==0
fine=json.loads(fine_json.read_text());data=json.loads(source_json.read_text());folder.mkdir(exist_ok=True)
rows=np.empty(len(data['original']),dtype=c.DOMAIN)
for i,original in enumerate(data['original']):
 vertices=original['vertices'];rows[i]['id']=i;rows[i]['nodes']=[v[0] for v in vertices];rows[i]['coordinates']=[v[1:4] for v in vertices]
 for j,v in enumerate(vertices):
  xx,xy,xz,yy,yz,zz=v[4];rows[i]['sensor'][j]=[[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]]
core=float(np.linalg.eigvalsh(rows['sensor']).min());assert np.max(abs(rows['sensor']-np.eye(3)))<2e-14
source=folder/'original.bin';payload=struct.pack('<QQd',0x4e41545352433344,len(rows),core)+rows.tobytes()
if source.exists():assert source.read_bytes()==payload
else:source.write_bytes(payload)
fixture=folder/'wall.dat';body=['NATIVE_BL_REPLAY_3D 2','4 2','.1 1.2 .3','0 0 0','1 0 0','1 1 0','0 1 0','0 1 2','0 2 3']
body+=[' '.join(format(float(v),'.17g') for v in p) for p in rows[0]['coordinates']]+[format(core,'.17g')]
body+=[' '.join(format(float(v),'.17g') for v in matrix.ravel()) for matrix in rows[0]['sensor']]
text='\n'.join(body)+'\n'
if fixture.exists():assert fixture.read_text()==text
else:fixture.write_text(text)
old=run.pins
run.pins=lambda:dict(old(),**{str(Path(__file__).resolve().relative_to(ROOT)):run.sha(Path(__file__).resolve())})
prep=folder/'coarse';c.inputs(prep,[fixture,source,source_json,fine_json])
run.invoke(prep,{'SU2_NATIVE_BL_CELL_CARRIER':str(source),'SU2_NATIVE_BL_CARRIER_SAMPLES':'8192','SU2_NATIVE_BL_PROPOSAL_SEED':'20261017'},fixture)
query=prep/'native_queries.bin';q=np.fromfile(query,dtype='<f8',offset=16).reshape(-1,4);assert len(q)==8192
result=prep/'density';c.inputs(result,[fixture,source,query])
run.invoke(result,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(query)},fixture)
values=c.records(result);y=np.array([np.longdouble(v['composed']) for v in values])*q[:,3]
value=float(y.mean());sem=float(y.std(ddof=1)/np.sqrt(len(y)));block=float(np.array([y[i::8].mean() for i in range(8)]).std(ddof=1)/np.sqrt(8))
reference=json.loads((BASE/'summary.json').read_text())['accepted_planar_RANS_mass']['reference']
coarse=dict(value=value,point_SE=sem,block_SE=block,indicator=3*max(sem,block),absolute_error=abs(value-reference),cells=len(rows))
fine.update(absolute_error=abs(fine['value']-reference),cells=len(data['cells']))
# The existing independent audit supplies the reference; these calls additionally compare the actual replay field.
spec=importlib.util.spec_from_file_location('oracle',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle);original,_=oracle.base.sources(data)
selection=np.linspace(0,len(q)-1,32,dtype=int);error=0.
for i in selection:
 p=tuple(oracle.F(float(v)) for v in q[i,:3]);rho=math.sqrt(np.linalg.det(oracle.matrix(oracle.target(original,p))))
 error=max(error,abs(float(values[i]['composed'])/rho-1))
assert error<1e-10
report=dict(status='PASS_SAME_FIELD_ACTUAL_ACCEPTED_PARTITION_COMPARISON',reference=reference,coarse=coarse,fine=fine,
 indicator_ratio=fine['indicator']/coarse['indicator'],observed_error_ratio=fine['absolute_error']/coarse['absolute_error'],
 independent_composed_density_checks=len(selection),maximum_relative_composed_difference=error,
 original_source_sha256=run.sha(source),accepted_mesh_source_sha256=run.sha(source_json),proven_bound=False,
 scope='Accepted planar RANS SA N4/M2 cube only. Complete M6 construction and general thin/curved/overlapping BL remain unqualified.')
c.save(folder/'comparison.json',report);print(json.dumps(report,indent=2),flush=True)
