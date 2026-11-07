"""Reuse exact-census analysis for the Mach-only attempt; no combined-target benchmark."""
from pathlib import Path
import hashlib,json
r=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57')
attempt=r/'m6_runs/euler_build244_mach_only_real_cfd50k'
stage_source=(r/'analyze_m6237_stages.py').read_text()
separator='\nimport json,pathlib,hashlib,time\n';assert stage_source.count(separator)==1
stage=stage_source.split(separator)[0].replace('euler_build237_signed_bounds_real_cfd50k',attempt.name)
exec(compile(stage,str(r/'analyze_m6237_stages.py'),'exec'),{})
progress=json.loads((attempt/'construction_progress_assessment.json').read_text())
if not any(e['stage']=='window' and e['phase']=='improvement' for e in progress['events']):raise SystemExit(0)
original=(r/'compare_quality227.py').read_text()
needle="for label,name in [('build225','euler_build225_entity_shell_real_cfd50k'),('build227','euler_build227_witness_real_cfd50k')]:"
assert original.count(needle)==1
source=original.split('matching=[];closest=[]')[0].replace(needle,"for label,name in [('mach_only','euler_build244_mach_only_real_cfd50k')]:")
needle="  if window['stage']!='window' or window['phase']!='improvement':continue\n";assert source.count(needle)==1
source=source.replace(needle,needle+"  if any(e['stage']=='quality_exit' and e['round']<=window['round'] for e in progress['events']):continue\n")
for old,new in [("if row.get('stage')!='window':continue", "if row.get('stage') not in ('window','exit'):continue"), ("if window['stage']!='window' or", "if window['stage'] not in ('window','exit') or")]:
    assert source.count(old)==1;source=source.replace(old,new)
context={};exec(compile(source,str(r/'compare_quality227.py'),'exec'),context)
d={'scope':'Mach-only first uninterrupted quality pass; no same-target baseline. Existing churn invariants and complete six-rank prefixes. Timers overlap; join-call share is not CPU idle. No mesh acceptance.', 'records':context['records']['mach_only'],'matching_rounds':[], 'analysis_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'stage_helper_sha256':hashlib.sha256(stage_source.encode()).hexdigest(),'quality_helper_sha256':hashlib.sha256(original.encode()).hexdigest()}
out=attempt/'quality_progress.json';out.write_text(json.dumps(d,indent=2)+'\n')
location_source=(r/'analyze_m6237_quality.py').read_text();marker='# Join the existing per-rank';assert location_source.count(marker)==1
for old,new in [("if e['stage']=='window' and", "if e['stage'] in ('window','exit') and"), ("if row['stage']=='window' and", "if row['stage']==event['stage'] and")]:
    assert location_source.count(old)==1;location_source=location_source.replace(old,new)
exec(compile(location_source[location_source.index(marker):],str(r/'analyze_m6237_quality.py'),'exec'),globals())
