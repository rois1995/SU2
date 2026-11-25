"""Validate the closure-on and closure-off public geometry fixture traces."""

import argparse
from collections import defaultdict
import json
from pathlib import Path
import struct

TOLERANCE = 1e-4
LENGTH = 1 / 32
CASES = ('improvement', 'lower_complete', 'lower_cropped', 'cache_first', 'cache_warm',
         'cache_competitors', 'cache_rebuilt')
MARK = 'FP2_P1_PUBLIC'
PREFIX = 'NATIVE_SURFACE_'
KINDS = frozenset(PREFIX + suffix for suffix in (
    'FACET_LEAF', 'FACET_END', 'WITNESS', 'WITNESS_CLOSED', 'PLANES', 'PLANE_GROUP',
    'GRAPH', 'QUERY_RESULT', 'MODEL', 'DOMAIN', 'TRACE_INCOMPLETE'))
SWITCH = PREFIX + 'CONFORMING_WITNESS enabled=1'


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def fields(text):
    return dict(part.split('=', 1) for part in text.split())


def read_cases(stream, enabled):
    current = None
    collected = {}
    switch_lines = 0
    for line in stream:
        line = line.rstrip()
        if line.startswith(PREFIX + 'CONFORMING_WITNESS '):
            require(line == SWITCH, 'invalid production switch receipt')
            switch_lines += 1
        elif line.startswith(MARK + ' '):
            _, direction, tail = line.split(' ', 2)
            row = fields(tail)
            name = row['case']
            if direction == 'BEGIN':
                require(current is None and name not in collected, f'unexpected beginning of {name}')
                current = name
                collected[name] = defaultdict(list)
            else:
                require(direction == 'END' and name == current, f'unmatched end of {name}')
                collected[name]['end'].append(row)
                current = None
        elif current:
            kind, _, tail = line.partition(' ')
            require(kind != PREFIX + 'TRACE_INCOMPLETE', f'incomplete production trace in {current}')
            if kind in KINDS and current != 'cache_competitors':
                collected[current][kind.removeprefix(PREFIX)].append(fields(tail))
    require(current is None and tuple(collected) == CASES, 'public-query trace cases are missing or out of order')
    require(switch_lines == int(enabled), 'production switch receipt count differs')
    return collected


def one(case, kind):
    require(len(case[kind]) == 1, f'expected one {kind} row, got {len(case[kind])}')
    return case[kind][0]


def children(row):
    return tuple(int(item) for item in row.rstrip(',').split(',')) if row else ()


def info(case, enabled):
    end = one(case, 'end')
    model = one(case, 'MODEL')
    result = one(case, 'QUERY_RESULT')
    bound = float.fromhex(end['bound'])
    require(struct.pack('>d', bound).hex() == result['bound'], 'bound differs from production query result')
    require(result['cache'] in ('miss', 'bypass'), 'face memo bypassed the witness')
    nodes = [row for row in case['DOMAIN'] if row['model'] == '0']
    require(nodes and nodes[0]['depth'] == '0', 'missing physical root node')
    root = nodes[0]
    original = one(case, 'WITNESS')
    require(len(case['WITNESS_CLOSED']) == int(enabled), 'unexpected number of closed witness rows')
    closed = one(case, 'WITNESS_CLOSED') if enabled else None
    if closed:
        require(closed['chords_in'] == original['chords'] and closed['chords_out'] == model['witness'],
                'closed witness sizes do not match production')
    planes = one(case, 'PLANES')
    require(planes['triangles'] == model['witness'], 'footprint built for a different witness size')
    return {'bound': end['bound'], 'full': int(model['full']), 'prepared_nodes': len(nodes),
            'root_upper': float(root['upper']), 'root_lower': float(root['lower']),
            'root_distance_work': int(root['distance_work']), 'cache': result['cache'],
            'witness': original, 'closure': closed, 'planes': planes}


def check_improvement(case, measured, enabled):
    depths = defaultdict(set)
    for row in case['FACET_LEAF']:
        depths[int(row['facet'])].add(int(row['depth']))
    require(set(depths) == {0, 1} and depths[0] == {0} and min(depths[1]) > 0,
            'facet leaves did not establish the depth discontinuity')
    require(float.fromhex(measured['bound']) <= TOLERANCE, 'improvement input failed to prove within')
    if not enabled:
        require(measured['prepared_nodes'] > 1 and measured['root_upper'] > TOLERANCE,
                'closure off did not require physical refinement')
        return {'0': sorted(depths[0]), '1': sorted(depths[1])}
    require(measured['prepared_nodes'] == 1 and measured['root_upper'] <= TOLERANCE,
            'closure on did not prove the root')
    closed = measured['closure']
    require(int(closed['splits']) > 0, 'root was proved without a split')
    source_facets = [int(item.split(':')[0]) for item in measured['witness']['list'].rstrip(',').split(',')]
    origin = children(closed['origin'])
    require(len(origin) == int(closed['chords_out']) and all(0 <= i < len(source_facets) for i in origin),
            'invalid closure origin list')
    require(any(group['status'] in ('accepted', 'trimmed') and
                {source_facets[origin[i]] for i in children(group.get('kept', group['chords']))} == {0, 1}
                for group in case['PLANE_GROUP']), 'no usable footprint joins the two facets')
    require(any(int(row['coverage']) > 0 and float(row['bound']) <= TOLERANCE for row in case['GRAPH']),
            'no passing footprint lift in the single-node proof')
    return {'0': sorted(depths[0]), '1': sorted(depths[1])}


def check_lower(measured, full, analytical):
    require(measured['full'] == full, f'wrong witness completeness, expected {full}')
    require(measured['root_distance_work'] > 0 and measured['root_upper'] > TOLERANCE,
            'physical root did not enter sample distance')
    bound = float.fromhex(measured['bound'])
    require(bound == measured['root_lower'] and bound > TOLERANCE, 'expected root lower-only cutoff')
    require(analytical - TOLERANCE <= bound <= analytical, 'reported lower bound misses its analytic enclosure')


def check_cache(cases, enabled):
    first, warm, rebuilt = (cases['cache_' + name] for name in ('first', 'warm', 'rebuilt'))
    require(not warm['WITNESS'] and not warm['WITNESS_CLOSED'], 'prepared scope reused no witness')
    evictions = int(one(cases['cache_competitors'], 'end')['evictions'])
    require(evictions > 0, 'no cache eviction observed')
    bounds = {}
    for case, name in ((first, 'first'), (warm, 'warm'), (rebuilt, 'rebuilt')):
        end = one(case, 'end')
        bounds[name] = {key: end[key] for key in ('mesh', 'model')}
        if name != 'warm':
            one(case, 'WITNESS')
            if enabled:
                require(int(one(case, 'WITNESS_CLOSED')['splits']) > 0, f'{name} scope was not closed')
            else:
                require(not case['WITNESS_CLOSED'], f'unexpected closure at {name}')
    require(bounds['first'] == bounds['warm'] == bounds['rebuilt'], 'cache rebuild changed bound bits')
    return {'bounds': bounds, 'evictions': evictions}


def validate(path, enabled):
    with path.open(encoding='utf-8', errors='replace') as stream:
        cases = read_cases(stream, enabled)
    result = {name: info(cases[name], enabled) for name in CASES[:3]}
    result['facet_depths'] = check_improvement(cases['improvement'], result['improvement'], enabled)
    check_lower(result['lower_complete'], 1, 7 * LENGTH / 4)
    check_lower(result['lower_cropped'], 0, 35 * LENGTH / 16)
    require(result['lower_complete']['root_lower'] < result['lower_cropped']['root_lower'],
            'the more distant cropped sample did not have the larger lower bound')
    result['cache'] = check_cache(cases, enabled)
    return result


def compare(off, on):
    for lower in ('lower_complete', 'lower_cropped'):
        require(abs(off[lower]['root_lower'] - on[lower]['root_lower']) <= TOLERANCE,
                f'{lower} switch bounds do not agree within the enclosure')
    require(int(on['improvement']['planes']['groups']) < int(off['improvement']['planes']['groups']),
            'closure did not reduce footprint group count')
    return {'completion': 'complete', 'off': off, 'on': on}


def self_check():
    lines = [f'{MARK} {direction} case={name}\n' for name in CASES for direction in ('BEGIN', 'END')]
    assert tuple(read_cases(lines, False)) == CASES
    assert tuple(read_cases([SWITCH, *lines], True)) == CASES
    try:
        read_cases([SWITCH, *lines], False)
    except ValueError:
        pass
    else:
        raise AssertionError('accepted an unexpected enabled marker')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--single', choices=('0', '1'))
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('paths', type=Path, nargs='*')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print('FP2-P1 public query trace parser self-check passed.')
    elif args.single is not None and len(args.paths) == 1:
        print(json.dumps({'completion': 'complete', **validate(args.paths[0], args.single == '1')}, indent=2))
    elif args.single is None and len(args.paths) == 3:
        off, on, output = args.paths
        result = compare(validate(off, False), validate(on, True))
        output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    else:
        parser.error('expected OFF_LOG ON_LOG OUTPUT_JSON, or --single SWITCH LOG')


if __name__ == '__main__':
    main()
