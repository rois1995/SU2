"""Reuse current-source fixture preparation in a new isolated data directory."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
selection=A/'post_refinement_cleanup_selection_v1.json'
source=R/'prepare_current237_witnesses.py'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
code=source.read_text()
changes=[
 ('out = run / "current_witnesses_v1"','out = run / "post_refinement_cleanup_inputs_v1"'),
 ('    assert any(row["physical_faces"] == 0 for row in chosen)\n',''),
 ('    assert any(row["physical_faces"] > 0 and not row["geometry_bad"] for row in chosen)\n',''),
 ('    assert any(row["physical_faces"] > 0 and row["geometry_bad"] for row in chosen)\n',''),
 ('for first, second, _ in metric["edges"]:\n                    values.extend((first, second, math.nan))','for first, second, length in metric["edges"]:\n                    assert length is not None and math.isfinite(length)\n                    values.extend((first, second, length))'),
]
for old,new in changes:
 assert code.count(old)==1,(old,code.count(old));code=code.replace(old,new)
before={str(p):sha(p) for p in (source,Path(__file__).resolve(),selection,A/'inspection_reader_status.json')}
context={'__file__':str(source),'__name__':'cleanup_input_preparation'}
exec(compile(code,str(source),'exec'),context);context['prepare'](A,selection)
assert before=={p:sha(p) for p in before}
p=A/'post_refinement_cleanup_inputs_v1/witness_preparation.json';report=json.loads(p.read_text())
report.update(schema='native_current_source263_preparation_v1',expected_unknown_q_J=0,production_metric_semantics='Current dependency nodal tensors transported onto candidate connectivity; empty M6 composition/edge breakpoints; no immutable original-global target.',preparer_reuse=dict(changes=changes,helper_hashes=before,transformed_sha256=hashlib.sha256(code.encode()).hexdigest()),scope='Four complete actual clean contexts and exact cached q/J/edge oracles; unchanged current-source fixture payload. DATA PREPARATION ONLY. No production operator or admissible replacement demonstrated; test-only clean-cleanup fixture extension must wait for frozen271 terminal state.')
p.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(report['cases']),paired_rows=report['paired_rows'],outputs=len(report['outputs']),seconds=report['seconds'])))
