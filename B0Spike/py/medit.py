"""Minimal Medit .mesh/.sol reader (debugging of single MMG calls)."""
import numpy as np
def read_mesh(p):
    tok = open(p).read().split()
    out = {}; i = 0
    while i < len(tok):
        k = tok[i]
        if k in ('Vertices', 'Tetrahedra', 'Triangles', 'Edges', 'RequiredTriangles', 'RequiredVertices', 'Corners', 'Ridges', 'RequiredEdges'):
            n = int(tok[i + 1]); w = {'Vertices': None, 'Tetrahedra': 5, 'Triangles': 4, 'Edges': 3}.get(k, 1)
            if k == 'Vertices':
                w = 4 if out.get('dim', 3) == 3 else 3
            out[k] = np.array(tok[i + 2:i + 2 + n * w], float).reshape(n, w); i += 2 + n * w
        elif k == 'Dimension':
            out['dim'] = int(tok[i + 1]); i += 2
        else:
            i += 1
    return out
def read_sol(p, n, dim=3):
    tok = open(p).read().split()
    i = tok.index('SolAtVertices'); m = dim * (dim + 1) // 2
    return np.array(tok[i + 4:i + 4 + n * m], float).reshape(n, m)
