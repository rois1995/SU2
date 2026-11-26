"""Write FP2-P3 reports from the selected campaign receipts."""
from argparse import ArgumentParser
import json
from pathlib import Path

from checkpoint import HERE, ORDER, build_dir, cut, fingerprint, verify_pins

PARTS=('B0','B1','B2a','B2b','B2c','B','C1','C2','C3','C4','C7','C5','C6')
NEVER=('18 FP1 cohorts','time rule')


def file_pin(path):
    return {'path':str(path),'sha256':cut.sha(Path(path))}


def curved_totals(case):
    log=Path(case['used']).with_name('test_outer.log')
    verify_pins({str(log):case['pins'][str(log)]})
    values={'rows':0,'prepared_nodes':0,'work':0}
    for line in log.read_text(errors='replace').splitlines():
        if not line.startswith('NATIVE_SURFACE_MODEL '):
            continue
        fields=dict(field.split('=',1) for field in line.split()[1:])
        values['rows']+=1
        values['prepared_nodes']+=int(fields['prepared'])
        values['work']+=int(fields['work'])
    return values


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
          'workflows':{},'comparisons':{},'curved_model_rows':{},'not_evaluated':list(NEVER)}
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
        if (curved:=children.get('curved')) and curved['data']['status']=='passed':
            data['curved_model_rows'][part]=curved_totals(curved['data'])
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


def accepted_reference(data, commit):
    assert data['label']=='FP2-P3: accepted; the closure is the default'
    assert all(part['status']=='passed' for part in data['parts'].values())
    checks={name:{key:part['seal'][key] for key in ('path','sha256')}
            for name,part in data['parts'].items()}
    workflows={}
    for name in ('C5','C6','C7'):
        phase=data['workflows'][name]
        assert data['comparisons'][name]['pair']['equal']
        runs=[]
        for selection in phase['runs']:
            row=selection['data']
            assert row['closure_variable'] is None
            receipt=file_pin(selection['path'])
            assert receipt['sha256']==selection['sha256']
            restart=file_pin(row['restart'])
            assert restart['sha256']==selection['isolated_restart']['sha256']
            case=Path(row['case'])
            assert len(row['hashes'])==6
            verify_pins({str(case / file):digest for file,digest in row['hashes'].items()})
            config=file_pin(case/'run.cfg')
            assert config['sha256']==row['config_sha256']
            runs.append({'case':str(case),'full_run_receipt':receipt,'isolated_restart_receipt':restart,
                         'effective_config':config,'payload_hashes':row['hashes'],
                         'native_maximum_seconds':{event['event']:event['native_seconds']
                                                   for event in row['events']}})
        assert len(runs)==2 and runs[0]['effective_config']['sha256']==runs[1]['effective_config']['sha256']
        workflows[phase['mode']]={'reference_case':runs[0]['case'],'runs':runs}
    prefixes={}
    for gate, selection in data['parts']['C3']['scoped_checks'].items():
        row=selection['data']
        assert row['pair']['passed'] and len(row['captures'])==2
        captures=[]
        for capture in row['captures']:
            case=Path(capture['case'])
            logs={path:digest for path,digest in capture['pins'].items()
                  if Path(path).is_relative_to(case) and Path(path).name=='stdout'}
            assert len(logs)==6
            validators=[file_pin(path) for path in sorted(case.glob('*gate_receipt.json'))]
            assert any(cut.read(path['path'])==capture['receipt'] for path in validators)
            captures.append({'case':str(case),'rank_logs':logs,'files':capture['pins'],
                             'provenance':file_pin(case/'provenance.json'),
                             'validator_receipts':validators})
        prefixes[gate]={'reference_case':captures[0]['case'],'captures':captures,
                        'evidence':file_pin(selection['path'])}
    g3=data['parts']['C4']['scoped_checks']['G3']
    case=Path(g3['data']['case'])
    physical={'case':str(case),'physical_receipt':file_pin(case/'g3_end_b_gate.json'),
              'evidence':file_pin(g3['path']),'edit_work':g3['data']['result']}
    saved=data['parts']['C2']['checks']['saved-current']
    assert saved['data']['pair']['status']=='passed'
    trials=[]
    for name in ('reference','repeat'):
        row=saved['data']['pair'][name]
        case=Path(row['case'])
        verify_pins({str(case/file):digest for file,digest in row['payloads'].items()})
        assert len(row['payloads'])==3
        trials.append({'case':str(case),'payload_hashes':row['payloads'],
                       'planning_projection':file_pin(row['planning_projection'])})
    return {'accepted':True,'accepted_commit':commit,'order_sha256':data['order_sha256'],
            **fingerprint(data['build_receipt']['data']),'gate_receipts':checks,'workflows':workflows,
            'prefixes':prefixes,'G3':physical,
            'saved_current':{'reference_case':trials[0]['case'],'trials':trials,
                             'evidence':file_pin(saved['path'])}}


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
           '[Source check](source_check.md) · [Production diff](../production.diff) · '
           '[Test code corrections](test_code_corrections.md)','',
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
    if data['curved_model_rows']:
        lines+=['The curved fixture totals count emitted model rows, including diagnostic queries.','']
        lines+=table(('Part','Model rows','Prepared nodes','Work'),(
            (part,row['rows'],row['prepared_nodes'],row['work'])
            for part,row in data['curved_model_rows'].items()))
    shared=data['parts']['C2'].get('checks',{})
    if collapse:=shared.get('first-collapse'):
        receipt=collapse['data']['subruns']['first-collapse'].get('switch_receipt')
        if receipt:
            lines+=table(('First-collapse ranks','Upper work','Work limit','Seconds to stop','Peak bytes'),[(
                receipt['captured_ranks'],receipt['observed_work'],receipt['work_limit'],
                receipt['seconds_until_stop'],receipt['observed_solver_peak_sum_bytes'])])
    if c492:=shared.get('exact-c492'):
        trial=c492['data']['subruns']['bl']
        if c492['data']['status']=='passed':
            raw=Path(trial['raw'])
            verify_pins({str(raw):trial['raw_sha256']})
            row=cut.read(raw)
            stage=cut.read(raw.with_name('actual_saved_surface_stage_rank0.json'))
            lines+=table(('C492 split count','Native admission s','Stop reason','Seconds through final proof',
                          'First differing RunA edit'),[(
                row['actual_splits'],stage['maximum_seconds'],stage['stop'],stage['seconds'],
                row['first_differing_edit'])])
            lines+=['The final proof and result accounting follow the native time-budget stop.','']
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
                    if first:=row['pair'].get('reference'):
                        pairs.append((part,f'{name} historical',first['historical_exact'],
                                      first['historical_first_difference']))
                for gate,sub in row.get('parts',{}).items():
                    pairs.append((part,gate,sub['status'],sub.get('first_difference')))
    lines+=['## Pairs and historical comparisons','']
    lines+=table(('Part','Comparison','Result','First difference'),pairs)
    for part,details in data['comparisons'].items():
        if comparison:=details.get('off_comparison'):
            path=Path(comparison['path'])
            verify_pins({str(path):comparison['sha256']})
            replay=cut.read(path)['replay_differences']
            lines+=[f"**{part} FX2 replay seconds findings** {compact(replay)}",'']
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
    parser.add_argument('--accepted-commit')
    args=parser.parse_args()
    data=collect(args.build,args.label)
    path=HERE/'reports'/f'FP2_P3_{args.build}'
    cut.save(path.with_suffix('.json'),data)
    path.with_suffix('.md').write_text(render(data))
    if args.accepted_commit:
        reference=accepted_reference(data,args.accepted_commit)
        cut.save(HERE/'accepted_reference.json',reference)
