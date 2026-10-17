from pathlib import Path
import sys,json,re,hashlib,shlex
import numpy as np
root=Path.cwd();base=root/'integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_transactions_v1';origin=base.parent/'construction_first_trial_v1'
sys.path.insert(0,str(root/'TestCases/adaptation/capability'))
from capcheck import read_su2
mesh=read_su2(origin/'Coarse.su2');tet=mesh.E
lines=(origin/'Coarse.su2.native_ref').read_text().splitlines();nmarkers,nfacets=map(int,lines[1].split());names=[shlex.split(x)[0] for x in lines[2:2+nmarkers]]
config=(origin/'run.cfg').read_text();wanted=re.search(r'(?m)^ADAP_BL_MARKER\s*=\s*\((.*?)\)',config).group(1).strip().split(',');markers={names.index(x.strip()) for x in wanted}
faces=set()
for line in lines[2+nmarkers:2+nmarkers+nfacets]:
 p=line.split()
 if int(p[1]) in markers:faces.add(tuple(sorted(int(p[i]) for i in (2,6,10))))
wall=[]
for i,t in enumerate(tet):
 if any(tuple(sorted(int(x) for j,x in enumerate(t) if j!=opposite)) in faces for opposite in range(4)):wall.append(i)
selected=set(wall[:128]);assert len(selected)==128
# Real untouched cells retain the full mesh's global ID floors; no synthetic geometry/ID override.
selected.add(len(tet)-1);selected.add(int(np.flatnonzero(np.any(tet==len(mesh.P)-1,axis=1))[0]))
replay_config=re.sub(r'(?m)^ADAP_NATIVE_RANKS\s*=.*$', 'ADAP_NATIVE_RANKS= 0',config)
local_config=base/'run.cfg'
if local_config.is_symlink():local_config.unlink()
if local_config.exists():assert local_config.read_text()==replay_config
else:local_config.write_text(replay_config)
for name in ('Coarse.su2','Coarse.su2.native_ref','sensor_original.bin'):
 dest=base/name
 if not dest.exists():dest.symlink_to(Path(__import__('os').path.relpath((origin/name).resolve(),base)))
 dest.resolve(strict=True).relative_to(root)
parts=['SU2_NATIVE_FIRST_ROW_REPLAY_1',str(len(selected))]+[str(i) for i in sorted(selected)]
file=base/'working_cells.dat';payload='\n'.join(parts)+'\n'
if file.exists():assert file.read_text()==payload
else:file.write_text(payload)
# No refit: the native replay reads every original node from the exact archived sensor file.
record=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(9,))]);raw=origin/'sensor_original.bin'
with raw.open('rb') as f:
 count=np.fromfile(f,dtype='<u8',count=1)[0];data=np.fromfile(f,dtype=record)
assert count==len(data)==len(mesh.P)
assert np.array_equal(data['id'],np.arange(len(data))) or len(np.unique(data['id']))==len(data)
assert np.array_equal(data['x'],mesh.P[data['id']])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
(base/'input_provenance.json').write_text(json.dumps({'original_config_sha256':sha(origin/'run.cfg'),'replay_config_sha256':sha(local_config),'mesh_sha256':sha(origin/'Coarse.su2'),'reference_sha256':sha(origin/'Coarse.su2.native_ref'),'sensor_sha256':sha(raw),'working_selection_sha256':sha(file),'donor_cells':len(tet),'working_cells':len(selected),'original_cells':len(tet),'selected_original_wall_cells':128,'maximum_original_node':len(mesh.P)-1,'maximum_original_cell':len(tet)-1,'rule':'All original tetrahedra and the full original wall reference are retained. No reduced donor support, adapted samples or omitted wall geometry.','scope':'Bounded native first-row stage; incomplete remeshing is rejected, not an accepted CFD mesh.'},indent=2)+'\n')
print('Prepared',len(selected),'working cells; replay retains ALL',len(tet),'original donors.')
