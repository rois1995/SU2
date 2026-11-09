"""Reuse293 saved-data checks for the actual complete FIRST296 replay only."""
from pathlib import Path
import contextlib,hashlib,io,json
R=Path(__file__).resolve().parent
helper=R/'validate_m6293_completion.py'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
before={str(p):sha(p) for p in (helper,Path(__file__).resolve())}
original=helper.read_text();source=original
changes=[
 ('euler_build293_bounded_reference_mach_only_real_cfd50k','euler_build296_boundary_reuse_mach_only_real_cfd50k'),
 ("('construction_progress_assessment.json', 'second_native_progress_assessment.json')","('construction_progress_assessment.json',)"),
 ('for event, (points_expected, cells_expected) in enumerate(((149051, 736323), (172001, 870422)), 1):',
  "for event in (1,):\n    cells_expected = native[event - 1]['totals']['cells']"),
 ("    mesh = capcheck.read_su2(A / names[0])","    mesh = capcheck.read_su2(A / names[0])\n    points_expected = len(mesh.P)\n    assert points_expected > 0"),
 ('assert mesh.dim == 3 and len(mesh.P) == points_expected and len(mesh.E) == cells_expected',
  'assert mesh.dim == 3 and len(mesh.E) == cells_expected'),
 ('value <= 1e-10','abs(value) <= 1e-10'),
 ('workflow_events_independently_verified=2','workflow_events_independently_verified=1'),
 ('euler_adapted293_event','euler_adapted296_event'),
 ('.adapted293-event','.adapted296-event'),
 ('workflow293_independent_validation.json','workflow296_independent_validation.json')]
for old,new in changes:
    assert source.count(old)==1,(old,source.count(old))
    source=source.replace(old,new)
assert source.count(' of 2:')==3
source=source.replace(' of 2:',' of 1:')
A=R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k'
assert not (A/'workflow296_independent_validation.json').exists()
with contextlib.redirect_stdout(io.StringIO()):
    exec(compile(source,str(helper),'exec'),{'__file__':str(helper.resolve()),'__name__':'first296_saved_data_reuse'})
assert before=={p:sha(p) for p in before}
receipt=A/'workflow296_independent_validation.json';result=json.loads(receipt.read_text())
assert result['workflow_events_independently_verified']==1 and len(result['events'])==1
reuse=dict(helper_hashes=before,transformed_sha256=hashlib.sha256(source.encode()).hexdigest(),changes=changes,
    additional_transform='Cycle total2 to1 only; exact saved-data checks retained. Actual output point count and complete native cell census replace historical expected mesh counts. Absolute transfer residual checked.',
    scope='First-adaptation replay296, not logical event2 or completion of the whole goal. Disk restart, BL and RANS qualification remain separate.')
result['validation_reuse']=reuse
receipt.write_text(json.dumps(result,indent=2)+'\n')
checkpoint=Path(result['events'][0]['preserved_checkpoint'])/'checkpoint.json'
metadata=json.loads(checkpoint.read_text());metadata['validation_reuse']=reuse
checkpoint.write_text(json.dumps(metadata,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('validation_helpers','validation_reuse','events')}))
print(json.dumps(result['events'][0]))
