"""Check and list the reviewable files for the terminal N21 GQ reference."""

from pathlib import Path
from runpy import run_path

from compare_prefix import PARSER, parse_case
from gq_baseline import CASE, ITEM, MANIFEST, WORK, cut

case_artifacts = run_path(str(ITEM / 'run_scoped.py'))['case_artifacts']


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    reference = cut.read(MANIFEST)
    status = cut.read(CASE / 'status.json')
    assert cut.terminal(cut.read(ITEM / 'gq_reference_controller_launch.json'))
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    assert reference['status_sha256'] == cut.sha(CASE / 'status.json')
    assert all(cut.sha(path) == digest for path, digest in reference['rank_stdout_sha256'].items())
    parsed = parse_case(CASE, 'GQ')
    times = [snapshot for value in parsed.values() for snapshot in value['timers']]
    result = {'passed': True, 'build': reference['build'], 'case': str(CASE),
              'clock_decisions_by_rank': {str(PARSER.stream_rank({}, path)): value['ownership']
                                          for path, value in parsed.items()},
              'timer_snapshots': len(times), 'fixedstorage': 4120}
    cut.save(ITEM / 'gq_reference_summary.json', result)
    files = case_artifacts(ITEM, WORK, CASE)
    files.update((cut.PHASE / 'build589_aliases.py', Path(__file__).resolve(),
                  ITEM / 'inputs/IMPLEMENTATION_8_5U.md', ITEM / 'inputs/SHA256SUMS'))
    cut.save(ITEM / 'gq_reference_paths.json', sorted(str(path) for path in files))
    print('N22 GQ baseline evidence sealed:', len(times), 'timer snapshots and', len(files), 'paths', flush=True)


if __name__ == '__main__':
    main()
