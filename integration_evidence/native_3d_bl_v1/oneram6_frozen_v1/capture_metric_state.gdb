set pagination off
frame 10
python
import gdb,json
from pathlib import Path
def vec(v):
 p=v['_M_impl']['_M_start'];return p,int(v['_M_impl']['_M_finish']-p)
def array(v,n):return [float(v['_M_elems'][k]) for k in range(n)]
rules,nrules=vec(gdb.parse_and_eval('integration3D'))
sensors,nsensors=vec(gdb.parse_and_eval('blMetric'))
match=[]
for i in range(nrules):
 c=rules[i]
 if int(c['global_cell'])!=32:continue
 ids=[int(c['nodes']['_M_elems'][k]) for k in range(4)]
 assert all(0<=k<nsensors for k in ids)
 match.append({'global_cell':32,'local_nodes':ids,'coordinates':[array(c['coordinates']['_M_elems'][k],3) for k in range(4)],'sensor':[[[float(sensors[k]['m'][i][j]) for j in range(3)] for i in range(3)] for k in ids]})
assert len(match)==1
layers=gdb.parse_and_eval('nativeLayers')['_M_ptr'].dereference()
walls,nwalls=vec(layers['walls']);captured=[]
for i in range(nwalls):
 w=walls[i];p,np=vec(w['coord']);e,ne=vec(w['conn'])
 assert np%3==0 and ne%4==0
 captured.append({'coordinates':[float(p[k]) for k in range(np)],'connectivity':[int(e[k]) for k in range(ne)],'first_height':float(w['layer']['firstHeight']),'growth':float(w['layer']['growth']),'thickness':float(w['layer']['thickness'])})
r={'scope':'Read-only passive GDB capture from owned failed rank0 ComputeMetric outer frame; current bounded sensor tensors and actual volume-oriented wall input, no inferior function calls. One initial CFD step from saved120-step state.','cell':match[0],'frozen_core':float(gdb.parse_and_eval('frozenCore')),'log_scale':float(gdb.parse_and_eval('logScale')),'complexity_trials':int(gdb.parse_and_eval('complexityTrials')),'walls':captured}
path=Path('integration_evidence/native_3d_bl_v1/oneram6_frozen_v1/cell32_runtime_state.json')
path.write_text(json.dumps(r,separators=(',',':'))+'\n')
print('Captured actual cell32,4 nodal sensor tensors, frozen core and',nwalls,'walls to',str(path))
end
detach
