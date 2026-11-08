"""Reuse existing complete-census churn/work analysis for frozen262."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
source=ROOT/"analyze_m6244_mach_only.py"
code=source.read_text()
old="euler_build244_mach_only_real_cfd50k";assert code.count(old)==2
code=code.replace(old,"euler_build262_compact_capture_mach_only_real_cfd50k")
old="if e['stage'] in ('window','exit') and";assert code.count(old)==1
code=code.replace(old,"if all(e['complete_flags'].values()) and e['stage'] in ('window','exit') and")
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
before={str(p):sha(p) for p in (source,Path(__file__).resolve())}
try:exec(compile(code,str(source),"exec"),{"__file__":str(source.resolve()),"__name__":"frozen262_quality_reuse"})
except SystemExit as e:assert e.code==0
assert before=={p:sha(p) for p in before}
a=ROOT/"m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k"
(a/"quality_parser_reuse.json").write_text(json.dumps({"helper_hashes":before,"transformed_sha256":hashlib.sha256(code.encode()).hexdigest(),"changes":["Both literal244attempt paths replacedby262","Require completeflags for window/exit location selection"],"scope":"Existing counters only; churn assertions and net q/J/geometry rates, overlapping timers not added; no acceptance or causal speedup claim."},indent=2)+"\n")
