#!/usr/bin/env python3
"""WP-E test-only tool: export a saved native rejected state (SaveRecords JSON) to an inspectable SU2 mesh.
Diagnostic output only; never an accepted adapted mesh. Usage: export_rejected_su2.py <state.json> <out.su2>"""
import hashlib, json, sys
from pathlib import Path

def main(src, dst):
    d = json.loads(Path(src).read_text())
    ids = sorted(n[0] for n in d['nodes'])
    index = {n: i for i, n in enumerate(ids)}
    coords = {n[0]: n[1:4] for n in d['nodes']}
    names = d['marker_names']
    marker = {f['id']: f['marker'] for f in d['facets']}
    out = ['NDIME= 3', f"NELEM= {len(d['cells'])}"]
    for i, c in enumerate(d['cells']):
        out.append('10 ' + ' '.join(str(index[n]) for n in c['nodes']) + f' {i}')
    out.append(f'NPOIN= {len(ids)}')
    for i, n in enumerate(ids):
        x = coords[n]
        out.append(f'{float(x[0])!r} {float(x[1])!r} {float(x[2])!r} {i}')
    groups = {}
    for s in d['surface']:
        groups.setdefault(names[marker[s['facet']]], []).append(s['nodes'])
    out.append(f'NMARK= {len(groups)}')
    for name in sorted(groups):
        out += [f'MARKER_TAG= {name}', f'MARKER_ELEMS= {len(groups[name])}']
        out += ['5 ' + ' '.join(str(index[n]) for n in f) for f in groups[name]]
    Path(dst).write_text('\n'.join(out) + '\n')
    meta = {'source': str(src), 'source_sha256': hashlib.sha256(Path(src).read_bytes()).hexdigest(),
            'output_sha256': hashlib.sha256(Path(dst).read_bytes()).hexdigest(), 'cells': len(d['cells']),
            'points': len(ids), 'markers': {k: len(v) for k, v in groups.items()},
            'scope': 'rejected private state for inspection; not an accepted mesh'}
    Path(str(dst) + '.json').write_text(json.dumps(meta, indent=1) + '\n')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
