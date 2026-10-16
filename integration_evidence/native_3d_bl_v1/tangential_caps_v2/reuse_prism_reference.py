"""Transfer the declared cube oracle only for exactly unchanged complete input and field generation."""
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/interacting_walls_v1'))
from audit_native import records
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 old,new=[Path(p).resolve() for p in sys.argv[1:]];receipts=[]
 for case in (old,new):
  r=json.loads(case.with_suffix('.json').read_text());assert r['status']=='PASS'
  assert all(sha(case/p)==h for p,h in r['artifacts'].items());receipts.append(r)
 a,b=receipts;assert a['contract']['environment']==b['contract']['environment'] and a['contract']['command']==b['contract']['command']
 for key,value in a['contract']['pins'].items():
  if key.endswith(('.cpp','.hpp')) and key not in ('Common/src/adaptation/CNativeRegion3D.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'):
   assert b['contract']['pins'][key]==value,key
 fixture='UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp';fixture_text=[]
 for case,receipt in zip((old,new),receipts):
  p=case/'source_snapshots'/fixture;assert sha(p)==receipt['contract']['pins'][fixture]
  fixture_text.append(p.read_text().split('TEST_CASE("Prismatic admission certifies')[0])
 assert fixture_text[0]==fixture_text[1], 'Original geometry/sensors/options and complete workflow fixture changed'
 proof_path=old/'independent.json';proof=json.loads(proof_path.read_text());assert proof['status']=='PASS_COMPLETE_REGIONAL_SINGLE_WALL_ORIGINAL_FIELD_GEOMETRY_AND_FULL_EDGES'
 assert proof['native_receipt_sha256']==sha(old.with_suffix('.json'))
 assert proof['helper_sha256']==sha(ROOT/'integration_evidence/native_3d_bl_v1/interacting_walls_v1/audit_single.py')
 kind=b['contract']['environment']['SU2_NATIVE_PRISM_SENSOR'];prefix='regional_'+{'SCALAR':'','CROSSING':'crossing_','TANGENTIAL':'tangential_','ROTATING':'rotating_'}[kind]+'finer_grouped_';checked=[]
 for stage,report in zip(('coupled_BL','released_Euler'),proof['frames']):
  p=old/report['file'];assert sha(p)==report['input_sha256'];data=json.loads(p.read_text());nodes,cells=records(new/'native.log',prefix+stage)
  actual=dict(data,nodes=[[n]+q for n,q in sorted(nodes.items())],cells=[{'id':c['id'],'nodes':c['nodes']} for c in cells],surface=[f for c in cells for f in c['surface']])
  assert actual==data,'Complete input differs: run the independent oracle on the changed mesh'
  checked.append(dict(stage=stage,cells=len(cells),reference_input_sha256=sha(p),reference_frame=report))
 result=dict(status='PASS_EXACT_COMPLETE_PRISM_REFERENCE_REUSE',frames=checked,old_native_receipt_sha256=sha(old.with_suffix('.json')),new_native_receipt_sha256=sha(new.with_suffix('.json')),independent_reference_sha256=sha(proof_path),field_fixture_prefix_sha256=hashlib.sha256(fixture_text[0].encode()).hexdigest(),helper_sha256=sha(Path(__file__)),scope='Both complete accepted input records exactly equal the previous independently audited records. Original source/geometry/options/workflow fixture prefix, environment/command and all collected field/geometry/solver source pins are unchanged. Native construction and removal pass anew; changed reconstruction and retention-test code are allowed. Reuses exact finite geometry/height/full independent metric proof, not unchanged binaries or new CFD/MPI/performance qualification.')
 (new/'independent_reference_reuse.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True)
if __name__=='__main__':main()
