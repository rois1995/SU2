"""Verify compact native3D results and summarize fixed-work paired timings without summing rank maxima."""
from pathlib import Path
import csv,json,statistics,sys
from run import sha
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"integration_evidence/native_3d_core_v1"))
from audit_lifecycle import parse,closes,ENGINE

def main(paths):
    if not paths:raise ValueError('Give at least one result folder')
    summaries=[];rows=[];identity=None
    for folder in map(lambda p:Path(p).resolve(),paths):
        record=json.loads((folder/'result.json').read_text())
        if record['status']!='PASS':raise ValueError('Failed campaign: '+str(folder))
        for name,digest in record['artifact_sha256'].items():
            p=folder/name;p.resolve().relative_to(folder)
            if not p.is_file() or sha(p)!=digest:raise ValueError('Artifact drift: '+str(p))
        key=(record['binary_sha256'],record['package_manifest_sha256'])
        if identity is None:identity=key
        elif identity!=key:raise ValueError('Different binary/package; report as separate campaigns')
        if record['mode'] not in ('timing','limits'):
            summaries.append(dict(folder=str(folder),mode=record['mode'],ranks=record['ranks'],status='PASS'));continue
        groups={};fixed={}
        for case in record['cases']:
            (folder/case['folder']).resolve().relative_to(folder)
            p=case['parameters'];workers=p['workers']
            if record['mode']=='timing':
                if workers in fixed and fixed[workers]!=case['fixed_work_sha256']:raise ValueError('Unmatched fixed work')
                fixed[workers]=case['fixed_work_sha256']
            audit=json.loads((folder/case['folder']/'independent_audit.json').read_text())
            if audit['status']!='PASS':raise ValueError('Failed independent audit')
            for row in audit['rank_stages']:rows.append(dict(campaign=folder.name,case=case['folder'],record='exclusive',**row))
            for line in (folder/case['folder']/'case.stdout').read_text().splitlines():
                if not line.startswith('NATIVE3D_DETAIL '):continue
                row=parse(line)
                if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
                elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
                elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
                elif row['stage']!='source':raise ValueError('Unknown nested detail stage')
                rows.append(dict(campaign=folder.name,case=case['folder'],record='nested',**row))
            if case['warmup']:continue
            for cycle in range(3):
                parent=[r for r in audit['rank_rows'] if r['cycle']==cycle]
                native=[[r for r in audit['native_rows'] if r['rank']==rank][cycle] for rank in range(p['ranks'])]
                edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
                elapsed=max(r['parent_wall'] for r in parent)
                sample=dict(repeat=p['repeat'],seconds=elapsed,accepted=edits,accepted_per_second=edits/elapsed,max_over_mean=elapsed/statistics.mean(r['parent_wall'] for r in parent),bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native))
                if record['mode']=='limits':summaries.append(dict(folder=str(folder),mode='limits',ranks=p['ranks'],workers=workers,size=p['size'],cycle=cycle,sample=sample,scope='Single practical-limit pilot; no repeated timing or fixed-work cross-partition speedup'))
                else:groups.setdefault((workers,p['detail'],cycle),[]).append(sample)
        for workers in fixed:
            for cycle in range(3):
                off=groups[workers,0,cycle];on=groups[workers,1,cycle]
                if {s['repeat'] for s in off}!={0,1,2} or {s['repeat'] for s in on}!={0,1,2}:raise ValueError('Need three paired repetitions')
                if len({s['accepted'] for s in off+on})!=1:raise ValueError('Accepted work changed')
                median=lambda samples:statistics.median(s['seconds'] for s in samples)
                summaries.append(dict(folder=str(folder),mode='timing',ranks=record['ranks'],workers=workers,cycle=cycle,off=off,on=on,off_median=median(off),on_median=median(on),on_off_ratio=median(on)/median(off)))
    destination=Path(paths[0]).resolve().parent/'native3d_summary.json'
    if destination.exists():raise FileExistsError('Keep previous summary; rename it before reanalysis')
    destination.write_text(json.dumps(dict(status='PASS',binary_package_identity=identity,campaigns=summaries,scope='Timing campaigns require three paired fixed-work samples per N/M/event; limits campaigns report single independently audited pilots without speedup claims. Max parent wall is elapsed; nested phases and rank-work seconds are separate. Changed N/M trajectories must be independently audited; no cross-rank fixed-work speedup, full MPI-wait/traffic/memory calibration or CFD affordability claim.'),indent=2)+'\n')
    if rows:
        keys=['campaign','case','record','stage','rank','ranks']+sorted({k for row in rows for k in row}-{'campaign','case','record','stage','rank','ranks'})
        with destination.with_suffix('.csv').open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    print('PASS',len(paths),'campaigns;',destination)
if __name__=='__main__':main(sys.argv[1:])
