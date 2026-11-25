"""Run FP2-P2 S with its frozen checkpoint, inherited watchdogs and isolated restart."""
from contextlib import contextmanager
from importlib.util import spec_from_file_location, module_from_spec
from pathlib import Path
from tempfile import TemporaryDirectory
import re
import runpy
import sys
from unittest.mock import patch

from build_p2 import HERE, cut
from run_shared import SHARED, environment, exports, markers, replace

C3 = HERE.parent / 'FP2_C3'
START = HERE / 'inputs/FP2_P2_VERTEX_WITNESS.md'
SELF = Path(__file__).resolve()
sys.path.insert(0, str(C3))
spec = spec_from_file_location('p2_inherited_s', C3 / 'run_fp2_b_g5.py')
runner = module_from_spec(spec)
spec.loader.exec_module(runner)
workflow, sha = runner.workflow, runner.sha
ORIGINAL_REFERENCE = runner.reference_sources
ORIGINAL_BOUND = runner.bound
CAMPAIGN_NAMESPACE = runner.prior.BASE['PRIOR']
ORIGINAL_CAMPAIGN = CAMPAIGN_NAMESPACE['bind_campaign']
ORIGINAL_CORE = workflow.core_source
ORIGINAL_RESTART = workflow.restart_source
OLD = C3 / 'G5/build664/S/run_receipt.json'
EXTRACT = HERE.parent.parent / 'nobl_clean_check_20261114/extract_markers.py'
SETTINGS = environment(1)
EXPORTS = exports(SETTINGS)


def work_path(label):
    return HERE / 'G5' / f'build{label}' / 'S'


def core_source():
    source = ORIGINAL_CORE()
    source = replace(source, '    env.update(clean_provenance["environment"])',
                     f'    env.update(clean_provenance["environment"])\n    env.update({SETTINGS!r})\n'
                     f'    i = argv.index("mpirun") + 1\n    argv[i:i] = {EXPORTS!r}')
    return replace(source, 'if key in clean_provenance["environment"]',
                   f'if key in clean_provenance["environment"] or key in {tuple(SETTINGS)!r}')


def restart_source(name, label, build):
    template, source = ORIGINAL_RESTART(name, label, build)
    if name != 'run_restart':
        return template, source
    # Bind switches into the generated controller after its environment cleanup.
    injection = (
        ('argv = ["nice"', f'env.update({SETTINGS!r})\nargv = ["nice"'),
        ('"mpirun", "--oversubscribe"', '"mpirun", ' + ', '.join(map(repr, EXPORTS)) + ', "--oversubscribe"'),
        ('or k == "TMPDIR"', f'or k == "TMPDIR" or k in {tuple(SETTINGS)!r}'),
    )
    anchor = '            compile(source, str(BASE), "exec")\n'
    addition = f'            for old, new in {injection!r}:\n'
    addition += '                assert source.count(old) == 1, old\n                source = source.replace(old, new, 1)\n'
    return template, replace(source, anchor, addition + anchor)


def prerequisites(build):
    from run_p2 import fingerprint, selected_summary, verify_campaign, verify_checkpoint
    from seal_d1 import verify_d1
    from seal_d2 import verify_d2
    directory = HERE / f'build{build["build"]}'
    d1_path, d1 = verify_d1(directory)
    d2_path, d2 = verify_d2(directory)
    assert d1['promote'] and d2['promote']
    checkpoint, stage = verify_checkpoint(directory)
    compared, campaign = verify_campaign(directory)
    assert stage['status'] in ('passed', 'OPEN')
    assert campaign['status'] == 'passed'
    assert len(campaign['trials']) == 6 and len(campaign['pairs']) == 3
    assert all(item['status'] == 'equal' for item in campaign['pairs'].values())
    lab_path = directory / 'p2_lab_checkpoint.json'
    lab = cut.read(lab_path)
    assert fingerprint(lab) == fingerprint(build)
    assert lab['status'] in ('passed', 'OPEN')
    assert lab['six_trials_qualified'] and lab['three_equal_pairs']
    for key, source in (('D1', d1_path), ('D2', d2_path), ('campaign', compared)):
        row = lab['parts'][key]
        assert row['path'] == str(source) and row['sha256'] == cut.sha(source), key
    stage2 = [selected_summary(build['build'], 'anatomy-N1'), *(
        HERE / 'shared' / directory.name / 'closure1' / name / 'case_summary.json' for name in SHARED)]
    for path in stage2:
        row = cut.read(path)
        assert row['status'] in ('passed', 'OPEN', 'not evaluated'), path
        for artifact, digest in row['pins'].items():
            assert cut.sha(artifact) == digest, artifact
    return [checkpoint, lab_path, compared, d1_path, d2_path, *stage2, START, START.with_suffix('.sha256'), SELF,
            Path(__file__).resolve().with_name('run_p2.py'), Path(__file__).resolve().with_name('run_shared.py'),
            *(Path(name) for name in campaign['pins'])]


def reference(payloads):
    before = runner.cut.read(C3 / 'build664/build_status.json')
    receipt = runner.prior.verify_run(OLD.parent, 'S', before, payloads)
    assert receipt['passed'] and len(receipt['hashes']) == 6
    return [*ORIGINAL_REFERENCE(payloads), OLD, C3 / 'build664/build_status.json',
            *(OLD.parent / name for name in receipt['artifacts'])]


def stage_markers(case, command):
    recorded = cut.read(command)
    assert all(recorded['environment'].get(k) == v for k, v in SETTINGS.items()), command
    assert all(recorded['argv'].count(name) == 1 for name in SETTINGS), command
    files = {int(p.parent.name.split('.')[-1]): p for p in case.glob('rank_output/*/rank.*/stderr')}
    assert len(files) == 6, case
    receipt = markers(files, 1)
    assert not receipt['mismatches'], receipt
    return receipt


def rank_timers(case):
    strip = runpy.run_path(str(EXTRACT))['strip_mpi_prefixes']
    groups = {'geometry_bound': 'geometry_bound', 'communication': 'communication_and_join_wait',
              'join_wait': 'communication_and_join_wait'}
    labels = ('geometry_bound', 'communication_and_join_wait', 'other')
    result = {}
    for path in sorted(case.glob('rank_output/*/rank.*/stdout')):
        rank = int(path.parent.name.split('.')[-1])
        event = 1
        sums = dict.fromkeys(labels, 0.)
        result[rank] = {}
        with path.open(errors='replace') as stream:
            for raw in stream:
                line = strip(raw)
                if 'NATIVE3D_TIMER ' in line:
                    row = dict(re.findall(r'(\w+)=([^\s]+)', line))
                    if row.get('root') == 'remesh' and row.get('boundary') == 'final':
                        field = row.get('operation', row.get('op'))
                        assert field is not None, row
                        sums[groups.get(field, 'other')] += float(row['exclusive_seconds'])
                if 'NATIVE3D_TIMER_SUMMARY ' in line:
                    row = dict(re.findall(r'(\w+)=([^\s]+)', line))
                    if row.get('root') == 'remesh' and row.get('boundary') == 'final':
                        assert sums['geometry_bound'] > 0, (path, event, row)
                        result[rank][event] = sums
                        event += 1
                        sums = dict.fromkeys(labels, 0.)
        assert set(result[rank]) == {1, 2}, (path, result[rank])
    assert len(result) == 6
    return result


def event_measurements(receipt):
    strip = runpy.run_path(str(EXTRACT))['strip_mpi_prefixes']
    case = Path(receipt['case'])
    path, = case.glob('rank_output/*/rank.0/stdout')
    handover = []
    state = None
    with path.open(errors='replace') as stream:
        for raw in stream:
            line = strip(raw)
            if 'NATIVE3D_LOCAL_SURFACE_STATE ' in line:
                state = dict(re.findall(r'(\w+)=([^\s]+)', line))
            if 'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final ' in line:
                handover.append(int(state['faces']) if state else None)
                state = None
    assert len(receipt['events']) == len(handover) == 2
    return [{**event, 'handover_faces': faces} for event, faces in zip(receipt['events'], handover)]


def observation(path):
    current, prior = [cut.read(p) for p in (path, OLD)]
    case, before = [Path(r['case']) for r in (current, prior)]
    disk = Path(current['restart']).parent
    return {'status': 'passed', 'reference': str(OLD), 'run': str(path), 'settings': SETTINGS,
            'switch_markers': {'full': stage_markers(case, case / 'provenance.json'),
                               'isolated_restart': stage_markers(disk, disk / 'command.json')},
            'events': {'build664': event_measurements(prior), 'candidate': event_measurements(current)},
            'final_rank_exclusive_seconds': {'build664': rank_timers(before), 'candidate': rank_timers(case)},
            'payloads': {name: {'build664': prior['hashes'][name], 'candidate': digest,
                                'equal': digest == prior['hashes'][name]}
                         for name, digest in current['hashes'].items()}}


@contextmanager
def bound():
    # Keep C3's driver basename for historical source pins and use P2 to launch the child.
    with ORIGINAL_BOUND(), patch.object(runner.prior, '__file__', str(SELF)):
        yield


def bind_campaign(mode, inputs):
    context = ORIGINAL_CAMPAIGN(mode, inputs)
    context.update(core_source=core_source, restart_source=restart_source)
    return context


def verify_child_scripts(work, label):
    full = (work / 'full_controller.py').read_text()
    restart = (work / f'run_restart{label}.py').read_text()
    assert f'env.update({SETTINGS!r})' in full and f'argv[i:i] = {EXPORTS!r}' in full, work
    assert repr(f'env.update({SETTINGS!r})\nargv = ["nice"') in restart, work
    assert repr('"mpirun", ' + ', '.join(map(repr, EXPORTS)) + ', "--oversubscribe"') in restart, work


def run():
    is_full = '--full' in sys.argv
    if is_full:
        build = Path(sys.argv[1])
        label = cut.read(build / 'build_status.json')['build']
        work = work_path(label)
        if '--retry-full' in sys.argv:
            work /= runner.FULL_RETRY
        verify_child_scripts(work, label)
    runner.bound = bound
    runner.work_path = work_path
    runner.prerequisite_sources = prerequisites
    runner.reference_sources = reference
    try:
        with patch.multiple(workflow, core_source=core_source, restart_source=restart_source), \
                patch.dict(CAMPAIGN_NAMESPACE, bind_campaign=bind_campaign):
            code = runner.main()
            if '--self-check' in sys.argv:
                with TemporaryDirectory(prefix='fp2-p2-s-script-check-') as root:
                    work = Path(root)
                    directory = HERE / 'build672'
                    bind_campaign('on', [SELF])['prepare'](work, 672, directory, runner.prior.config_for('S'))
                    verify_child_scripts(work, 672)
                    restart = runpy.run_path(str(work / 'run_restart672.py'))
                    source = restart['runner_context']().load_base().retargeted_source()
                    assert f'env.update({SETTINGS!r})' in source
                    assert all(repr(key) in source for key in SETTINGS)
                print('Generated S and effective restart controllers contain both process switches.', flush=True)
    except (ValueError, RuntimeError, AssertionError, KeyError, OSError) as error:
        if is_full or '--only' not in sys.argv:
            raise
        build = Path(sys.argv[1])
        label = cut.read(build / 'build_status.json')['build']
        root = work_path(label)
        failure = next((path for path in (root / runner.FULL_RETRY / 'failed_attempt.json',
                                          root / 'failed_attempt.json') if path.is_file()), None)
        root.mkdir(parents=True, exist_ok=True)
        cut.save(root / 'case_summary.json', {'status': 'FAILED', 'settings': SETTINGS,
                'first_failure': cut.read(failure)['first_observed_failure'] if failure else str(error),
                'raw': str(failure) if failure else None})
        raise
    if not is_full and '--only' in sys.argv:
        build = Path(sys.argv[1])
        label = cut.read(build / 'build_status.json')['build']
        root = work_path(label)
        if code == 0:
            path, = (p for p in (root / 'run_receipt.json', root / runner.FULL_RETRY / 'run_receipt.json') if p.is_file())
            cut.save(root / 'case_summary.json', observation(path))
        elif code == 75:
            cut.save(root / 'case_summary.json', {'status': 'OPEN', 'settings': SETTINGS, 'reason': 'wrapper watchdog'})
    return code


if __name__ == '__main__':
    raise SystemExit(run())
