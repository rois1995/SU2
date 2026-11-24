"""Exact predicates for the FP2-M3 offline chord analysis."""

from bisect import bisect_left, bisect_right
from collections import defaultdict
from dataclasses import dataclass, field
from fractions import Fraction
from functools import lru_cache
import math
import struct


@lru_cache(maxsize=65536)
def exact_word(word):
    if len(word) != 16:
        raise ValueError('invalid binary64 trace word width')
    value, = struct.unpack('>d', bytes.fromhex(word))
    if not math.isfinite(value):
        raise ValueError('nonfinite binary64 trace word')
    return Fraction.from_float(value)


def triple(word):
    values = word.rstrip(',').split(',')
    if len(values) != 9:
        raise ValueError('expected nine trace words')
    numbers = tuple(map(exact_word, values))
    return numbers[:3], numbers[3:6], numbers[6:]


@dataclass(frozen=True, slots=True)
class Leaf:
    facet: int
    ordinal: int
    depth: int
    parameter: tuple
    points: tuple
    error: Fraction
    parameter_grid: tuple = field(init=False, repr=False, compare=False)
    point_grid: tuple = field(init=False, repr=False, compare=False)

    def __post_init__(self):
        if not 0 <= self.depth <= 1074:
            raise ValueError('invalid dyadic leaf depth')
        parameters = []
        extent = 1 << self.depth
        for corner in self.parameter:
            if any(w < 0 or w.denominator.bit_length() - 1 > self.depth for w in corner):
                raise ValueError('non-barycentric or non-dyadic leaf corner')
            integer_corner = tuple(scaled(w, self.depth) for w in corner)
            if sum(integer_corner) != extent:
                raise ValueError('non-barycentric or non-dyadic leaf corner')
            parameters.append(integer_corner)
        object.__setattr__(self, 'parameter_grid', tuple(parameters))
        object.__setattr__(self, 'point_grid', tuple(integer_point(p) for p in self.points))

    @classmethod
    def read(cls, row):
        return cls(int(row['facet']), int(row['ordinal']), int(row['depth']), triple(row['parameter']),
                   triple(row['points']), exact_word(row['error']))


@dataclass(frozen=True, slots=True)
class Facet:
    vertices: tuple
    entity: tuple
    bin: int
    retained_pieces: int


def facet_edges(facet):
    return (tuple(sorted((facet.vertices[i], facet.vertices[(i + 1) % 3]))) for i in range(3))


def edge_neighbours(facets):
    uses = defaultdict(list)
    for ident, facet in facets.items():
        for edge in facet_edges(facet):
            uses[facet.entity, edge].append(ident)
    neighbours = {}
    for (_, edge), ids in uses.items():
        if len(ids) > 2:
            raise ValueError(f'nonmanifold same-entity reference edge {edge}')
        if len(ids) == 2:
            left, right = ids
            neighbours[left, edge] = right
            neighbours[right, edge] = left
    return neighbours


@dataclass(slots=True)
class Edge:
    index: tuple
    leaf: Leaf
    line: tuple | None
    interval: tuple | None
    reference: tuple | None
    reference_interval: tuple | None


def scaled(value, exponent):
    return value.numerator << (exponent - value.denominator.bit_length() + 1)


def edge_line(first, last):
    dx, dy = last[0] - first[0], last[1] - first[1]
    divisor = math.gcd(dx, dy)
    if not divisor:
        return None, None
    dx, dy = dx // divisor, dy // divisor
    if dx < 0 or (dx == 0 and dy < 0):
        dx, dy = -dx, -dy
    axis = int(dx == 0)
    return (dx, dy, dx * first[1] - dy * first[0]), tuple(sorted((first[axis], last[axis])))


def reference_interval(first, last, facet):
    zero = next((i for i in range(3) if not first[i] and not last[i]), None)
    if zero is None:
        return None, None
    low, high = sorted(facet.vertices[i] for i in range(3) if i != zero)
    axis = facet.vertices.index(high)
    return (low, high), tuple(sorted((first[axis], last[axis])))


class IntervalIndex:
    """Index intervals by span and start, so disjoint intervals on one line are skipped."""

    def __init__(self, edges, attribute):
        lengths = defaultdict(list)
        for e in edges:
            lo, hi = getattr(e, attribute)
            lengths[hi - lo].append((lo, e))
        self.spans = []
        for length, entries in lengths.items():
            entries.sort(key=lambda x: x[0])
            starts, ordered = zip(*entries, strict=True)
            self.spans.append((length, starts, ordered))

    def overlaps(self, interval):
        lo, hi = interval
        for length, starts, edges in self.spans:
            first = bisect_right(starts, lo - length)
            last = bisect_left(starts, hi, first)
            yield from edges[first:last]


class EdgeIndex:
    def __init__(self, leaves, facets):
        self.intra = defaultdict(list)
        self.inter = defaultdict(list)
        self.by_index = {}
        self.indexes = {}
        depth = max((leaf.depth for leaf in leaves), default=0)
        for chord, leaf in enumerate(leaves):
            facet = facets[leaf.facet]
            shift = depth - leaf.depth
            scaled_corners = (leaf.parameter_grid if not shift else
                              [tuple(w << shift for w in corner) for corner in leaf.parameter_grid])
            for k in range(3):
                first, last = scaled_corners[k], scaled_corners[(k + 1) % 3]
                line, interval = edge_line(first, last)
                ref, ref_interval = reference_interval(first, last, facet)
                edge = Edge((chord, k), leaf, line, interval, ref, ref_interval)
                self.by_index[edge.index] = edge
                if interval is not None:
                    self.intra[leaf.facet, line].append(edge)
                if ref and ref_interval[0] != ref_interval[1]:
                    self.inter[leaf.facet, ref].append(edge)

    def peers(self, scope, key, attribute, interval):
        if interval is None:
            return ()
        edges = scope.get(key, ())
        if len(edges) <= 8:
            return edges
        index_key = key, attribute
        index = self.indexes.get(index_key)
        if index is None:
            index = self.indexes[index_key] = IntervalIndex(edges, attribute)
        return index.overlaps(interval)

    def same_facet(self, edge):
        peers = self.peers(self.intra, (edge.leaf.facet, edge.line), 'interval', edge.interval)
        return (peer for peer in peers if peer.index[0] != edge.index[0])

    def other_facet(self, edge, neighbour):
        return self.peers(self.inter, (neighbour, edge.reference), 'reference_interval', edge.reference_interval)


def relation(left, right):
    if left is None or right is None or max(left[0], right[0]) >= min(left[1], right[1]):
        return None
    if left == right:
        return 'same_segment'
    if left[0] <= right[0] and right[1] <= left[1]:
        return 'coarse'
    if right[0] <= left[0] and left[1] <= right[1]:
        return 'fine'
    return 'other'


def relations(edge, peers, attribute):
    return [(kind, peer) for peer in peers
            if (kind := relation(getattr(edge, attribute), getattr(peer, attribute))) is not None]


def classify_edge(edge, same, other, neighbour, support):
    # Keep the priority at each tier even when a coarse and a fine overlap both exist.
    def select(overlaps, scope):
        names = {kind for kind, _ in overlaps}
        for kind in ('coarse', 'fine'):
            if kind in names:
                return f'tj_{scope}_{kind}'
        return 'same_segment' if 'same_segment' in names else None

    category = select(same, 'intra')
    if category:
        return category
    if not edge.reference:
        return 'other' if same else 'crop_intra'
    if neighbour is None:
        return 'entity_border'
    if neighbour not in support:
        return 'support_border'
    return select(other, 'inter') or ('other' if other else 'crop_inter')


def vector_sub(a, b):
    return tuple(x - y for x, y in zip(a, b, strict=True))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b, strict=True))


def exponent(points):
    return max(x.denominator.bit_length() - 1 for point in points for x in point)


def integer_point(point):
    scale = exponent((point,))
    return scale, tuple(scaled(x, scale) for x in point)


class Projection:
    """The face and chord midpoint use a shared exact dyadic frame."""

    def __init__(self, points, tolerance):
        if tolerance <= 0:
            raise ValueError('nonpositive tolerance')
        self.scale = exponent(points)
        first, second, third = (tuple(scaled(x, self.scale) for x in p) for p in points)
        self.origin = first
        self.u, self.v = vector_sub(second, first), vector_sub(third, first)
        self.uu, self.uv, self.vv = dot(self.u, self.u), dot(self.u, self.v), dot(self.v, self.v)
        self.area_squared = self.uu * self.vv - self.uv * self.uv
        if not self.area_squared:
            raise ValueError('degenerate physical triangle')
        u, v = self.u, self.v
        self.normal = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
        self.distance_limit = (10 * tolerance) ** 2

    def inside(self, first, last):
        first_scale, a = first
        last_scale, b = last
        size = max(self.scale, first_scale, last_scale)
        left, right = size - first_scale, size - last_scale
        midpoint = tuple((x << left) + (y << right) for x, y in zip(a, b, strict=True))
        increase = size + 1 - self.scale
        delta = vector_sub(midpoint, tuple(v << increase for v in self.origin))
        ua, va = dot(delta, self.u), dot(delta, self.v)
        beta = ua * self.vv - va * self.uv
        gamma = va * self.uu - ua * self.uv
        denom = self.area_squared << increase
        if any(not denom <= 50 * v <= 49 * denom for v in (denom - beta - gamma, beta, gamma)):
            return False
        distance = dot(delta, self.normal) ** 2
        return distance * self.distance_limit.denominator <= (
            (self.area_squared << (2 * (size + 1))) * self.distance_limit.numerator)


def leaf_intervals(leaves, facet, model_edge):
    found = []
    for leaf in leaves:
        for i in range(3):
            edge, interval = reference_interval(leaf.parameter[i], leaf.parameter[(i + 1) % 3], facet)
            if edge == model_edge and interval[0] != interval[1]:
                found.append(interval)
    return sorted(found)
