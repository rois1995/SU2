"""Verify saved actual runs without rerunning adaptation; FAIL cases remain FAIL."""
from pathlib import Path
import hashlib,json,re,sys
folder=Path(__file__).resolve().parent
where=folder.parent;root=where.parent.parent
sys.dont_write_bytecode=True
sys.path.insert(0,str(where))
from audit_lifecycle import parse,nested_stage,transport_counters,termination_counters
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def receipt(name,expected):
 p=where/name/'validation.json';v=json.loads(p.read_text())
 assert v['status']==expected,(name,v['status'])
 for k in ('source_drift','object_drift','archive_drift','tool_drift'):
  if k in v:assert v[k]==[],(name,k)
 for n,h in v['artifact_sha256'].items():assert sha(p.parent/n)==h,(name,n)
 return v

build=receipt('termination_build_v1','PASS')
assert len(build['actual_objects_sha256'])==68 and len(build['thin_archive_member_sha256'])==774
assert all(sha(root/n)==h for n,h in build['source_sha256'].items())
builddir=root/'integration_evidence/build-integrated-v2'
assert sha(builddir/'UnitTests/test_driver')==build['binary_sha256']
assert all(sha(builddir/n)==h for n,h in build['actual_objects_sha256'].items())
assert all(sha(Path(n))==h for n,h in {**build['library_sha256'],**build['thin_archive_member_sha256']}.items())
regressions=[]
for ranks in (1,2,4):
 name=f'termination_mpi{ranks}_v1';v=receipt(name,'PASS');p=where/name
 assert v['binary_sha256']==build['binary_sha256']
 assert v['build_validation_sha256']==sha(where/'termination_build_v1/validation.json')
 assert all(s['exit_code']==0 for s in v['stages'])
 passes=re.findall(r'All tests passed \(\d+ assertions in 69 test cases\)',(p/'case.stdout').read_text())
 assert len(passes)==ranks
 a=json.loads((p/'independent_audit.json').read_text())
 assert a['status']=='PASS' and len(a['frames'])==12*ranks and len(a['states'])==6*ranks
 assert a['actual_output_roundtrips']==6*ranks and a['adversaries_rejected']==5
 if ranks==1:assert 'All tests passed (48 assertions in 2 test cases)' in (p/'mmg_serial.stdout').read_text()
 regressions.append(dict(ranks=ranks,cases_per_rank=69,frames=len(a['frames']),state_pairs=len(a['states']),actual_output_roundtrips=a['actual_output_roundtrips'],validation_sha256=sha(p/'validation.json')))

lifecycle=receipt('termination_lifecycle_mpi1_v1','PASS')
assert lifecycle['binary_sha256']==build['binary_sha256']
assert lifecycle['build_validation_sha256']==sha(where/'termination_build_v1/validation.json')
assert all(s['exit_code']==0 for s in lifecycle['stages'])
lifecycle_audit=json.loads((where/'termination_lifecycle_mpi1_v1/lifecycle_audit.json').read_text())
assert lifecycle_audit['status']=='PASS' and len(lifecycle_audit['frames'])==6
lifecycle_engines=[r for r in lifecycle_audit['nested_rank_details'] if r['stage'] in ('refine','coarsen')]
assert len(lifecycle_engines)==6 and all(termination_counters(r) and r['complete']==1 for r in lifecycle_engines)

names=('transport_limit_n8_n1_m1_v1','transport_limit_n8_n1_m1_v2')
failed=[receipt(n,'FAIL') for n in names]
shared=('parameters.json','case_environment.json','envelope.cfg','envelope_input.su2','envelope_s0_initial.json')
for n in shared:assert (where/names[0]/n).read_bytes()==(where/names[1]/n).read_bytes(),n
for v,n in zip(failed,names):
 assert [(s['label'],s['exit_code']) for s in v['stages']][-1]==('case',1)
 assert not any(s['label'].startswith('independent_') for s in v['stages'])
 text=(where/n/'case.stdout').read_text()
 assert 'assertions: 5039 | 5038 passed | 1 failed' in text
 assert 'REQUIRE( result.status==CRemeshResult::Status::COMPLETE )' in text
 assert 'N=1 M=1 cycle=0' in text
 assert not (where/n/'independent_audit.json').exists()

counter_names=set("rank ranks workers complete detailed initial final refine_accepted coarsen_accepted tracked_bytes tracked_calls tracked_import_bytes tracked_import_calls tracked_parent_bytes tracked_parent_calls tracked_worker_bytes tracked_worker_calls native_bound requests hits misses invalid_queries failures evictions compositions batches retries candidates payloads maximum_patch moved arcs cells_before cells_after work_before work_after directory_elections rounds attempted accepted rejected conflicts edge_evaluations collapse_geometry_checks collapse_filtered edge_reused reconnected inserted maximum_donor_patch maximum_owned donor_reused edge_separated donor_batches donor_candidates donor_filtered donor_payloads batch_retries query_retries private_query_retries".split())
rows=[];equal_counters=[]
for n in names:
 lines=(where/n/'case.stdout').read_text().splitlines()
 parent=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
 details=[parse(s) for s in lines if s.startswith('NATIVE3D_DETAIL ')]
 assert len(parent)==1 and len(details)==7
 assert parent[0]['complete']==0 and parent[0]['initial']==1536 and parent[0]['final']==0
 assert transport_counters(parent[0])
 for r in details:nested_stage(r)
 rows.append(dict(parent=parent[0],details={r['stage']:r for r in details}))
for label in ('parent','import','weights','return','source','partition','refine','coarsen'):
 old=rows[0]['parent'] if label=='parent' else rows[0]['details'][label]
 new=rows[1]['parent'] if label=='parent' else rows[1]['details'][label]
 keys=sorted(set(old)&set(new)&counter_names)
 for k in keys:assert old[k]==new[k] and int(old[k])==old[k],(label,k)
 equal_counters.append(dict(stage=label,keys=keys))
assert sum(len(r['keys']) for r in equal_counters)>100
refine=rows[1]['details']['refine'];coarsen=rows[1]['details']['coarsen']
assert termination_counters(refine) and termination_counters(coarsen)
assert (refine['rounds'],refine['accepted'],refine['final_owned_cells'],refine['cached_measured_cells'],refine['cached_invalid_cells'],refine['remaining_edges'],refine['remaining_repairs'])==(457,457,3552,3552,0,0,0)
assert (coarsen['rounds'],coarsen['attempted'],coarsen['accepted'],coarsen['rejected'],coarsen['final_owned_cells'],coarsen['cached_measured_cells'],coarsen['cached_invalid_cells'],coarsen['remaining_edges'],coarsen['remaining_repairs'])==(512,512,272,240,2489,2489,0,91,0)
initial=json.loads((where/names[1]/'envelope_s0_initial.json').read_text())
assert (len(initial['nodes']),len(initial['cells']),len(initial['surface']))==(405,1536,512)
engine=(root/'Common/src/adaptation/CNativeEngine3D.cpp').read_text()
assert '(coarsening && !selection.queue.empty())' in engine
assert 'Native 3D adaptation round limit reached before strict metric completion.' in engine
fixture=(root/'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp').read_text()
assert 'CHECK(State(driver->Geometry(),driver->Solver())==before)' in fixture and 'CHECK_FALSE(result.onAccepted)' in fixture
package=root/'integration_evidence/native_cluster_3d_core_v1/package_checks_v19'
pv=json.loads((package/'validation.json').read_text());assert pv['status']=='PASS'
for n,h in pv['artifact_sha256'].items():assert sha(package/n)==h
for n,h in pv['source_sha256'].items():assert sha(root/n)==h
result=dict(status='PASS',adaptation_status='FAIL',classification='Coarsening round budget exhausted with pending candidate queue; zero cached invalid cells and complete cached coverage.',binary_sha256=build['binary_sha256'],regressions=regressions,lifecycle=dict(ranks=1,frames=6,successful_engine_rows=6,validation_sha256=sha(where/'termination_lifecycle_mpi1_v1/validation.json')),failed_validation_sha256={n:sha(where/n/'validation.json') for n in names},identical_input_sha256={n:sha(where/names[1]/n) for n in shared},equal_work_counters=equal_counters,diagnostic_rows=rows[1],source_sha256={n:sha(root/n) for n in ('Common/src/adaptation/CNativeEngine3D.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','integration_evidence/native_3d_core_v1/audit_lifecycle.py')},scope='Saved-evidence verification only. Cached private validity is not an independently audited accepted mesh. Original event FAIL and all guards remain unchanged. No accepted/rejected private mesh was exported, no cycle after0 ran, and accepted CFD state/reference preservation is checked by the existing C++ fixture before its sole failing COMPLETE assertion. No performance/scaling claim.')
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
(folder/'REVIEW.md').write_text("""# Native 3D first practical size limit and termination diagnostics

The diagnostic review passes; both adaptation runs remain **FAIL**. The frozen
n8 wedge has405 vertices,1536 tetrahedra and512 planar boundary triangles.
N1M1, targets64/16/64, aspect1.5, spatial rotation/variation0.125, quality>=0.20,
sliver>=0.05, length<=1.8, rounds512 and noise0 are unchanged.

Cycle0 refines successfully privately (457 edits,3552 tetrahedra). Coarsening
uses512 attempts:272 accepted and240 rejected, leaving2489 private tetrahedra.
The newly emitted counters show2489 cached measurements, zero cached invalid
cells,91 pending edge candidates and zero pending repairs. Engine completion
explicitly requires the coarsening queue to be empty. This is budget/queue
incompletion, rather than a recorded remaining frozen-field violation.
The original and diagnostic binaries have identical input/config/source-field
files and all compared integer work/communication counters; see validation.json.

The entire remesh is rejected. The original C++ fixture passes accepted CFD
current/BDF/reference preservation and no-acceptance-callback checks before its
sole COMPLETE assertion fails (5038 assertions pass, one fails). No private
rejected grid is exported; envelope_input.su2 and envelope_s0_initial.json are
inspectable input evidence. No accepted-geometry/current-BDF/output audit ran,
and no later cycle ran. Cached validity is not an independent mesh certificate.

Five O(1) local termination counters were added to EngineStats, captured at every
exit and emitted in existing detailed engine rows. No new loop/collective,
operator, target or acceptance change was introduced. The real full68-object,
774-member MPI/MMG/CGNS build passes. Actual MPI1/2/4 each pass69 selected cases
per rank and independent bridge audits:12/24/48 frames,6/12/24 state pairs and
6/12/24 actual SU2/CGNS output/readback checks. Serial MMG2D/3D passes48 assertions
in two cases.30 package parser/mocked checks pass, including missing/fractional/
negative/corrupt termination counters. These are sequential contended correctness
checks, with sources/objects/archives/tools pinned before and after.
The newly built driver also passes the actual serial lifecycle: six OFF/ON
controls compute the metric/Hessian, remesh, replace geometry/solver/BDF and
write meshes; its independent audit validates six successful engine rows with
all termination counters and complete coverage/empty queues.

The n8 failure is a practical limit of the current bounded attempt policy. Do
not remove the pending-queue acceptance check, increase rounds or soften the
frozen gates to relabel this campaign as passing. The next implementation step
must distinguish scalable useful coarsening from repeated blocked proposals,
retain bounded transactions and validate its changed workload independently.
Local elapsed times are diagnostic only. Goal1 remains active: repeated cluster
performance, instrumentation overhead, larger accepted envelope and full MPI
wait/traffic/vendor/allocator calibration are still unproven.

Recheck saved evidence without numerical launches:

    PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/termination_limit_review_v1/review.py

This also checks the currently qualified binary and source/object/archive pins;
run before replacing that build. Historical raw FAIL logs are preserved.
""")
print('PASS saved review; adaptation remains FAIL',folder)
