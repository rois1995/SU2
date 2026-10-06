"""Independent frozen-P1 and topology/altitude audit of saved small native BL fixtures."""
import argparse
import csv
import hashlib
from fractions import Fraction
import json
import math
from pathlib import Path


def mesh(path):
    lines = iter(Path(path).read_text().splitlines())
    cells, points, markers = [], [], {}
    for line in lines:
        if line.startswith('NELEM='):
            cells = [tuple(map(int, next(lines).split()[1:4])) for _ in range(int(line.split('=')[1]))]
        elif line.startswith('NPOIN='):
            points = [tuple(map(float, next(lines).split()[:2])) for _ in range(int(line.split('=')[1]))]
        elif line.startswith('MARKER_TAG='):
            name = line.split('=')[1].strip()
            count = int(next(lines).split('=')[1])
            markers[name] = [tuple(map(int, next(lines).split()[1:3])) for _ in range(count)]
    return points, cells, markers


def cross(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def audit(donor_path, metric_path, candidate_path, height, wall_tags=("lower_a", "lower_b"), fast=False, extension_limit=0):
    paths = tuple(Path(path).resolve() for path in (donor_path, metric_path, candidate_path))
    input_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    donor, donor_cells, donor_markers = mesh(donor_path)
    with Path(metric_path).open() as handle:
        tensors = [tuple(map(float, row)) for row in list(csv.reader(handle))[1:]]
    assert len(tensors) == len(donor)
    p, cells, markers = mesh(candidate_path)
    misses = 0
    def field(x):
        nonlocal misses
        for ids in donor_cells:
            a,b,c = (donor[i] for i in ids)
            det = cross(a,b,c)
            weights = (cross(x,b,c)/det, cross(a,x,c)/det, cross(a,b,x)/det)
            if min(weights) >= -1e-12:
                return tuple(sum(weights[k]*tensors[ids[k]][j] for k in range(3)) for j in range(3))
        misses += 1
        raise ValueError(f'Query outside donor: {x}')
    locator = None
    if fast:
        from frozen_field_audit import FrozenField
        locator = FrozenField(donor, donor_cells, tensors, donor_markers, extension_limit)
        field = locator
    def quality(ids):
        a,b,c = (p[i] for i in ids)
        center = tuple((a[j]+b[j]+c[j])/3 for j in range(2))
        xx,xy,yy = field(center)
        s = 0
        for u,v in ((a,b),(b,c),(c,a)):
            dx,dy = v[0]-u[0],v[1]-u[1]
            s += xx*dx*dx + 2*xy*dx*dy + yy*dy*dy
        return 2*math.sqrt(3)*cross(a,b,c)*math.sqrt(xx*yy-xy*xy)/s
    def length(edge):
        a,b = (p[i] for i in edge)
        dx,dy = b[0]-a[0],b[1]-a[1]
        samples = []
        for x in (a,((a[0]+b[0])/2,(a[1]+b[1])/2),b):
            xx,xy,yy = field(x)
            samples.append(math.sqrt(xx*dx*dx+2*xy*dx*dy+yy*dy*dy))
        return (samples[0]+4*samples[1]+samples[2])/6
    edges, signs, qualities = {}, [], []
    for e,ids in enumerate(cells):
        xyz = [tuple(Fraction(x) for x in p[i]) for i in ids]
        signs.append(cross(*xyz)>0)
        qualities.append(quality(ids))
        for k in range(3):
            a,b = ids[k],ids[(k+1)%3]
            edges.setdefault(tuple(sorted((a,b))), []).append((e,a,b))
    exposed = {edge for edge, records in edges.items() if len(records)==1}
    physical = {tuple(sorted(edge)) for group in markers.values() for edge in group}
    manifold = all(len(rows)==1 or (len(rows)==2 and rows[0][1:]==rows[1][:0:-1]) for rows in edges.values())
    wall_bases = {tuple(sorted(edge)) for tag,group in markers.items() if tag in wall_tags for edge in group}
    protected = {edges[edge][0][0] for edge in wall_bases}
    apices = {next(i for i in cells[edges[edge][0][0]] if i not in edge) for edge in wall_bases}
    height_errors = []
    for edge in wall_bases:
        ids = cells[edges[edge][0][0]]
        a,b = (p[i] for i in edge)
        altitude = abs(cross(*(p[i] for i in ids)))/math.dist(a,b)
        height_errors.append(abs(altitude/height-1))
    lengths = {edge:length(edge) for edge in edges}
    bad = [{'cell': e, 'quality': q, 'nodes': ids, 'coordinates': [p[i] for i in ids],
            'wall_cell': e in protected, 'wall_apices': sorted(set(ids)&apices)}
           for e,(ids,q) in enumerate(zip(cells,qualities)) if q < .18 or max(lengths[tuple(sorted((ids[k],ids[(k+1)%3])))] for k in range(3)) > 1.8]
    profile = []
    for lower,upper in (((0,1),(1,2),(2,4),(4,float('inf'))) if tuple(wall_tags)==('lower_a','lower_b') else []):
        values = []
        for edge in edges:
            a,b = (p[i] for i in edge)
            distance = (a[1]+b[1])/2/height
            # Project edges inclined at least 45 degrees onto the flat-wall normal.
            # These are spacing distributions, not asserted stacks of geometric rows.
            if lower <= distance < upper and abs(b[1]-a[1]) >= abs(b[0]-a[0]) and a!=b:
                xx,xy,yy = field(((a[0]+b[0])/2,(a[1]+b[1])/2))
                values.append(abs(b[1]-a[1])*math.sqrt(yy))
        values.sort()
        profile.append({'distance_over_h0': [lower, upper if math.isfinite(upper) else None],
                        'inclined_edges':len(values),'normal_metric_projection_quantiles':
                        [values[int((len(values)-1)*q)] for q in (0,.25,.5,.75,1)] if values else []})
    if input_hashes != {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}:
        raise RuntimeError('Audit inputs changed during evaluation')
    return {'donor':str(donor_path),'candidate':str(candidate_path), 'input_files_sha256':input_hashes,
            'points':len(p),'cells':len(cells),
            'all_exact_positive':all(signs),'manifold_oriented_edges':manifold,'physical_equals_exposed':physical==exposed,
            'disk_euler':len(p)-len(edges)+len(cells), 'min_quality':min(qualities),'max_simpson_length':max(lengths.values()),
            'wall_faces':len(wall_bases),'max_relative_height_error':max(height_errors,default=0), 'outside_donor_queries':misses, 'numeric_roundoff_queries':locator.roundoff if locator else None,
            'physical_extension_queries':locator.extensions if locator else None,
            'max_physical_extension':locator.max_extension if locator else None,
            'normal_spacing_profile':profile, 'bad_cells':bad}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory',type=Path)
    parser.add_argument('--cycle',type=int,default=7)
    parser.add_argument('--height',type=float,default=.0045)
    parser.add_argument('--suffix',default='rejected')
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    result = []
    for method in ('barycentric','conservative'):
        prefix = args.directory/f'native_bl_{method}_cycle_{args.cycle}'
        result.append(audit(str(prefix)+'_donor.su2',str(prefix)+'_metric.csv',str(prefix)+'_'+args.suffix+'.su2',args.height))
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    for entry in result:
        print(json.dumps(entry,indent=2))
