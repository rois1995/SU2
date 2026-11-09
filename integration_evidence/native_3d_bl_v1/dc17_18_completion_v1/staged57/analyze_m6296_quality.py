"""Existing whole-field quality/churn comparison at complete first-event censuses."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parent
helper=R/'compare_quality227.py';original=helper.read_text();source=original
changes=[('euler_build225_entity_shell_real_cfd50k','euler_build293_bounded_reference_mach_only_real_cfd50k'),('euler_build227_witness_real_cfd50k','euler_build296_boundary_reuse_mach_only_real_cfd50k'),('225','293'),('227','296')]
for old,new in changes:source=source.replace(old,new)
needle="  if window['stage']!='window' or window['phase']!='improvement':continue\n"
assert source.count(needle)==1
source=source.replace(needle,needle+"  if any(e['stage']=='quality_exit' and e['round']<=window['round'] for e in progress['events']):continue\n")
assert source.count('assert matching\n')==1
source=source.replace('assert matching\n','# Trajectories can differ; compare nearest measured complete quality times too.\n')
exec(compile(source,str(helper),'exec'),{'__file__':str(helper)})
out=R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k/quality_comparison293.json'
d=json.loads(out.read_text());d['analysis_reuse']=dict(helper=str(helper),helper_sha256=hashlib.sha256(original.encode()).hexdigest(),transformed_sha256=hashlib.sha256(source.encode()).hexdigest(),changes=changes,scope='Cumulative churn is comparable only before the first shape exit; subsequent geometry/size reconstruction is assessed separately. Join-call time is not total MPI wait. Nearest census times differ as reported; no interpolation or causal speedup claim.')
out.write_text(json.dumps(d,indent=2)+'\n')
