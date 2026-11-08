"""Seal actual-query and connected serial/MPI evidence before full M6."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent;assert len(sys.argv)==4
cases=[Path(p).resolve() for p in sys.argv[1:]];out=R/'certification282_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
 assert case.parent==(R/'cases').resolve()
 status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
 command=json.loads((case/'command.json').read_text());assert command['build']=='build282' and command['ranks']==(6 if number==2 else 1)
 text=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text());summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',text)]
 assert len(summaries)==(6 if number==2 else 1)
 assert all(a>0 and b>=(4 if number==0 else 80 if number==1 else 117) for a,b in summaries)
 for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
 for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
 stages.append({'case':str(case),'status':status,'summaries':summaries})
query=json.loads((cases[0]/'native_final_geometry_result.json').read_text());assert not query['source_queried'] and query['both_complete'] and query['both_within_tolerance'] and not query['full_work_exhausted']
assert query['support_facets']==37202 and query['physical_faces']==53406 and query['full_work']<=20000000
pins[str(cases[0]/'native_final_geometry_result.json')]=sha(cases[0]/'native_final_geometry_result.json');pins[str(Path(__file__))]=sha(Path(__file__))
assert pins=={p:sha(p) for p in pins}
result={'time':time.time(),'all_required_checks_passed':True,'mpi_ranks':6,'source_pins_sha256':sha(R/'build282_source_pins.json'),'stages':stages,'actual_saved_third_entity':query,'qualification_pins':pins,'full_mesh_accepted':False,'scope':'Strict optional subset graph witness validated on actual third final entity both directions, pinches/true holes/curves/resources/cache, serial and MPI integration. Immediately return to complete same-seed M6; not acceptance, transfer, adapted CFD or restart.'}
out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages')}))
