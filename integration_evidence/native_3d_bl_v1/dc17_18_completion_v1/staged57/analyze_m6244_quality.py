"""Reuse verified stage/churn analysis for frozen244; never change repair decisions."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parent
stage_source=(r/'analyze_m6237_stages.py').read_text()
separator='\nimport json,pathlib,hashlib,time\n'
assert stage_source.count(separator)==1
stage=stage_source.split(separator)[0].replace('euler_build237_signed_bounds_real_cfd50k','euler_build244_visibility_real_cfd50k')
exec(compile(stage,str(r/'analyze_m6237_stages.py'),'exec'),{})
p=r/'compare_quality227.py';original=p.read_text()
source=original.replace('euler_build225_entity_shell_real_cfd50k','BASELINE_ATTEMPT').replace('euler_build227_witness_real_cfd50k','CURRENT_ATTEMPT')
source=source.replace('225','BASELINE_LABEL').replace('227','CURRENT_LABEL')
source=source.replace('BASELINE_ATTEMPT','euler_build237_signed_bounds_real_cfd50k').replace('CURRENT_ATTEMPT','euler_build244_visibility_real_cfd50k')
source=source.replace('BASELINE_LABEL','237').replace('CURRENT_LABEL','244')
needle="  if window['stage']!='window' or window['phase']!='improvement':continue\n"
assert source.count(needle)==1
source=source.replace(needle,needle+"  if any(e['stage']=='quality_exit' and e['round']<=window['round'] for e in progress['events']):continue\n")
exec(compile(source,str(p),'exec'),{'__file__':str(p)})
out=r/'m6_runs/euler_build244_visibility_real_cfd50k/quality_comparison237.json'
d=json.loads(out.read_text());d['analysis_reuse']={'wrapper':str(Path(__file__)), 'wrapper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'stage_helper_sha256':hashlib.sha256(stage_source.encode()).hexdigest(),'quality_helper_sha256':hashlib.sha256(original.encode()).hexdigest(),'executed_quality_source_sha256':hashlib.sha256(source.encode()).hexdigest(),'transformation':'Existing stage parser/self-check and first uninterrupted quality-pass churn checks; baseline237/current244 labels only. Matching round is not identical batch history after entry rounds differ. No interpolated counts or source-isolated timing claim.'}
out.write_text(json.dumps(d,indent=2)+'\n')

# Reuse the established exact-census location/ladder joins, including their invariants.
out=out.relative_to(Path.cwd()) if out.is_absolute() else out
location_helper=r/'analyze_m6237_quality.py';location_source=location_helper.read_text()
marker='# Join the existing per-rank'
assert location_source.count(marker)==1
exec(compile(location_source[location_source.index(marker):],str(location_helper),'exec'),globals())
