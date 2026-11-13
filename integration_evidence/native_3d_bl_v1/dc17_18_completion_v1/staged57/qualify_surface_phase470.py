"""Verify the connected provisional reconstruction milestone without claiming BL readiness."""
import hashlib,json,re
from pathlib import Path
from assess_surface_transaction470 import terminal_receipt
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def verify(case):
    state=json.loads((case/'status.json').read_text())
    assert state['state']=='terminal' and state['exit_code']==0 and state['pins_verified']
    pins=json.loads((case/'pins.json').read_text())
    for p,h in pins.items():assert sha(p)==h,p
    receipt=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return dict(case=str(case),status=state,pins_verified=len(pins),actual_handles_terminal=True)
serial=R/'cases/curved470_surface_transaction_serial_20261113T012249_1303657'
focus=R/'cases/curved470_physical_row_serial_20261113T012224_1302549'
mpi=max((R/'cases').glob('curved470_surface_phase_mpi6_*'),key=lambda p:p.stat().st_mtime)
receipts=[verify(c) for c in (focus,serial,mpi)]
log=re.sub(r'\x1b\[[0-9;]*m','',(mpi/'run.log').read_text())
passes=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',log)
assert len(passes)==6 and all(int(n)==32 for _,n in passes),passes
rows=[json.loads((mpi/f'native_actual_cavity_correction_mpi_rank{r}.json').read_text()) for r in range(6)]
for rank,row in enumerate(rows):
    assert row['rank']==rank
    for key in ('metric_accepted','height_accepted','geometry_accepted','parameters_accepted','concurrent_discovery_verified','empty_rank_verified','cached_recomputed_agree','stale_publish_rejected','published'):assert row[key],(rank,key)
assert rows[5]['owned_after']==0 and sum(r['owned_after'] for r in rows)==300956
assert rows[0]['support_cells']==16 and rows[0]['replacement_cells']==14 and rows[0]['apex']==27313
assert not rows[0]['height_strict']
a=json.loads((R/'surface_transaction470_assessment.json').read_text())
trial=next(x for x in a['operations'] if x['attempt']==19)
assert trial['old_cells']==3 and trial['new_cells']==10 and trial['published']
for key in ('metric','parameters','contact_protocol','contact','preserved_heights','provisional_height','reserved'):assert trial[key],key
assert trial['new_geometry_bad']==trial['old_geometry_bad'] and trial['new_geometry_excess']<trial['old_geometry_excess']
build=json.loads((R/'build470_status.json').read_text());assert build['completed_build'] and build['test_binary_changed'] and not build['authorized_stop']
assert 'no work to do' in json.loads((R/'build470_dryrun_completion.json').read_text())['output']
for p,h in build['binaries'].items():assert sha(p)==h
resources=[json.loads(x) for x in (mpi/'resources.jsonl').read_text().splitlines()]
report=dict(surface_phase_mpi_qualified=True,goal_complete=False,strict_BL_qualified=False,receipts=receipts,build=build,
    mpi_cases_per_rank=32,mpi_assertions_unordered=[int(a) for a,_ in passes],actual_serial_reconstruction=trial,mpi_control=rows,
    largest_sampled_tree_rss_bytes=max(x['sampled_process_tree_rss_bytes'] for x in resources),
    scope=['Strict/provisional reconstruction, protected passing rows and curved first-child fixtures on every rank.',
    'First-row engine, coupled-corner repair, empty ranks, resource expiry and constraints under MPI.',
    'Actual saved M6 cross-owner growth/reservation/conflict/stale rollback and cache/recompute control uses existing coarsening correction.',
    'The newly admitted actual 3to10 M6 reconstruction is serial evidence; its exact cavity was not replayed through MPI publication.',
    'No geometry closure, tangential readiness, first-row readiness or accepted BL/RANS workflow is established.'])
(R/'surface_phase470_qualification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(mpi_cases_per_rank=32,assertions=report['mpi_assertions_unordered'],all_actual_handles_terminal=True,surface_phase_mpi_qualified=True,strict_BL_qualified=False)))
