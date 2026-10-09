"""Run sequential SGE native-adaptation comparisons and export selected evidence."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import time
from collect_native_cluster_results import collect
from native_process_sampler import compute_processes

ROOT = Path(__file__).resolve().parent.parent

def sha(path):
    with path.open('rb') as stream:
        digest=hashlib.sha256()
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()

def closed(path):
    path=path.resolve();path.relative_to(ROOT);return path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kind',choices=('frozen_euler_to_bl','frozen_bl_to_euler','actual_euler_to_bl','actual_bl_to_euler'),default='frozen_euler_to_bl')
    parser.add_argument('--binary',type=Path,required=True);parser.add_argument('--machinefile',type=Path,required=True)
    parser.add_argument('--ranks',type=int,required=True);parser.add_argument('--workers',default='')
    parser.add_argument('--repeat',type=int,default=3);parser.add_argument('--events',type=int,default=10)
    parser.add_argument('--timeout',type=int,default=14400);args=parser.parse_args()
    assert 1<=args.ranks<=192 and args.repeat>=1 and args.events>=1 and args.timeout>0
    workers=list(dict.fromkeys(map(int,args.workers.split(':')))) if args.workers else list(dict.fromkeys((args.ranks,max(1,args.ranks//2),max(1,args.ranks//4))))
    assert workers and all(1<=m<=args.ranks for m in workers)
    variants=[(args.ranks,'NO')]+[(m,'YES') for m in workers]
    binary=closed(args.binary);assert binary.is_file()
    pack=ROOT/'integration_evidence/native_cluster_campaign_v1'
    fixture=pack/'inputs'/args.kind
    input_manifest=json.loads((pack/'input_manifest.json').read_text())['files']
    assert all(sha(path)==input_manifest[str(path.relative_to(pack))]['sha256'] for path in fixture.iterdir() if path.is_file()),'Campaign input changed'
    frozen=args.kind.startswith('frozen')
    job=os.environ.get('JOB_ID','manual')
    assert re.fullmatch(r'[A-Za-z0-9_-]+',job)
    raw=ROOT/'ClusterRaw'/f'{job}_{args.kind}';raw.mkdir(parents=True,exist_ok=False)
    # Scheduler host allocation is copied, so no runtime data path escapes the repository.
    machinefile=raw/'machinefile';shutil.copy2(args.machinefile,machinefile)
    pins=json.loads((pack/'source_pins.json').read_text())
    assert all(sha(ROOT/name)==digest for name,digest in pins.items()),'Source differs from built C++ checkpoint'
    binary_sha=sha(binary)
    environment=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
    try:
        revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True,stderr=subprocess.DEVNULL).strip()
    except (OSError,subprocess.CalledProcessError):
        revision=None  # Compute nodes may lack Git; source hashes still determine the validated C++.
    metadata=dict(kind=args.kind,CFD_ranks=args.ranks,adaptation_workers=workers,partition_variants=variants,repetitions=args.repeat,events=args.events,
                  binary=str(binary.relative_to(ROOT)),binary_sha256=binary_sha,source_revision=revision,source_pins_sha256=sha(pack/'source_pins.json'),
                  source_pins_checked=len(pins),machinefile=machinefile.read_text(),scheduler={key:os.environ.get(key) for key in ('JOB_ID','NSLOTS','QUEUE','PE')})
    build_options=next((parent/'meson-info/intro-buildoptions.json' for parent in binary.parents if (parent/'meson-info/intro-buildoptions.json').is_file()),None)
    metadata['meson_build_options']=json.loads(build_options.read_text()) if build_options else None
    for command in (['lscpu'],['gcc','--version'],['mpicxx','--version'],['mpirun','--version'],['ldd',str(binary)]):
        try:
            result=subprocess.run(command,capture_output=True,text=True)
            metadata[' '.join(command[:2])]=dict(exit=result.returncode,stdout=result.stdout,stderr=result.stderr)
        except OSError as error:
            metadata[' '.join(command[:2])]=dict(unavailable=str(error))
    jobout=ROOT/'ClusterResults'/'jobs'/f'{job}_{args.kind}';jobout.mkdir(parents=True,exist_ok=False)
    (jobout/'campaign.json').write_text(json.dumps(metadata,indent=2)+'\n')
    shutil.copy2(__file__,jobout/Path(__file__).name)
    failures=[]
    for rep in range(args.repeat):
        # Rotate order to avoid always assigning the same worker count the coldest run.
        for m,repartition in variants[rep%len(variants):]+variants[:rep%len(variants)]:
            tag=f'{job}_{args.kind}_n{args.ranks}_m{m}_p{repartition}_r{rep+1}'
            case=raw/tag;case.mkdir()
            for item in fixture.iterdir():
                assert item.is_file() and not item.is_symlink(),item
                shutil.copy2(item,case/item.name)
            cfg=(case/'run.cfg').read_text()
            cfg=re.sub(r'^ADAP_NATIVE_(?:RANKS|REPARTITION)\s*=.*\n?','',cfg,flags=re.M)
            cfg=re.sub(r'^MESH_FILENAME\s*=.*$','MESH_FILENAME= input.su2',cfg,flags=re.M)
            if not frozen:
                freq=int(re.search(r'^ADAP_FREQ\s*=\s*(\d+)',cfg,re.M)[1])
                cfg=re.sub(r'^TIME_ITER\s*=.*$',f'TIME_ITER= {(args.events+1)*freq}',cfg,flags=re.M)
            cfg+=f'\nADAP_NATIVE_REPARTITION= {repartition}\nADAP_NATIVE_RANKS= {m}\n'
            (case/'run.cfg').write_text(cfg)
            env=dict(environment)
            if frozen:env.update(SU2_NATIVE_AIRFOIL_CONFIG=str(case/'run.cfg'),SU2_NATIVE_FROZEN_METRIC=str(case/'frozen_sensor.csv'))
            tail=['[NativeFrozenAirfoil2D]','--use-colour','no'] if frozen else ['-t','1','run.cfg']
            command=['mpirun','-n',str(args.ranks),'-machinefile',str(machinefile),str(binary)]+tail
            record=dict(phase='preparing',working_directory=str(case),ranks=args.ranks,adaptation_ranks=m,repartition=repartition,command=command,binary_sha256=binary_sha,source_revision=metadata['source_revision'],source_pins_sha256=metadata['source_pins_sha256'],
                        inputs_sha256={p.name:sha(p) for p in case.iterdir() if p.is_file()},machine_samples=[])
            evidence=case/'run_evidence.json'
            def save():evidence.write_text(json.dumps(record,indent=2)+'\n')
            start=time.monotonic();save()
            with (case/'solver.log').open('x') as log:
                child=subprocess.Popen(command,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
                record.update(phase='running',child_pid=child.pid);save()
                while True:
                    try:code=child.wait(timeout=5);break
                    except subprocess.TimeoutExpired:
                        sample=dict(elapsed_seconds=time.monotonic()-start,host=os.uname().nodename,loadavg=Path('/proc/loadavg').read_text().strip(),compute_processes=compute_processes(case))
                        pressure=Path('/proc/pressure/cpu')
                        sample['cpu_pressure']=pressure.read_text().strip() if pressure.exists() else None
                        owned=[item for item in sample['compute_processes'] if item['owned_solver']]
                        sample.update(owned_ranks_observed=len(owned),observed_aggregate_rank_rss_kib=sum(item['memory_kib'].get('VmRSS',0) for item in owned))
                        record['machine_samples'].append(sample);save()
                        if sample['elapsed_seconds']>=args.timeout:
                            os.killpg(child.pid,signal.SIGTERM)
                            try:child.wait(timeout=10)
                            except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
                            code=124;break
            log=(case/'solver.log').read_text()
            success=code==0 and (log.count('All tests passed')==args.ranks if frozen else 'Exit Success' in log)
            record.update(phase='terminal',solver_exit=code,elapsed_seconds=time.monotonic()-start,success=success,max_child_rss_kib=resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss,
                          memory_scope='Host-local samples; partial on multiple nodes. Maximum child RSS is cumulative across this campaign, not aggregate rank memory. Frozen rank CSVs provide per-rank HWM.')
            record.pop('child_pid',None);save()
            result=collect(case,ROOT/'ClusterResults'/'cases'/tag)
            print(tag,'solver',code,result['status'],flush=True)
            if not success or result['missing']:failures.append(tag)
    assert sha(binary)==binary_sha,'Binary changed during campaign'
    assert all(sha(ROOT/name)==digest for name,digest in pins.items()),'Source changed during campaign'
    (jobout/'summary.json').write_text(json.dumps(dict(status='PASS_EXECUTION_AND_COLLECTION' if not failures else 'FAILED_CASES',failures=failures,scope='Execution/collection only. Independent mesh/metric/transport audits and matched-work timing assessment remain required.'),indent=2)+'\n')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
