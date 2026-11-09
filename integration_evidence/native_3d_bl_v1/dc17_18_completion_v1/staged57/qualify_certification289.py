"""Seal the connected exact-face certificate milestone, then require full M6 acceptance."""
from pathlib import Path
import hashlib,json,re,sys,time
R=Path(__file__).resolve().parent;assert len(sys.argv)==6
cases=[Path(p).resolve() for p in sys.argv[1:]];out=R/'certification289_qualification.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
pins={};stages=[]
for number,case in enumerate(cases):
 assert case.parent==(R/'cases').resolve()
 status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 assert json.loads((case/'terminal_handles.json').read_text())['all_owned_handles_terminal']
 command=json.loads((case/'command.json').read_text());assert command['build']=='build289' and command['ranks']==(6 if number==4 else 1)
 text=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text());summaries=[list(map(int,x)) for x in re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases?\)',text)]
 assert len(summaries)==(6 if number==4 else 1)
 assert all(a>0 and b>=[12,1,1,90,120][number] for a,b in summaries)
 for p,h in json.loads((case/'pins.json').read_text()).items():assert pins.setdefault(p,h)==h
 for name in ('status.json','terminal_handles.json','command.json','pins.json','run.log'):pins[str(case/name)]=sha(case/name)
 stages.append({'case':str(case),'status':status,'summaries':summaries})
comparisons=[]
for case,direction in zip(cases[1:3],['source','reverse']):
 q=json.loads((case/'prepared_border_comparison.json').read_text());assert q['direction']==direction and q['same_actual_input']
 assert q['common_domain_decisions_and_bounds_identical'] and q['common_domain_work_ratio']==1
 assert q['completed_new_domains']==q['completed_old_domains']
 assert q['new_serial_tree_cpu_seconds']<.9*q['old_serial_tree_cpu_seconds']
 comparisons.append(q)
 for name in ('prepared_border_comparison.json','native_final_geometry_source_result.json'):pins[str(case/name)]=sha(case/name)
for p in [Path(__file__),R/'assess_final289.py',R/'check_face286_separation.py',R/'face286_separation_oracle.json',R/'build289_status.json']:pins[str(p)]=sha(p)
assert json.loads((R/'face286_separation_oracle.json').read_text())['mismatches']==0
build=json.loads((R/'build289_status.json').read_text());assert build['completed_build'] and build['pins_verified']
cfd=R.parents[3]/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(cfd)==build['production_cfd_sha256_after'];pins[str(cfd)]=sha(cfd)
assert pins=={p:sha(p) for p in pins}
result={'time':time.time(),'all_required_checks_passed':True,'mpi_ranks':6,'source_pins_sha256':sha(R/'build289_source_pins.json'),'stages':stages,'actual_saved_direction_comparisons':comparisons,'qualification_pins':pins,'full_mesh_accepted':False,'actual_saved_entity_certificate_complete':False,'scope':'Immutable prepared nonconvex border coordinates reused across point/segment/face containment, preserving exact domain bounds/order/work in BOTH saved directions with lower serial CPU. Construction and resident bytes accounted; independent notch controls, affected serial and ONE MPI6 milestone. Bounded actual certificates incomplete; full same-seed TWO-event M6 must pass unchanged finalBoth, transfer, adapted CFD and disk restart.'}
out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('qualification_pins','stages','actual_saved_direction_comparisons')}))
