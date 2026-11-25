"""Write FP2-P3 reports from the selected campaign receipts."""
from argparse import ArgumentParser
import json
from pathlib import Path

from checkpoint import HERE, ORDER, build_dir, cut, fingerprint, verify_pins

PARTS=('B0','B1','B2a','B2b','B2c','B','C1','C2','C3','C4','C7','C5','C6')
NEVER=('18 FP1 cohorts','time rule')


def read_selection(path):
    row=cut.read(path)
    verify_pins(row.get('pins',{}))
    return {'path':str(path),'sha256':cut.sha(path),'data':row}


def collect(label, outcome):
    build=build_dir(label)
    receipt=read_selection(build/'build_status.json')
    expected=fingerprint(receipt['data'])
    data={'label':outcome,'build':label,'build_receipt':receipt,
          'production_diff':str(HERE/'production.diff'),'order_sha256':cut.sha(ORDER),'parts':{},
          'workflows':{},'comparisons':{},'not_evaluated':list(NEVER)}
    for part in PARTS:
        path=build/f'{part}.json'
        if not path.is_file():
            data['parts'][part]={'status':'not evaluated'}
            data['not_evaluated'].append(part)
            continue
        seal=read_selection(path)
        assert fingerprint(seal['data'])==expected
        children={}
        for name, item in seal['data']['receipts'].items():
            candidate=read_selection(Path(item['path']))
            assert candidate['sha256']==item['sha256'] and candidate['data']['status']==item['status']
            children[name]=candidate
        data['parts'][part]={'status':seal['data']['status'],'seal':seal,'checks':children}
        if 'scoped' in children:
            parent=children['scoped']['data']
            actual={}
            for name,item in parent['parts'].items():
                found=read_selection(Path(item['path']))
                assert found['data']['status']==item['status']
                actual[name]=found
            data['parts'][part]['scoped_checks']=actual
        if 'workflow' in children:
            summary=children['workflow']['data']
            runs=[]
            for trial in summary['runs']:
                row=read_selection(Path(trial['receipt']))
                assert row['sha256']==trial['sha256']
                restart=Path(row['data']['restart'])
                restart_receipt=read_selection(restart)
                key=str(restart.relative_to(Path(row['path']).parent))
                assert row['data']['artifacts'][key]==restart_receipt['sha256']
                runs.append({**row,'isolated_restart':restart_receipt})
            data['workflows'][part]={'mode':summary['mode'],'runs':runs}
            data['comparisons'][part]={key:summary[key] for key in (
                'pair','first_difference','historical_payloads','off_comparison','s_report') if key in summary}
    return data


def compact(value):
    if value is None:
        return '—'
    if isinstance(value, float):
        return f'{value:.6g}'
    if isinstance(value, (list,dict)):
        return json.dumps(value,separators=(',',':')).replace('|','\\|')
    return str(value).replace('|','\\|')


def table(columns,rows):
    return ['| '+' | '.join(columns)+' |','| '+' | '.join('---' for _ in columns)+' |',
            *('| '+' | '.join(map(compact,row))+' |' for row in rows),'']


def render(data):
    lines=[f"# FP2-P3 build{data['build']}",'',data['label'],'',
           '[Signed order](../inputs/FP2_P3_CLOSURE_DEFAULT.md) · '
           '[Source check](source_check.md) · [Production diff](../production.diff)','',
           '## Gate results','']
    results=[]
    for key,item in data['parts'].items():
        results.append((key,item['status'],None))
        for subkey,check in item.get('checks',{}).items():
            row=check['data']
            if key=='B':
                continue
            results.append((f'{key} {subkey}',row['status'],row.get('first_difference')))
            for name,value in row.get('parts',{}).items():
                if isinstance(value,dict) and 'status' in value:
                    results.append((f'{key} {name}',value['status'],value.get('first_difference')))
    lines+=table(('Gate','Status','First finding'),results)
    lines+=['## Control measurements','']
    if ctl:=data['parts']['B1'].get('checks',{}).get('N1'):
        row=ctl['data']
        lines+=table(('Elapsed s','Projection','Classification work','Traversal work','Scope build work'),[(
            row['elapsed_seconds'],row['identity']['status'],*(row['stage_work'].get(key)
            for key in ('classification_work','traversal_work','scope_build_work')))])
    pairs=[]
    for part,details in data['comparisons'].items():
        for key in ('pair','historical_payloads','off_comparison'):
            if key in details:
                value=details[key]
                pairs.append((part,key,value.get('equal',value.get('exact_difference_count')),
                              value.get('first_difference')))
    for part,checks in (('C2',('saved-current',)),('C3',('scoped',))):
        for name in checks:
            if selected:=data['parts'][part].get('checks',{}).get(name):
                row=selected['data']
                if row.get('pair'):
                    pairs.append((part,name,row['pair']['status'],row['pair'].get('first_difference')))
                for gate,sub in row.get('parts',{}).items():
                    pairs.append((part,gate,sub['status'],sub.get('first_difference')))
    lines+=['## Pairs and historical comparisons','']
    lines+=table(('Part','Comparison','Result','First difference'),pairs)
    g3=[]
    for part in ('B2a','C4'):
        if check:=data['parts'][part].get('scoped_checks',{}).get('G3'):
            row=check['data'].get('result',{})
            g3.append((part,row.get('accepted_splits'),row.get('common_edit_count'),
                       row.get('first_mismatch'),row.get('observer_adjusted_entry_work')))
    if g3:
        lines+=['## G3 evidence','']
        lines+=table(('Part','Splits','Common edits','First mismatch','Observer-adjusted entry work'),g3)
    prefix=[]
    for gate,check in data['parts']['C3'].get('scoped_checks',{}).items():
        history=check['data'].get('historical',{})
        prefix.extend((gate,key,values.get('changed'),values.get('missing'),values.get('extra'))
                      for key,values in history.get('projected_rows',{}).items())
        if history:
            lines += [f"**{gate} historical first difference** {compact(history.get('first_difference'))}",
                      f"**{gate} historical ownership and decisions** {compact(history.get('ownership'))}",'']
    if prefix:
        lines+=table(('Gate','Projected row type','Changed','Missing','Extra'),prefix)
    lines+=['## Workflow measurements','']
    events=[]
    rank_times=[]
    for part,item in data['workflows'].items():
        for trial,handle in enumerate(item['runs'],1):
            row=handle['data']
            restart=handle['isolated_restart']['data']
            for event in row['events']:
                events.append((part,item['mode'],trial,event['event'],event['native_seconds'],
                               event.get('proof_faces'),event.get('handover_faces'),
                               event.get('actual_mesh_counts'),event.get('job_peak_rss_bytes'),
                               event.get('minimum_timer_coverage'),restart.get('seconds')))
            for rank,rank_events in row['rank_timers'].items():
                for event,seconds in rank_events.items():
                    rank_times.append((part,trial,rank,event,seconds['geometry_bound'],
                                       seconds['communication_and_join_wait'],seconds['other']))
    lines+=table(('Part','Mode','Trial','Event','Native s','Proof faces','Hand-over faces','Mesh counts',
                  'Peak bytes','Timer coverage','Restart s'),events)
    lines+=table(('Part','Trial','Rank','Event','Geometry bound s','Communication and join wait s','Other s'),
                 rank_times)
    if references:=data['comparisons'].get('C7',{}).get('s_report'):
        lines+=['## S beside build664 and build672','']
        lines+=table(('Run','Event','Native s','Proof faces','Hand-over faces','Cells','Job peak bytes'),(
            (name,event['event'],event['native_seconds'],event.get('proof_faces'),event.get('handover_faces'),
             event.get('actual_mesh_counts',{}).get('cells'),event.get('job_peak_rss_bytes'))
            for name,record in references.items() for event in record['events']))
        lines+=table(('Run','Rank','Event','Geometry bound s','Communication and join wait s','Other s'),(
            (name,rank,event,seconds['geometry_bound'],seconds['communication_and_join_wait'],seconds['other'])
            for name,record in references.items() for rank,events_by_rank in record['rank_timers'].items()
            for event,seconds in events_by_rank.items()))
        names=tuple(references)
        lines+=table(('Payload',*names),(
            (key,*(references[name]['payload_hashes'][key] for name in names))
            for key in references[names[0]]['payload_hashes']))
    lines+=['## Not evaluated','',*(f'- {name}' for name in data['not_evaluated']),'']
    return '\n'.join(lines)


if __name__=='__main__':
    parser=ArgumentParser(description=__doc__)
    parser.add_argument('build',type=int)
    parser.add_argument('label')
    args=parser.parse_args()
    data=collect(args.build,args.label)
    path=HERE/'reports'/f'FP2_P3_{args.build}'
    cut.save(path.with_suffix('.json'),data)
    path.with_suffix('.md').write_text(render(data))
