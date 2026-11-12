"""Verify focused serial and one MPI6 surface-census integration milestone."""
from pathlib import Path
import hashlib,json,time,re
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
checks=[]
for mode in ('serial','mpi6'):
    case=max((R/'cases').glob('curved448_surface_required_handoff_'+mode+'_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    receipt=json.loads((case/'terminal_handles.json').read_text());identities=receipt['sampled_identities']+[receipt['wrapper_excluded']];checked=[]
    for row in identities:
        try:
            v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split();terminal=int(v[19])!=row['start_ticks'] or v[0]=='Z';state=v[0]
        except FileNotFoundError:terminal=True;state='absent'
        assert terminal,row;checked.append(dict(**row,terminal=terminal,state=state))
    final=dict(all_owned_handles_terminal=True,wrapper_verified_terminal=True,actual_host_proc=True,identities=checked,checked_unix=time.time())
    (case/'terminal_handles_actual.json').write_text(json.dumps(final,indent=2)+'\n')
    log=re.sub(r'\x1b\[[0-9;]*m','',(case/'run.log').read_text())
    passed=re.findall(r'All tests passed \(.*?\)',log)
    assert len(passed)==(1 if mode=='serial' else 6),passed
    checks.append(dict(mode=mode,case=str(case),status=status,passed=passed,verified_pins=len(pins),all_actual_handles_terminal=True))
result=dict(surface_required_mpi_qualified=True,cases=checks,
    scope='Known required surface debt handoff, real clean coarsening control, positive full replacements/current metrics, empty ranks, ownership, curved/height and resource regressions.',
    limits='Focused facilities; actual saved-target M6 stage and generated BL/final closure remain separate obligations.',script_sha256=sha(__file__))
(R/'surface_required448_qualification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
