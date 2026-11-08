"""Reuse exact initial field comparison for coupled repair build267."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
source=ROOT/"verify_m6260_initial.py"
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
changes=[("euler_build260_paired_geometry_priority_mach_only_real_cfd50k","euler_build267_coupled_geometry_repair_mach_only_real_cfd50k"),("actual frozen260 attempt","actual frozen267 attempt")]
code=source.read_text()
for old,new in changes:
    assert code.count(old)==1,old
    code=code.replace(old,new)
before={str(p):sha(p) for p in (source,Path(__file__).resolve())}
exec(compile(code,str(source),"exec"),{"__file__":str(source.resolve()),"__name__":"initial267_reuse"})
assert before=={p:sha(p) for p in before}
a=ROOT/"m6_runs/euler_build267_coupled_geometry_repair_mach_only_real_cfd50k"
(a/"initial_verifier_reuse.json").write_text(json.dumps({"changes":changes,"helper_hashes":before,"transformed_sha256":hashlib.sha256(code.encode()).hexdigest(),"scope":"Existing completed original-grid restart only;no adapted CFD/acceptance claim."},indent=2)+"\n")
