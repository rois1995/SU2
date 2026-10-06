"""Small queue/generator checks: python3 B0Spike/py/check_jobs.py (no remeshing)."""
import inspect
import shlex
import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

import b0queue
import mkjobs
import e0_g0


def check_injections(jobs):
    count = 0
    for job in jobs:
        _, wd, command = job.split('|', 2)
        args = shlex.split(command)
        for i, arg in enumerate(args):
            if arg != '--inject':
                continue
            assert i + 1 < len(args), job
            spec = args[i + 1].split(':')
            assert len(spec) == 3, job
            stage, piece, step = spec
            assert stage in ['planning', 'allocation', 'migration', 'mmg', 'interpolation', 'validation', 'splice', 'commit'], job
            # Generic jobs have no frozen plan from which to obtain real labels.
            assert piece in ['first', 'largest'], job
            assert step.isascii() and step.isdigit(), job
            assert wd.endswith(f'_{stage}_{piece}_s{step}'), job
            count += 1
    return count


def e0_runner_regression():
    script = Path(__file__).resolve().parents[3] / 'E0_RUNS.sh'
    if not script.exists():
        print('E0_RUNS.sh unavailable: skipping external runner checks')
        return
    text = script.read_text()
    # Source the actual helper functions without launching a stage or touching E0.
    helpers = text[text.index('gate() {'):text.index('case "${1:-}" in')]
    with tempfile.TemporaryDirectory(prefix='e0_runner_') as tmp:
        root = Path(tmp)
        harness = root / 'check.sh'
        harness.write_text('set -euo pipefail\n' + helpers + r"""
gate() { :; }
E="$1"
job "$E/timed" 0.05 sleep 1
[[ $(cut -d' ' -f1 "$E/timed/FAILED") == 124 && ! -e "$E/timed/DONE" ]]
job "$E/independent" 1 true
[[ $(cat "$E/independent/DONE") == 0 ]]
# DONE must skip without rewriting commands/logs, even if the command would fail.
cp "$E/independent/command.txt" "$E/saved"
job "$E/independent" 1 false
cmp "$E/saved" "$E/independent/command.txt"
# run() also skips before reading unavailable allow/reference inputs.
run box3d "$E/independent" 1 5400
cmp "$E/saved" "$E/independent/command.txt"
dependent_job "$E/timed" "$E/blocked" 1 touch ran
[[ -f "$E/blocked/FAILED" && ! -e "$E/blocked/DONE" && ! -e "$E/blocked/ran" ]]
dependent_job "$E/independent" "$E/dependent" 1 true
[[ $(cat "$E/dependent/DONE") == 0 ]]
job "$E/exit2" 1 bash -c 'exit 2'
[[ $(cut -d' ' -f1 "$E/exit2/FAILED") == 2 ]]
# Failed jobs have no DONE, so retry can succeed; its old failure evidence remains.
job "$E/timed" 1 true
[[ $(cat "$E/timed/DONE") == 0 && -e "$E/timed/FAILED" ]]
[[ "$failures" == 3 ]]
[[ $(case_limit box3d 5400) == 16200 && $(case_limit m60 1800) == 5400 ]]
[[ $(case_limit naca1 5400) == 5400 && $(case_limit box3c 60) == 180 ]]
""")
        subprocess.run(['bash', str(harness), str(root)], check=True, capture_output=True, text=True)
        for case, seconds in [('box3d', '2700s'), ('m60', '2700s'), ('naca1', '900s')]:
            wd = root / case; wd.mkdir()
            with patch.object(e0_g0.os, 'getloadavg', return_value=(0, 0, 0)), \
                    patch.object(e0_g0.subprocess, 'run') as call:
                call.return_value.returncode = 124
                assert e0_g0.run(['binary', 'mask', 'input', '--artifacts', wd / 'mask'], wd) is False
                assert call.call_args[0][0][2] == seconds
                assert (wd / 'mask.FAILED').exists()



def main():
    e0_runner_regression()
    b0queue.SLOTS = 1
    b0queue.MAXLOAD = 7
    with tempfile.TemporaryDirectory(prefix='b0queue_check_') as d:
        root = Path(d)
        job, wd = root / 'jobs', root / 'run'
        job.write_text(f'1|{wd}|timeout --kill-after=1s 5 true\n')
        # First poll exceeds the limit: no job may be started until load falls.
        load = [7.01]
        b0queue.load1 = lambda: load[0]
        def retry(_):
            if load[0] > 7:
                assert not wd.exists(), 'started above the load limit'
                load[0] = 7
        b0queue.time.sleep = retry
        b0queue.main(str(job))
        assert (wd / 'DONE').read_text() == '0\n'
        before = (wd / 'log.txt').stat().st_mtime_ns
        b0queue.main(str(job))
        assert before == (wd / 'log.txt').stat().st_mtime_ns
        job.write_text(f'2|{wd}|true\n')
        try:
            b0queue.main(str(job))
        except ValueError:
            pass
        else:
            raise AssertionError('oversized job was not rejected')
        # All default-callable generators, plus the two-rank injection variant.
        # Redirect paths so generation never reads the active experiment series.
        with patch.object(mkjobs, 'R', str(root)):
            generated = []
            for name, generator in inspect.getmembers(mkjobs, inspect.isfunction):
                if all(p.default is not inspect.Parameter.empty for p in inspect.signature(generator).parameters.values()):
                    generated.extend(generator())
            generated.extend(mkjobs.n1inj('2'))
            assert check_injections(generated) == 256
            more = mkjobs.n1more()
            assert sum('_largest_s1|' in j for j in more) == 32
            assert all('_all_s1' not in j for j in generated)
            jobs = mkjobs.n5()
        for selector in ['1', '', 'all', 'nonsense']:
            try:
                check_injections([f'1|{root}/sbase_mmg_{selector}_s0|binary --inject mmg:{selector}:0'])
            except AssertionError:
                pass
            else:
                raise AssertionError(f'invalid generic selector accepted: {selector!r}')
    assert len(jobs) == 16
    assert all('--background 1' in j and 'timeout --kill-after=30s 9000' in j for j in jobs)
    assert sum('/m60/' in j for j in jobs) == 2
    assert sum('/box3d/' in j for j in jobs) == 2
    print('PASS: E0 set-e timeout/failure continuation, dependency blocking, retry, DONE skip and 3D limits; load retry, exit status, restart skip, slot rejection, 16 timed N5 jobs, '
          'all generators / 256 valid piece injections, 32 explicit largest-piece replacements, obsolete selectors rejected')


if __name__ == '__main__':
    main()
