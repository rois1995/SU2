"""Reuse the existing quality/churn checker for build289 versus frozen build291."""
from pathlib import Path
import json,hashlib
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/compare_quality227.py');original=p.read_text()
source=original.replace('euler_build225_entity_shell_real_cfd50k','OLD_ATTEMPT').replace('euler_build227_witness_real_cfd50k','NEW_ATTEMPT')
source=source.replace('225','289').replace('227','291').replace('OLD_ATTEMPT','euler_build289_prepared_borders_mach_only_real_cfd50k').replace('NEW_ATTEMPT','euler_build291_facewise_certificate_mach_only_real_cfd50k')
# Cumulative churn is comparable only within the first uninterrupted quality pass.
needle="  if window['stage']!='window' or window['phase']!='improvement':continue\n"
assert source.count(needle)==1
source=source.replace(needle,needle+"  if any(e['stage']=='quality_exit' and e['round']<=window['round'] for e in progress['events']):continue\n")
assert source.count('assert matching\n')==1
source=source.replace('assert matching\n','# Equal-round matches are optional; nearest measured quality times remain reported.\n')
exec(compile(source,str(p),'exec'),{'__file__':str(p)})
out=p.parent/'m6_runs/euler_build291_facewise_certificate_mach_only_real_cfd50k/quality_comparison289.json';d=json.loads(out.read_text());d['analysis_reuse']={'original_helper':str(p),'original_sha256':hashlib.sha256(original.encode()).hexdigest(),'executed_source_sha256':hashlib.sha256(source.encode()).hexdigest(),'wrapper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'transformation':'Equal-round matches optional; existing225-vs227 checker with explicit baseline289/current291 labels and attempt paths. Cumulative comparison stops at first quality exit; reconstruction changes the census outside those churn counters. Latest-window location/ladder assessment remains separate. No runtime changes.'};out.write_text(json.dumps(d,indent=2)+'\n')

# Join the existing per-rank location/ladder records by their actual census serial, not window ordinal.
attempt=out.parent
progress=json.loads((attempt/'construction_progress_assessment.json').read_text())
events=[e for e in progress['events'] if e['stage']=='window' and e['phase']=='improvement']
if not events:
    print('No complete quality window yet; initial entry preserved.');raise SystemExit(0)
event=events[-1]
locations={};ladders={};service={}
for rank in range(6):
    path=attempt/f'rank_output/1/rank.{rank}/stdout';receipt=progress['source_prefixes'][str(path)]
    raw=path.read_bytes()[:receipt['bytes']];assert hashlib.sha256(raw).hexdigest()==receipt['sha256']
    window=[]
    for line in raw.decode(errors='replace').splitlines():
        if not line.startswith('NATIVE3D_QUALITY '):continue
        row=dict(item.split('=',1) for item in line.split()[1:] if '=' in item)
        if row['stage']=='window' and int(row['round'])==event['round']:
            assert int(row['rank'])==rank and row['complete']=='1';window.append(row)
    assert len(window)==1
    service[str(rank)]=window[0];census=window[0]['census'];found={}
    for line in raw.decode(errors='replace').splitlines():
        prefix=line.split(' ',1)[0]
        if prefix not in ('NATIVE3D_QUALITY_LOCATION','NATIVE3D_QUALITY_LADDER'):continue
        row=dict(item.split('=',1) for item in line.split()[1:] if '=' in item)
        assert int(row['rank'])==rank
        if row['census']!=census:continue
        key=(prefix,row.get('kind'));assert key not in found;found[key]=row
    assert len(found)==4
    location=found[('NATIVE3D_QUALITY_LOCATION',None)];locations[str(rank)]=location
    stage=event['rank_rows'][str(rank)]
    for stem in ('q','j'):
        assert int(location[stem+'_surface'])+int(location[stem+'_interior'])==int(stage[stem+'_bad'])
        assert sum(map(int,location[stem+'_miss_hist'].split(',')))==int(stage[stem+'_bad'])
        assert sum(int(location[stem+'_'+name]) for name in ('feature','no_feature','feature_unknown'))==int(stage[stem+'_bad'])
    assert sum(map(int,location['geometry_excess_hist'].split(',')))==int(stage['geometry_bad'])
    assert max(int(stage['q_bad']),int(stage['j_bad']))<=int(location['bad_union'])<=int(stage['q_bad'])+int(stage['j_bad'])
    assert 0<=int(location['short05'])<=int(location['short09'])<=int(location['bad_union'])
    ladders[str(rank)]={str(k):found[('NATIVE3D_QUALITY_LADDER',str(k))] for k in range(3)}
totals={name:sum(int(row[name]) for row in locations.values()) for name in
        ('q_surface','q_interior','j_surface','j_interior','q_feature','q_no_feature','q_feature_unknown',
         'j_feature','j_no_feature','j_feature_unknown','bad_union','short09','short05')}
histograms={name:[sum(int(row[name].split(',')[i]) for row in locations.values()) for i in range(length)]
            for name,length in [('q_miss_hist',6),('j_miss_hist',6),('geometry_excess_hist',6),('geometry_authority_types',8)]}
yield_by_kind={}
for kind,name in enumerate(('coordinate_moves','bounded_optimizer','collapse_reconnect')):
    rows=[ladders[str(rank)][str(kind)] for rank in range(6)]
    values={field:sum(int(row[field]) for row in rows) for field in
            ('trials','certified','commits','optimizer_work','q_removed','j_removed')}
    values.update({field:sum(float(row[field]) for row in rows) for field in ('seconds','debt_removed')})
    assert 0<=values['commits']<=values['certified']<=values['trials']
    values['rejected']=[sum(int(row['rejected'].split(',')[i]) for row in rows) for i in range(8)]
    yield_by_kind[name]=values
service_totals={name:sum(int(row[name]) for row in service.values()) for name in
                ('commits','q_retired','q_introduced','j_retired','j_introduced','unmeasured_churn',
                 'construction_edits','queue','blocked_features','stuck_shape','stuck_required',
                 'missing_features','private_tracked_collectives')}
assert sum(v['commits'] for v in yield_by_kind.values())==service_totals['commits']
if service_totals['unmeasured_churn']==service_totals['construction_edits']==0:
    for stem in ('q','j'):
        assert sum(v[stem+'_removed'] for v in yield_by_kind.values())==service_totals[stem+'_retired']-service_totals[stem+'_introduced']
header=Path('Common/include/adaptation/CNativeEngine3D.hpp')
result={'round':event['round'],'native_elapsed':event['maximum_native_elapsed'],'totals':event['totals'],
        'same_round_baseline_available':any(e['round']==event['round'] for e in d['matching_rounds']),
        'census_by_rank':{k:int(v['census']) for k,v in service.items()},
        'complete_flags':event['complete_flags'],'service_totals':service_totals,'location_totals':totals,
        'histograms':histograms,'ladder_window_yield':yield_by_kind,'per_rank_location':locations,
        'per_rank_ladder':ladders,'source_prefixes':progress['source_prefixes'],
        'analysis_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'counter_definition_header_sha256':hashlib.sha256(header.read_bytes()).hexdigest(),
        'rejection_order':['geometry_topology','height','shape','progress','measurement','contact','dependency','no_candidate'],
        'scope':'Existing complete-census counters only; no new runtime instrumentation or repair choices. Location interior means no physical face, not necessarily free vertices. Ladder timers overlap and kernel move/debt counters do not isolate published global gain. Rejection flags may overlap; geometry type flags need not be exclusive. Short-edge correlation is not construction causality. Geometry histograms retain source-defined bins; no inferred physical-error units.'}
(attempt/'quality_counter_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'round':result['round'],'census_by_rank':result['census_by_rank'],'location_totals':totals,
                  'stuck_shape':service_totals['stuck_shape'],'missing_features':service_totals['missing_features'],
                  'private_tracked_collectives':service_totals['private_tracked_collectives']}))
