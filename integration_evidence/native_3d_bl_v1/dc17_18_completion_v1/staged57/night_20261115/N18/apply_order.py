"""Install the already pinned N18 amendment after the N6 terminal checkpoint."""
from pathlib import Path
import subprocess
import sys

N18 = Path(__file__).resolve().parent
sys.path.insert(0, str(N18.parent / 'N6'))
from run_cut import N6, cut

INPUTS = N18 / 'inputs'
PHASE = cut.PHASE / 'phase_N6'


def git(*args):
    return subprocess.check_output(['git', *map(str, args)], text=True, cwd=cut.REPO).strip()


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    result = cut.read(N6 / 'qualification_checkpoint.json')
    assert result['passed'] and git('rev-parse', 'HEAD') == result['commit']
    audit = cut.read(PHASE / 'terminal_audit.json')
    assert audit['actual_host_proc'] and audit['all_owned_handles_terminal']
    assert all(cut.terminal(row) for row in audit['identities'])
    assert not git('diff', '--cached', '--name-only')
    order = cut.REPO / 'AGENTS.md'
    patch = INPUTS / 'work_order_2_N18.patch'
    preparation = cut.read(INPUTS / 'amendment_preparation.json')
    assert cut.sha(order) == preparation['active_n6_source_pin_preserved']
    assert cut.sha(patch) == preparation['amendment_patch_sha256']
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=INPUTS, check=True)
    git('apply', '--check', '--whitespace=error', patch)
    previous = INPUTS / 'AGENTS_before_N18.md'
    previous.write_bytes(order.read_bytes())
    git('apply', '--whitespace=error', patch)
    receipt = INPUTS / 'amendment_applied.json'
    cut.save(receipt, {'n6_checkpoint': result['commit'], 'order_before_sha256': cut.sha(previous),
                       'order_after_sha256': cut.sha(order), 'I8R2_verified': True, 'I8R2_read_deferred': True,
                       'n6_terminal_audit_sha256': cut.sha(PHASE / 'terminal_audit.json')})
    progress_file = cut.NIGHT / 'progress.md'
    with progress_file.open('a') as file:
        file.write('\nN18 amendment applied after the passed terminal N6 checkpoint. The pinned copy '
                   'matched the user SHA256. Next, read the complete I8R2 and complete the N18 source check.\n')
    git('add', '-f', '--', order, progress_file, Path(__file__).resolve(), *INPUTS.iterdir())
    git('diff', '--cached', '--check', '--', order, progress_file, Path(__file__).resolve())
    git('commit', '-m', 'Amend work order 2 with the step 8.5r2 retry')
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    head = git('rev-parse', 'HEAD')
    cut.save(N18 / 'amendment_checkpoint.json', {'commit': head, 'i8r2_sha256': cut.sha(INPUTS / 'IMPLEMENTATION_8_5R2.md')})
    with progress_file.open('a') as file:
        file.write(f'\nN18 work-order amendment pushed as `{head[:10]}`.\n')
    print(head, flush=True)


if __name__ == '__main__':
    main()
