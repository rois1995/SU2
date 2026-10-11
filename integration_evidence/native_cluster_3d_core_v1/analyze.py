"""Verify compact native3D results and summarize fixed-work paired timings without summing rank maxima."""
from pathlib import Path
import csv,json,statistics,sys
from run import sha,mpip_report
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"integration_evidence/native_3d_core_v1"))
from audit_lifecycle import parse,closes,ENGINE,nested_stage
from profile_summary import profiles

def main(paths):
    if not paths:raise ValueError('Give at least one result folder')
    summaries=[];rows=[];rank_profiles=[];mpi_profiles=[];identity=None
    for folder in map(lambda p:Path(p).resolve(),paths):
        record=json.loads((folder/'result.json').read_text())
        if record['status']!='PASS':raise ValueError('Failed campaign: '+str(folder))
        for name,digest in record['artifact_sha256'].items():
            p=folder/name;p.resolve().relative_to(folder)
            if not p.is_file() or sha(p)!=digest:raise ValueError('Artifact drift: '+str(p))
        key=(record['binary_sha256'],record['package_manifest_sha256'])
        if identity is None:identity=key
        elif identity!=key:raise ValueError('Different binary/package; report as separate campaigns')
        if record['mode']=='lifecycle':
            for case in record['cases']:
                work=folder/case['folder'];work.resolve().relative_to(folder)
                audit=json.loads((work/'lifecycle_audit.json').read_text())
                if audit['status']!='PASS':raise ValueError('Failed lifecycle audit')
                native=[parse(line) for line in (work/'case.stdout').read_text().splitlines() if line.startswith('NATIVE3D ')]
                for row in audit['nested_rank_details']:nested_stage(row)
                rank_profiles.append(dict(campaign=folder.name,case=case['folder'],
                    **profiles(native,audit['rank_stages'],audit['nested_rank_details'])))
                rows.extend(dict(campaign=folder.name,case=case['folder'],record='exclusive',**row) for row in audit['rank_stages'])
                rows.extend(dict(campaign=folder.name,case=case['folder'],record='nested',**row) for row in audit['nested_rank_details'])
                summaries.append(dict(folder=str(folder),mode='lifecycle',ranks=record['ranks'],
                                      fixed_work_timings=audit['fixed_work_timings'],status='PASS'))
            continue
        if record['mode'] not in ('timing','limits','mpi-profile'):
            summaries.append(dict(folder=str(folder),mode=record['mode'],ranks=record['ranks'],status='PASS'));continue
        groups={};fixed={}
        for case in record['cases']:
            (folder/case['folder']).resolve().relative_to(folder)
            p=case['parameters'];workers=p['workers']
            if record['mode'] in ('timing','mpi-profile'):
                if workers in fixed and fixed[workers]!=case['fixed_work_sha256']:raise ValueError('Unmatched fixed work')
                fixed[workers]=case['fixed_work_sha256']
            if record['mode']=='mpi-profile':
                if p.get('detail')!=1 or p.get('mpi_profile') not in (0,1):
                    raise ValueError('Invalid paired MPI profiling controls')
                if p['mpi_profile'] and mpip_report(folder/case['folder'],p['ranks'])!=case.get('mpi_profile_report'):
                    raise ValueError('mpiP report identity changed')
                if p['mpi_profile']:mpi_profiles.append(dict(campaign=folder.name,case=case['folder'],parameters=p,warmup=case['warmup'],**case['mpi_profile_report']))
            audit=json.loads((folder/case['folder']/'independent_audit.json').read_text())
            if audit['status']!='PASS':raise ValueError('Failed independent audit')
            for row in audit['rank_stages']:rows.append(dict(campaign=folder.name,case=case['folder'],record='exclusive',**row))
            details=[]
            for line in (folder/case['folder']/'case.stdout').read_text().splitlines():
                if not line.startswith('NATIVE3D_DETAIL '):continue
                row=parse(line)
                nested_stage(row);details.append(row)
                rows.append(dict(campaign=folder.name,case=case['folder'],record='nested',**row))
            rank_profiles.append(dict(campaign=folder.name,case=case['folder'],warmup=case['warmup'],
                parameters=p,**profiles(audit['native_rows'],audit['rank_stages'],details)))
            if case['warmup']:continue
            for cycle in range(3):
                parent=[r for r in audit['rank_rows'] if r['cycle']==cycle]
                native=[[r for r in audit['native_rows'] if r['rank']==rank][cycle] for rank in range(p['ranks'])]
                edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
                elapsed=max(r['parent_wall'] for r in parent)
                sample=dict(repeat=p['repeat'],seconds=elapsed,accepted=edits,accepted_per_second=edits/elapsed,max_over_mean=elapsed/statistics.mean(r['parent_wall'] for r in parent),bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native))
                if record['mode']=='mpi-profile':sample['process_seconds']=case['stages'][0]['seconds']
                if record['mode']=='limits':summaries.append(dict(folder=str(folder),mode='limits',ranks=p['ranks'],workers=workers,size=p['size'],cycle=cycle,sample=sample,scope='Single practical-limit pilot; no repeated timing or fixed-work cross-partition speedup'))
                else:groups.setdefault((workers,p['mpi_profile'] if record['mode']=='mpi-profile' else p['detail'],cycle),[]).append(sample)
        for workers in fixed:
            for cycle in range(3):
                off=groups[workers,0,cycle];on=groups[workers,1,cycle]
                if {s['repeat'] for s in off}!={0,1,2} or {s['repeat'] for s in on}!={0,1,2}:raise ValueError('Need three paired repetitions')
                if len({s['accepted'] for s in off+on})!=1:raise ValueError('Accepted work changed')
                median=lambda samples:statistics.median(s['seconds'] for s in samples)
                summaries.append(dict(folder=str(folder),mode=record['mode'],ranks=record['ranks'],workers=workers,cycle=cycle,off=off,on=on,off_median=median(off),on_median=median(on),on_off_ratio=median(on)/median(off)))
                if record['mode']=='mpi-profile':
                    process_median=lambda samples:statistics.median(s['process_seconds'] for s in samples)
                    summaries[-1].update(process_on_off_ratio=process_median(on)/process_median(off),
                        profiling_scope='Parent ratios cover adaptation; process ratios also include fixture/output/report finalization. MPI API elapsed is not pure wait; callsite/histogram data remain in the full mpiP reports.')
    destination=Path(paths[0]).resolve().parent/'native3d_summary.json'
    if destination.exists():raise FileExistsError('Keep previous summary; rename it before reanalysis')
    destination.write_text(json.dumps(dict(status='PASS',binary_package_identity=identity,campaigns=summaries,rank_profiles=rank_profiles,mpi_api_profiles=mpi_profiles,scope='Timing campaigns require three paired fixed-work samples per N/M/event; limits campaigns report single independently audited pilots without speedup claims. Max parent wall is elapsed; nested phases and rank-work seconds are separate. Changed N/M trajectories must be independently audited; no cross-rank fixed-work speedup, full MPI-wait/traffic/memory calibration or CFD affordability claim.'),indent=2)+'\n')
    if rows:
        keys=['campaign','case','record','stage','rank','ranks']+sorted({k for row in rows for k in row}-{'campaign','case','record','stage','rank','ranks'})
        with destination.with_suffix('.csv').open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    print('PASS',len(paths),'campaigns;',destination)
if __name__=='__main__':main(sys.argv[1:])
