"""Small written packet stream for the analysis and collector self-checks."""

import gzip
from io import BytesIO, StringIO
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

from analyze_provenance import analyze, first_m1_batch, load_facets
from collect_provenance import drain_stderr, m2


FACETS = """facet,body,sheet,marker,v0,v1,v2,bin,retained_pieces
0,0,0,0,10,11,12,4,3
1,0,0,0,11,10,13,3,1
2,0,0,0,12,10,14,6,0
"""
MODEL_EDGES = """v0,v1,facet0,facet1,bin
10,11,0,1,4
10,12,0,2,5
"""

# On F0 the first corners of chords 0 and 1 lie on the same interior parameter
# line. Chord 1 covers half of the coarse edge of chord 0. Chord 2's local edge
# 0 uses the complete (10,11) reference edge. Chord 3, on F1, uses half of it
# with reversed local corner order. F2 is in support but supplies no chord.
PARAMETERS = (
    ((.5, .5, 0), (0, .5, .5), (.5, 0, .5)),
    ((.5, .5, 0), (.25, .5, .25), (.25, .25, .5)),
    ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
    ((0, 1, 0), (.5, .5, 0), (0, .5, .5)),
)
POINTS = ((1/4, 1/4, 0), (1/2, 1/4, 0), (1/4, 1/2, 0))


def word(value):
    return struct.pack('>d', value).hex()


def vector(points):
    return ''.join(word(x) + ',' for p in points for x in p)


def build(facet, corners, depths, end=True):
    context = f'authority=0x1 facet={facet} resolution={word(.125)}'
    rows = [f'NATIVE_SURFACE_FACET_BEGIN {context}']
    for ordinal, (parameter, depth) in enumerate(zip(corners, depths, strict=True)):
        # Shift points per chord so the open portion contains no coincident key.
        d = (facet + ordinal) / 128
        points = tuple((x + d, y + d, z) for x, y, z in POINTS)
        rows.append(f'NATIVE_SURFACE_FACET_LEAF {context} ordinal={ordinal} depth={depth} '
                    f'parameter={vector(parameter)} points={vector(points)} error={word(0)}')
    if end:
        rows.append(f'NATIVE_SURFACE_FACET_END {context} leaves={len(corners)}')
    return rows


def encoded(lines):
    return ('\n'.join(lines) + '\n').encode()


def written_stream():
    begin = 'FP2_M2 Q begin ordinal=1 bin=4 direct_eligible=0 longest_edge=1 sheet=0'
    end = 'FP2_M2 Q end ordinal=1 answer=1 seconds=0.25'
    query = (f'NATIVE_SURFACE_QUERY authority=0x1 budget=0x2 start=3 tolerance={word(.125)} '
             f'support=0,1,2, triangles={vector(((0, 0, 0), (1, 0, 0), (0, 1, 0)))};')
    result = f'NATIVE_SURFACE_QUERY_RESULT authority=0x1 budget=0x2 start=3 cache=miss work=7 bound={word(.25)}'
    witness = f'NATIVE_SURFACE_WITNESS authority=0x1 resolution={word(.125)} chords=4 list=0:0,0:1,0:2,1:0,'
    groups = [f'NATIVE_SURFACE_PLANE_GROUP index={i} axis=2 sign=1 status=single chords={i},' for i in range(4)]
    open_row = 'NATIVE_SURFACE_PLANE_OPEN open_count=6 split_count=0 edges=0.0,1.0,2.0,3.0,2.2,3.2,'
    planes = ('NATIVE_SURFACE_PLANES triangles=4 groups=4 accepted=0 invalid=0 storage_refused=0 '
              'boundary_trimmed=0 trimmed_triangles=0')
    return [begin, query, *build(0, PARAMETERS[:3], (1, 2, 0)), *build(1, PARAMETERS[3:], (1,)),
            *build(2, PARAMETERS[3:], (1,), end=False), witness, *groups, open_row, planes, result, end]


def run():
    m2.self_check()
    rows = written_stream()
    facets = load_facets(StringIO(FACETS))
    completion = 'FP2_M3 FORWARD ordinal=1 class=miss root_enough=0'
    packets, analysis = analyze(BytesIO(encoded([*rows, completion])), facets)
    cut = (rows[0].replace('ordinal=1', 'ordinal=2'),
           next(x for x in rows if x.startswith('NATIVE_SURFACE_WITNESS')))
    _, analysis_with_cut = analyze(BytesIO(encoded([*rows, completion, *cut])), facets)
    assert analysis_with_cut.batches['incomplete'] == 1
    assert analysis_with_cut.forwards['not_classifiable'] == 1
    group = analysis.groups[4, 0].report(0)
    assert group['forwards']['measured'] == 1, analysis.report()
    expected = dict.fromkeys(('tj_intra_coarse', 'tj_intra_fine', 'tj_inter_coarse', 'tj_inter_fine',
                              'crop_inter', 'entity_border'), 1)
    counted = {k: v['total'] for k, v in group['open_edges_per_batch'].items() if v['total']}
    assert counted == expected, counted
    assert group['tj_pairs_per_forward']['intra']['mean'] == group['tj_pairs_per_forward']['inter']['mean'] == 1
    assert group['tj_depth_difference']['intra']['1'] == group['tj_depth_difference']['inter']['1'] == 1
    assert group['merged_inside_sets']['border,crop,tj']['count'] == 1
    fixture = ['FP2_M1 TRACE_BEGIN ordinal=1', *rows[1:-1], 'FP2_M1 TRACE_END ordinal=1']
    witness, leaves = first_m1_batch(BytesIO(encoded(fixture)))
    assert witness['list'] == '0:0,0:1,0:2,1:0,' and [leaf.ordinal for leaf in leaves] == [0, 1, 2, 0]

    with TemporaryDirectory(prefix='fp2-m3-provenance-') as work:
        directory = Path(work)
        edge_file = directory / 'edges.csv'
        edge_file.write_text(MODEL_EDGES, encoding='utf-8')
        leaf_report = packets.tables.report('0x1', facets, edge_file)
        published = leaf_report['publication']
        assert published['complete'] == published['complete_inside_step_c'] == 2 and published['incomplete'] == 1
        resolution = leaf_report['resolutions'][word(.125)]
        assert resolution['facet_centroid_bins'][4]['largest_depth_histogram'] == {2: 1}
        assert resolution['facet_centroid_bins'][4]['mixed_depth']['count'] == 1
        assert resolution['edge_midpoint_bins'][4]['different_intervals']['count'] == 1
        assert resolution['edge_midpoint_bins'][5]['with_both_tables_published'] == 0
        ordinary = StringIO()
        drain_stderr(BytesIO(encoded(rows) + b'ordinary\n'), ordinary, directory)
        with gzip.open(directory / 'provenance.log.gz', 'rb') as stream:
            collected = stream.read()
        assert completion.encode() in collected
        assert ordinary.getvalue() == 'ordinary\n'
        assert analyze(BytesIO(collected), facets)[1].report() == analysis.report()
    return {'completion': 'complete', 'published_facets': 2, 'cut_facet_builds': 1, 'cut_batches': 1,
            'open_edges_by_class': expected, 'inside_tj_pairs': {'intra': 1, 'inter': 1},
            'matched_m1_packet': witness, 'collector_round_trip': 'complete'}
