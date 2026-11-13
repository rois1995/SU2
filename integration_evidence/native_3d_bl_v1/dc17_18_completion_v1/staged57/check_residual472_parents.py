"""Replay four original parent faces through existing production guidance; no source change."""
import json,os,sys,tempfile
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
terminal=json.loads((R/'residual472_query_replay_terminal_actual.json').read_text())
assert terminal['all_owned_handles_terminal']
for row in terminal['identities']:
    try:
        f=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
        assert int(f[19])!=row['start_ticks'] or f[0]=='Z',row
    except FileNotFoundError:pass
query=R/'residual472_query_replays/old_parent_faces_v2.bin'
provenance=query.with_suffix('.provenance.json')
reference=R/'m6_bl_inputs/rans_metric_entry364_v1/m6.su2.native_ref'
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(reference),SU2_NATIVE_TRANSACTION_HAUSD='0.0001',
 SU2_NATIVE_GEOMETRY_QUERY_INPUT=str(query),OMPI_MCA_osc='pt2pt',
 OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix='parent472-',dir=scratch))
case='curved472_current_parent_faces_serial';checks.CASES[case]=(1,'[NativeM6GeometryQueries3D]')
seen={};original_sample=checks.sample_tree;original_save=checks.save
own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
def sample(pid):
    rows=original_sample(pid)
    for row in rows:
        try:
            f=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
            seen[(row['pid'],int(f[19]))]=dict(pid=row['pid'],start_ticks=int(f[19]))
        except FileNotFoundError:pass
    return rows
checks.sample_tree=sample
def save(path,value):
    if Path(path).name=='pins.json':
        for p in (query,provenance,reference,Path(__file__).resolve(),R/'prepare_residual472_parent_faces.py'):
            value[str(p)]=checks.sha(p)
    if Path(path).name=='command.json':
        value['argv'][value['argv'].index('timeout')+1]='300';value['timeout_seconds']=300
    original_save(path,value)
checks.save=save;sys.argv=['run_cases.py','build472',case]
try:checks.main()
except SystemExit as e:code=e.code or 0
folder=max((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime)
(folder/'terminal_handles.json').write_text(json.dumps(dict(sampled_identities=list(seen.values()),wrapper_excluded=own),indent=2)+'\n')
print(json.dumps(dict(case=str(folder),exit_code=code)),flush=True)
raise SystemExit(code)
