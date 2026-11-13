"""Bounded exact child-face certificates; acceptance always remains at1e-4."""
import json,os,sys,time,tempfile
from pathlib import Path
import run_cases as checks
R=Path(__file__).resolve().parent
manifest=R/'residual472_query_replays/manifest.json';data=json.loads(manifest.read_text())
reference=R/'m6_bl_inputs/rans_metric_entry364_v1/m6.su2.native_ref'
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(SU2_NATIVE_TRANSACTION_REFERENCE=str(reference),SU2_NATIVE_TRANSACTION_HAUSD='0.0001',OMPI_MCA_osc='pt2pt')
scratch=R.parents[3]/'integration_evidence/build-integrated-v2/compile-tmp'
os.environ['OMPI_MCA_orte_tmpdir_base']=tempfile.mkdtemp(prefix='residual472query-',dir=scratch)
initial=time.monotonic();original_save=checks.save;original_sample=checks.sample_tree;receipts=[]
own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
for q in data['queries']:
    for label,path in q['inputs'].items():
        remaining=int(300-(time.monotonic()-initial))
        if remaining<=0:raise RuntimeError('Shared300s exact query replay deadline exhausted.')
        query=Path(path).resolve();os.environ['SU2_NATIVE_GEOMETRY_QUERY_INPUT']=str(query)
        case='curved472_query_'+q['context']+'_'+str(q['index'])+'_'+label
        checks.CASES[case]=(1,'[NativeM6ReverseGeometry3D]');seen={}
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
                for p in (query,reference,manifest,Path(__file__).resolve(),R/'residual472_trace_analysis.json'):
                    value[str(p.resolve())]=checks.sha(p)
            if Path(path).name=='command.json':
                value['argv'][value['argv'].index('timeout')+1]=str(remaining);value['timeout_seconds']=remaining
            original_save(path,value)
        checks.save=save;sys.argv=['run_cases.py','build472',case]
        try:checks.main()
        except SystemExit as e:code=e.code or 0
        folder=max((R/'cases').glob(case+'_*'),key=lambda p:p.stat().st_mtime)
        live=[]
        for row in seen.values():
            try:
                f=Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()
                if int(f[19])==row['start_ticks'] and f[0]!='Z':live.append(row)
            except FileNotFoundError:pass
        handles=dict(actual_host_proc=True,all_owned_handles_terminal=not live,sampled_identities=list(seen.values()),wrapper_excluded=own)
        (folder/'terminal_handles.json').write_text(json.dumps(handles,indent=2)+'\n');assert not live,live
        assert code==0,(case,code)
        result=json.loads((folder/'native_final_geometry_source_result.json').read_text())
        row=dict(context=q['context'],index=q['index'],label=label,case=str(folder),result=result,query_input=path)
        receipts.append(row);(R/'residual472_query_replay_progress.json').write_text(json.dumps(receipts,indent=2)+'\n')
        print(json.dumps(row),flush=True)
(R/'residual472_query_replay_completed.json').write_text(json.dumps(dict(receipts=receipts,seconds=time.monotonic()-initial,goal_complete=False,production_changed=False),indent=2)+'\n')
