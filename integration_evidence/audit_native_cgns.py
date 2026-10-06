"""Independent small-fixture CGNS/HDF5 mesh topology and coordinate audit."""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import h5py
import numpy as np


def audit(path):
    with h5py.File(path) as f:
        zone = f['Base/Zone']
        assert zone.attrs['label'] == b'Zone_t'
        xy = np.column_stack([zone[f'GridCoordinates/Coordinate{axis}/ data'][()].ravel() for axis in ('X','Y')])
        assert np.isfinite(xy).all()
        size = zone[' data'][()].ravel()
        cells, markers = [], {}
        for name,obj in zone.items():
            if isinstance(obj,h5py.Group) and obj.attrs.get('label') == b'Elements_t':
                code = int(obj[' data'][()].ravel()[0])
                ids = obj['ElementConnectivity/ data'][()].ravel().astype(np.int64)-1
                assert len(ids) > 0 and ids.min() >= 0 and ids.max() < len(xy)
                if code == 5:  # CGNS TRI_3
                    cells.extend(map(tuple,ids.reshape(-1,3).tolist()))
                elif code == 3:  # CGNS BAR_2
                    markers[name] = list(map(tuple,ids.reshape(-1,2).tolist()))
                else:
                    raise ValueError(f'Unexpected element type {code} in {name}')
        assert len(xy) == int(size[0]) and len(cells) == int(size[1])
        edges, positive = {}, True
        for e,cell in enumerate(cells):
            a,b,c = [tuple(Fraction(float(v)) for v in xy[i]) for i in cell]
            positive &= (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]) > 0
            for k in range(3):
                a,b = cell[k],cell[(k+1)%3]
                edges.setdefault(tuple(sorted((a,b))),[]).append((a,b))
        assert positive
        assert all(len(rows)==1 or (len(rows)==2 and rows[0]==rows[1][::-1]) for rows in edges.values())
        exposed = {key for key,rows in edges.items() if len(rows)==1}
        physical = {tuple(sorted(edge)) for group in markers.values() for edge in group}
        assert physical == exposed
        assert set(markers) == {'left','right','upper','lower_a','lower_b'}
        assert set(zone['ZoneBC'].keys()) == set(markers)
        assert len(xy)-len(edges)+len(cells) == 1
        assert len(set(map(tuple,xy.tolist()))) == len(xy)
        assert np.max(np.abs(xy.min(axis=0)-[0,0])) < 1e-14
        upper = [.04,.02] if path.name.startswith('native_produced_bl') else [2,1]
        assert np.max(np.abs(xy.max(axis=0)-upper)) < 1e-14
        return {'file':str(path),'points':len(xy),'cells':len(cells), 'physical_faces':{k:len(v) for k,v in markers.items()},
                'finite_unique_coordinates':True,'all_exact_positive':True,'oriented_manifold_edges':True,
                'physical_equals_exposed':True,'disk_euler':1,'coordinate_bounds':[[0,0],upper],
                'target_metric_audited':False}

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('directory',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    entries=[audit(path) for path in sorted(args.directory.glob('audit_np*/*.cgns'))]
    assert entries, 'No saved CGNS artifacts'
    args.output.write_text(json.dumps(entries,indent=2)+'\n')
    print(f'PASS: {len(entries)} independently audited CGNS meshes (geometry/topology/marker scope).')
