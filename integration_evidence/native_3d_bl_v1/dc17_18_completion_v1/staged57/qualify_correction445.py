"""Qualify bounded curved contraction only; never BL generation or final mesh acceptance."""
import hashlib,json,re
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
mpi=R/'cases/curved445_surface_cavity_correction_mpi6_20261112T145233_1633856'
serial=R/'cases/curved443_surface_cavity_correction_serial_20261112T143447_1589098'
guards=R/'cases/curved443_correction_guards_serial_20261112T143405_1587329'
repo=R.parents[3]
allowed={str(repo/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_correction_mpi.inc'),str(repo/'integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver')}
receipts=[]
for case in (guards,serial,mpi):
 status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
 terminal=json.loads((case/'terminal_handles_actual.json').read_text());assert terminal['all_owned_handles_terminal'] and terminal['wrapper_verified_terminal']
 pins=json.loads((case/'pins.json').read_text());changed={p for p,h in pins.items() if sha(p)!=h}
 assert changed==(set() if case==mpi else allowed),(case,changed)
 receipts.append(dict(case=str(case),status=status,reused_scope_differences=sorted(changed),status_sha256=sha(case/'status.json')))
rows=[json.loads((mpi/f'native_actual_cavity_correction_mpi_rank{rank}.json').read_text()) for rank in range(6)]
for rank,row in enumerate(rows):
 assert row['rank']==rank
 assert all(row[k] for k in ['metric_accepted','height_accepted','geometry_accepted','parameters_accepted','concurrent_discovery_verified','empty_rank_verified','cached_recomputed_agree','stale_publish_rejected','published'])
assert rows[0]['support_cells']==16 and rows[0]['replacement_cells']==14 and rows[0]['apex']==27313
assert not rows[0]['height_strict'] and rows[0]['q_bad_observed']==rows[0]['j_bad_observed']==10
assert rows[5]['owned_after']==0 and sum(r['owned_after'] for r in rows)==300956
log=re.sub(r'\x1b\[[0-9;]*m','',(mpi/'run.log').read_text())
passes=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',log)
assert len(passes)==6 and all(int(cases)==10 for _,cases in passes),passes
build=json.loads((R/'build445_status.json').read_text());assert build['completed_build'] and build['test_binary_changed'] and not build['authorized_stop']
assert 'no work to do' in json.loads((R/'build445_dryrun_completion.json').read_text())['output']
old=json.loads((R/'build443_status.json').read_text())
cfdbin=str(repo/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD')
assert build['binaries'][cfdbin]==old['binaries'][cfdbin]==sha(cfdbin)
resources=[json.loads(x) for x in (mpi/'resources.jsonl').read_text().splitlines()]
report=dict(operator_mpi_qualified=True,goal_complete=False,production_checkpoint='c0b2b3ef42',build=build,receipts=receipts,ranks=rows,
 mpi_cases_per_rank=10,mpi_assertion_counts_unordered=[int(a) for a,_ in passes],
 sampled_tree_RSS_max_bytes=max(r['sampled_process_tree_rss_bytes'] for r in resources),
 serial_actual_assessment=str(R/'cavity_correction443_serial_assessment.json'),
 proof_scope=['Actual saved M6 curved cavity support crosses migrated owners; rank5 remains empty.',
 'Concurrent 10-step growth and 1-step unchanged-support correction stay aligned.',
 'Complete grown reservation conflicts with an actual published neighbor that was disjoint from seed support.',
 'Exact geometry, current associations, provisional height preservation and surface tangential admission retained.',
 'Retained nodal tensors and recomputed post-transport planning agree; stale publication and resource refusal restore accepted bytes.',
 'Atomic owner-routed publication produces 300956 positive tets; q/J remain observational.'],
 reused_serial_evidence='Only the unused MPI-test fixture and linked test binary differ from443 serial receipts. Production sources, command, input/helper/runtime pins and CFD binary are unchanged; current445 MPI execution confirms the shared operator/transport behavior.',
 limits=['Not strict first-height success, generated BL coverage/progression, complete combined-edge conformity or accepted mesh.',
 'The original serial published mesh stays unchanged; the MPI publication is a bounded test case, not the RANS workflow.',
 'Sampled RSS is not true peak; CPU/collective timers cannot separate useful work from MPI polling.',
 'No pyAMG runtime/software-equivalence comparison.'],
 qualification_script_sha256=sha(__file__))
(R/'cavity_correction445_qualification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(operator_mpi_qualified=True,mpi_cases_per_rank=10,owned_after=300956,strict_height=False,BL_accepted=False,sampled_tree_RSS_max_bytes=report['sampled_tree_RSS_max_bytes'])))
