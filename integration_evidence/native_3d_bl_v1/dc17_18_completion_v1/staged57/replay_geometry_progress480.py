"""One existing geometry query tests certificate sufficiency for an actual construction decision."""
import json,os,sys,time,tempfile
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
manifest=R/'geometry_progress480_manifest.json';m=json.loads(manifest.read_text());query=Path(m['query_input']);assert checks.sha(query)==m['query_sha256']
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
reference=R/'prepared_reference441.native_ref'
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(reference),SU2_NATIVE_TRANSACTION_HAUSD='0.0001',SU2_NATIVE_GEOMETRY_QUERY_INPUT=str(query),OMPI_MCA_osc='pt2pt')
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp';os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='geometryprogress480-',dir=scratch)
case='curved480_geometry_progress_query';checks.CASES[case]=(1,'[NativeM6ReverseGeometry3D]');seen={};original_sample=checks.sample_tree;original_save=checks.save
own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
def sample(pid):
    rows=original_sample(pid)
    for row in rows:
        try:
            f=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split();seen[(row['pid'],int(f[19]))]=dict(pid=row['pid'],start_ticks=int(f[19]))
        except FileNotFoundError:pass
    return rows
checks.sample_tree=sample

def save(path,value):
    if Path(path).name=='pins.json':
        for p in (query,reference,manifest,Path(__file__).resolve(),R/'prepare_geometry_progress480.py',R/'geometry_refusals477_assessment.json',R/'geometry_refusals477_queries.json.gz',R/'saved_publication479_assessment.json'):value[str(p.resolve())]=checks.sha(p)
    if Path(path).name=='command.json':value['argv'][value['argv'].index('timeout')+1]='300';value['timeout_seconds']=300
    original_save(path,value)
checks.save=save;sys.argv=['run_cases.py','build479',case]
try:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')
except SystemExit as e:code=e.code or 0
folder=max((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime);live=[]
for row in seen.values():
    try:
        f=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
        if int(f[19])==row['start_ticks'] and f[0]!='Z':live.append(row)
    except FileNotFoundError:pass
(folder/'terminal_handles.json').write_text(json.dumps(dict(actual_host_proc=True,all_owned_handles_terminal=not live,sampled_identities=list(seen.values()),wrapper_excluded=own),indent=2)+'\n');assert not live
print(json.dumps(dict(case=str(folder),exit_code=code)),flush=True);raise SystemExit(code)
