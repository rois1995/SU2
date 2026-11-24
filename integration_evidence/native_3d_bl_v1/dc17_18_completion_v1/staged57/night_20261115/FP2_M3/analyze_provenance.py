"""Offline joins and reports for the FP2-M3 provenance stream."""

import argparse
from collections import Counter, defaultdict
import csv
from dataclasses import dataclass, field
from functools import lru_cache
import gzip
import hashlib
import json
from pathlib import Path
import re
import struct

from collect_provenance import NEW_ROWS, PREFIX
import analyze_trace as m2
from provenance_geometry import (EdgeIndex, Facet, Leaf, Projection, classify_edge, edge_neighbours,
                                 exact_word, leaf_intervals, relations, triple)

ANSI = re.compile(rb'\x1b\[[0-?]*[ -/]*[@-~]')
STATUSES = ('single', 'zero_sign', 'storage_refused', 'invalid', 'trimmed', 'accepted')
OPEN_CLASSES = ('tj_intra_coarse', 'tj_intra_fine', 'tj_inter_coarse', 'tj_inter_fine',
                'entity_border', 'support_border', 'same_segment', 'crop_intra', 'crop_inter', 'other')
INSIDE_CLASSES = (*OPEN_CLASSES, 'key_split')
SPLIT_CLASSES = ('axis_sign', 'same_projection')
MERGED = {k: ('tj' if k.startswith('tj_') else 'crop' if k.startswith('crop_') else
              'border' if k.endswith('_border') else k) for k in INSIDE_CLASSES}
INVALID = (KeyError, ValueError, OverflowError, struct.error)
BINS = range(5)
MESH_BINS = range(7)


def fields(text):
    return dict(part.split('=', 1) for part in text.decode('ascii').split())


def csv_list(word):
    return word.rstrip(',').split(',') if word else []


def indices(word, sep):
    pair = tuple(map(int, word.split(sep)))
    if len(pair) != 2 or min(pair) < 0:
        raise ValueError('invalid index pair')
    return pair


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


@dataclass
class LeafBuild:
    inside: bool
    leaves: dict = field(default_factory=dict)
    invalid: bool = False


class LeafTables:
    def __init__(self):
        self.building = {}
        self.published = {}
        self.counts = Counter()

    def feed(self, kind, row, inside):
        key = row['authority'], int(row['facet']), row['resolution']
        if kind == PREFIX + b'FACET_BEGIN':
            need(exact_word(key[2]) > 0, 'invalid leaf resolution')
            self.counts['incomplete'] += key in self.building
            self.building[key] = LeafBuild(inside)
            return
        build = self.building.get(key)
        if build is None:
            self.counts['row_without_begin'] += 1
            return
        if kind == PREFIX + b'FACET_LEAF':
            ordinal = int(row['ordinal'])
            if ordinal in build.leaves:
                build.invalid = True
            try:
                build.leaves[ordinal] = Leaf.read(row)
            except INVALID:
                build.leaves[ordinal] = None
                build.invalid = True
            return
        del self.building[key]
        size = int(row['leaves'])
        if size != len(build.leaves) or any(i not in build.leaves for i in range(size)):
            self.counts['incomplete'] += 1
            return
        leaves = tuple(build.leaves[i] for i in range(size))
        old = self.published.get(key)
        self.counts['complete'] += 1
        self.counts['complete_inside_step_c'] += build.inside
        if old is not None:
            self.counts['replaced'] += 1
            self.counts['changed'] += old[0] != leaves
        self.counts['not_usable'] += build.invalid
        self.published[key] = leaves, build.inside, build.invalid

    def join(self, witness):
        found = []
        for token in csv_list(witness['list']):
            facet, ordinal = indices(token, ':')
            table = self.published.get((witness['authority'], facet, witness['resolution']))
            found.append(table[0][ordinal] if table and not table[2] and ordinal < len(table[0]) else None)
        return tuple(found)

    def report(self, authority, facets, edge_file):
        return {'publication': {**self.counts, 'incomplete': self.counts['incomplete'] + len(self.building)},
                'pass_authority': authority,
                'published_total': sum(k[0] == authority for k in self.published),
                'published_built_inside_step_c': sum(k[0] == authority and v[1]
                                                     for k, v in self.published.items()),
                'resolutions': leaf_report(self.published, authority, facets, edge_file),
                'cache_snapshot': {b: {
                    'facets': sum(f.bin == b for f in facets.values()),
                    'zero': sum(f.bin == b and not f.retained_pieces for f in facets.values()),
                    'positive': sum(f.bin == b and f.retained_pieces > 0 for f in facets.values()),
                } for b in MESH_BINS}}


def edge_points(leaves, edge, attribute='points'):
    chord, k = edge
    points = getattr(leaves[chord], attribute)
    return points[k], points[(k + 1) % 3]


class Batch:
    def __init__(self, witness, leaves):
        self.witness = witness
        self.leaves = leaves
        self.groups = []
        self.opens = []
        self.splits = []
        self.reason = None

    def close(self, planes):
        try:
            self.validate(planes)
        except INVALID as error:
            self.reason = self.reason or str(error)
        return self

    def validate(self, planes):
        n = int(self.witness['chords'])
        need(n == len(self.leaves) == int(planes['triangles']), 'chord totals differ')
        need(all(leaf is not None for leaf in self.leaves), 'missing usable leaf')
        need(len(self.groups) == int(planes['groups']), 'group total differs')
        membership = {}
        for i, group in enumerate(self.groups):
            need(int(group['index']) == i and group['status'] in STATUSES, 'bad group index or status')
            int(group['axis'])
            int(group['sign'])
            chords = list(map(int, csv_list(group['chords'])))
            need(bool(chords) and chords == sorted(chords), 'group chords missing or unsorted')
            for chord in chords:
                need(0 <= chord < n and chord not in membership, 'groups are not a partition')
                membership[chord] = i
            if group['status'] == 'trimmed':
                need(set(map(int, csv_list(group['kept']))) <= set(chords), 'bad trimmed group membership')
        need(len(membership) == n, 'groups are not a partition')
        need(len(self.opens) == 1, 'expected one open row')
        open_row = self.opens[0]
        self.open_edges = tuple(indices(token, '.') for token in csv_list(open_row['edges']))
        need(len(self.open_edges) == len(set(self.open_edges)) == int(open_row['open_count']), 'bad open count')
        need(all(chord < n and k < 3 for chord, k in self.open_edges), 'open edge out of range')
        need(len(self.splits) == int(open_row['split_count']), 'bad split count')
        keys = set()
        for split in self.splits:
            first, second = split_edges(split)
            a, b = int(split['group']), int(split['other_group'])
            need(0 <= first[0] < second[0] < n and all(0 <= edge[1] < 3 for edge in (first, second)) and
                 a != b and membership[first[0]] == a and membership[second[0]] == b and
                 2 <= int(split['chords']) <= n, 'bad split indices')
            key = tuple(sorted(edge_points(self.leaves, first)))
            need(key not in keys and key == tuple(sorted(edge_points(self.leaves, second))),
                 'duplicate or inconsistent split key')
            keys.add(key)


def split_edges(row):
    return tuple((int(row[prefix + 'triangle']), int(row[prefix + 'edge'])) for prefix in ('', 'other_'))


class Forward:
    def __init__(self, row):
        self.ordinal = int(row['ordinal'])
        self.bin = int(row.get('bin', 0))
        self.ended = False
        self.open_batch = None
        self.batches = []
        self.queries = []
        self.results = []
        self.malformed = False

    def cut(self):
        if self.open_batch is not None:
            self.batches.append(('incomplete', self.open_batch))
            self.open_batch = None

    def row(self, kind, row, tables):
        if kind == PREFIX + b'QUERY':
            self.queries.append(row)
        elif kind == PREFIX + b'QUERY_RESULT':
            self.results.append(row)
        elif kind == PREFIX + b'WITNESS':
            self.cut()
            self.open_batch = Batch(row, ())
            self.open_batch.leaves = tables.join(row)
        elif kind == PREFIX + b'PLANES':
            batch, self.open_batch = self.open_batch, None
            if batch is None:
                batch = Batch({}, ())
                batch.reason = 'planes without witness'
            else:
                batch.close(row)
            self.batches.append(('not_usable' if batch.reason else 'usable', batch))
        elif self.open_batch is not None:
            batch = self.open_batch
            if kind == PREFIX + b'PLANE_GROUP':
                batch.groups.append(row)
            elif kind == PREFIX + b'PLANE_OPEN':
                batch.opens.append(row)
            elif kind == PREFIX + b'PLANE_SPLIT':
                batch.splits.append(row)
            elif kind == PREFIX + b'TRACE_INCOMPLETE':
                batch.reason = 'trace incomplete'

    def geometry(self):
        need(len(self.queries) == len(self.results) == 1, 'expected one paired outer query')
        query, result = self.queries[0], self.results[0]
        key = lambda row: tuple(row[k] for k in ('authority', 'budget', 'start'))
        need(key(query) == key(result), 'outer query/result keys differ')
        triangles = query['triangles']
        need(triangles.count(';') == 1 and triangles.endswith(';'), 'expected one physical triangle')
        return (query['authority'], frozenset(map(int, csv_list(query['support']))),
                Projection(triple(triangles[:-1]), exact_word(query['tolerance'])))


PACKETS = NEW_ROWS | {PREFIX + name for name in (b'QUERY', b'QUERY_RESULT', b'PLANES')}
FACET_ROWS = {PREFIX + name for name in (b'FACET_BEGIN', b'FACET_LEAF', b'FACET_END')}


class Packets:
    def __init__(self, fixture=False):
        self.fixture = fixture
        self.tables = LeafTables()
        self.forward = None
        self.errors = Counter()

    def detach(self):
        old, self.forward = self.forward, None
        if old:
            old.cut()
        return old

    def read(self, raw):
        if self.fixture and b'\x1b' in raw:
            raw = ANSI.sub(b'', raw)
        kind, _, tail = raw.partition(b' ')
        kind = kind.rstrip()
        if raw.startswith((b'FP2_M1 TRACE_BEGIN ' if self.fixture else b'FP2_M2 Q begin ')):
            old = self.detach()
            self.forward = Forward(fields(raw.split(maxsplit=3 if not self.fixture else 2)[-1]))
            return old, None
        if raw.startswith((b'FP2_M1 TRACE_END ' if self.fixture else b'FP2_M2 Q end ')):
            ordinal = int(fields(raw.split(maxsplit=3 if not self.fixture else 2)[-1])['ordinal'])
            if self.forward and self.forward.ordinal == ordinal and not self.forward.ended:
                self.forward.cut()
                self.forward.ended = True
                if self.fixture:
                    return self.detach(), None
            else:
                self.errors['unmatched_end'] += 1
            return None, None
        if not self.fixture and raw.startswith(b'FP2_M3 FORWARD '):
            row = fields(raw.split(maxsplit=2)[-1])
            if self.forward and self.forward.ordinal == int(row['ordinal']):
                return self.detach(), row
            self.errors['unmatched_summary'] += 1
        inside = self.forward is not None and not self.forward.ended
        if kind in FACET_ROWS or (inside and kind in PACKETS):
            try:
                need(raw.endswith(b'\n'), 'partial trace row')
                row = fields(tail)
                if kind in FACET_ROWS:
                    self.tables.feed(kind, row, inside and not self.fixture)
                else:
                    self.forward.row(kind, row, self.tables)
            except INVALID:
                self.errors[kind.decode()] += 1
                if inside and kind not in FACET_ROWS:
                    if self.forward.open_batch is not None:
                        self.forward.open_batch.reason = 'malformed packet row'
                    else:
                        self.forward.malformed = True
        return None, None


def first_m1_batch(stream):
    """Return the first usable batch in a paired M1 production mark."""
    packets = Packets(fixture=True)
    for line in stream:
        if isinstance(line, str):
            line = line.encode()
        forward, _ = packets.read(line)
        if forward and forward.ended:
            for status, batch in forward.batches:
                if status == 'usable':
                    return batch.witness, batch.leaves
    return None


def load_facets(source):
    facets = {}
    for row in csv.DictReader(source):
        ident = int(row['facet'])
        need(ident not in facets, 'repeated facet in facet_vertices.csv')
        facet = Facet(tuple(int(row[k]) for k in ('v0', 'v1', 'v2')),
                      tuple(row[k] for k in ('body', 'sheet', 'marker')),
                      int(row['bin']), int(row['retained_pieces']))
        need(facet.bin in MESH_BINS and len(set(facet.vertices)) == 3, 'invalid facet geometry or bin')
        facets[ident] = facet
    need(bool(facets), 'empty facet table')
    return facets


def share(count, total):
    return {'count': count, 'denominator': total, 'share': m2.divide(count, total)}


def leaf_report(tables, authority, facets, edge_file):
    by_resolution = defaultdict(dict)
    for (owner, facet, resolution), (leaves, inside, unusable) in tables.items():
        if owner == authority:
            by_resolution[resolution][facet] = leaves, inside, unusable
    with edge_file.open(encoding='utf-8', newline='') as source:
        model_edges = [(int(r['facet0']), int(r['facet1']), (int(r['v0']), int(r['v1'])), int(r['bin']))
                       for r in csv.DictReader(source)]
    need(bool(model_edges), 'empty sealed model edge table')
    reports = {}
    for resolution, facet_tables in sorted(by_resolution.items(), key=lambda entry: exact_word(entry[0])):
        counts = {b: Counter() for b in MESH_BINS}
        depths = {b: Counter() for b in MESH_BINS}
        missing_facet = 0
        for ident, (leaves, inside, unusable) in facet_tables.items():
            if ident not in facets:
                missing_facet += 1
                continue
            b = facets[ident].bin
            counts[b]['published'] += 1
            counts[b]['built_inside_step_c'] += inside
            if unusable or not leaves:
                counts[b]['not_usable_for_depth'] += 1
                continue
            levels = {leaf.depth for leaf in leaves}
            counts[b]['depth_denominator'] += 1
            counts[b]['mixed_depth'] += len(levels) > 1
            depths[b][max(levels)] += 1
        edge_totals = {b: Counter() for b in MESH_BINS}

        @lru_cache(maxsize=8192)
        def intervals(facet, edge):
            return leaf_intervals(facet_tables[facet][0], facets[facet], edge)

        for left, right, edge, b in model_edges:
            if left not in facet_tables or right not in facet_tables:
                continue
            edge_totals[b]['both_published'] += 1
            if (left not in facets or right not in facets or
                    any(not facet_tables[f][0] or facet_tables[f][2] for f in (left, right))):
                edge_totals[b]['not_usable'] += 1
                continue
            edge_totals[b]['compared'] += 1
            edge_totals[b]['different'] += intervals(left, edge) != intervals(right, edge)
        reports[resolution] = {
            'facets_without_export_row': missing_facet,
            'facet_centroid_bins': {b: {
                **{k: counts[b][k] for k in ('published', 'built_inside_step_c', 'not_usable_for_depth')},
                'largest_depth_histogram': dict(depths[b]),
                'mixed_depth': share(counts[b]['mixed_depth'], counts[b]['depth_denominator']),
            } for b in MESH_BINS},
            'edge_midpoint_bins': {b: {
                'with_both_tables_published': edge_totals[b]['both_published'],
                'not_usable': edge_totals[b]['not_usable'],
                'different_intervals': share(edge_totals[b]['different'], edge_totals[b]['compared']),
            } for b in MESH_BINS},
        }
    return reports


def add_pairs(pairs, edge, adjacent):
    for kind, peer in adjacent:
        if kind in ('fine', 'coarse'):
            key = tuple(sorted((edge.index, peer.index)))
            pairs[key] = peer.leaf.depth - edge.leaf.depth if kind == 'coarse' else edge.leaf.depth - peer.leaf.depth


def leaf_example(leaf):
    return {'facet': leaf.facet, 'ordinal': leaf.ordinal, 'depth': leaf.depth,
            'parameter': [[str(w) for w in corner] for corner in leaf.parameter]}


class BatchStats:
    def __init__(self, batch, facets, neighbours, support, projection, examples, other, ordinal):
        self.group_statuses = Counter(row['status'] for row in batch.groups)
        self.open_counts = Counter()
        self.split_counts = Counter()
        self.inside = set()
        self.pairs = {'intra': {}, 'inter': {}}
        self.open_examples = []
        self.split_examples = []
        self.large_splits = 0
        index = EdgeIndex(batch.leaves, facets)
        for key in batch.open_edges:
            edge = index.by_index[key]
            neighbour = neighbours.get((edge.leaf.facet, edge.reference))
            same = relations(edge, index.same_facet(edge), 'interval')
            common = (relations(edge, index.other_facet(edge, neighbour), 'reference_interval')
                      if neighbour is not None else [])
            category = classify_edge(edge, same, common, neighbour, support)
            self.open_counts[category] += 1
            inside = projection.inside(*edge_points(batch.leaves, key, 'point_grid'))
            if inside:
                self.inside.add(category)
                add_pairs(self.pairs['intra'], edge, same)
                add_pairs(self.pairs['inter'], edge, common)
            if examples or (category == 'other' and len(other) < 50):
                row = {'chord': key[0], 'edge': key[1], 'class': category, 'inside': inside}
                if inside:
                    row['leaf'] = leaf_example(edge.leaf)
                if examples:
                    self.open_examples.append(row)
                if category == 'other' and len(other) < 50:
                    other.append({'ordinal': ordinal, **row, 'leaf': leaf_example(edge.leaf)})
        for split in batch.splits:
            first, second = split_edges(split)
            left, right = (batch.groups[int(split[k])] for k in ('group', 'other_group'))
            category = 'axis_sign' if any(left[k] != right[k] for k in ('axis', 'sign')) else 'same_projection'
            self.split_counts[category] += 1
            self.large_splits += int(split['chords']) > 2
            inside = projection.inside(*edge_points(batch.leaves, first, 'point_grid'))
            if inside:
                self.inside.add('key_split')
            if examples:
                row = {**split, 'class': 'key_split', 'subclass': category, 'inside': inside}
                if inside:
                    row['leaf'] = leaf_example(batch.leaves[first[0]])
                    row['other_leaf'] = leaf_example(batch.leaves[second[0]])
                self.split_examples.append(row)


class Tables:
    def __init__(self):
        self.cohort = Counter()
        self.batch_counts = Counter()
        self.group_counts = Counter()
        self.group_status = Counter()
        self.open_counts = Counter()
        self.split_counts = Counter()
        self.large_splits = 0
        self.inside_counts = Counter()
        self.merged = Counter({'empty': 0})
        self.pair_counts = {'intra': Counter(), 'inter': Counter()}
        self.depth_delta = {'intra': Counter(), 'inter': Counter()}

    def add(self, collected):
        self.cohort['measured'] += 1
        inside = set()
        pairs = Counter()
        for stats in collected:
            self.group_counts[sum(stats.group_statuses.values())] += 1
            self.group_status.update(stats.group_statuses)
            self.open_counts.update(stats.open_counts)
            self.split_counts.update(stats.split_counts)
            self.large_splits += stats.large_splits
            inside.update(stats.inside)
            for scope, counts in stats.pairs.items():
                pairs[scope] += len(counts)
                self.depth_delta[scope].update(counts.values())
        self.inside_counts.update(inside or {'none'})
        self.merged[','.join(sorted({MERGED[k] for k in inside})) or 'empty'] += 1
        for scope, counts in self.pair_counts.items():
            counts[pairs[scope]] += 1

    def report(self, root):
        forwards = self.cohort['measured']
        batches = sum(self.group_counts.values())
        groups = sum(self.group_status.values())
        return {
            'forwards': {k: self.cohort[k] for k in
                         ('eligible', 'measured', 'no_batch', 'incomplete', 'not_usable', 'not_classifiable')},
            'batch_counts': {k: self.batch_counts[k] for k in ('0', '1', 'more')},
            'groups_per_batch': m2.histogram(self.group_counts),
            'group_status': {k: share(self.group_status[k], groups) for k in STATUSES},
            'open_edges_per_batch': {k: mean(self.open_counts[k], batches) for k in OPEN_CLASSES},
            'split_keys_per_batch': {'all': mean(sum(self.split_counts.values()), batches),
                                     'above_2_chords': mean(self.large_splits, batches),
                                     **{k: mean(self.split_counts[k], batches) for k in SPLIT_CLASSES}},
            'inside_forwards': {k: share(self.inside_counts[k], forwards) for k in (*INSIDE_CLASSES, 'none')},
            'merged_inside_sets': ({k: share(v, forwards) for k, v in sorted(self.merged.items())}
                                   if root == 0 else None),
            'tj_pairs_per_forward': {k: m2.histogram(v) for k, v in self.pair_counts.items()},
            'tj_depth_difference': {k: {
                'pairs': sum(v.values()), '1': v[1], '2': v[2],
                'more': sum(count for depth, count in v.items() if depth > 2),
                'nonpositive': sum(count for depth, count in v.items() if depth < 1),
            } for k, v in self.depth_delta.items()},
        }


def mean(total, n):
    return {'total': total, 'n': n, 'mean': m2.divide(total, n)}


class Analysis:
    def __init__(self, facets):
        self.facets = facets
        self.neighbours = edge_neighbours(facets)
        self.groups = {(b, root): Tables() for b in BINS for root in (0, 1)}
        self.forwards = Counter()
        self.batches = Counter()
        self.reasons = Counter()
        self.authorities = Counter()
        self.authority_by_marks = Counter()
        self.examples = []
        self.other = []

    def add(self, forward, row):
        if not forward:
            return
        self.forwards['all'] += 1
        self.authority_by_marks.update({q['authority'] for q in forward.queries if 'authority' in q})
        statuses = Counter(status for status, _ in forward.batches)
        self.batches.update(statuses)
        self.reasons.update(batch.reason for status, batch in forward.batches if batch.reason)
        if not row or not forward.ended:
            self.forwards['not_classifiable'] += 1
            self.reasons['missing paired mark or M2 completion'] += 1
            return
        classification = row['class']
        root = int(row['root_enough'])
        if classification not in m2.CLASSES or (forward.bin, root) not in self.groups:
            self.forwards['not_classifiable'] += 1
            self.reasons['invalid M2 completion'] += 1
            return
        self.forwards[classification] += 1
        if classification not in ('miss', 'bypass'):
            return
        group = self.groups[forward.bin, root]
        group.cohort['eligible'] += 1
        batch_count = len(forward.batches)
        group.batch_counts[str(batch_count) if batch_count < 2 else 'more'] += 1
        try:
            need(not forward.malformed, 'malformed forward packet')
            authority, support, projection = forward.geometry()
            self.authorities[authority] += 1
        except INVALID as error:
            self.forwards['not_classifiable'] += 1
            self.reasons[str(error)] += 1
            group.cohort['not_classifiable'] += 1
            return
        for excluded in ('incomplete', 'not_usable'):
            if statuses[excluded]:
                group.cohort[excluded] += 1
                self.forwards['with_' + excluded + '_batch'] += 1
        if not forward.batches:
            group.cohort['no_batch'] += 1
            self.forwards['no_batch'] += 1
        if not batch_count or statuses['usable'] != batch_count:
            return
        if any(leaf.facet not in self.facets for _, batch in forward.batches for leaf in batch.leaves):
            group.cohort['not_classifiable'] += 1
            self.forwards['not_classifiable'] += 1
            self.reasons['witness facet absent from export'] += 1
            return
        save = forward.bin == max(BINS) and root == 0 and len(self.examples) < 20
        collected = []
        examples = []
        for _, batch in forward.batches:
            stats = BatchStats(batch, self.facets, self.neighbours, support, projection, save, self.other,
                               forward.ordinal)
            collected.append(stats)
            if save:
                examples.append({'witness': batch.witness, 'groups': batch.groups,
                                 'open_edges': stats.open_examples, 'split_keys': stats.split_examples})
        if save:
            self.examples.append({'ordinal': forward.ordinal, 'bin': forward.bin,
                                  'root_enough': root, 'class': classification, 'batches': examples})
        group.add(collected)
        self.forwards['measured'] += 1

    def report(self):
        return {'completion': 'complete',
                'forwards': dict(self.forwards),
                'batches': dict(self.batches),
                'exclusion_reasons': dict(self.reasons),
                'measured_outer_authorities': dict(self.authorities),
                'marked_outer_authorities': dict(self.authority_by_marks),
                'groups': {b: {root: self.groups[b, root].report(root) for root in (0, 1)} for b in BINS}}


def analyze(stream, facets):
    packets = Packets()
    result = Analysis(facets)
    for line in stream:
        try:
            forward, row = packets.read(line)
            result.add(forward, row)
        except INVALID as error:
            packets.errors['malformed_mark_or_summary'] += 1
            result.reasons[str(error)] += 1
    result.add(packets.detach(), None)
    return packets, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='*', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        from self_check_provenance import run
        print(json.dumps(run(), indent=2))
        return
    if len(args.paths) != 5:
        parser.error('expected STREAM FACET_VERTICES MODEL_EDGES ANALYSIS_JSON EXAMPLES_JSON')
    stream_path, facet_path, edges_path, output, examples = args.paths
    with facet_path.open(encoding='utf-8', newline='') as source:
        facets = load_facets(source)
    with gzip.open(stream_path, 'rb') as stream:
        packets, result = analyze(stream, facets)
    report = result.report()
    with stream_path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    report['stream'] = {'file': stream_path.name, 'bytes': stream_path.stat().st_size, 'sha256': digest}
    report['malformed_rows'] = dict(packets.errors)
    authorities = result.authority_by_marks.most_common()
    need(len(authorities) == 1, 'expected one authority inside marked step-C forwards')
    report['leaf_tables'] = packets.tables.report(authorities[0][0], facets, edges_path)
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    examples.write_text(json.dumps({'forward_count': len(result.examples), 'forwards': result.examples,
                                   'other': result.other}, indent=2, sort_keys=True) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
