"""Summarize no-inline perf stacks of native remeshing; CPU samples include MPI waiting."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import tempfile


def inspect(path):
    sites, mpi_apis, counters = Counter(), Counter(), Counter()
    def sample(lines):
        if not lines or not lines[0].startswith(('SU2_CFD ', 'test_driver ')): return
        symbols = [line.strip().split(' ', 1)[-1].rsplit(' (', 1)[0] for line in lines[1:]]
        native = [s for s in symbols if 'SU2Native' in s.split('(', 1)[0] or 'CNativeRemesher::' in s.split('(', 1)[0]]
        if not native: return
        counters['native_samples'] += 1
        matches = re.search(r'(SU2Native\w*2D::[\w:]+|CNativeRemesher::\w+)', native[0])
        sites[matches[1] if matches else native[0][:140]] += 1
        mpi = [s for s in symbols if re.search(r'\bP?MPI_\w+', s)]
        if mpi:
            counters['native_with_MPI_API_frame'] += 1
            mpi_apis[re.search(r'\bP?MPI_\w+', mpi[0])[0]] += 1
    lines = []
    with path.open() as stream:
        for line in stream:
            if line.strip(): lines.append(line)
            else: sample(lines); lines = []
    sample(lines)
    return dict(scope='No-inline sampled CPU stacks, nearest visible native symbol and visible MPI API; includes waiting and truncated-stack limitations, not additive wall-phase timing',
                counters=dict(counters), nearest_native_sites=sites.most_common(), mpi_apis=mpi_apis.most_common(),
                source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(), checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('stacks', type=Path, nargs='?')
    p.add_argument('output', type=Path, nargs='?')
    p.add_argument('--selftest', action='store_true')
    a = p.parse_args()
    if a.selftest:
        with tempfile.TemporaryDirectory() as tmp:
            fixture = Path(tmp) / 'stacks.txt'
            fixture.write_text('SU2_CFD 42 cpu-clock:\n  123 MPI_Alltoall (libmpi.so)\n'
                               '  234 SU2NativeBoundary2D::World::exchange (SU2_CFD)\n\n'
                               'SU2_CFD 42 cpu-clock:\n  567 CSolver::ComputeMetric (/SU2_NativeIntegrated/SU2_CFD)\n\n'
                               'test_driver 99 cpu-clock:u:\n'
                               '  678 SU2NativeBoundary2D::FieldPatch::evaluate (test_driver)\n\n')
            result = inspect(fixture)
            assert result['counters'] == dict(native_samples=2, native_with_MPI_API_frame=1)
            assert result['mpi_apis'] == [('MPI_Alltoall', 1)]
        print('Stack classifier selftest PASS')
        raise SystemExit(0)
    if a.stacks is None or a.output is None: p.error('Supply stacks and output, or --selftest')
    assert not a.output.exists(), 'Preserve existing evidence'
    result = inspect(a.stacks)
    a.output.write_text(json.dumps(result, indent=2)+'\n')
    print(result['counters'], result['nearest_native_sites'][:8], result['mpi_apis'])
