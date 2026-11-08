"""Reuse sealed partial-capture residual localization for terminal260."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
source=ROOT/"analyze_geometry256_residuals.py"
code=source.read_text()
changes=[("euler_build256_canonical_geometry_debt_mach_only_real_cfd50k","euler_build260_paired_geometry_priority_mach_only_real_cfd50k"),("== 78","== 4"),("All78cached","All4cached")]
for old,new in changes:
    assert code.count(old)==1,old
    code=code.replace(old,new)
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
before={str(p):sha(p) for p in (source,Path(__file__).resolve())}
exec(compile(code,str(source),"exec"),{"__file__":str(source),"__name__":"__main__"})
assert before=={p:sha(p) for p in before}
attempt=ROOT/"m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
(attempt/"geometry260_localization_reuse.json").write_text(json.dumps({"scope":"Read-only checked256localizer reused with explicit path/count substitutions;NOglobalstarcompleteness inference from partialshards.","changes":changes,"helper_hashes_before":before,"helper_hashes_after":before,"transformed_sha256":hashlib.sha256(code.encode()).hexdigest()},indent=2)+"\n")
