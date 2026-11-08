"""Reuse sealed actual-face/exact-edge delivery analysis; preserve unknown shape."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent;source=R/'target_delivery253_v1/analyze_leading_edge.py'
A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
out=A/'leading_edge_delivery_reproduction_v1.json';assert not out.exists()
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
code=source.read_text()
changes=[
 ('euler_build247_canonical_mach_only_real_cfd50k','euler_build262_compact_capture_mach_only_real_cfd50k'),
 ("assert row['cache_valid'] and row['q_measured'] and row['j_measured'] and row['geometry_measured']","assert row['cache_valid'] and row['geometry_measured'];unknown_q_J[0]+=not row['q_measured'];unknown_q_J[1]+=not row['j_measured']"),
 ("for rank in range(6):","unknown_q_J=[0,0]\nfor rank in range(6):"),
 ("assert totals=={'q_bad':1148,'j_bad':484,'geometry_bad':1044},totals","assert totals=={'q_bad':0,'j_bad':0,'geometry_bad':4} and unknown_q_J==[4,4],(totals,unknown_q_J)"),
 ("(HERE/'leading_edge_diagnosis.json').write_text","(A/'leading_edge_delivery_reproduction_v1.json').write_text"),
 ('complete247 cached production','complete262 cached production'),
]
for old,new in changes:
 assert code.count(old)==1,(old,code.count(old));code=code.replace(old,new)
before={str(p):sha(p) for p in (source,Path(__file__).resolve(),A/'capture_ready.json',A/'inspection_reader_status.json')}
context={'__file__':str(source),'__name__':'delivery262_reuse'};exec(compile(code,str(source),'exec'),context)
report=json.loads(out.read_text());report['unknown_q_J_counts']={'q':4,'J':4};report['whole_field_shape_certified']=False
prior_path=R/'target_delivery253_v1/leading_edge_diagnosis.json';prior=json.loads(prior_path.read_text())
report['comparison247']=[dict(y=b['y'],triangles247=b['current_triangles'],triangles262=a['current_triangles'],triangle_ratio262_over247=a['current_triangles']/b['current_triangles']) for a,b in zip(report['stations'],prior['stations']) if a['y']==b['y']]
assert len(report['comparison247'])==5
report['scope']+=' Four q/J cells are UNKNOWN, so known-bad zeros do not certify shape. Same geometric station definitions as247; direction bins are diagnostic, not cause/certificate proof.'
report['analysis_reuse']=dict(source_hashes=before,transformed_sha256=hashlib.sha256(code.encode()).hexdigest(),changes=changes,comparison247_sha256=sha(prior_path))
assert before=={p:sha(p) for p in before}
out.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print(json.dumps(dict(comparison=report['comparison247'],lengths=[dict(y=x['y'],span=x['production_metric_lengths_span_dominant']['median'],nose=x['production_metric_lengths_nose_dominant']['median'],extent_ratio=x['geometric_span_over_nose_extent']['median']) for x in report['stations']])))
