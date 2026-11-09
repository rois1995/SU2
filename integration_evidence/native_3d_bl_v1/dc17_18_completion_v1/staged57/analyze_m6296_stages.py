"""Existing first-event stage checker,293 baseline versus296 boundary reuse."""
from pathlib import Path
import contextlib,hashlib,io,json
R=Path(__file__).resolve().parent
helper=R/'analyze_m6237_stages.py'
original=helper.read_text();source=original
changes=[('euler_build235_geometry_coupled_real_cfd50k', 'euler_build293_bounded_reference_mach_only_real_cfd50k'), ('euler_build237_signed_bounds_real_cfd50k', 'euler_build296_boundary_reuse_mach_only_real_cfd50k'), ('235', '293'), ('237', '296')]
for old,new in changes:source=source.replace(old,new)
namespace={'__name__':'first_event296_comparison','__file__':str(helper.resolve())}
with contextlib.redirect_stdout(io.StringIO()):exec(compile(source,str(helper),'exec'),namespace)
A=R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k'
receipt=dict(helper=str(helper),helper_sha256=hashlib.sha256(original.encode()).hexdigest(),
    transformed_sha256=hashlib.sha256(source.encode()).hexdigest(),changes=changes,
    scope='Existing complete first-event parser and matching stages. Comparisons may follow different quality trajectories. Counters/timers overlap; join is not total MPI wait. No acceptance or causal speedup claim.')
(A/'stage_parser_reuse.json').write_text(json.dumps(receipt,indent=2)+'\n')
progress=json.loads((A/'construction_progress_assessment.json').read_text())
print(json.dumps({'complete_groups':len(progress['events']),'latest':{k:v for k,v in progress['events'][-1].items() if k!='rank_rows'}} if progress['events'] else {'complete_groups':0}))
