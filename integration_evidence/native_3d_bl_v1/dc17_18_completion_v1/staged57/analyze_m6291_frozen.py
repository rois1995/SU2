"""Reuse existing counted-work stage comparison for coupled repair build289."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
source=ROOT/"analyze_m6237_stages.py"
changes=[("euler_build235_geometry_coupled_real_cfd50k","euler_build289_prepared_borders_mach_only_real_cfd50k"),("euler_build237_signed_bounds_real_cfd50k","euler_build291_facewise_certificate_mach_only_real_cfd50k"),("235","289"),("237","291")]
code=source.read_text()
for old,new in changes:
    assert old in code,old
    code=code.replace(old,new)
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
before={str(p):sha(p) for p in (source,Path(__file__).resolve())}
exec(compile(code,str(source),"exec"),{"__file__":str(source.resolve()),"__name__":"frozen291_stage_reuse"})
assert before=={p:sha(p) for p in before}
a=ROOT/"m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k"
(a/"stage_parser_reuse.json").write_text(json.dumps({"changes":changes,"helper_hashes":before,"transformed_sha256":hashlib.sha256(code.encode()).hexdigest(),"scope":"Existing six-rank complete-prefix counters; countedwork and counts versus289, no performance baseline or acceptance claim."},indent=2)+"\n")
