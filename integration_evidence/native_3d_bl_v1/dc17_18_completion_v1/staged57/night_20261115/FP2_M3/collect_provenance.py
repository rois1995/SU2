"""Keep the original M2 measurements and save M3 packets for offline analysis."""

from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import sys
from unittest.mock import patch


HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_M2'))
import collect_trace as m2

STREAM = 'provenance.log.gz'
RECEIPT = 'provenance_receipt.json'
PREFIX = b'NATIVE_SURFACE_'
NEW_ROWS = frozenset(PREFIX + name for name in (
    b'FACET_BEGIN', b'FACET_LEAF', b'FACET_END', b'WITNESS', b'PLANE_GROUP',
    b'PLANE_OPEN', b'PLANE_SPLIT', b'TRACE_INCOMPLETE'))


def packet_kind(line):
    return line.partition(b' ')[0].rstrip()


def keep(line):
    kind = packet_kind(line)
    return (kind in NEW_ROWS or kind in (PREFIX + b'PLANES', PREFIX + b'QUERY_RESULT') or
            line.startswith((PREFIX + b'QUERY authority=', b'FP2_M2 Q ', b'FP2_M2 PHASE ')))


class Provenance:
    def __init__(self, path):
        self.path = path
        self.stream = None
        self.error = None
        self.counts = Counter()
        try:
            self.stream = gzip.open(path, 'wb', compresslevel=1)
        except OSError as error:
            self.error = str(error)

    def write(self, line):
        kind = packet_kind(line)
        if line.startswith(b'FP2_'):
            kind = b' '.join(line.split(maxsplit=2)[:2])
        self.counts[kind.decode('ascii')] += 1
        if self.stream is not None and self.error is None:
            try:
                self.stream.write(line)
            except OSError as error:
                self.error = str(error)

    def receipt(self):
        if self.stream is not None:
            try:
                self.stream.close()
            except OSError as error:
                self.error = self.error or str(error)
        size = digest = None
        try:
            if self.path.is_file():
                size = self.path.stat().st_size
                with self.path.open('rb') as source:
                    digest = hashlib.file_digest(source, 'sha256').hexdigest()
        except OSError as error:
            self.error = self.error or str(error)
        return {'completion': 'failed' if self.error else 'complete', 'file': self.path.name,
                'bytes': size, 'sha256': digest, 'row_counts': dict(self.counts), 'error': self.error}


def drain_stderr(child_stderr, ordinary, output_dir):
    """Keep draining even if compressed evidence cannot be written."""
    out = Path(output_dir)
    provenance = Provenance(out / STREAM)
    base = m2.TraceAnalyzer

    class WithProvenance(base):
        def feed(self, line):
            if keep(line):
                provenance.write(line)
            if packet_kind(line) not in NEW_ROWS:
                super().feed(line)

        def close_forward(self, answer, seconds=0):
            forward = self.forward
            totals = forward.totals
            enough = int(totals['prepared_physical_nodes'] == 1 and totals['prepared_within_tolerance'] == 1)
            provenance.write(f'FP2_M3 FORWARD ordinal={forward.ordinal} class={forward.classify(answer)} '
                             f'root_enough={enough}\n'.encode())
            super().close_forward(answer, seconds)

    try:
        with patch.object(m2, 'TraceAnalyzer', WithProvenance):
            return m2.drain_stderr(child_stderr, ordinary, out)
    finally:
        receipt = provenance.receipt()
        try:
            temporary = out / (RECEIPT + '.tmp')
            temporary.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
            temporary.replace(out / RECEIPT)
        except OSError as error:
            try:
                m2.write_ordinary(ordinary, f'FP2-M3 provenance receipt write failed: {error}\n'.encode())
            except OSError:
                pass
