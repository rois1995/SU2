"""Extend the M2 trace tables with closure counters without changing its classifications."""

from collections import Counter
from io import BytesIO, StringIO
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'FP2_M2'))
import analyze_trace as trace
import collect_trace as collector

MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=1'
CLOSED = b'NATIVE_SURFACE_WITNESS_CLOSED'
INCOMPLETE = b'NATIVE_SURFACE_TRACE_INCOMPLETE'
CLOSURE_FIELDS = ('chords_in', 'chords_out', 'splits', 'children', 'chain2', 'chain3', 'chain4',
                  'chain5_8', 'reject_long', 'reject_tie', 'reject_fan_key', 'reject_chain_key',
                  'reject_not_finite', 'reject_orientation', 'reject_projection', 'cap', 'left_contacts')
FOOTPRINT_FIELDS = ('groups', 'accepted', 'invalid', 'boundary_trimmed')


class Forward(trace.Forward):
    def __init__(self, *args):
        super().__init__(*args)
        self.closure = Counter()
        self.closure_incomplete = False

    def closure_row(self, tail):
        values = trace.fields(tail)
        totals = {key: int(values[key.encode()]) for key in CLOSURE_FIELDS}
        left = totals.pop('left_contacts')
        chords_in, chords_out = totals['chords_in'], totals['chords_out']
        origin = values[b'origin'].removesuffix(b',')
        indices = [int(value) for value in origin.split(b',')] if origin else []
        if (left < -1 or totals['cap'] not in (0, 1)
                or any(value < 0 for value in totals.values()) or len(indices) != chords_out
                or any(index < 0 or index >= chords_in for index in indices)):
            raise ValueError('invalid closure counters or origin list')
        self.closure.update(totals)
        self.closure['rows_scan_failed' if left == -1 else 'rows_scan_succeeded'] += 1
        if left >= 0:
            self.closure['left_contacts'] += left


class Group(trace.Group):
    def __init__(self):
        super().__init__()
        self.closure = Counter()

    def add(self, forward, classification, seconds):
        super().add(forward, classification, seconds)
        if classification in ('miss', 'bypass'):
            self.closure.update(forward.closure)
            self.closure['forwards_incomplete_closure_row'] += forward.closure_incomplete

    def report(self):
        report = super().report()
        report['plane_sums'] = {key: self.planes[key] for key in FOOTPRINT_FIELDS}
        report['closure'] = {key: self.closure[key] for key in (*CLOSURE_FIELDS, 'rows_scan_succeeded',
                                                               'rows_scan_failed',
                                                               'forwards_incomplete_closure_row')}
        return report


class Analyzer(trace.TraceAnalyzer):
    def __init__(self, *args):
        super().__init__(*args)
        self.groups = {key: Group() for key in self.groups}

    def add_row(self, line):
        super().add_row(line)
        if self.forward is None:
            return
        kind, _, tail = line.partition(b' ')
        if kind == CLOSED:
            try:
                self.forward.closure_row(tail)
            except (KeyError, ValueError):
                self.forward.closure_incomplete = True
        elif kind == INCOMPLETE and b'site=closed' in tail.split():
            self.forward.closure_incomplete = True


def drain_stderr(child_stderr, ordinary, output_dir):
    # Keep the process marker in the unfiltered log used by the switch attestation.
    def lines():
        for line in child_stderr:
            if line.rstrip() == MARKER.encode():
                try:
                    collector.write_ordinary(ordinary, line)
                except OSError:
                    yield line
            else:
                yield line

    with patch.object(collector, 'TraceAnalyzer', Analyzer), patch.object(trace, 'Forward', Forward), \
            patch.object(collector, 'TRACE_PREFIXES',
                         (b'NATIVE_SURFACE_', b'NATIVE_GEOMETRY_WORK ', b'FP2_M2 Q ', b'FP2_M2 PHASE ')):
        return collector.drain_stderr(lines(), ordinary, output_dir)


def self_check():
    template = ' '.join(f'{key}={2 if key in ("chords_in", "chords_out") else 0}'
                        for key in CLOSURE_FIELDS if key != 'left_contacts')
    begin = 'FP2_M2 Q begin ordinal=1 bin=0 direct_eligible=0'
    end = 'FP2_M2 Q end ordinal=1 answer=1 seconds=0.1'
    query = 'NATIVE_SURFACE_QUERY authority=0x1 budget=0x2 start=3 tolerance=3ff0000000000000 support=2,'
    answer = 'NATIVE_SURFACE_QUERY_RESULT authority=0x1 budget=0x2 start=3 cache=miss work=100'
    plane = ('NATIVE_SURFACE_PLANES groups=4 accepted=3 invalid=1 boundary_trimmed=2'
             + ' ' + ' '.join(f'{key}=0' for key in trace.PLANES_FIELDS if key not in FOOTPRINT_FIELDS))
    rows = [MARKER, begin, query, plane, f'{CLOSED.decode()} {template} left_contacts=-1 origin=0,1,',
            f'{CLOSED.decode()} {template.replace("cap=0", "cap=1")} left_contacts=4 origin=0,1,',
            f'{INCOMPLETE.decode()} site=closed', answer, end]
    with TemporaryDirectory(prefix='fp2_p2_trace_') as directory:
        ordinary = StringIO()
        report = drain_stderr(BytesIO(('\n'.join(rows) + '\n').encode()), ordinary, directory)
    group = report['groups']['0']
    assert ordinary.getvalue() == MARKER + '\n'
    assert report['completion'] == 'complete'
    assert group['calls_by_class']['miss'] == 1 and group['closure']['chords_in'] == 4
    assert group['closure']['left_contacts'] == 4 and group['closure']['cap'] == 1
    assert group['plane_sums'] == {'groups': 4, 'accepted': 3, 'invalid': 1, 'boundary_trimmed': 2}
    assert all(group['closure'][key] == 1 for key in
               ('rows_scan_succeeded', 'rows_scan_failed', 'forwards_incomplete_closure_row'))


if __name__ == '__main__':
    self_check()
