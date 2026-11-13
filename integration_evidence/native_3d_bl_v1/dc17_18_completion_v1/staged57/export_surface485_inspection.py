"""Export the latest preserved physical surface; no target recomputation or native work."""
import json
from pathlib import Path
import numpy as np
import meshio
from assess_surface_transaction470 import sha
from inspect_native_failure3d import _read_owned,_tensor9
from surface_seed_density import surface_seed_density
R=Path(__file__).resolve().parent
checkpoint=R/'saved_surface485_checkpoint.json';state=json.loads(checkpoint.read_text())
comparison=json.loads((R/'corrected_surface488_assessment.json').read_text())
assert comparison['input_mesh_unchanged'] and comparison['all_actual_handles_terminal'] and not comparison['result']['published']
for key in ('owned','parameters','prepared_reference','composed_target','configuration'):
 item=state[key];assert sha(item['path'])==item['sha256']
source=Path(state['owned']['path']);reference=Path(state['prepared_reference']['path'])
with reference.open() as f:
 assert f.readline().strip()=='SU2_NATIVE_REFERENCE_3D 9'
 counts=f.readline().split();names=[json.loads(f.readline()) for _ in range(int(counts[0]))]
 markers={}
 for _ in range(int(counts[1])):
  row=f.readline().split();assert len(row)==14
  facet,marker=map(int,row[:2]);assert facet not in markers and 0<=marker<len(names);markers[facet]=marker
faces=[];nodes={};seen=set();standard=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
for cell in _read_owned(source,state['current_cells'],200_000_000):
 for slot,binding in enumerate(cell['physical']):
  if not binding['present']:continue
  assert binding['facet'] in markers and cell['sensor_present']
  face=[cell['nodes'][i] for i in standard[slot]];key=tuple(sorted(n['id'] for n in face));assert key not in seen;seen.add(key)
  p=np.array([n['xyz'] for n in cell['nodes']]);assert np.linalg.det((p[1:]-p[0]).T)>0
  for i in standard[slot]:
   n=cell['nodes'][i];value=(tuple(n['xyz']),tuple(cell['sensor'][6*i:6*i+6]));assert nodes.setdefault(n['id'],value)==value
  faces.append((tuple(n['id'] for n in face),markers[binding['facet']],binding['facet'],cell['id']))
density,density_metadata=surface_seed_density(state,nodes)
out=R/'inspection_surface485';out.mkdir(exist_ok=True)
outputs={}
def write(name,selected):
 ids=sorted({i for f in selected for i in f[0]});index={i:k for k,i in enumerate(ids)}
 pts=np.array([nodes[i][0] for i in ids]);tri=np.array([[index[i] for i in f[0]] for f in selected],dtype=np.int64)
 tensors=np.array([_tensor9(nodes[i][1]) for i in ids])
 mesh=meshio.Mesh(pts,[('triangle',tri)],point_data={'global_node_id':np.array(ids,dtype=np.uint64),
  'transported_sensor_tensor':tensors,'Density_seed_reference_interp':np.array([density[i] for i in ids]),'last_published_root_vertex':np.array([i==64065077 for i in ids],dtype=np.uint8)},
  cell_data={key:[np.array([f[col] for f in selected],dtype=np.uint64)] for key,col in [('marker',1),('reference_facet_id',2),('adjacent_volume_cell_id',3)]})
 path=out/name
 if path.exists():
  old=meshio.read(path);assert np.array_equal(old.points,pts) and np.array_equal(old.cells[0].data,tri)
  assert all(np.array_equal(mesh.point_data[k],v) for k,v in old.point_data.items())
  assert all(np.array_equal(mesh.cell_data[k][0],v[0]) for k,v in old.cell_data.items())
 meshio.write(path,mesh,binary=True)
 back=meshio.read(path);assert np.array_equal(back.points,pts) and np.array_equal(back.cells[0].data,tri)
 assert all(np.array_equal(back.point_data[k],v) for k,v in mesh.point_data.items())
 assert all(np.array_equal(back.cell_data[k][0],v[0]) for k,v in mesh.cell_data.items())
 assert all(len(v)==len(pts) for v in back.point_data.values()) and all(len(v[0])==len(tri) for v in back.cell_data.values())
 outputs[name]=dict(path=str(path.resolve()),sha256=sha(path),points=len(pts),triangles=len(tri),bytes=path.stat().st_size,readback_verified=True)
 return pts,tri
write('all_boundaries.vtu',faces)
wing=[f for f in faces if names[f[1]]=='WING3D'];pts,tri=write('wing_surface.vtu',wing)
# Orthographic preview only; VTU retains exact double coordinates and connectivity.
from PIL import Image,ImageDraw
img=Image.new('RGB',(2100,1050),'white');draw=ImageDraw.Draw(img)
views=[('Top: x / y',pts[:,[0,1]],pts[:,2]),('Oblique',np.column_stack((pts[:,0]-.35*pts[:,1],pts[:,2]+.38*pts[:,1])),pts@np.array([-.35,-1,.38]))]
for column,(title,xy,depth) in enumerate(views):
 lo=xy.min(axis=0);span=np.maximum(xy.max(axis=0)-lo,1e-12);scale=min(950/span[0],900/span[1]);screen=(xy-lo)*scale
 screen[:,0]+=column*1050+50;screen[:,1]=1000-screen[:,1]
 for a,b,c in tri[np.argsort(depth[tri].mean(axis=1))]:
  draw.polygon([tuple(screen[a]),tuple(screen[b]),tuple(screen[c])],fill=(245,248,250),outline=(65,83,98))
 draw.text((column*1050+50,25),title,fill='black')
draw.text((50,60),'Provisional M6 surface: latest485 state, unchanged through test488; not accepted / no BL first row',fill='black')
img.save(out/'surface_preview.png')
rho=np.array([density[i] for i in sorted({i for f in wing for i in f[0]})]);rho_min,rho_max=float(rho.min()),float(rho.max())
colors=np.array([(68,1,84),(59,82,139),(33,145,140),(94,201,98),(253,231,37)],dtype=float)
def color(value):
 t=np.clip((value-rho_min)/max(rho_max-rho_min,1e-30),0,1)*4;k=min(int(t),3)
 return tuple(np.rint(colors[k]*(1-(t-k))+colors[k+1]*(t-k)).astype(int))
colored=Image.new('RGB',(2100,1120),'white');draw=ImageDraw.Draw(colored)
for column,(title,xy,depth) in enumerate(views):
 lo=xy.min(axis=0);span=np.maximum(xy.max(axis=0)-lo,1e-12);scale=min(950/span[0],900/span[1]);screen=(xy-lo)*scale
 screen[:,0]+=column*1050+50;screen[:,1]=1000-screen[:,1]
 for a,b,c in tri[np.argsort(depth[tri].mean(axis=1))]:
  draw.polygon([tuple(screen[a]),tuple(screen[b]),tuple(screen[c])],fill=color(float(rho[[a,b,c]].mean())))
 draw.text((column*1050+50,25),title,fill='black')
draw.text((50,55),'Saved CFD seed density mapped for visualization; provisional surface, no adapted CFD solution',fill='black')
for x in range(1000):draw.line([(50+x,1040),(50+x,1060)],fill=color(rho_min+(rho_max-rho_min)*x/999))
draw.text((50,1080),f'{rho_min:.6g}',fill='black');draw.text((970,1080),f'{rho_max:.6g}  (restart Density units)',fill='black')
colored.save(out/'density_preview.png')
receipt=dict(checkpoint=str(checkpoint.resolve()),checkpoint_sha256=sha(checkpoint),source_owned=state['owned'],
 latest_test=comparison['case'],latest_test_published=False,provisional=True,final_mesh_accepted=False,first_row_ready=False,
 markers=dict(enumerate(names)),density=density_metadata,density_helper_sha256=sha(R/'surface_seed_density.py'),density_preview_sha256=sha(out/'density_preview.png'),outputs=outputs,preview_sha256=sha(out/'surface_preview.png'),export_script_sha256=sha(__file__),
 metric_semantics='transported_sensor_tensor is the current 3D nodal Mach-sensor tensor, restricted to surface nodes; NOT the composed flow/BL target or tangential planning tensor. Full policy/target remains in saved_surface485_checkpoint.json.',
 quality_semantics='Global q/J/geometry certification is unavailable for this saved state; no fabricated quality arrays exported.',
 stage_height=state['stage_height'],final_height=state['final_height'],volume_cells=state['current_cells'],volume_nodes=state['current_nodes'])
(out/'provenance.json').write_text(json.dumps(receipt,indent=2)+'\n')
(out/'README.md').write_text('''# Latest provisional M6 surface\n\nOpen `wing_surface.vtu` in ParaView and choose **Surface With Edges**. `all_boundaries.vtu` additionally includes farfield and symmetry; threshold cell array `marker` (0 FARFIELD, 1 SYMMETRY, 2 WING3D).\n\nThese are actual physical boundary triangles of the saved build485 mesh. Tests486–488 published no change. The mesh is provisional, not accepted, and no BL first row has been established.\n\n`transported_sensor_tensor` contains nine row-major components of the current transported 3D nodal sensor tensor. It is **not** the composed flow/BL target or a separate tangential surface metric. `last_published_root_vertex` marks the latest moved root point. Reference facet and adjacent volume-cell IDs support tracing individual faces. Global q/J and geometry certification were not available and are omitted.\n\n`Density_seed_reference_interp` is fluid density from the saved RANS CFD seed, sampled at the current surface geometry associations (original vertices, edge-linear and facet-barycentric). It is visualization-only, not conservative transfer or a CFD solution computed on this provisional mesh. Select this Point Data field in ParaView to color the surface. Values retain saved restart Density units. `density_preview.png` shows the distribution; metadata records source hashes and interpolation details.\n\n`surface_preview.png` is an orthographic wireframe preview; use VTU for exact coordinates and interactive inspection. `provenance.json` records source hashes, counts and successful readback of every exported point/cell array. Native checkpoint files and complete composed target remain unchanged.\n''')
print(json.dumps(outputs,indent=2))
