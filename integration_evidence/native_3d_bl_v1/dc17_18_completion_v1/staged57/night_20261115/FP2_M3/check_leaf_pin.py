#!/usr/bin/env python3
"""Check a traced leaf batch against the immutable cubic nets of the curved fixture."""
import argparse
from collections import Counter, defaultdict
from fractions import Fraction
import json
import math
from pathlib import Path

from analyze_provenance import exact_word, first_m1_batch


# MappedFacetReference::CubicNet order, with coefficients of the degree-three Bernstein basis.
TERMS = (
    (1, 3, 0, 0), (1, 0, 3, 0), (1, 0, 0, 3), (3, 2, 1, 0), (3, 1, 2, 0),
    (3, 0, 2, 1), (3, 0, 1, 2), (3, 1, 0, 2), (3, 2, 0, 1), (6, 1, 1, 1),
)


def read_cubics(path):
    with path.open(encoding='ascii') as sidecar:
        if sidecar.readline().split() != ['SU2_NATIVE_REFERENCE_3D', '9']:
            raise ValueError('Expected a version-9 curved fixture sidecar')
        markers, facets, analytic, affine, errors, _, _, _, _, curves, _, _ = sidecar.readline().split()
        for _ in range(sum(map(int, (markers, facets, analytic, affine, errors)))):
            next(sidecar)
        nets = {}
        for _ in range(int(curves)):
            facet_id, *coords = sidecar.readline().split()
            facet = int(facet_id)
            if len(coords) != 30 or facet in nets:
                raise ValueError(f'Invalid/duplicate pinned cubic net for facet {facet}')
            # EncodeReference writes 17 digits and LoadReference rounds to binary64.
            nets[facet] = tuple(Fraction.from_float(float(x)) for x in coords)
    return nets


def evaluate(net, parameter):
    w, u, v = parameter
    weights = [c * w**i * u**j * v**k for c, i, j, k in TERMS]
    return tuple(sum(weight * net[3 * index + d] for index, weight in enumerate(weights)) for d in range(3))


def check(trace, reference):
    with trace.open('rb') as stream:
        packet = first_m1_batch(stream)
    if packet is None:
        return {'status': 'not evaluated', 'reason': 'No complete witness within a paired production M1 trace.'}

    witness, leaves = packet
    resolution = exact_word(witness['resolution'])
    if resolution <= 0 or not leaves:
        raise ValueError('Expected a positive resolution and at least one chord in a complete witness')
    tolerance = resolution / 16
    nets = read_cubics(reference)
    maximum = Fraction(0)
    depths = Counter()
    ordinals = defaultdict(set)
    first_failure = None
    corners = 0
    for leaf in leaves:
        net = nets[leaf.facet]
        depths[leaf.depth] += 1
        ordinals[leaf.facet].add(leaf.ordinal)
        for k, (parameter, stored) in enumerate(zip(leaf.parameter, leaf.points, strict=True)):
            actual = evaluate(net, parameter)
            distance_sq = sum((a - b)**2 for a, b in zip(actual, stored, strict=True))
            maximum = max(maximum, distance_sq)
            corners += 1
            if distance_sq > tolerance**2 and first_failure is None:
                first_failure = {'facet': leaf.facet, 'ordinal': leaf.ordinal, 'depth': leaf.depth, 'corner': k,
                                 'distance': math.sqrt(float(distance_sq)), 'distance_squared_exact': str(distance_sq)}

    return {'status': 'failed' if first_failure else 'passed', 'witness': witness, 'chords': len(leaves),
            'corners_checked': corners, 'depths': dict(sorted(depths.items())),
            'ordinals': {facet: sorted(seen) for facet, seen in sorted(ordinals.items())},
            'limit': float(tolerance), 'limit_exact': str(tolerance), 'maximum_distance': math.sqrt(float(maximum)),
            'maximum_distance_squared_exact': str(maximum), 'first_failure': first_failure,
            'arithmetic': 'Exact rationals from binary64 inputs, using squared distance versus (resolution / 16)^2'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    try:
        result = check(args.trace, args.reference)
    except (ArithmeticError, KeyError, OSError, StopIteration, ValueError) as error:
        result = {'status': 'failed', 'reason': f'{type(error).__name__}: {error}'}
    print(json.dumps(result, indent=2, sort_keys=True))
    return int(result['status'] == 'failed')


if __name__ == '__main__':
    raise SystemExit(main())
