"""Run FP2-P3 focused fixtures, G1 and the closure-off N1 control."""
from argparse import ArgumentParser
from contextlib import contextmanager
import json
import os
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

from build_p3 import HERE, PATCH, cut, verify_build
from checkpoint import ORDER, fingerprint, host_preflight, pins_for, require, seal, verify_pins

sys.path.insert(0, str(HERE.parent / 'FP2_P2'))
import run_p2 as p2
from seal_d1 import stage_totals

SWITCH, TRACE = p2.SWITCH, p2.TRACE
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=0'
FOCUSED = ('off-serial', 'off-mpi', 'curved')
B0 = (*FOCUSED, 'public-off')
C1 = (*p2.G1, *FOCUSED, *p2.HIDDEN)
BASE = '8f1325ef35'
N1_SECONDS = 3261


def settings(off, trace, observer=False):
    env = {**p2.m3.environment(trace, tolerance=observer)}
    if off:
        env[SWITCH] = '0'
    return env


def bind_argv(argv, off):
    i = argv.index('env')
    if not off:
        argv[i+1:i+1] = ['-u', SWITCH]
    if 'mpirun' in argv:
        i = argv.index('mpirun') + 1
        keys = (SWITCH, TRACE) if off else (TRACE,)
        argv[i:i] = [element for key in keys for element in ('-x', key)]


@contextmanager
def bind_case(off):
    original = cut.save

    def save(path, value):
        if path.name == 'command.json' and 'argv' in value:
            bind_argv(value['argv'], off)
        return original(path, value)

    os.environ.pop(SWITCH, None)
    with patch.object(cut, 'save', save):
        yield


def prepare(build):
    with patch.object(p2.lab, 'HERE', p2.inherited.c1.B):
        row, binary, pins = p2.inherited.c3.prior.BASELINE_PREPARE(build)
    assert verify_build(row['build']) == row
    pins.update(pins_for((ORDER, ORDER.with_suffix('.sha256'), PATCH, Path(__file__),
                          Path(p2.__file__), HERE / 'reports/source_check.md', build / 'build_plan.json')))
    return row, binary, pins


def fixture_root(label, part, mode):
    return HERE / 'fixtures' / f'build{label}' / part / mode


def rejoin(path):
    with patch.object(p2, 'MARKER', MARKER):
        return p2.rejoin_serial_marker(path)


def switch_receipt(case, env, ranks, entries, incomplete=False):
    command = case / 'command.json'
    argv = cut.read(command)['argv']
    off = SWITCH in env
    for key, value in env.items():
        assert [word for word in argv if word.startswith(key+'=')] == [key+'='+value], (case, key)
    assert off or (f'{SWITCH}=0' not in argv and any(argv[i:i+2] == ['-u',SWITCH] for i in range(len(argv))))
    if ranks > 1:
        assert any(argv[i:i+2] == ['-x',SWITCH] for i in range(len(argv))) == off, case
        assert any(argv[i:i+2] == ['-x',TRACE] for i in range(len(argv))), case
    logs = {0: case / 'test_outer.log'} if ranks == 1 else {
        int(path.parent.name.rsplit('.', 1)[1]): path for path in case.glob('rank_output/*/rank.*/stderr')}
    counts = {}
    for rank in range(ranks):
        log = logs.get(rank)
        if log is None or not log.is_file():
            counts[str(rank)] = None
            continue
        with p2.lab.archived.open_log(log) as stream:
            counts[str(rank)] = sum(p2.a.ANSI.sub('', line).strip() == MARKER for line in stream)
        if rank == 0 and ranks == 1 and off and counts['0'] == 0:
            derived = rejoin(log)
            if derived != log:
                counts['0'] = 1
    missing = next((rank for rank in range(ranks) if counts[str(rank)] != int(off and rank in entries)
                    and not (incomplete and counts[str(rank)] in (None, 0))), None)
    files = [command, *(p for p in logs.values() if p.is_file())]
    files.extend(p for name in (p2.REJOINED,p2.REJOIN_RECEIPT) if (p:=case/name).is_file())
    return {'environment': {**{SWITCH: None}, **env}, 'query_evidence_ranks': sorted(entries),
            'rank_marker_counts': counts, 'passed': missing is None, 'pins': pins_for(files),
            'first_failure': None if missing is None else {'rank': missing, 'field': 'switch marker count',
                     'expected': int(off and missing in entries), 'observed': counts[str(missing)]}}


def check_script(script, *args):
    proc = subprocess.run([sys.executable, '-B', str(script), *map(str,args)],
                          capture_output=True, text=True)
    row = {'returncode': proc.returncode, 'stderr': proc.stderr}
    try:
        row['result'] = json.loads(proc.stdout)
    except json.JSONDecodeError:
        row['stdout'] = proc.stdout
    return row


def run_fixture(build, part, mode):
    label = cut.read(build/'build_status.json')['build']
    root = fixture_root(label, part, mode)
    summary = root/'case_summary.json'
    if summary.is_file():
        row = cut.read(summary)
        verify_pins(row['pins'])
        return summary, row
    row, binary, pins = prepare(build)
    if part == 'C1':
        pins.update(pins_for(require(label,'B')))
    off = part == 'B0' or mode == 'public-off'
    env = settings(off, mode in ('curved','public-off','public-on'))
    if mode in p2.G1:
        target = root/'inherited_summary.json'
        with bind_case(off), patch.multiple(p2.m3, base_dir=lambda *_: root,
                summary_path=lambda *_: target, prepare=lambda _: (row,binary,pins.copy()),
                environment=lambda *_: env):
            source, data = p2.m3.run_g1(build, mode, None)
        if source != target:
            source.rename(target)
        (cohort,) = data['cohorts'].values()
        used = Path(cohort['used'])
        ranks = {'G1-serial': 1, 'G1-mpi2': 2, 'G1-mpi4': 4}[mode]
        marker = switch_receipt(used.parent,env,ranks,set(),incomplete=data['status']!='done')
        verdict = 'passed' if data['status']=='done' else 'FAILED'
        details = cohort
        evidence = [target,used,used.with_name('test_wait.json'),used.with_name('pins.json')]
    else:
        hidden = p2.m1.fixtures.hidden
        hidden.clear_options()
        launch = hidden.c1.run_case
        if mode in FOCUSED:
            with patch.object(p2,'BASE',BASE):
                ref, historical = p2.expected_fixture(mode)
            pins.update(pins_for((ref,Path(historical['result']['case'])/'result.json')))
            spec = ((1,'[.][NativeFP2M1Support]',1) if mode=='curved'
                    else p2.m1.fixtures.FOCUSED[p2.m1.OFF[mode]])
        else:
            historical = None
            spec = ((1,'[.][NativeFP2P1ClosureDirect]',2) if mode=='direct'
                    else (1,'[.][NativeFP2P1ClosurePublic]',1))
        ranks = spec[0]
        with bind_case(off), patch.object(hidden.c1,'run_case',lambda *args: launch(*args,environment=env)):
            result, evidence = p2.m1.fixtures.focus(root,binary,fingerprint(row),pins,spec)
        case = Path(result['case'])
        used = case/'result.json'
        verdict = {'passed':'passed','open_r6':'OPEN','failed':'FAILED'}[result['status']]
        entries = {0} if mode in ('curved','public-off','public-on') else set()
        marker = switch_receipt(case,env,ranks,entries,incomplete=verdict!='passed')
        details = {'catch': result['catch_summaries'],
                   'historical_catch': historical['result']['catch_summaries'] if historical else None}
        if part=='B0' and historical and details['catch']!=details['historical_catch']:
            verdict='FAILED'
            details['first_difference']={'field':'Catch summaries','expected':details['historical_catch'],
                                         'observed':details['catch']}
        if verdict=='passed' and mode=='curved':
            log = rejoin(case/'test_outer.log') if off else case/'test_outer.log'
            for key, checker, args in (('trace',p2.m1.HERE/'check_fixture_trace.py',()),
                         ('leaf',p2.M3/'check_leaf_pin.py',(case/'fp2_m1_curved_strip.native_ref',))):
                details[key]=check_script(checker,log,*args)
                pins.update(pins_for((checker,*args)))
                if details[key]['returncode']:
                    verdict='FAILED'
        if verdict=='passed' and mode.startswith('public-'):
            checker=HERE/'check_public_query.py'
            pins.update(pins_for((checker,)))
            details['public_trace']=check_script(checker,'--single',int(not off),case/'test_outer.log')
            if details['public_trace']['returncode']:
                verdict='FAILED'
    record={**fingerprint(row),'mode':mode,'part':part,'status':verdict if marker['passed'] else 'FAILED',
            'used':str(used),'details':details,'switches':marker,'pins':{**pins,**marker['pins'],**pins_for(evidence)}}
    cut.save(summary,record)
    return summary,record


def trial_environment(_mode):
    return settings(True,False,True)


def lab_command(case,binary,config,sensor,ranks,surface,_mode):
    argv=p2.m2.LAB_COMMAND(case,binary,config,sensor,ranks,surface)
    i=argv.index('nice')
    argv[i:i]=[f'{key}={value}' for key,value in trial_environment(None).items()]
    return argv


def run_control(build):
    status,_,pins=prepare(build)
    label=status['build']
    support=require(label,'B0')
    root=HERE/'lab'/f'build{label}'/'closure0'/'N1'/'trial1'
    summary=root/'case_summary.json'
    if summary.exists():
        row=cut.read(summary)
        verify_pins(row['pins'])
        return summary,row
    reference=p2.baseline('N1')
    support.extend((p2.m1.BASE_PROOF,reference[0],Path(stage_totals.__code__.co_filename)))
    used=[]
    evidence={}
    for index, attempt in enumerate(('attempt1','watchdog_retry')):
        seconds=N1_SECONDS*(3 if index else 1)
        original=p2.lab.parse_rows
        def parse(path):
            return original(rejoin(path) if path.name=='test_outer.log' else path)
        host_preflight()
        with patch.multiple(p2,HERE=HERE,__file__=__file__,prepare=prepare,trial_environment=trial_environment,
                            launch_recipe=lab_command,TIMEOUTS={'N1':seconds},MARKER=MARKER), \
                patch.object(p2.lab,'parse_rows',parse):
            case,result=p2.attempt(build,'default-N1',root/attempt,support)
            used.append(case/'measurement.json')
            process = cut.read(case / 'owned_processes.json')
            assert process['all_observed_terminal'] and all(cut.terminal(handle) for handle in process['identities'])
            assert result['wait']['terminal'] and cut.terminal(result['wait']['controller'])
            exact=p2.identity('N1',reference,result)
            marker=switch_receipt(case,trial_environment(None),1,{0},incomplete=not result['passed'])
            evidence=p2.watchdog(case,result,bool(index),exact,marker)
        if evidence.get('status')!='retry_allowed':
            break
        support.extend((used[-1],case/'watchdog.json'))
    status_label='passed'
    first=None
    work={}
    if exact['status']=='different':
        status_label,first='FAILED',exact['first_difference']
    elif not marker['passed']:
        status_label,first='FAILED',marker['first_failure']
    elif not result['passed']:
        status_label='OPEN' if evidence.get('status')=='open' else 'FAILED'
        first=p2.m2.first_finding(result,evidence)
    elif exact['status']!='equal':
        status_label,first='FAILED',{'field':'N1 exact projection','observed':exact['status']}
    else:
        try:
            work=stage_totals(result)
        except (KeyError,ValueError,TypeError) as error:
            status_label,first='FAILED',{'field':'stage measurement','observed':str(error)}
        else:
            if work['classification_work']!=463538756:
                status_label,first='FAILED',{'field':'classification_work','expected':463538756,
                                             'observed':work['classification_work']}
    terminal = [measurement.with_name(name) for measurement in used
                for name in ('test_launch.json','test_wait.json','owned_processes.json','provenance.json',
                             'command.json','run.cfg','test_host_preflight.json')]
    part={**fingerprint(status),'status':status_label,'used':str(used[-1]),'identity':exact,
          'elapsed_seconds':result['wait']['elapsed_seconds'],'stage_work':work,'switches':marker,
          'first_difference':first,'watchdog':evidence,
          'pins':{**pins,**marker['pins'],**pins_for((*support,*used,*terminal))}}
    cut.save(summary,part)
    return summary,part


def run(label,part):
    build=HERE/f'build{label}'
    sources={}
    if part=='B1':
        path,row=run_control(build)
        sources['N1']=path
    else:
        for mode in B0 if part=='B0' else C1:
            if not (fixture_root(label,part,mode)/'case_summary.json').exists():
                host_preflight()
            path,row=run_fixture(build,part,mode)
            sources[mode]=path
            print(part,mode,row['status'],flush=True)
        if part=='C1' and all(cut.read(sources[mode])['status']=='passed' for mode in ('public-off','public-on')):
            verify=HERE/'check_public_query.py'
            paths=[Path(cut.read(sources[mode])['used']).with_name('test_outer.log')
                   for mode in ('public-off','public-on')]
            output=build/'public_query_comparison.json'
            proc=subprocess.run([sys.executable,'-B',str(verify),*map(str,paths),str(output)],check=False)
            receipt=output.with_name('public_comparison_receipt.json')
            note=fingerprint(cut.read(build/'build_status.json'))
            cut.save(receipt,{**note,'status':'FAILED' if proc.returncode else 'passed',
                              'pins':pins_for([verify,*paths]+([output] if output.exists() else []))})
            sources['public comparison']=receipt
    path,row=seal(label,part,sources,('B0',) if part=='B1' else ('B',) if part=='C1' else ())
    print(part,row['status'],path,flush=True)
    return p2.TERMINAL[row['status']]


if __name__=='__main__':
    parser=ArgumentParser(description=__doc__)
    parser.add_argument('label',type=int,nargs='?')
    parser.add_argument('part',choices=('B0','B1','C1'),nargs='?')
    parser.add_argument('--owned',type=Path)
    opts=parser.parse_args()
    if opts.owned:
        seconds=cut.read(opts.owned/'p2_mode.json')['seconds_limit']
        assert seconds in (N1_SECONDS,3*N1_SECONDS)
        with patch.multiple(p2,trial_environment=trial_environment,launch_recipe=lab_command,TIMEOUTS={'N1':seconds}):
            raise SystemExit(p2.owned(opts.owned))
    raise SystemExit(run(opts.label,opts.part))
