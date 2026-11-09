"""Seal native acceptance and transfer separately from failed checkpoint publication."""
from pathlib import Path
import hashlib,json,math,re,sys,time
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k'
B=R/'m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
assert json.loads((A/'terminal_handles.json').read_text())['all_owned_handles_terminal']
status=json.loads((A/'status.json').read_text());assert status['state']=='terminal' and status['pins_verified'] and not status['changed']
pins=json.loads((A/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
source=R/'analyze_m6289_terminal.py';code=source.read_text().split('x,y=window(A),window(B)')[0]
namespace={'__file__':str(source)};exec(compile(code,str(source),'exec'),namespace)
current=namespace['window'](A);prior=namespace['window'](B)
log=(A/'run.log').read_text();reason='Native3D reference decoding exceeds storage budget.'
assert reason in log and 'Writing the adapted mesh of cycle 1:' in log
assessment=json.loads((A/'workflow_assessment.json').read_text())
assert assessment['events'][0]['native']['native_event_accepted']
details=[dict(x.split('=',1) for x in line.split()[1:] if '=' in x) for line in log.splitlines() if line.startswith('NATIVE3D_DETAIL stage=refine ')]
assert len(details)==6 and {int(x['rank']) for x in details}==set(range(6))
assert all(x['stop']=='complete' and x['complete']=='1' for x in details)
table=log.split('Per field: S_n content and S_d content',1)[1].split('States not admissible after the projection',1)[0]
residuals={}
for line in table.splitlines():
    if ':' not in line:continue
    name,values=line.split(':',1);tokens=values.split()
    if name.strip() in ('Density','Momentum-x','Momentum-y','Momentum-z','Energy'):
        assert len(tokens)==7;value=float(tokens[-2]);assert math.isfinite(value) and value<=1e-10
        residuals[name.strip()]=value
assert len(residuals)==5
assert re.search(r'States not admissible after the projection[^\n]*: 0;',log)
assert 'Mesh replaced in ' in log
mesh=A/'adapted_mesh_adap_00001.su2';assert mesh.stat().st_size<128<<20
sys.path.insert(0,str(R.parents[3]/'TestCases/adaptation/capability'))
import capcheck,numpy as np
m=capcheck.read_su2(mesh);assert m.dim==3 and len(m.P)==149051 and len(m.E)==736323
assert np.isfinite(m.P).all() and m.E.min()>=0 and m.E.max()<len(m.P)
resources=[json.loads(line) for line in (A/'resources.jsonl').read_text().splitlines()]
out=dict(schema='native291_terminal_publication_diagnosis_v1',time=time.time(),native_accepted=True,
    native=current,previous_refused_native=prior,mesh_points=len(m.P),mesh_cells=len(m.E),
    mesh_file=dict(path=str(mesh),bytes=mesh.stat().st_size,sha256=sha(mesh)),
    conservative_transfer_completed=True,true_mass_residuals=residuals,projected_inadmissible_states=0,
    target_integral_gate='Successful pinned distributed transfer enforces scaled target defect<=1e-12 before publication; numerical final target defects are not printed. Old/new domain integral change is not that measurement.',
    checkpoint_publication_failed=True,failure=reason,native_ref_present=(Path(str(mesh)+'.native_ref')).exists(),
    continued_adapted_cfd_verified=False,disk_restart_verified=False,complete_workflow_events=0,
    all_pins_verified=True,pin_count=len(pins),manual_stop_sent=False,
    sampled_tree_peak_rss_bytes=max(x['sampled_process_tree_rss_bytes'] for x in resources),
    memory_note='Sampled aggregate RSS, not physical peak.',
    artifact_hashes={str(A/name):sha(A/name) for name in ['run.log','status.json','terminal_handles.json','workflow_assessment.json','pins.json']},
    reused_window_parser=dict(path=str(source),sha256=sha(source),executed_prefix_sha256=hashlib.sha256(code.encode()).hexdigest()),
    next_target='Bounded reference encoding/loading admission and usable disk persistence; preserve qualified geometry proof and construction.')
(A/'terminal_publication_diagnosis.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({k:v for k,v in out.items() if k not in ('native','previous_refused_native','artifact_hashes','reused_window_parser')}))
