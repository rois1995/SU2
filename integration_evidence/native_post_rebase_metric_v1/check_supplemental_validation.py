"""Supplement original-reference RAE and fixed-point output checks after bounded smokes."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
sys.path.insert(0,str(root/'integration_evidence'))
from airfoil_reference_audit import reference_audit,original_wall
from audit_native_unsteady import capcheck,read_mesh,state
out=root/'integration_evidence/native_post_rebase_metric_v1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
record=dict(status='PASS',scope='Original-reference geometry plus fixed-point output/positivity; no fixed-point discarded donor-target or transferred-history certificate')
rae=out/'rae-steady-smoke-n4m3'; original=out/'frozen-12000/rae/green_gauss-mpi1/original_reference.su2'
reference=reference_audit(original,rae/'mesh_adap_00001.su2')
for row in reference:
 assert row['all_declared_features_exactly_retained']
 assert abs(row['covered_arc_over_period']-1)<1e-12
 assert row['max_corresponding_parameter_deviation']<=1e-6+1e-12
 assert row['max_vertex_projection_distance']<=1e-6+1e-12
p,e,_=original_wall(rae/'input.su2');pp,ee,_=original_wall(rae/'mesh_adap_00001.su2')
assert np.array_equal(p,pp) and np.array_equal(e,ee)
record['rae_original_reference']=reference
fp=out/'vortex-fixed-point-mmg-n4-v2'; meshpath=fp/'mesh_00000.su2'
mesh=read_mesh(meshpath);old=read_mesh(fp/'input.su2')
gates=dict(**capcheck.check_validity(mesh),**capcheck.check_markers(old,mesh),**capcheck.check_geometry(old,mesh,False,1e-8))
assert all(value[0]=='PASS' for value in gates.values()),gates
assert 'ADAP_REMESHER= MMG' in (fp/'run.cfg').read_text()
assert len(list(fp.glob('mesh_*.su2')))==1
assert sorted(p.name for p in fp.glob('flow_*.vtu'))==[f'flow_{i:05d}.vtu' for i in range(3)]
assert sorted(p.name for p in fp.glob('solution_*.dat'))==[f'solution_{i:05d}.dat' for i in range(3)]
checks=[]
for i in range(3):
 restart,_=state(mesh,fp/f'solution_{i:05d}.dat');vtu,positive=state(mesh,fp/f'flow_{i:05d}.vtu')
 assert np.max(np.abs(restart-vtu))<=1e-12*max(1,float(np.max(np.abs(restart))))
 checks.append(dict(step=i,**positive))
log=(fp/'solver.log').read_text()
assert log.count('discarded (screen output only).')==1 and log.count('accepted (files and history).')==1
assert '2 window solves over 1 windows' in log and 'Exit Success' in log
record['fixed_point']=dict(points=len(mesh.P),triangles=len(mesh.E),mesh_gates=gates,accepted_steps=checks)
paths=[original,rae/'input.su2',rae/'mesh_adap_00001.su2',Path(str(rae/'mesh_adap_00001.su2')+'.native_ref'),*fp.iterdir()]
record['input_sha256']={str(p.relative_to(root)):sha(p) for p in paths if p.is_file()}
record['checker_sha256']=sha(Path(__file__))
receipt=out/'supplemental_validation.json';assert not receipt.exists()
receipt.write_text(json.dumps(record,indent=2)+'\n')
print('PASS: immutable RAE wall reference; fixed-point topology, geometry, accepted step pairing/positivity and discard policy. No discarded donor-target claim.')
