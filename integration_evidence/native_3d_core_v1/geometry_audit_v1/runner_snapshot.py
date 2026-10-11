from pathlib import Path
import hashlib,json,sys,copy
import h5py
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');where=root/'integration_evidence/native_3d_core_v1';sys.path.insert(0,str(where))
from audit_geometry_bridge import signature,su2_signature,cgns_signature,affine,states,reject,MARKERS
folder=where/'geometry_audit_v1';folder.mkdir(exist_ok=False)
(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes())
base=where/'partition_controls_v2/mpi1_repeat1_meshes/engine_m1_initial.json';data=json.loads(base.read_text())
# A bounded positive box fixture with one face split into two physical marker names.
labels=sorted(MARKERS);data['marker_names']=labels
for i,f in enumerate(data['facets']):f['marker']=i%7
for i,f in enumerate(data['surface']):f['facet']=data['facets'][i]['id']
scale=4.
for c in data['original']:
 for v in c['vertices']:
  x,y,z=v[1:4];v[4]=[scale+.125*x,.03125*x,0.,scale+.125*y,0.,scale+.125*z]
accepted=[dict(id=n[0],point=n[1:],states=[factor*v for factor in (1.,1.03125,.96875) for v in affine(n[1:])],metric=[scale+.125*n[1],.03125*n[1],0.,scale+.125*n[2],0.,scale+.125*n[3]]) for n in data['nodes']]
state=dict(sensor_scale=scale,donor=[{k:v for k,v in r.items() if k!='metric'} for r in accepted],accepted=accepted)
metrics=states(data,state);expected=signature(data)
points={n[0]:tuple(n[1:]) for n in data['nodes']};ids=sorted(points);local={id:i for i,id in enumerate(ids)}
lines=['NDIME= 3','NELEM= '+str(len(data['cells']))]
lines+=['10 '+' '.join(str(local[n]) for n in c['nodes'])+' '+str(i) for i,c in enumerate(data['cells'])]
lines+=['NPOIN= '+str(len(ids))]+[' '.join(format(x,'.17g') for x in points[id])+' '+str(local[id]) for id in ids]
lines+=['NMARK= 7'];facets={f['id']:labels[f['marker']] for f in data['facets']}
for label in labels:
 skin=[f['nodes'] for f in data['surface'] if facets[f['facet']]==label]
 lines+=['MARKER_TAG= '+label,'MARKER_ELEMS= '+str(len(skin))]+['5 '+' '.join(str(local[n]) for n in f) for f in skin]
(folder/'fixture.su2').write_text('\n'.join(lines)+'\n');assert su2_signature(folder/'fixture.su2')==expected
with h5py.File(folder/'fixture.cgns','w') as file:
 def group(parent,name,label,values=None):
  g=parent.create_group(name);g.attrs['label']=label.encode()
  if values is not None:g.create_dataset(' data',data=values)
  return g
 base=group(file,'Base','CGNSBase_t',[3,3]);zone=group(base,'Zone','Zone_t',[[len(ids)],[len(data['cells'])],[0]])
 grid=group(zone,'GridCoordinates','GridCoordinates_t')
 for i,axis in enumerate('XYZ'):group(grid,'Coordinate'+axis,'DataArray_t',[points[id][i] for id in ids])
 tet=group(zone,'Tetrahedra','Elements_t',[10,0]);group(tet,'ElementRange','IndexRange_t',[1,len(data['cells'])]);group(tet,'ElementConnectivity','DataArray_t',[local[n]+1 for c in data['cells'] for n in c['nodes']])
 bc=group(zone,'ZoneBC','ZoneBC_t');next=len(data['cells'])+1
 for label in labels:
  skin=[f['nodes'] for f in data['surface'] if facets[f['facet']]==label];last=next+len(skin)-1
  tri=group(zone,label,'Elements_t',[5,0]);group(tri,'ElementRange','IndexRange_t',[next,last]);group(tri,'ElementConnectivity','DataArray_t',[local[n]+1 for f in skin for n in f])
  mark=group(bc,label,'BC_t');group(mark,'PointRange','IndexRange_t',[[next],[last]]);next=last+1
assert cgns_signature(folder/'fixture.cgns')==expected
bad=copy.deepcopy(state);bad['accepted'][0]['states'][0]+=.01;reject(lambda:states(data,bad))
bad=copy.deepcopy(state);bad['accepted'][0]['metric'][0]+=1;reject(lambda:states(data,bad))
bad=copy.deepcopy(state);bad['donor'].pop();reject(lambda:states(data,bad))
with h5py.File(folder/'fixture.cgns','r+') as f:f['Base/Zone/ZoneBC/'+labels[0]+'/PointRange/ data'][0,0]=1
reject(lambda:cgns_signature(folder/'fixture.cgns'))
with h5py.File(folder/'fixture.cgns','r+') as f:f['external']=h5py.ExternalLink('/tmp/unavailable-native-check.cgns','/')
reject(lambda:cgns_signature(folder/'fixture.cgns'))
(folder/'fixture.json').write_text(json.dumps(data,indent=2)+'\n');(folder/'fixture_states.json').write_text(json.dumps(state,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
result=dict(status='PASS',auditor_sha256={p.name:sha(p) for p in [where/'audit_geometry_bridge.py',where/'audit_coupled.py',where/'audit_cavities.py']},baseline_sha256=sha(base),valid_cases=['original-P1 metric/current/BDF2 states','actual SU2 parse','independent CGNS HDF5 parse'],adversaries_rejected=5,metric_checks=metrics,scope='Auditor self-checks on manufactured serialized box/state fixtures only; no actual SU2 bridge run. Retained CGNS fixture deliberately contains the rejected BC/external-link corruption.')
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS',folder)
