"""Bounded native first-row validation; preserve the timed-out full-driver baseline."""
import hashlib,json,sys
from pathlib import Path
from fractions import Fraction as F
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
import run
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;PILOT='--pilot' in sys.argv
original_pins=run.pins
SOURCES=['Common/src/adaptation/CNativeEngine3D.cpp','Common/include/adaptation/CNativeEngine3D.hpp',
 'Common/src/adaptation/CNativeDistributed3D.cpp','Common/include/adaptation/CNativeDistributed3D.hpp',
 'Common/src/adaptation/CNativeRemesher3D.cpp','Common/src/adaptation/CNativeImport3D.cpp',
 'Common/src/adaptation/CNativeReferenceIO3D.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py','integration_evidence/native_3d_core_v1/audit_coupled.py']
INPUTS=['run.cfg','Coarse.su2','Coarse.su2.native_ref','sensor_original.bin','working_cells.dat','input_provenance.json']
def pins():
 return dict(original_pins(),**{p:run.sha(ROOT/p) for p in SOURCES},**{str((BASE/p).relative_to(ROOT)):run.sha(BASE/p) for p in INPUTS})
run.pins=pins

def invoke(name,ranks,filter,env={}):
 if name=='serial_controls' and (BASE/name/'result.json').exists():
  # This control filter reads no runtime case config/data; retain the original passing receipt after a replay-only config correction.
  folder=BASE/name;proof=folder/'provenance.json';old=json.loads(proof.read_text());result=json.loads((folder/'result.json').read_text())
  assert result['exit_code']==0 and result['log_sha256']==run.sha(folder/'run.log') and result['provenance_sha256']==run.sha(proof)
  assert old['parameters']==env=={} and old['ranks']==ranks and old['test_filter']==filter
  current=pins();ignored={str((BASE/p).relative_to(ROOT)) for p in INPUTS}
  assert all(current[p]==v for p,v in old['pins'].items() if p not in ignored)
  (BASE/'serial_controls_reuse.json').write_text(json.dumps({'status':'PASS_SCOPED_REUSE','original_provenance_sha256':run.sha(proof),
   'original_result_sha256':run.sha(folder/'result.json'),'log_sha256':run.sha(folder/'run.log'),
   'basis':'Identical binary/build/native-source pins and static-control filter. These controls consume none of the runtime M6 input files; only replay config/provenance changed.',
   'current_native_pins':{p:v for p,v in current.items() if p not in ignored}},indent=2)+'\n')
  print('REUSE_STATIC_CONTROLS_AFTER_REPLAY_CONFIG_CORRECTION',flush=True);return result
 return run.invoke(BASE/name,env,BASE/'working_cells.dat',ranks,test_filter=filter)

def determinant(t):
 a,b,c,d=[[F(x) for x in p] for p in t]
 x=[[p[i]-a[i] for i in range(3)] for p in (b,c,d)]
 return x[0][0]*(x[1][1]*x[2][2]-x[1][2]*x[2][1])-x[0][1]*(x[1][0]*x[2][2]-x[1][2]*x[2][0])+x[0][2]*(x[1][0]*x[2][1]-x[1][1]*x[2][0])

def certify(records):
 result=[];seen={};volume=F(0);faces={}
 for cid,ids,x,boundary in records:
  assert len(set(ids))==4
  for i,q in zip(ids,x):
   assert i not in seen or seen[i]==q;seen[i]=q
  vol=determinant(x);assert vol>0;volume+=vol/6
  sig=[tuple(sorted(x)),[]]
  for f,index in enumerate(((1,2,3),(0,3,2),(0,1,3),(0,2,1))):
   face=tuple(sorted(ids[j] for j in index));faces[face]=faces.get(face,0)+1
   if boundary[f] is not None:sig[1].append((boundary[f],tuple(sorted(x[j] for j in index))))
  sig[1].sort();result.append((sig[0],tuple(sig[1])))
 assert all(n in (1,2) for n in faces.values())
 skin=tuple(sorted((face,tuple(seen[i] for i in face)) for face,n in faces.items() if n==1))
 return tuple(sorted(result)),volume,skin

def original():
 sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
 from capcheck import read_su2
 mesh=read_su2(BASE/'Coarse.su2');wanted=list(map(int,(BASE/'working_cells.dat').read_text().split()[2:]))
 lines=(BASE/'Coarse.su2.native_ref').read_text().splitlines();nmarker,nfacet=map(int,lines[1].split());physical={}
 for line in lines[2+nmarker:2+nmarker+nfacet]:
  p=line.split();physical[tuple(sorted(int(p[j]) for j in (2,6,10)))]=int(p[0])
 rows=[]
 for cid in wanted:
  ids=list(map(int,mesh.E[cid]));x=[tuple(map(float,mesh.P[i])) for i in ids]
  if determinant(x)<0:ids[0],ids[1]=ids[1],ids[0];x[0],x[1]=x[1],x[0]
  boundary=[physical.get(tuple(sorted(ids[j] for j in index))) for index in ((1,2,3),(0,3,2),(0,1,3),(0,2,1))]
  rows.append((cid,ids,x,boundary))
 return certify(rows)

def geometry(folder):
 lines=(folder/'first_row_rejected.dat').read_text().splitlines();assert len(lines)-1==int(lines[0]);rows=[]
 for line in lines[1:]:
  p=line.split();assert len(p)==25
  ids=[int(p[1+4*j]) for j in range(4)];x=[tuple(map(float,p[2+4*j:5+4*j])) for j in range(4)]
  rows.append((int(p[0]),ids,x,[int(p[18+2*f]) if int(p[17+2*f]) else None for f in range(4)]))
 ids=sorted({i for _,n,_,_ in rows for i in n});index={n:j for j,n in enumerate(ids)};points={i:p for _,n,x,_ in rows for i,p in zip(n,x)}
 # Small geometry-only rejected subset; never present it as a whole wing or accepted CFD grid.
 values=lambda x:' '.join(map(str,x))
 text='<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid><Piece NumberOfPoints="%d" NumberOfCells="%d">'%(len(ids),len(rows))
 text+='<Points><DataArray type="Float64" NumberOfComponents="3" format="ascii">'+values(format(v,'.17g') for i in ids for v in points[i])+'</DataArray></Points>'
 text+='<Cells><DataArray type="Int64" Name="connectivity" format="ascii">'+values(index[i] for _,n,_,_ in rows for i in n)+'</DataArray><DataArray type="Int64" Name="offsets" format="ascii">'+values(4*(i+1) for i in range(len(rows)))+'</DataArray><DataArray type="UInt8" Name="types" format="ascii">'+values(10 for _ in rows)+'</DataArray></Cells>'
 text+='<PointData><DataArray type="UInt64" Name="GlobalPointID" format="ascii">'+values(ids)+'</DataArray><DataArray type="UInt8" Name="NewPoint" format="ascii">'+values(int(i>json.loads((BASE/'input_provenance.json').read_text())['maximum_original_node']) for i in ids)+'</DataArray></PointData>'
 text+='<CellData><DataArray type="UInt64" Name="GlobalCellID" format="ascii">'+values(cid for cid,_,_,_ in rows)+'</DataArray><DataArray type="UInt8" Name="NewCell" format="ascii">'+values(int(cid>json.loads((BASE/'input_provenance.json').read_text())['maximum_original_cell']) for cid,_,_,_ in rows)+'</DataArray></CellData></Piece></UnstructuredGrid></VTKFile>'
 (folder/'first_row_rejected.vtu').write_text(text+'\n')
 return certify(rows)

invoke('serial_controls',1,'[NativeInteriorBatch3D],[NativeFirstLayerBatch3D],[NativeFirstLayerEngine3D],[NativeMetricReuse3D],[NativePolicy3D],[NativeBatchEngine3D]')
if not PILOT:invoke('mpi_controls',4,'[NativeInteriorBatch3D],[NativeFirstLayerBatch3D],[NativeFirstLayerEngine3D],[NativePolicy3D],[NativeBatchLookup3D],[NativeBatchPlanning3D],[NativeBatchEngine3D]')
rows=[];original_signature,original_volume,original_skin=original()
for ranks in ((2,) if PILOT else (2,4)):
 signatures={}
 for batch in (1,8):
  name=f'native_np{ranks}_b{batch}';folder=BASE/name
  invoke(name,ranks,'[NativeFirstRowReplay3D]',{'SU2_NATIVE_FIRST_ROW_ROOT':str(BASE),'SU2_NATIVE_FIRST_ROW_BATCH':str(batch)})
  reports=[json.loads((folder/f'first_row_rank{i}.json').read_text()) for i in range(ranks)]
  assert all(r['global_work']==64 and not r['accepted_mesh'] and r['CFD_steps']==0 for r in reports)
  assert sum(r['published'] for r in reports)==64
  assert all(r['maximum_batch']<=batch for r in reports)
  signature,volume,skin=geometry(folder);assert volume==original_volume and skin==original_skin
  signatures[batch]=(signature,volume,skin)
  entry={'ranks':ranks,'batch':batch,'reports':reports,'cells':len(signature),'exact_volume':str(volume),'skin_faces':len(skin),
         'status':'PASS_BOUNDED_PRIVATE_STAGE_NOT_ACCEPTED_MESH'}
  (folder/'independent.json').write_text(json.dumps(entry,indent=2)+'\n');rows.append(entry)
 assert signatures[1]==signatures[8],'Batching changed exact geometry, bindings, volume or outer skin.'
 pair=rows[-2:]
 assert max(r['batches'] for r in pair[1]['reports'])<max(r['batches'] for r in pair[0]['reports'])
 (BASE/f'comparison_np{ranks}.json').write_text(json.dumps({'rows':pair,'same_exact_geometry_bindings_volume_skin':True,
      'limits':['Native first-row stage, not complete remeshing or solution transfer.','Batch1 and8 use the same new interior reservation policy, not an old-binary timing.']},indent=2)+'\n')
if PILOT:
 print('PASS_SERIAL_CONTROLS_AND_DIRECT_M6_MPI2_PILOT',flush=True);raise SystemExit(0)
invoke('curved_runtime_np4',4,'[NativeCurvedRuntime3D]')
invoke('mpi4_m2',4,'[NativeBLPolicyRuntime3D]',{'SU2_NATIVE_POLICY_WORKERS':'2'})
folder=BASE/'mpi4_m2'
(BASE/'mpi4_m2.json').write_text(json.dumps({'status':'PASS','contract':{'selected_workers':2},
 'artifacts':{p.name:run.sha(p) for p in folder.iterdir() if p.is_file()}},indent=2)+'\n')
import importlib.util
spec=importlib.util.spec_from_file_location('policy_audit',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit);audit.HERE=BASE;sys.argv=['audit.py','4','2'];audit.main()
(BASE/'summary.json').write_text(json.dumps({'status':'PASS_FIRST_ROW_BATCHED_INTERIOR_TRANSACTIONS','source_pins':pins(),
 'verification_helper_sha256':run.sha(Path(__file__)),'full_goal_complete':False,'accepted_M6_mesh':False,
 'limits':['First-row geometry/positivity/skin/volume/equality, not final full-field quality or CFD accuracy.',
 'No serial full-source M6 run forced beyond existing2GiB native admission.','No full-event speedup; original-field uncertainty/correction and complete M6 construction remain open.']},indent=2)+'\n')
print('PASS_FIRST_ROW_CONNECTED_BATCH',flush=True)
