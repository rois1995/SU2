"""Sequential fixed-work native3D probe comparison; tiny serial controls, not MPI scaling."""
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

root=Path(__file__).resolve().parents[2]
inputs=dict(baseline=Path(sys.argv[1]).resolve(),candidate=Path(sys.argv[2]).resolve())
folder=Path(sys.argv[3]).resolve()
print('Comparison working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
runner=Path(__file__).resolve();initial_runner=hashlib.sha256(runner.read_bytes()).hexdigest()
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',LC_ALL='C')
records=[];status='FAIL';receipts={};binaries={};golden=None
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
try:
    assert len(sys.argv)==4
    for label,path in inputs.items():
        receipt=json.loads((path/'validation.json').read_text());receipts[label]=receipt
        assert receipt['status']=='PASS' and not receipt['changed_during_run']
        assert json.loads((path/'independent_audit.json').read_text())['status']=='PASS'
        binary=path/'coupled_probe';binaries[label]=binary
        assert sha(binary)==receipt['artifact_sha256']['coupled_probe']
        frames={p.name:sha(p) for p in path.glob('*.json') if p.name.endswith(('_initial.json','_adapted.json','_coarsened.json'))}
        assert len(frames)==15
        if golden is None:golden=frames
        else:assert golden==frames,'different independently audited meshes'
        (folder/(label+'_producer_receipt.json')).write_text(json.dumps(receipt,indent=2)+'\n')
    changed=[p for p,h in receipts['baseline']['source_sha256'].items() if receipts['candidate']['source_sha256'].get(p)!=h]
    assert changed==['Common/src/adaptation/CNativePredicates3D.cpp'],changed
    old=json.loads((inputs['baseline']/'probe_summary.json').read_text())
    fixed=[{k:v for k,v in row.items() if not k.endswith('_seconds') and k!='field_timings'} for row in old]
    assert sum(r['accepted_splits']+r['accepted_coarsenings'] for r in fixed)==182
    order=[('baseline','OFF'),('candidate','OFF'),('candidate','ON'),('baseline','ON')]
    for repetition in range(1,4):
        for label,mode in (order if repetition%2 else list(reversed(order))):
            out=folder/(label+'_'+mode.lower()+'_r'+str(repetition));out.mkdir()
            print('Case working and retained folder:',out,flush=True)
            host=subprocess.check_output(['ps','-e','-o','pid,comm,pcpu','--sort=-pcpu'],text=True)
            (out/'host_processes_before.txt').write_text(host)
            load=Path('/proc/loadavg').read_text().strip()
            command=['/usr/bin/time','-v','-o',str(out/'resources.txt'),'nice','-n','19',str(binaries[label]),str(out),mode]
            start=time.monotonic();p=subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
            wall=time.monotonic()-start
            (out/'probe.stdout').write_text(p.stdout);(out/'probe.stderr').write_text(p.stderr)
            assert p.returncode==0,(label,mode,repetition,p.stderr)
            rows=json.loads((out/'probe_summary.json').read_text())
            observed=[{k:v for k,v in row.items() if not k.endswith('_seconds') and k!='field_timings'} for row in rows]
            assert observed==fixed,'changed workload/metric residuals'
            for name,h in golden.items():assert sha(out/name)==h,name
            for row in rows:
                assert row['field_timings']==(mode=='ON')
                exclusive=sum(v for k,v in row.items() if k in ('field_build_seconds','selection_seconds','geometry_seconds',
                    'metric_seconds','commit_seconds','final_gate_seconds','output_seconds','other_seconds'))
                assert abs(exclusive-row['parent_seconds'])<=1e-12*max(1,row['parent_seconds'])
                for key in ('fine_query_seconds','fine_edge_seconds','fine_edge_trace_seconds','coarse_edge_seconds'):
                    assert row[key]>=0 and (mode=='ON' or row[key]==0)
            resources=(out/'resources.txt').read_text();peak=None;user=system=None
            for line in resources.splitlines():
                if 'Maximum resident set size (kbytes):' in line:peak=int(line.rsplit(':',1)[1])
                if 'User time (seconds):' in line:user=float(line.rsplit(':',1)[1])
                if 'System time (seconds):' in line:system=float(line.rsplit(':',1)[1])
            assert peak is not None and user is not None and system is not None
            parent=sum(row['parent_seconds'] for row in rows)
            records.append(dict(label=label,field_timings=mode,repetition=repetition,command=command,
                exit_code=p.returncode,process_wall_seconds=wall,summed_case_parent_seconds=parent,
                accepted_edits=182,accepted_edits_per_parent_second=182/parent,peak_rss_kib=peak,
                process_cpu_seconds=user+system,load_before=load,
                exclusive_seconds={key:sum(r[key] for r in rows) for key in ('field_build_seconds','selection_seconds',
                  'geometry_seconds','metric_seconds','commit_seconds','final_gate_seconds','output_seconds','other_seconds')}))
            print(label,mode,'repeat',repetition,'parent seconds',parent,flush=True)
    summary=[]
    for label in inputs:
        for mode in ('OFF','ON'):
            rows=[r for r in records if r['label']==label and r['field_timings']==mode]
            times=[r['summed_case_parent_seconds'] for r in rows]
            summary.append(dict(label=label,field_timings=mode,repetitions=len(rows),median_seconds=statistics.median(times),
                                minimum_seconds=min(times),maximum_seconds=max(times),maximum_rss_kib=max(r['peak_rss_kib'] for r in rows)))
    med={(r['label'],r['field_timings']):r['median_seconds'] for r in summary}
    (folder/'comparison.json').write_text(json.dumps(dict(summary=summary,records=records,
      off_median_speed_ratio=med['baseline','OFF']/med['candidate','OFF'],
      timer_overhead_ratio={label:med[label,'ON']/med[label,'OFF'] for label in inputs},
      fixed_work=fixed,mesh_sha256=golden,
      scope='Three alternating-order one-core serial repetitions per binary/detail mode, identical independently audited manufactured meshes and work counts. Parent includes field setup/selection/geometry/metric/commit/final/output. CPU/RSS/process wall retained. Not a distributed scaling or CFD affordability qualification.'),indent=2)+'\n')
    status='PASS'
finally:
    if sha(runner)!=initial_runner:status='FAIL'
    for label,binary in binaries.items():
        if sha(binary)!=receipts[label]['artifact_sha256']['coupled_probe']:status='FAIL'
    (folder/'runner_snapshot.py').write_bytes(runner.read_bytes())
    result=dict(status=status,runner_initial_sha256=initial_runner,runner_final_sha256=sha(runner),records=records,
                artifact_sha256={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()})
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(status,folder,flush=True)
    if status!='PASS':raise RuntimeError('Fixed-work comparison failed; evidence retained')
