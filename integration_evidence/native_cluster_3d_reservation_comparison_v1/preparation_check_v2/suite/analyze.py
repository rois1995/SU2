"""Verify downloaded compact results and summarize paired operational costs for independently audited scheduler trajectories, spread and overhead."""
from pathlib import Path
import argparse,copy,json,statistics,sys
from campaign import runtime
from common import sha

def spread(values):return dict(median=statistics.median(values),minimum=min(values),maximum=max(values),values=values)

def verified_runtime(folder,parameters,stored):
    observed=runtime(folder,parameters)
    if any(not isinstance(value,dict) or not isinstance(value.get('profiles'),dict) for value in (observed,stored)):
        raise ValueError('Stored runtime summary differs from actual log')
    comparable=copy.deepcopy(observed)
    old_events=stored['profiles'].get('nested_detail_events',[])
    for current,old in zip(comparable['profiles'].get('nested_detail_events',[]),old_events):
        for key in ('partition_active_load','proposal_outcomes'):
            if key not in old['profile']:current['profile'].pop(key,None)
    if comparable!=stored:raise ValueError('Stored runtime summary differs from actual log')
    return observed

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('results',nargs='+',type=Path)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise FileExistsError('Keep prior analysis; choose a fresh output')
    groups={};identity=None;profiles=[];api_profiles=[]
    for folder in a.results:
        folder=folder.resolve();record=json.loads((folder/'result.json').read_text())
        if record.get('comparison_kind')!='rotating_reservation' or record['status']!='PASS' or record['mode']!='timing':raise ValueError('Need successful timing result')
        if any(f.is_symlink() for f in folder.rglob('*')):raise ValueError('Result link found')
        for name,digest in record['artifact_sha256'].items():
            path=folder/name;path.resolve().relative_to(folder)
            if sha(path)!=digest:raise ValueError('Result artifact drift: '+name)
        if identity is None:identity=record['manifest_sha256']
        elif identity!=record['manifest_sha256']:raise ValueError('Different comparison bundles')
        for case in record['cases']:
            if case.get('lifecycle'):continue
            q=case['parameters']
            sys.path[:0]=[str(folder/'provenance/shared/core'),str(folder/'provenance/shared/audits')]
            observed=verified_runtime(folder/case['folder'],q,case['runtime'])
            if case['audit']['status']!='PASS':raise ValueError('Case not audited')
            if record.get('mpi_profile') and q['mpi_profile']:
                from run import mpip_report
                if mpip_report(folder/case['folder'],q['ranks'])!=case.get('mpi_profile_report'):raise ValueError('MPI API report identity changed')
                api_profiles.append(dict(campaign=folder.name,case=case['folder'],parameters=q,report=case['mpi_profile_report']))
            profiles.append(dict(campaign=folder.name,case=case['folder'],parameters=q,warmup=case['warmup'],profiles=observed['profiles']))
            if case['warmup']:continue
            for sample in observed['samples']:
                kind='mpi-profile' if record.get('mpi_profile') else 'detail'
                key=(q['ranks'],q['workers'],q['size'],sample['cycle'],kind,q.get('mpi_profile',q['detail']),case['side'])
                groups.setdefault(key,[]).append(dict(repeat=q['repeat'],process_seconds=case['stages'][0]['seconds'],**sample))
    comparisons=[];overhead=[]
    for ranks,workers,size,cycle,kind in sorted({key[:5] for key in groups}):
        for detail in (0,1):
            base=sorted(groups[ranks,workers,size,cycle,kind,detail,'baseline'],key=lambda r:r['repeat'])
            cand=sorted(groups[ranks,workers,size,cycle,kind,detail,'candidate'],key=lambda r:r['repeat'])
            if len(base)!=3 or len(cand)!=3 or [r['repeat'] for r in base]!=[0,1,2] or [r['repeat'] for r in cand]!=[0,1,2]:raise ValueError('Three complete paired repetitions required')
            if any(len({r['accepted'] for r in rows})!=1 for rows in (base,cand)):raise ValueError('Accepted work changed within a scheduler')
            comparisons.append(dict(ranks=ranks,workers=workers,size=size,cycle=cycle,control_kind=kind,detail=detail,
                                    baseline=spread([r['seconds'] for r in base]),candidate=spread([r['seconds'] for r in cand]),
                                    paired_candidate_baseline_ratio=spread([b['seconds']/x['seconds'] for x,b in zip(base,cand)]),
                                    baseline_accepted_per_second=spread([r['accepted_per_second'] for r in base]),
                                    candidate_accepted_per_second=spread([r['accepted_per_second'] for r in cand]),baseline_accepted=base[0]['accepted'],candidate_accepted=cand[0]['accepted'],comparison_kind='independently_audited_changed_trajectory'))
        for side in ('baseline','candidate'):
            off=sorted(groups[ranks,workers,size,cycle,kind,0,side],key=lambda r:r['repeat'])
            on=sorted(groups[ranks,workers,size,cycle,kind,1,side],key=lambda r:r['repeat'])
            if {r['accepted'] for r in off} != {r['accepted'] for r in on}:raise ValueError('Accepted work changed between detail/profiler controls')
            overhead.append(dict(ranks=ranks,workers=workers,size=size,cycle=cycle,control_kind=kind,side=side,
                                 paired_on_off_ratio=spread([y['seconds']/x['seconds'] for x,y in zip(off,on)])))
    result=dict(status='PASS',manifest_sha256=identity,comparisons=comparisons,instrumentation_overhead=overhead,rank_profiles=profiles,mpi_api_profiles=api_profiles,
                comparison_kind='rotating_reservation',scope='Three paired same-N/M operational-cost repetitions. Artifacts repeat within each scheduler, but accepted meshes/work and later frozen P1 references may differ between schedulers; rank-max parent wall, no sum of phase maxima or nested costs. Different N/M groups have separately audited trajectories, not automatic fixed-work scaling. Wrapper bytes/CPU are not physical wire traffic/pure wait. Manufactured metric/flow fields cannot establish physical-CFD affordability or fullGoal1 completion.')
    a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS',len(comparisons),'comparisons',a.output)
if __name__=='__main__':main()
