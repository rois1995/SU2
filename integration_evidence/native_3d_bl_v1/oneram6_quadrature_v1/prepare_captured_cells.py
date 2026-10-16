#!/usr/bin/env python3
"""Prepare actual M6 trial-field replays and independent wall-contact diagnostics."""
from pathlib import Path
import contextlib,csv,hashlib,io,json,runpy,sys
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];CASE=HERE.parent/'oneram6_frozen_v2'
with contextlib.redirect_stdout(io.StringIO()):oracle=runpy.run_path(str(HERE/'check_normal_integral.py'))
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
mesh=read_su2(CASE/'Coarse.su2');tri=mesh.P[mesh.M['WING3D']]
# Reuse the exact-triangle oracle with the complete wing, instead of cell13's candidate subset.
g=oracle['distance'].__globals__;g['tri']=tri;g['a']=tri[:,0];g['ab']=tri[:,1]-g['a'];g['ac']=tri[:,2]-g['a']
g['n']=np.cross(g['ab'],g['ac']);g['n']/=np.linalg.norm(g['n'],axis=1)[:,None]
g['d00']=np.einsum('ij,ij->i',g['ab'],g['ab']);g['d01']=np.einsum('ij,ij->i',g['ab'],g['ac']);g['d11']=np.einsum('ij,ij->i',g['ac'],g['ac']);g['det']=g['d00']*g['d11']-g['d01']**2
wall_points=mesh.P[np.unique(mesh.M['WING3D'])];tolerance=64*np.finfo(float).eps*max(np.abs(wall_points).max(),np.ptp(wall_points,axis=0).max())
original=HERE/'cell22_probe/native_wall_unit.dat';lines=original.read_text().splitlines();npoints,nfaces=map(int,lines[1].split());body=lines[:3+npoints+nfaces]
assert body[0]=='NATIVE_BL_REPLAY_3D 2' and len(lines)==len(body)+9
out=HERE/'captured_cells_v1';out.mkdir(exist_ok=False)
for path in sorted(CASE.glob('captured_cell_*.json')):
 captured=json.loads(path.read_text());gid=captured['global_cell'];cell=np.array(captured['coordinates']);sensor=np.array(captured['sensor']);nodes=captured['nodes']
 assert np.array_equal(cell,mesh.P[nodes]) and set(nodes)==set(map(int,mesh.E[gid]))
 assert sensor.shape==(4,3,3) and np.all(np.isfinite(sensor)) and np.all(np.linalg.eigvalsh((sensor+sensor.transpose(0,2,1))*.5)>0)
 folder=out/('cell'+str(gid));folder.mkdir();fixture=folder/'replay.dat'
 fixture.write_text('\n'.join(body+[' '.join(format(x,'.17g') for x in p) for p in cell]+[format(captured['core'],'.17g')]+[' '.join(format(x,'.17g') for x in m.ravel()) for m in sensor])+'\n')
 edges=[(i,j) for i in range(4) for j in range(i+1,4)];faces=[tuple(i for i in range(4) if i!=k) for k in range(4)]
 probes=np.vstack([cell,np.array([(cell[i]+cell[j])*.5 for i,j in edges]),np.array([cell[list(face)].mean(0) for face in faces])])
 distances=oracle['distance'](probes)
 report={'global_cell':gid,'nodes':nodes,'coordinates':cell.tolist(),'core':captured['core'],'complexity_trial':captured['complexity_trial'],'log_scale':captured['log_scale'],'raw_sensor':sensor.tolist(),'maximum_sensor_asymmetry':float(np.max(abs(sensor-sensor.transpose(0,2,1)))),'wall_tolerance':float(tolerance),'vertex_distances':distances[:4].tolist(),'edge_midpoint_distances':distances[4:10].tolist(),'face_centroid_distances':distances[10:].tolist(),'edges':edges,'faces':faces,'vertex_contacts':[i for i,d in enumerate(distances[:4]) if d<=tolerance],'scope':'Actual recorded runtime trial/core/nodal tensors and exact coordinates; complete original native wall orientation retained. Independent point/edge-midpoint/face-centroid distances are diagnostics, not a proof of complete entity contact. Raw roundoff asymmetry is preserved, not symmetrized.','sha256':{'captured':hashlib.sha256(path.read_bytes()).hexdigest(),'fixture':hashlib.sha256(fixture.read_bytes()).hexdigest(),'helper':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'oracle':hashlib.sha256((HERE/'check_normal_integral.py').read_bytes()).hexdigest()}}
 (folder/'input.json').write_text(json.dumps(report,indent=2)+'\n');print('ACTUAL_TRIAL',gid,'vertex distances',report['vertex_distances'],'contacts',report['vertex_contacts'],'max asymmetry',report['maximum_sensor_asymmetry'],flush=True)
