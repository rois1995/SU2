"""Seal actual-query and connected serial/MPI evidence before full M6."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent;assert len(sys.argv)==4
cases=[Path(p).resolve() for p in sys.argv[1:]];out=R/'certification283_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
 assert case.parent==(R/'cases').resolve()
 status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
 command=json.loads((case/'command.json').read_text());assert command['build']=='build283' and command['ranks']==(6 if number==2 else 1)
 text=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text());summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',text)]
 assert len(summaries)==(6 if number==2 else 1)
 assert all(a>0 and b>=(4 if number==0 else 80 if number==1 else 117) for a,b in summaries)
 for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
 for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
 stages.append({'case':str(case),'status':status,'summaries':summaries})
query=json.loads((cases[0]/'native_final_geometry_result.json').read_text())
assert not query['source_queried'] and query['support_facets']==37202 and query['physical_faces']==53406 and query['full_work']<=20000000
assessment=json.loads((cases[0]/'direction_trace_assessment.json').read_text())
assert assessment['certified_equal_facet_parameter_share']>=.93 and assessment['classification_decisions_match282']
assert (query['both_complete'] and query['both_within_tolerance']) or query['full_work_exhausted']
pins[str(cases[0]/'direction_trace_assessment.json')]=sha(cases[0]/'direction_trace_assessment.json')
pins[str(cases[0]/'native_final_geometry_result.json')]=sha(cases[0]/'native_final_geometry_result.json');pins[str(Path(__file__))]=sha(Path(__file__))
build=json.loads((R/'build283_status.json').read_text());assert build['completed_build'] and build['pins_verified']
cfd=R.parents[3]/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(cfd)==build['production_cfd_sha256_after'];pins[str(cfd)]=sha(cfd)
pins[str(R/'build283_status.json')]=sha(R/'build283_status.json')
pins[str(R/'assess_final283.py')]=sha(R/'assess_final283.py')
assert pins=={p:sha(p) for p in pins}
result={'time':time.time(),'all_required_checks_passed':True,'mpi_ranks':6,'source_pins_sha256':sha(R/'build283_source_pins.json'),'stages':stages,'actual_saved_third_entity':query,'qualification_pins':pins,'full_mesh_accepted':False,'scope':'Strict optional subset graph witness and classification early exit validated by bounded actual third-entity coverage, unchanged classification decisions, pinches/true holes/curves/resources/cache, serial and MPI integration. The 20M actual certificate remains incomplete; complete certification must pass in the unchanged native M6 workflow. Immediately return to complete same-seed M6; not acceptance, transfer, adapted CFD or restart.'}
result['actual_saved_entity_certificate_complete']=query['both_complete']
result['qualification_scope']='Changed implementation behavior and bounded actual progress only; whole-mesh certification remains required in M6.'
out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages')}))
