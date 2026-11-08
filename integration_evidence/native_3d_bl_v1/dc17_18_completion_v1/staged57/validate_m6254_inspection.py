"""Independent readback of the saved terminal254 mesh and array associations."""
from pathlib import Path
import hashlib,json,time
import meshio
import numpy as np
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k';D=A/'inspection'
started=time.monotonic();assert json.loads((A/'inspection_export_terminal_handles.json').read_text())['all_owned_handles_terminal']
inputs=[D/n for n in ['rejected_mesh.vtu','rejected_mesh.su2','bad_cells.vtu','cavities.vtu','inspect_manifest.json']]
sha=lambda p:hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
before={str(p):sha(p) for p in inputs};manifest=json.loads(inputs[-1].read_text())
assert manifest['full_mesh_measurements_complete'] and manifest['bad_set_complete'] and manifest['cavity_complete'] and not manifest['partial']
v=meshio.read(inputs[0]);s=meshio.read(inputs[1]);N=len(v.points);tet=v.cells_dict['tetra'];K=len(tet)
assert np.array_equal(v.points,s.points) and np.array_equal(tet,s.cells_dict['tetra'])
assert K==manifest['actual_full_mesh_cells']==741236 and N==manifest['actual_full_mesh_points']
assert len(np.unique(v.point_data['global_node_id']))==N
for key,arr in v.point_data.items():assert len(arr)==N,key
for key,blocks in v.cell_data.items():assert len(blocks)==1 and len(blocks[0])==K,key
assert not {'q','J','geometry_excess_l2'}&v.point_data.keys()
assert {'q','J','geometry_excess_l2','metric_edge_min','metric_edge_max'}<=v.cell_data.keys()
M=v.point_data['captured_nodal_metric'].reshape(N,3,3);assert np.all(v.point_data['captured_nodal_metric_available']==1)
assert np.isfinite(M).all() and np.array_equal(M,M.transpose(0,2,1));eig=np.linalg.eigvalsh(M);assert (eig>0).all()
fields={k:blocks[0] for k,blocks in v.cell_data.items()};assert all(np.isfinite(fields[k]).all() for k in ['q','J','geometry_excess_l2','metric_edge_min','metric_edge_max'])
counts={'q_bad':int((fields['q']<.2).sum()),'j_bad':int((fields['J']<.05).sum()),'geometry_bad':int(fields['geometry_bad'].sum())}
assert counts=={'q_bad':251,'j_bad':205,'geometry_bad':95};assert np.all(fields['cache_valid']==1)
P=v.points;volume=np.einsum('ni,ni->n',np.cross(P[tet[:,1]]-P[tet[:,0]],P[tet[:,2]]-P[tet[:,0]]),P[tet[:,3]]-P[tet[:,0]])/6
assert (volume>0).all()
bad=meshio.read(inputs[2]);cavities=meshio.read(inputs[3]);assert len(bad.cells_dict['tetra'])==manifest['bad_rows_total']
assert set(bad.cell_data['global_cell_id'][0])<=set(cavities.cell_data['global_cell_id'][0])
after={str(p):sha(p) for p in inputs};assert before==after
result={'state':'terminal','exit_code':0,'all_reader_checks_passed':True,'seconds':time.monotonic()-started,'actual_points':N,'actual_cells':K,'measured_bad_counts':counts,'point_tensor_components':9,'q_J_geometry_are_CellData':True,'array_lengths_correct':True,'coordinates_tetrahedra_equal_su2':True,'captured_metric_available_points':N,'minimum_metric_eigenvalue':float(eig.min()),'minimum_positive_volume':float(volume.min()),'bad_cells':len(bad.cells_dict['tetra']),'cavity_cells':len(cavities.cells_dict['tetra']),'input_hashes_before':before,'input_hashes_after':after,'helper_sha256':sha(Path(__file__)),'meshio_version':meshio.__version__,'numpy_version':np.__version__,'scope':'Actual rejected mesh integrity and captured fields;no GUI rendering,strict certificate recomputation,accepted mesh or CFD transfer proof.'}
(A/'inspection_reader_status.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['actual_points','actual_cells','measured_bad_counts','bad_cells','cavity_cells','seconds']}))
