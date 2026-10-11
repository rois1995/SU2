"""Run a frozen native3D bundle sequentially; no Git, build or external input links."""
from pathlib import Path
from mpi_profile import summary as mpi_summary
import argparse,gzip,hashlib,json,os,re,shutil,signal,subprocess,sys,time

SELECTION='[NativeReference3D],[NativeMesh3D],[NativeDonorReuse3D],[NativeCollapseFilter3D],[NativeCoplanar3D],[NativeInsertion3D],[NativeAutomaticRepair3D],[NativeBudget3D],[NativeDefault3D],[NativeBridge2D],[NativeBridge3D]'
RAW=tuple(f'envelope_s{c}{suffix}.json' for c in range(3) for suffix in ('_initial','','_states'))+tuple(f'envelope_s2_written_{fmt}_reload{suffix}.json' for fmt in ('SU2','CGNS') for suffix in ('','_target'))

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def verify_package(folder):
    folder=Path(folder).resolve();record=json.loads((folder/'manifest.json').read_text())
    if record['status']!='PREPARED_NOT_VALIDATED':raise ValueError('Wrong bundle status')
    if any(p.is_symlink() for p in folder.rglob('*')):raise ValueError('Bundle contains a symbolic link')
    actual={str(p.relative_to(folder)) for p in folder.rglob('*') if p.is_file()}
    if 'bin/test_driver' not in record['files_sha256'] or actual!=set(record['files_sha256'])|{'manifest.json'}:
        raise ValueError('Incomplete bundle or unexpected unpinned file')
    for name,digest in record['files_sha256'].items():
        p=folder/name
        p.resolve().relative_to(folder)
        if not p.is_file() or sha(p)!=digest:
            raise ValueError('Frozen bundle changed: '+name)
    return record

def mpi_command(ranks,machinefile,bundle,selection,profile=False):
    args=['mpirun','-n',str(ranks),'-machinefile',str(Path(machinefile).resolve())]
    if profile:
        # Preload only the application, never mpirun or the independent auditor.
        args+=['env','LD_PRELOAD='+str(bundle/'bin/libmpiP.so'),'MPIP=-k 4 -y -p -n -t 0']
    return args+[str(bundle/'bin/test_driver'),selection,'--use-colour','no']

def mpip_report(work,ranks):
    reports=list(work.glob('*.mpiP'))
    if len(reports)!=1:raise ValueError('Expected one full mpiP report; preloading may have failed')
    report=reports[0];text=report.read_text()
    tasks=re.findall(r'^@ MPI Task Assignment\s*:\s*(\d+)\s',text,re.M)
    if sorted(map(int,tasks))!=list(range(ranks)):
        raise ValueError('mpiP report does not cover all CFD ranks')
    if not all(label in text for label in ('@ Version','MPI Time (seconds)','Callsites:','Allreduce')):
        raise ValueError('Incomplete mpiP report')
    return dict(file=report.name,sha256=sha(report),summary=mpi_summary(text,ranks),
                scope='Whole-process MPI API/callsite statistics, including assertions/output. API elapsed is not pure wait or physical wire traffic.')

def command(args,cwd,label,env,seconds):
    start=time.monotonic();record=dict(command=list(map(str,args)),label=label,cwd=str(cwd))
    with (cwd/(label+'.stdout')).open('w') as out,(cwd/(label+'.stderr')).open('w') as err:
        p=subprocess.Popen(args,cwd=cwd,env=env,stdout=out,stderr=err,start_new_session=True)
        try:p.wait(timeout=seconds)
        except subprocess.TimeoutExpired:
            os.killpg(p.pid,signal.SIGTERM)
            try:p.wait(timeout=10)
            except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait()
            record['timeout']=True
    record.update(exit_code=p.returncode,seconds=time.monotonic()-start)
    return record

def compress_json(folder):
    packed={}
    for p in sorted(folder.glob('*.json')):
        if not p.name.startswith(('envelope_s','lifecycle_m','runtime_')):continue
        data=p.read_bytes();destination=p.with_suffix(p.suffix+'.gz')
        if destination.exists():raise FileExistsError(destination)
        with destination.open('wb') as f:
            with gzip.GzipFile(filename='',mode='wb',fileobj=f,mtime=0) as z:z.write(data)
        if gzip.decompress(destination.read_bytes())!=data:raise ValueError('Compression verification failed')
        packed[p.name]=dict(sha256=hashlib.sha256(data).hexdigest(),gzip_sha256=sha(destination),bytes=len(data),packed_bytes=destination.stat().st_size)
        p.unlink()
    return packed

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',type=Path,required=True);parser.add_argument('--results',type=Path,required=True)
    parser.add_argument('--mode',choices=('correctness','lifecycle','timing','limits','mpi-profile'),required=True)
    parser.add_argument('--ranks',type=int,choices=(1,2,4),required=True)
    parser.add_argument('--gate',type=Path,action='append',default=[]);parser.add_argument('--machinefile',type=Path,required=True);parser.add_argument('--timeout',type=int,default=7200)
    options=parser.parse_args();bundle=options.package.resolve();manifest=verify_package(bundle)
    if not options.machinefile.is_file() or options.timeout<1:raise ValueError('Missing machinefile or invalid timeout')
    if options.mode in ('correctness','limits') and options.ranks!=4:raise ValueError('Correctness/limits allocation requires four slots')
    if options.mode=='mpi-profile':
        if 'bin/libmpiP.so' not in manifest['files_sha256']:
            raise ValueError('Prepare a new bundle with --mpip-library first')
        if os.environ.get('LD_PRELOAD') or os.environ.get('MPIP'):
            raise ValueError('Unset inherited LD_PRELOAD and MPIP for paired profiling controls')
    result=options.results.resolve();result.mkdir(exist_ok=False,parents=True)
    print('Working and retained result folder:',result,flush=True)
    # Retain code/inputs/provenance once, excluding binaries and the build tree.
    (result/'package').mkdir()
    for name in manifest['files_sha256']:
        if name.startswith('bin/'):continue
        source=bundle/name;destination=result/'package'/name;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,destination)
    shutil.copyfile(bundle/'manifest.json',result/'package/manifest.json')
    shutil.copyfile(options.machinefile,result/'scheduler_machinefile.txt')
    env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',LC_ALL='C')
    for key in tuple(env):
        if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
    tools=bundle/'integration_evidence/native_3d_core_v1'
    def run(work,ranks,selection,local_env=env,parameters=None):
        work.mkdir()
        if parameters is not None:(work/'parameters.json').write_text(json.dumps(parameters,indent=2)+'\n')
        (work/'case_environment.json').write_text(json.dumps({k:v for k,v in local_env.items() if k.startswith('SU2_NATIVE3D_ENVELOPE_') or k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')},indent=2)+'\n')
        print('Case working and retained folder:',work,flush=True)
        stages=[command(mpi_command(ranks,options.machinefile,bundle,selection,bool((parameters or {}).get('mpi_profile'))),work,'case',local_env,options.timeout)]
        (work/'stages.json').write_text(json.dumps(stages,indent=2)+'\n')
        out=(work/'case.stdout').read_text()
        if stages[0]['exit_code'] or stages[0].get('timeout') or 'No test cases matched' in out or out.count('All tests passed')!=ranks:
            (work/'stages.json').write_text(json.dumps(stages,indent=2)+'\n')
            raise RuntimeError('Test failure; original files retained in '+str(work))
        if selection==SELECTION and len(re.findall(r'All tests passed \(\d+ assertions in 69 test cases\)',out))!=ranks:
            raise RuntimeError('Unexpected configured regression selection/count')
        if selection.startswith('MMG interface:') and '48 assertions in 2 test cases' not in out:
            raise RuntimeError('Incomplete MMG regression')
        return stages
    def audit(work,script,*args):
        record=command([sys.executable,str(tools/script),str(work),*map(str,args)],work,'audit',env,options.timeout)
        (work/'audit_stage.json').write_text(json.dumps(record,indent=2)+'\n')
        if record['exit_code'] or record.get('timeout'):raise RuntimeError('Audit failure; original files retained in '+str(work))
        return record
    status='FAIL';cases=[];stages=[];failure=None
    try:
        for gate in options.gate:
            check=json.loads((gate/'result.json').read_text())
            if check['status']!='PASS' or check['package_manifest_sha256']!=sha(bundle/'manifest.json'):
                raise RuntimeError('Required previous campaign failed or used another package: '+str(gate))
        for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['mpirun','--version'],'mpi_version')]:
            stages.append(command(args,result,label,env,30))
        versions=command([sys.executable,'-c','import numpy,h5py; print("numpy",numpy.__version__,"h5py",h5py.__version__)'],result,'audit_dependencies',env,30)
        stages.append(versions)
        if versions['exit_code']:raise RuntimeError('numpy/h5py required for independent auditors')
        if options.mode=='correctness':
            work=result/'mmg_serial';s=run(work,1,'MMG interface: 2D round trip,MMG interface: 3D round trip')
            cases.append(dict(folder=work.name,stages=s))
            for ranks in (1,2,4):
                work=result/f'unit_mpi{ranks}';s=run(work,ranks,SELECTION)
                cases.append(dict(folder=work.name,stages=s))
            for ranks,workers,fmt in ((1,1,'su2'),(4,2,'cgns')):
                work=result/f'restart_n{ranks}_m{workers}_{fmt}'
                p=dict(ranks=ranks,workers=workers,size=2,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format=fmt)
                local=dict(env,**{f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=2,workers=workers,anisotropy=1,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=512,repeat=0,detail=1,restart=(1 if fmt=='su2' else 2)).items()})
                s=run(work,ranks,'[NativeEnvelope3D]',local,p);s.append(audit(work,'audit_envelope.py'))
                cases.append(dict(folder=work.name,parameters=p,stages=s,packed=compress_json(work)))
        elif options.mode=='lifecycle':
            work=result/f'lifecycle_n{options.ranks}';s=run(work,options.ranks,'[NativeLifecycle3D]')
            s.append(audit(work,'audit_lifecycle.py',options.ranks,work/'case.stdout'))
            cases.append(dict(folder=work.name,stages=s,packed=compress_json(work)))
        elif options.mode=='limits':
            # A single pilot per N/M, stopping on the first failure; do not turn this into a speedup claim.
            for ranks,workers in ((1,1),(2,2),(4,2),(4,4)):
                work=result/f'limit_size8_n{ranks}_m{workers}'
                p=dict(ranks=ranks,workers=workers,size=8,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=0,detail=1,restart_format='none')
                local=dict(env,**{f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=8,workers=workers,anisotropy=1,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=512,repeat=0,detail=1,restart=0).items()})
                s=run(work,ranks,'[NativeEnvelope3D]',local,p);s.append(audit(work,'audit_envelope.py'))
                cases.append(dict(folder=work.name,parameters=p,warmup=False,stages=s,fixed_work_sha256={name:sha(work/name) for name in RAW},packed=compress_json(work)))
        else:
            # One warm-up, then three paired repetitions. OFF/ON order alternates; each starts from the same input.
            for workers in sorted({options.ranks,max(1,options.ranks//2)}):
                reference=None
                for repeat in (-1,0,1,2):
                    for switch in ((1,) if repeat<0 else ((0,1) if repeat%2==0 else (1,0))):
                        profiling=options.mode=='mpi-profile'
                        detail=1 if profiling else switch
                        work=result/f'n{options.ranks}_m{workers}_r{repeat}_{"p" if profiling else "d"}{switch}'
                        p=dict(ranks=options.ranks,workers=workers,size=4,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=512,repeat=max(0,repeat),detail=detail,restart_format='none')
                        if profiling:p['mpi_profile']=switch
                        local=dict(env,**{f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=4,workers=workers,anisotropy=1,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=512,repeat=max(0,repeat),detail=detail,restart=0).items()})
                        s=run(work,options.ranks,'[NativeEnvelope3D]',local,p);s.append(audit(work,'audit_envelope.py'))
                        report=mpip_report(work,options.ranks) if profiling and switch else None
                        if profiling and not switch and list(work.glob('*.mpiP')):
                            raise ValueError('Profiler output appeared in the disabled control')
                        data={name:sha(work/name) for name in RAW}
                        if reference is None:reference=data
                        elif data!=reference:raise RuntimeError('Fixed-work grid/source/state/readback differs; retained '+str(work))
                        cases.append(dict(folder=work.name,parameters=p,warmup=repeat<0,stages=s,fixed_work_sha256=data,mpi_profile_report=report,packed=compress_json(work)))
        verify_package(bundle);status='PASS'
    except Exception as error:
        failure=str(error);raise
    finally:
        record=dict(status=status,failure=failure,mode=options.mode,ranks=options.ranks,package_manifest_sha256=sha(bundle/'manifest.json'),binary_sha256=manifest['files_sha256']['bin/test_driver'],mpi_profile_library_sha256=manifest['files_sha256'].get('bin/libmpiP.so'),cases=cases,stages=stages,
                    scope='Actual native3D configured test driver and independent existing exact audits. Case wall includes test assertions/output; remesh parents and phases are retained in raw logs. Audit/compression/collection outside adaptation. Frozen manufactured envelope timings are distinct from actual metric/Hessian lifecycle. No complete traffic/wait/allocator calibration, larger-rank/scaling/fullGoal1 or physical-unsteady affordability certificate.')
        record['artifact_sha256']={str(p.relative_to(result)):sha(p) for p in result.rglob('*') if p.is_file()}
        (result/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(status,result,flush=True)
    if status!='PASS':raise RuntimeError('Campaign failed')
if __name__=='__main__':main()
