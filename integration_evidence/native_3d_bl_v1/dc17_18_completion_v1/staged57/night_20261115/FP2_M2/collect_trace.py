"""Drain the traced ANATOMY child's own stderr pipe into bounded evidence."""

from argparse import ArgumentParser
from contextlib import ExitStack
from io import BytesIO, StringIO
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory

from analyze_trace import TraceAnalyzer


SUMMARY_FILE = 'anatomy.json'
SAMPLE_FILE = 'anatomy_sample.log'
UNRESOLVED_FILE = 'anatomy_unresolved.log'
TRACE_PREFIXES = (b'NATIVE_', b'FP2_M2 Q ', b'FP2_M2 PHASE ')


class EvidenceFile:
    def __init__(self, path, stack):
        self.path = path
        self.stream = None
        self.error = None
        try:
            self.stream = stack.enter_context(path.open('wb'))
        except OSError as error:
            self.error = error

    def write(self, line):
        if self.stream is not None and self.error is None:
            try:
                self.stream.write(line)
            except OSError as error:
                self.error = error

    def finish(self):
        if self.stream is not None:
            try:
                self.stream.close()
            except OSError as error:
                if self.error is None:
                    self.error = error


def write_ordinary(stream, line):
    output = getattr(stream, 'buffer', None)
    if output is None:
        stream.write(line.decode('utf-8', 'replace'))
        stream.flush()
    else:
        output.write(line)
        output.flush()


def drain_stderr(child_stderr, ordinary, output_dir):
    """Run until child stderr closes. Parsing and output errors never block the child."""
    output_dir = Path(output_dir)
    with ExitStack() as stack:
        sample = EvidenceFile(output_dir / SAMPLE_FILE, stack)
        unresolved = EvidenceFile(output_dir / UNRESOLVED_FILE, stack)
        analyzer = TraceAnalyzer(sample, unresolved)
        processing_failed = False
        ordinary_failed = False
        try:
            for line in child_stderr:
                if line.startswith(TRACE_PREFIXES):
                    if not processing_failed:
                        try:
                            analyzer.feed(line)
                        except Exception as error:
                            analyzer.error(f'trace processing failed: {error}', line)
                            processing_failed = True
                    elif line.startswith(b'NATIVE_SURFACE_UNRESOLVED '):
                        unresolved.write(line)
                        analyzer.unresolved_lines += 1
                elif not ordinary_failed:
                    try:
                        write_ordinary(ordinary, line)
                    except OSError as error:
                        analyzer.error(f'writing ordinary stderr failed: {error}')
                        ordinary_failed = True
        except OSError as error:
            analyzer.error(f'reading child stderr failed: {error}')
        for artifact in (sample, unresolved):
            artifact.finish()
            if artifact.error is not None:
                analyzer.error(f'writing {artifact.path.name} failed: {artifact.error}')
        report = analyzer.finish()
    try:
        pending = output_dir / (SUMMARY_FILE + '.tmp')
        pending.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n', encoding='utf-8')
        pending.replace(output_dir / SUMMARY_FILE)
    except (OSError, ValueError) as error:
        report['completion'] = 'failed'
        report['stream']['error_count'] += 1
        report['stream']['errors'].append({'message': f'writing {SUMMARY_FILE} failed: {error}'})
        try:
            write_ordinary(ordinary, f'FP2-M2 collector failed to write {SUMMARY_FILE}: {error}\n'.encode())
        except OSError:
            pass
    return report


def self_check():
    tolerance = '3ff0000000000000'
    query = 'NATIVE_SURFACE_QUERY authority=0x1 budget=0x2 start=3 tolerance=' + tolerance + ' support=2,3, triangles=0;'
    result = 'NATIVE_SURFACE_QUERY_RESULT authority=0x1 budget=0x2 start=3 cache=miss work=100 bound=' + tolerance
    domain = ('NATIVE_SURFACE_DOMAIN model=0 facet=0 depth=0 upper=1 lower=0 upper_work=50 distance_work=10 '
              'parameter=1,0,0;0,1,0;0,0,1;')
    begin = 'FP2_M2 Q begin ordinal={} bin=0 direct_eligible={} longest_edge=0.001 sheet=0'
    end = 'FP2_M2 Q end ordinal={} answer={} seconds=0.25'
    lines = [
        'child diagnostic', 'FP2_M2 PHASE name=proof begin', query, result,
        'NATIVE_GEOMETRY_WORK operation=BoundPhysicalSurface total=100 exclusive=3',
        'FP2_M2 PHASE name=proof end',
        begin.format(1, 0), query,
        'NATIVE_SURFACE_QUERY tolerance=1 model=0 mesh=1 facets=2,3, points=0,0,0;',
        'NATIVE_SURFACE_MODEL pieces=8 witness=7 full=0 prepared=0 work=3',
        'NATIVE_SURFACE_PLANES triangles=8 groups=1 accepted=1 invalid=0 storage_refused=0 boundary_trimmed=2 trimmed_triangles=2',
        'NATIVE_SURFACE_GRAPH coverage=1 bound=1 coverage_work=15 lift_work=20',
        domain, result, 'NATIVE_GEOMETRY_WORK operation=Sample total=10 exclusive=10', end.format(1, 1),
        begin.format(2, 1), end.format(2, 0),
        begin.format(3, 0), query, result.replace('cache=miss', 'cache=hit'), end.format(3, 1),
        begin.format(4, 0), query, end.format(4, 'threw'),
        begin.format(5, 0), query, 'NATIVE_SURFACE_UNRESOLVED triangle=0,0,0;',
    ]
    with TemporaryDirectory(prefix='fp2_m2_trace_') as temporary:
        output = Path(temporary)
        ordinary = StringIO()
        report = drain_stderr(BytesIO(('\n'.join(lines) + '\n').encode()), ordinary, output)
        assert ordinary.getvalue() == 'child diagnostic\n'
        assert report == json.loads((output / SUMMARY_FILE).read_text())
        assert report['completion'] == 'partial'
        assert report['stream']['forwards_started'] == 5 and report['stream']['forwards_ended'] == 4
        assert report['phases']['proof']['paired_by_cache']['miss'] == 1
        assert report['phases']['proof']['exclusive_work']['BoundPhysicalSurface'] == 3
        assert report['groups']['direct_eligible_1']['calls_by_class']['direct_only'] == 1
        group = report['groups']['0']
        assert group['calls_by_class'] == {'incomplete': 2, 'direct_only': 0, 'hit': 1, 'miss': 1, 'bypass': 0}
        assert group['prepared_physical_nodes'] == {'n': 1, 'mean': 1, 'median': 1, 'p90': 1, 'maximum': 1}
        assert group['root_enough_share'] == 1 and group['root_ratio']['<=1']['share'] == 1
        assert group['means_per_planes_row']['triangles'] == 8 and group['graph_within_tolerance_share'] == 1
        assert [row['work'] for row in group['result_work_parts'].values()] == [15, 20, 15, 10, 40]
        assert (output / UNRESOLVED_FILE).read_text() == lines[-1] + '\n'
        assert (output / SAMPLE_FILE).read_text().splitlines() == lines[6:]

        trace = TraceAnalyzer(BytesIO(), BytesIO(), sample_limit=1)
        for line in (begin.format(1, 0), end.format(1, 0), begin.format(2, 0), end.format(2, 0)):
            trace.feed((line + '\n').encode())
        report = trace.finish()
        assert report['completion'] == 'complete' and report['stream']['sample_lines'] == 2
        trace = TraceAnalyzer(BytesIO(), BytesIO())
        trace.feed(b'FP2_M2 Q begin ordinal=1 bin=9 direct_eligible=0\n')
        assert trace.finish()['completion'] == 'failed'


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('output_dir', type=Path, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if args.output_dir is None:
        parser.error('output_dir is required')
    report = drain_stderr(sys.stdin.buffer, sys.stderr, args.output_dir)
    return int(report['completion'] == 'failed')


if __name__ == '__main__':
    raise SystemExit(main())
