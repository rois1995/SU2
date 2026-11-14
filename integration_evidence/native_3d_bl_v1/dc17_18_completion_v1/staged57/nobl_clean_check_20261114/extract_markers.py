"""Read existing native markers; reuse the measurement phase-boundary semantics."""
import collections,importlib.util,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'measurement_20261114_v2'))
import analyze_B as old
old.self_check()
PHASE={'initialization_collapse':'collapse_setup','collapse':'collapse_setup','surface':'surface_setup','surface_with_setup':'surface_setup','surface_with_setup_until_failed_return':'surface_setup','surface_until_failed_return':'surface_setup','interior':'interior','interior_until_failed_return':'interior','phase_setup_repartition_staging':'repartition_setup','repartition_commit':'repartition_setup','quality':'quality','quality_until_failed_return':'quality','transition':'alternation_setup','finish':'finish'}
def extract(path):
    path=Path(path)
    if path.is_dir():
        files=sorted(path.glob('rank_output/*/rank.*/stdout'))
        assert len(files)==6, files
        ranks={}; all_details=collections.defaultdict(list)
        for f in files:
            part=extract(f); rank=f.parent.name.split('.')[-1]
            assert set(part['ranks'])=={rank}, (f,list(part['ranks']))
            ranks.update(part['ranks'])
            for r,items in part['all_refine_details'].items():all_details[r].extend(items)
        for r,items in all_details.items():
            unique={}
            for row in items:
                signature=tuple(sorted((k,str(v)) for k,v in row.items() if not k.startswith('source_')))
                unique.setdefault(signature,row)
            ranks[r]['refine_details']=list(unique.values())
        return {'source':str(path),'rank_log_sources':list(map(str,files)),'ranks':ranks,'semantics':part['semantics']}
    stages=collections.defaultdict(list);details=collections.defaultdict(list)
    for number,line in enumerate(Path(path).open(errors='replace'),1):
        matches=list(re.finditer(r'NATIVE3D_(?:STAGE|DETAIL)\s+',line))
        for idx,m in enumerate(matches):
            part=line[m.start():matches[idx+1].start() if idx+1<len(matches) else len(line)]; row=old.parse_fields(part);row['source_line']=number;row['source_file']=str(path)
            if 'rank' not in row:continue
            rank=int(row['rank'])
            if row['record']=='NATIVE3D_STAGE' and 'elapsed' in row:
                row['elapsed']=float(row['elapsed']);stages[rank].append(row)
            elif row['record']=='NATIVE3D_DETAIL' and row.get('stage')=='refine':details[rank].append(row)
    result={}
    for rank,rows in stages.items():
        events=[]
        for number,inv in enumerate(old.split_invocations(rows),1):
            intervals=old.intervals_for_invocation(inv,number);bins={}
            for gap in intervals:
                name=PHASE.get(gap['phase'],gap['phase']);bucket=bins.setdefault(name,{'seconds':0.,'counters':{}});bucket['seconds']+=gap['wall_interval_s']
                for k,v in gap['counter_deltas'].items():bucket['counters'][k]=bucket['counters'].get(k,0)+v
            assert abs(sum(g['wall_interval_s'] for g in intervals)-inv[-1]['elapsed'])<1e-5
            events.append({'invocation':number,'stages':inv,'intervals':intervals,'bins':bins,'exit_marker_present':inv[-1].get('stage')=='exit'})
        result[str(rank)]={'events':events,'refine_details':details[rank]}
    return {'source':str(path),'ranks':result,'all_refine_details':{str(r):v for r,v in details.items()},'semantics':'Disjoint marker-boundary bins; collapse/surface/repartition/alternation include setup. Failed returns included in active phase. No assumed missing exits.'}
if __name__=='__main__':
    assert len(sys.argv)==3,'extract_markers.py LOG OUTPUT'
    data=extract(sys.argv[1]);Path(sys.argv[2]).write_text(json.dumps(data,indent=2)+'\n')
    print({r:[round(e['stages'][-1]['elapsed'],3) for e in v['events']] for r,v in data['ranks'].items()})
