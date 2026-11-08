"""Independent full-topology readback, preserving actual unknown cached q/J."""
from pathlib import Path
import hashlib,json,math,time
import meshio
import numpy as np
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k';D=A/'inspection'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
started=time.monotonic();assert not (A/'inspection_reader_status.json').exists()
assert json.loads((A/'inspection_export_terminal_handles.json').read_text())['all_owned_handles_terminal']
export=json.loads((A/'inspection_export_status.json').read_text());assert export['exit_code']==0 and export['pins_verified']
ready=json.loads((A/'capture_ready.json').read_text());assert ready['owned_rows']==ready['expected_owned_rows']
inputs=[D/n for n in ('rejected_mesh.vtu','rejected_mesh.su2','bad_cells.vtu','cavities.vtu','inspect_manifest.json')]+[A/'capture_ready.json',Path(__file__)]
before={str(p):sha(p) for p in inputs};manifest=json.loads((D/'inspect_manifest.json').read_text())
assert manifest['final_mesh_complete'] and manifest['final_su2_complete'] and manifest['captured_nodal_metric_complete']
assert not manifest['final_mesh_accepted'] and not manifest['missing_ranks']
assert manifest['bad_cells_omitted']==0 and manifest['cavity_cells_omitted']==0
u=manifest['unknown_metric_rows'];assert u>0 and manifest['partial'] and not manifest['full_mesh_measurements_complete']
assert set(manifest['partial_reasons'])=={'rank 5 metrics source is incomplete',f'{u} cells have unknown metric cache'}
v=meshio.read(D/'rejected_mesh.vtu');s=meshio.read(D/'rejected_mesh.su2');P=v.points;tet=v.cells_dict['tetra'];N=len(P);K=len(tet)
assert np.array_equal(P,s.points) and np.array_equal(tet,s.cells_dict['tetra'])
assert K==ready['owned_rows']==manifest['actual_full_mesh_cells']==manifest['full_mesh_exported_cells']
assert N==manifest['actual_full_mesh_points'] and len(np.unique(v.point_data['global_node_id']))==N
for name,array in v.point_data.items():assert len(array)==N,name
for name,blocks in v.cell_data.items():assert len(blocks)==1 and len(blocks[0])==K,name
assert not {'q','J','geometry_excess_l2'}&v.point_data.keys()
assert {'q','J','geometry_excess_l2','metric_edge_min','metric_edge_max'}<=v.cell_data.keys()
M=v.point_data['captured_nodal_metric'].reshape(N,3,3);assert np.all(v.point_data['captured_nodal_metric_available']==1)
assert np.isfinite(M).all() and np.array_equal(M,M.transpose(0,2,1));eig=np.linalg.eigvalsh(M);assert (eig>0).all()
f={name:blocks[0] for name,blocks in v.cell_data.items()};ids=f['global_cell_id'];assert len(np.unique(ids))==K and np.all(f['cache_valid']==1)
for name in ('q','J','geometry_excess_l2','metric_edge_min','metric_edge_max'):assert not np.isinf(f[name]).any()
assert np.isfinite(f['geometry_excess_l2']).all()
unknown=np.isnan(f['q'])|np.isnan(f['J']);assert int(unknown.sum())==u
assert not np.any((np.isnan(f['metric_edge_min'])|np.isnan(f['metric_edge_max']))&~unknown)
counts={'known_q_bad':int((f['q']<.2).sum()),'known_j_bad':int((f['J']<.05).sum()),'geometry_bad':int(f['geometry_bad'].sum()),'q_unknown':int(np.isnan(f['q']).sum()),'j_unknown':int(np.isnan(f['J']).sum())}
assert counts['known_q_bad']==ready['native_totals']['q_bad_known'] and counts['known_j_bad']==ready['native_totals']['j_bad_known']
assert counts['geometry_bad']==ready['native_totals']['geometry_bad']==4
# Check every unknown cell directly against its compact producer record and flags.
wanted={int(ids[i]):int(i) for i in np.flatnonzero(unknown)};found={}
for p in sorted(A.glob('adapted_mesh_inspection_rank*_manifest.json')):
 assert sha(p)==ready['files'][str(p)]['sha256'];before[str(p)]=sha(p)
 m=json.loads(p.read_text());inputs.append(p)
 if m['metrics_complete']:continue
 rawpath=A/m['files']['metrics'];inputs.append(rawpath)
 assert sha(rawpath)==ready['files'][str(rawpath)]['sha256'];before[str(rawpath)]=sha(rawpath)
 with rawpath.open() as source:
  for line in source:
   row=json.loads(line);assert isinstance(row,list) and len(row)==9
   if row[0] not in wanted:continue
   cid,mask=row[0],row[2];i=wanted[cid];assert cid not in found
   for name,slot,bit in (('q',3,1),('J',4,2),('geometry_excess_l2',5,3)):
    value=row[slot];measured=bool(mask&(1<<bit));assert measured==(value is not None)
    assert math.isnan(float(f[name][i])) if value is None else float(f[name][i])==value
   lengths=[e[2] for e in row[6]];assert len(lengths)==6
   for name,fn in (('metric_edge_min',min),('metric_edge_max',max)):
    assert math.isnan(float(f[name][i])) if any(x is None for x in lengths) else float(f[name][i])==fn(lengths)
   found[cid]={'rank':m['rank'],'q_measured':bool(mask&2),'j_measured':bool(mask&4),'geometry_measured':bool(mask&8)}
   if len(found)==len(wanted):break
assert set(found)==set(wanted)
volume=np.einsum('ni,ni->n',np.cross(P[tet[:,1]]-P[tet[:,0]],P[tet[:,2]]-P[tet[:,0]]),P[tet[:,3]]-P[tet[:,0]])/6;assert (volume>0).all()
bad=meshio.read(D/'bad_cells.vtu');cav=meshio.read(D/'cavities.vtu');cores=bad.cell_data['global_cell_id'][0]
assert len(cores)==manifest['bad_rows_total']==4
order=np.argsort(ids);core_indices=order[np.searchsorted(ids[order],cores)];assert np.array_equal(ids[core_indices],cores)
vertices=np.unique(tet[core_indices]);expected=ids[np.isin(tet,vertices).any(axis=1)]
assert set(map(int,expected))==set(map(int,cav.cell_data['global_cell_id'][0]))
contexts=[]
for cid,index in zip(cores,core_indices):
 nodes=tet[index];contexts.append({'id':int(cid),'four_star_union_size':int(np.isin(tet,nodes).any(axis=1).sum()),'vertex_star_sizes':[int((tet==node).any(axis=1).sum()) for node in nodes]})
for p in inputs:
 if str(p) not in before:before[str(p)]=sha(p)
after={str(p):sha(p) for p in inputs};assert before==after
result={'state':'terminal','exit_code':0,'all_reader_checks_passed':True,'seconds':time.monotonic()-started,'actual_points':N,'actual_cells':K,'measured_bad_counts':counts,'whole_field_q_J_bad_counts_known':False,'unknown_cell_records':found,'point_tensor_components':9,'q_J_geometry_are_CellData':True,'array_lengths_correct':True,'coordinates_tetrahedra_equal_su2':True,'captured_metric_available_points':N,'minimum_metric_eigenvalue':float(eig.min()),'minimum_positive_volume':float(volume.min()),'bad_cells':len(cores),'cavity_cells':len(expected),'known_core_global_vertex_stars_complete':True,'per_core_complete_contexts':contexts,'diagnostic_partial_reasons':manifest['partial_reasons'],'input_hashes_before':before,'input_hashes_after':after,'helper_sha256':sha(Path(__file__)),'meshio_version':meshio.__version__,'numpy_version':np.__version__,'scope':'Full actual rejected topology/tensors and array associations independently read back. Known4geometry-core globalstars complete. Unknownq/J remainexplicit;not allbadset proof,strictcertificate,acceptedmesh,transfer oradaptedCFD/restart.'}
(A/'inspection_reader_status.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ('actual_points','actual_cells','measured_bad_counts','cavity_cells','per_core_complete_contexts','seconds')}))
