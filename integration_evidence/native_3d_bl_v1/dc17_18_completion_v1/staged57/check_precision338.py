"""Pinned focused serial / one MPI milestone for sole-destination routing."""
import json,os,sys,tempfile,time
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
mode=sys.argv[1];assert mode in ('precision',)
assert json.loads((R/'build338_terminal_handles.json').read_text())['all_owned_handles_terminal']
case='curved338_'+mode
filters={'precision':'[NativeMetricOutputPrecision3D]'}
checks.CASES[case]=(1,filters[mode])
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ.update(OMPI_MCA_osc='pt2pt',OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix=case+'-',dir=scratch))
seen={};original=checks.sample_tree
own={'pid':os.getpid(),'start_ticks':int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19])}
def sample(pid):
    rows=original(pid)
    for row in rows:
        try:
            v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
            row.update(start_ticks=int(v[19]),user_cpu_ticks=int(v[11]),system_cpu_ticks=int(v[12]))
            seen[(row['pid'],int(v[19]))]=dict(pid=row['pid'],start_ticks=int(v[19]))
        except (FileNotFoundError,ProcessLookupError):pass
    return rows
checks.sample_tree=sample
original_save=checks.save
def save_pinned(path,value):
    if Path(path).name=='pins.json':
        value[str(Path(__file__).resolve())]=checks.sha(Path(__file__))
        parser=R.parents[3]/'TestCases/adaptation/capability/capcheck.py'
        value[str(parser.resolve())]=checks.sha(parser)
    original_save(path,value)
checks.save=save_pinned
sys.argv=['run_cases.py','build338',case]
try:checks.main()
except SystemExit as e:code=e.code or 0
attempt=sorted((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime)[-1]
live=[]
for row in seen.values():
    try:
        v=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
        if int(v[19])==row['start_ticks']:live.append(row)
    except (FileNotFoundError,ProcessLookupError):pass
receipt=dict(actual_host_proc=True,all_owned_handles_terminal=not live,live_owned_handles=live,
             sampled_identities=list(seen.values()),wrapper_excluded=own,checked_unix=time.time())
(attempt/'terminal_handles.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'attempt':str(attempt),'exit_code':code,'sampled_terminal':not live}),flush=True)
assert not live,live
if code==0:
    import numpy as np
    sys.path.insert(0,str(R.parents[3]/'TestCases/adaptation/capability'));import capcheck
    try:
        points,fields,precision=capcheck.read_vtu(attempt/'native_metric_binary64.vtu')
        assert precision=='Float64' and points.shape==(27,3)
        expected_points=np.array([(i%3/2,(i//3)%3/2,(i//9)/2) for i in range(27)],dtype='<f8')
        assert np.array_equal(points.view('<u8'),expected_points.view('<u8'))
        for k,name in enumerate(('XX','XY','XZ','YY','YZ','ZZ')):
            expected=(1. if k in (0,3,5) else .125)+np.ldexp(np.arange(27)*6+k+1,-40)
            actual=fields['Metric_'+name];assert actual.shape==(27,)
            assert np.array_equal(actual.view('<u8'),expected.view('<u8')),name
        result=dict(passed=True,precision=precision,points=27,metric_components=6,bitwise_values_checked=27*9,parser='capcheck.read_vtu',file_sha256=checks.sha(attempt/'native_metric_binary64.vtu'))
    except Exception as error:
        result=dict(passed=False,error=str(error));code=1
    (attempt/'bitwise_roundtrip.json').write_text(json.dumps(result,indent=2)+'\n')
    status=json.loads((attempt/'status.json').read_text());status.update(cpp_exit_code=status['exit_code'],exit_code=code,bitwise_roundtrip=result)
    (attempt/'status.json').write_text(json.dumps(status,indent=2)+'\n');print(json.dumps(result),flush=True)
raise SystemExit(code)
