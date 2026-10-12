"""Serialized standard/rotating comparison with separately certified trajectories; no node Git or builds."""
from pathlib import Path
import argparse,json,math,os,re,shutil,sys
from common import sha,verify,core

def schedule(ranks):
    for workers in sorted({ranks,max(1,ranks//2)}):
        for repeat in (-1,0,1,2):
            for detail in ((1,) if repeat<0 else ((0,1) if repeat%2==0 else (1,0))):
                for side in (('baseline','candidate') if (repeat+detail)%2==0 else ('candidate','baseline')):
                    yield workers,repeat,detail,side

def runtime(work,p):
    from audit_envelope import parse,closes,transport_counters,nested_stage,stage
    from profile_summary import profiles,NATIVE
    lines=(work/'case.stdout').read_text().splitlines()
    native=[parse(s) for s in lines if s.startswith('NATIVE3D ')]
    envelope=[parse(s) for s in lines if s.startswith('ENVELOPE ')]
    stages=[parse(s) for s in lines if s.startswith('NATIVE3D_STAGE ')]
    details=[parse(s) for s in lines if s.startswith('NATIVE3D_DETAIL ')]
    if len(native)!=3*p['ranks'] or len(envelope)!=3*p['ranks']:raise ValueError('Incomplete event/rank rows')
    for row in native:
        if row['complete']!=1 or row['ranks']!=p['ranks'] or row['workers']!=p['workers']:raise ValueError('Incomplete native event')
        closes(row,NATIVE);transport_counters(row)
    for row in stages:stage(row)
    for row in details:nested_stage(row)
    if len([r for r in stages if r['stage']=='envelope'])!=(3*p['ranks'] if p['detail'] else 0):raise ValueError('Wrong detail control')
    samples=[]
    for cycle in range(3):
        parent=[r for r in envelope if r['cycle']==cycle]
        if len(parent)!=p['ranks'] or {r['rank'] for r in parent}!=set(range(p['ranks'])):raise ValueError('Missing parent rank')
        edits=sum([r for r in native if r['rank']==rank][cycle]['refine_accepted']+[r for r in native if r['rank']==rank][cycle]['coarsen_accepted'] for rank in range(p['ranks']))
        seconds=max(r['parent_wall'] for r in parent)
        if seconds<=0:raise ValueError('Invalid parent wall')
        samples.append(dict(cycle=cycle,seconds=seconds,accepted=edits,accepted_per_second=edits/seconds))
    return dict(samples=samples,profiles=profiles(native,stages,details),native_rows=native,envelope_rows=envelope)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',type=Path,required=True);parser.add_argument('--results',type=Path,required=True)
    parser.add_argument('--machinefile',type=Path,required=True);parser.add_argument('--ranks',type=int,choices=(1,2,4),required=True)
    parser.add_argument('--mode',choices=('correctness','timing'),required=True)
    parser.add_argument('--gate',type=Path);parser.add_argument('--size',type=int,choices=(4,8),default=8)
    parser.add_argument('--timeout',type=int,default=7200)
    parser.add_argument('--mpi-profile',action='store_true',help='Paired existing mpiP controls, detail always ON')
    a=parser.parse_args();bundle=a.package.resolve();manifest=verify(bundle);old=core(bundle)
    if manifest.get('comparison_kind')not in ('rotating_reservation','candidate_batch'):raise ValueError('Wrong scheduler comparison bundle')
    if not a.machinefile.is_file() or a.timeout<1:raise ValueError('Invalid machinefile/timeout')
    if a.mode=='correctness' and a.ranks!=4:raise ValueError('Correctness needs four slots')
    if a.mode=='timing':
        if not a.gate:raise ValueError('Successful comparison correctness gate required')
        gate=json.loads((a.gate/'result.json').read_text())
        if gate['status']!='PASS' or gate['mode']!='correctness' or gate['manifest_sha256']!=sha(bundle/'comparison_manifest.json'):
            raise ValueError('Comparison correctness gate failed or bundle changed')
    if a.mpi_profile:
        if a.mode!='timing' or os.environ.get('LD_PRELOAD') or os.environ.get('MPIP'):
            raise ValueError('MPI profiling needs timing mode and no inherited profiler controls')
        for side in ('baseline','candidate'):
            if 'bin/libmpiP.so' not in old.verify_package(bundle/side)['files_sha256']:
                raise ValueError('Prepare both packages with the same MPI-compatible mpiP library/license')
        if sha(bundle/'baseline/bin/libmpiP.so')!=sha(bundle/'candidate/bin/libmpiP.so'):
            raise ValueError('Profiling libraries differ')
    result=a.results.resolve();result.mkdir(parents=True,exist_ok=False)
    print('Working and retained:',result,flush=True)
    shutil.copyfile(a.machinefile,result/'scheduler_machinefile.txt')
    provenance=result/'provenance';provenance.mkdir()
    shutil.copyfile(bundle/'comparison_manifest.json',provenance/'comparison_manifest.json')
    shutil.copytree(bundle/'suite',provenance/'suite')
    shutil.copytree(bundle/'shared',provenance/'shared')
    for side in ('baseline','candidate'):
        target=provenance/side;target.mkdir()
        shutil.copyfile(bundle/side/'manifest.json',target/'manifest.json')
        for name in manifest['files_sha256']:
            prefix=side+'/'
            if name.startswith(prefix) and not name.startswith(prefix+'bin/') and not name.endswith('/manifest.json'):
                dest=target/name[len(prefix):];dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(bundle/name,dest)
    env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',LC_ALL='C')
    for key in tuple(env):
        if key.startswith('SU2_NATIVE3D_ENVELOPE_'):del env[key]
    cases=[];host=[];status='FAIL';failure=None
    try:
        for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['mpirun','--version'],'mpi_version')]:
            host.append(old.command(args,result,label,env,30))
        if a.mode=='correctness':
            for side in ('baseline','candidate'):
                package=bundle/side;work=result/side
                cmd=[sys.executable,str(package/'integration_evidence/native_cluster_3d_core_v1/run.py'),
                     '--package',str(package),'--results',str(work),'--mode','correctness','--ranks','4','--machinefile',str(a.machinefile.resolve()),'--timeout',str(a.timeout)]
                local=dict(env)
                if manifest['comparison_kind']=='candidate_batch':
                    local['SU2_NATIVE3D_ENVELOPE_BATCH']='1' if side=='baseline' else '8'
                row=old.command(cmd,result,side+'_correctness',local,a.timeout*10)
                if row['exit_code'] or row.get('timeout') or json.loads((work/'result.json').read_text())['status']!='PASS':raise RuntimeError(side+' correctness failed')
                cases.append(dict(side=side,folder=side,stages=[row]))
        else:
            from audit_envelope import WEDGE_MARKERS
            from audit_geometry_bridge import signature,su2_signature,cgns_signature
            references={}
            # Actual Hessian/CFD lifecycle controls precede the frozen-metric timings on both binaries.
            for side in ('baseline','candidate'):
                work=result/(side+'_lifecycle');work.mkdir()
                row=old.command(old.mpi_command(a.ranks,a.machinefile,bundle/side,'[NativeLifecycle3D]'),work,'case',env,a.timeout)
                if row['exit_code'] or row.get('timeout'):raise RuntimeError(side+' lifecycle failed')
                audit=old.command([sys.executable,str(bundle/'shared/audits/audit_lifecycle.py'),str(work),str(a.ranks),str(work/'case.stdout')],work,'audit',env,a.timeout)
                if audit['exit_code'] or audit.get('timeout'):raise RuntimeError(side+' lifecycle audit failed')
                cases.append(dict(side=side,folder=work.name,lifecycle=True,stages=[row,audit],packed=old.compress_json(work)))
            for workers,repeat,detail,side in schedule(a.ranks):
                switch=detail;detail=1 if a.mpi_profile else switch
                work=result/f'n{a.ranks}_m{workers}_{side}_r{repeat}_{"p" if a.mpi_profile else "d"}{switch}';work.mkdir()
                print('CASE_FOLDER='+str(work),flush=True)
                p=dict(ranks=a.ranks,workers=workers,size=a.size,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,
                       diagnostic_rounds=1024 if a.size==8 else 512,repeat=max(0,repeat),detail=detail,restart_format='none')
                if a.mpi_profile:p['mpi_profile']=switch
                local=dict(env,**{f'SU2_NATIVE3D_ENVELOPE_{k.upper()}':str(v) for k,v in dict(size=a.size,workers=workers,anisotropy=1,varying=1,direction=1,height_power=0,wedge=1,aligned=1,rounds=p['diagnostic_rounds'],repeat=max(0,repeat),detail=detail,restart=0).items()})
                if manifest['comparison_kind']=='candidate_batch':
                    p['candidate_batch']=1 if side=='baseline' else 8
                    local['SU2_NATIVE3D_ENVELOPE_BATCH']=str(p['candidate_batch'])
                (work/'parameters.json').write_text(json.dumps(p,indent=2)+'\n')
                (work/'case_environment.json').write_text(json.dumps({k:v for k,v in local.items() if k.startswith('SU2_NATIVE3D_ENVELOPE_')},indent=2)+'\n')
                run=old.command(old.mpi_command(a.ranks,a.machinefile,bundle/side,'[NativeEnvelope3D]',profile=a.mpi_profile and bool(switch)),work,'case',local,a.timeout)
                text=(work/'case.stdout').read_text()
                if run['exit_code'] or run.get('timeout') or len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',text))!=a.ranks:
                    raise RuntimeError('Numerical campaign failed; all files retained: '+str(work))
                fingerprints={name:sha(work/name) for name in old.RAW}
                reference_key=(side,workers)
                if reference_key not in references:
                    row=old.command([sys.executable,str(bundle/'shared/audits/audit_envelope.py'),str(work)],work,'full_audit',env,a.timeout)
                    if row['exit_code'] or row.get('timeout'):raise RuntimeError('First full independent audit failed')
                    certificate=json.loads((work/'independent_audit.json').read_text())
                    if certificate['status']!='PASS':raise RuntimeError('Independent audit did not pass')
                    accepted=[r for r in certificate['frames'] if r['file'] in {f'envelope_s{i}.json' for i in range(3)}]
                    if len(accepted)!=3 or any(not all(math.isfinite(r[k]) for k in ('minimum_mean_ratio','minimum_scaled_jacobian','maximum_length')) or not r['strict_metric_gate'] or r['minimum_mean_ratio']<.20 or r['minimum_scaled_jacobian']<.05 or r['maximum_length']>1.8 for r in accepted):
                        raise ValueError('Strict q/J/L completion contract failed')
                    references[reference_key]=dict(folder=work.name,files=fingerprints,certificate_sha256=sha(work/'independent_audit.json'))
                    other_key=('candidate' if side=='baseline' else 'baseline',workers)
                    if other_key in references and fingerprints['envelope_s0_initial.json']!=references[other_key]['files']['envelope_s0_initial.json']:
                        raise ValueError('Initial geometry/frozen target differs across schedulers')
                    audit_mode='fresh_full_geometry';audit_stage=row
                else:
                    if fingerprints!=references[reference_key]['files']:
                        raise RuntimeError('Trajectory changed within one scheduler/worker group; retain and independently audit before claiming repeatable cost: '+str(work))
                    audit_mode='exact_reference_equivalence';audit_stage=None
                data=json.loads((work/'envelope_s2.json').read_text())
                for cycle in range(3):
                    current=json.loads((work/f'envelope_s{cycle}.json').read_text())
                    if su2_signature(work/f'envelope_mesh_{cycle+1:05d}.su2',WEDGE_MARKERS)!=signature(current,WEDGE_MARKERS):raise ValueError('Actual SU2 output changed')
                for fmt in ('SU2','CGNS'):
                    path=work/('envelope_s2_written_'+fmt+('.su2' if fmt=='SU2' else '.cgns'))
                    if (su2_signature(path,WEDGE_MARKERS) if fmt=='SU2' else cgns_signature(path,WEDGE_MARKERS))!=signature(data,WEDGE_MARKERS):raise ValueError('Actual output/readback changed')
                profiler=old.mpip_report(work,a.ranks) if a.mpi_profile and switch else None
                if a.mpi_profile and not switch and list(work.glob('*.mpiP')):raise ValueError('Profiler output in OFF control')
                measured=runtime(work,p)
                audit=dict(status='PASS',mode=audit_mode,reference=references[reference_key],current_files_sha256=fingerprints,
                           scope='Fresh full audit per scheduler and N/M, or exact current equality to that scheduler reference. Different schedulers and N/M groups are independently certified trajectories, not fixed work.')
                (work/'comparison_audit.json').write_text(json.dumps(audit,indent=2)+'\n')
                case=dict(side=side,folder=work.name,parameters=p,warmup=repeat<0,stages=[run]+([audit_stage] if audit_stage else []),
                          fixed_work_sha256=fingerprints,runtime=measured,audit=audit,mpi_profile_report=profiler)
                if work.name==references[reference_key]['folder']:
                    case['packed']=old.compress_json(work)
                else:
                    removed={}
                    for f in list(work.iterdir()):
                        if f.is_file() and (f.name.startswith('envelope_s') and f.suffix=='.json' or f.suffix in ('.su2','.cgns','.native_ref','.csv')):
                            removed[f.name]=sha(f);f.unlink()
                    case['deduplicated']=removed
                cases.append(case)
        verify(bundle);status='PASS'
    except Exception as error:
        failure=str(error);print('FAIL',failure,flush=True)
    finally:
        record=dict(status=status,failure=failure,mode=a.mode,ranks=a.ranks,size=a.size,mpi_profile=a.mpi_profile,manifest_sha256=sha(bundle/'comparison_manifest.json'),packages=manifest['packages'],cases=cases,host=host,
                    comparison_kind=manifest['comparison_kind'],scope='Actual serialized two-scheduler correctness/lifecycle and paired operational-cost campaign. Independently audited different mesh/P1 trajectories are not fixed work. Timings use rank-max parent wall, excluding external audits; measured process wall includes assertions/output. Nested scopes are not added twice. Wrapper traffic is not wire traffic; elapsed/CPU is not pure wait. No physical-CFD affordability or automatic fullGoal1 claim.')
        record['artifact_sha256']={str(f.relative_to(result)):sha(f) for f in result.rglob('*') if f.is_file()}
        (result/'result.json').write_text(json.dumps(record,indent=2)+'\n')
    print(status,result,flush=True);raise SystemExit(status!='PASS')
if __name__=='__main__':main()
