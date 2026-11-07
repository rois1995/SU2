"""Reuse the existing quality/churn checker for build235 versus frozen build237."""
from pathlib import Path
import json,hashlib
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/compare_quality227.py');original=p.read_text()
source=original.replace('euler_build225_entity_shell_real_cfd50k','OLD_ATTEMPT').replace('euler_build227_witness_real_cfd50k','NEW_ATTEMPT')
source=source.replace('225','235').replace('227','237').replace('OLD_ATTEMPT','euler_build235_geometry_coupled_real_cfd50k').replace('NEW_ATTEMPT','euler_build237_signed_bounds_real_cfd50k')
exec(compile(source,str(p),'exec'),{'__file__':str(p)})
out=p.parent/'m6_runs/euler_build237_signed_bounds_real_cfd50k/quality_comparison235.json';d=json.loads(out.read_text());d['analysis_reuse']={'original_helper':str(p),'original_sha256':hashlib.sha256(original.encode()).hexdigest(),'executed_source_sha256':hashlib.sha256(source.encode()).hexdigest(),'transformation':'Existing225-vs227 checker with explicit baseline235/current237 labels and attempt paths; no runtime changes.'};out.write_text(json.dumps(d,indent=2)+'\n')
