"""Serialize four original parent faces for the existing NativeM6GeometryQueries3D v2 reader.

Preparation only: this does not run the C++ query helper or imply that the faces pass.
"""
from pathlib import Path
import hashlib
import json
import math
import struct

HERE = Path(__file__).resolve().parent
INVENTORY = HERE / 'residual471_inventory.json'
OUT = HERE / 'residual472_query_replays'
BINARY = OUT / 'old_parent_faces_v2.bin'
PROVENANCE = OUT / 'old_parent_faces_v2.provenance.json'
HEADER = struct.Struct('<QQQQdQ')
FACE = struct.Struct('<QQQQ')
POINT = struct.Struct('<QdddQQQdd')
MAGIC = 0x4d36515545525931
VERSION = 2
GEOMETRY_VERSION = 18
TOLERANCE = 1e-4
QUOTA = 396836624
TARGETS = (
    ('current_rank4_geometry', 'target_edge_star', 215614),
    ('current_rank4_geometry', 'target_edge_star', 222691),
    ('current_rank5_clearance', 'target_vertex_star', 275002),
    ('current_rank5_clearance', 'target_vertex_star', 297126),
)
EXPECTED_FACES = {
    215614: (29524, (18153, 18152, 18151)),
    222691: (36051, (18153, 18162, 18152)),
    275002: (10640, (8012, 8011, 8010)),
    297126: (16718, (8010, 8031, 8012)),
}


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def main():
    assert HEADER.size == 48 and FACE.size == 32 and POINT.size == 72
    inventory = json.loads(INVENTORY.read_text())
    assert inventory['schema'] == 'residual471_inventory_v1'
    assert inventory['aggregate_owned_rows'] == 303757
    pins_file = Path(inventory['pins_file'])
    assert sha(pins_file) == inventory['pins_sha256']
    pins = json.loads(pins_file.read_text())
    assert len(pins) == 1740
    for row in inventory['input_shards']:
        owned = Path(row['owned_path'])
        assert owned.stat().st_size == row['bytes']
        assert sha(owned) == row['sha256'] == row['assessment_payload_sha256']
    assert sha(Path(inventory['inventory_source'])) == inventory['inventory_source_sha256']

    selected = []
    for context, star_key, cell_id in TARGETS:
        star = inventory[star_key]
        assert star['cell_count'] <= 64
        if star_key == 'target_edge_star':
            assert star['within_64_cell_bound']
        cell = next((x for x in star['cells'] if x['cell_id'] == cell_id), None)
        assert cell is not None and cell['version'] == 0
        assert len(cell['physical_faces']) == 1
        physical = cell['physical_faces'][0]
        node_by_id = {n['id']: n for n in cell['nodes']}
        ids = tuple(physical['nodes'])
        assert (physical['facet'], ids) == EXPECTED_FACES[cell_id]
        assert len(ids) == 3 and len(set(ids)) == 3 and all(i in node_by_id for i in ids)
        points = []
        for node_id in ids:
            node = node_by_id[node_id]
            xyz = tuple(float(x) for x in node['xyz'])
            assert len(xyz) == 3 and all(math.isfinite(x) for x in xyz)
            # Query bind format: original mesh vertex, dimension 0, feature=node ID, version 18, u=v=0.
            points.append({'id': node_id, 'xyz': xyz, 'feature': node_id,
                           'version': GEOMETRY_VERSION, 'dimension': 0, 'u': 0.0, 'v': 0.0})
        selected.append({'context': context, 'star': star_key, 'cell': cell_id,
                         'rank': cell['rank'], 'facet': physical['facet'],
                         'local_face': physical['local_face'], 'nodes': points})
    assert [(x['context'], x['cell']) for x in selected] == [
        ('current_rank4_geometry', 215614), ('current_rank4_geometry', 222691),
        ('current_rank5_clearance', 275002), ('current_rank5_clearance', 297126)]
    assert len({x['facet'] for x in selected}) == 4

    payload = bytearray(HEADER.pack(MAGIC, VERSION, GEOMETRY_VERSION, len(selected), TOLERANCE, QUOTA))
    for row in selected:
        ids = [n['id'] for n in row['nodes']]
        payload.extend(FACE.pack(*ids, row['facet']))
        for n in row['nodes']:
            payload.extend(POINT.pack(n['id'], *n['xyz'], n['feature'], n['version'],
                                      n['dimension'], n['u'], n['v']))
        payload.extend(struct.pack('<Q', 0))  # Ask the C++ reader to discover support.
    assert len(payload) == HEADER.size + len(selected) * 256 == 1072
    magic, version, geom, count, tol, quota = HEADER.unpack_from(payload)
    assert (magic, version, geom, count, tol, quota) == (MAGIC, VERSION, GEOMETRY_VERSION, 4, TOLERANCE, QUOTA)
    cursor = HEADER.size
    for row in selected:
        assert FACE.unpack_from(payload, cursor) == (*[n['id'] for n in row['nodes']], row['facet'])
        cursor += FACE.size
        for n in row['nodes']:
            got = POINT.unpack_from(payload, cursor)
            expected = (n['id'], *n['xyz'], n['feature'], n['version'], n['dimension'], n['u'], n['v'])
            assert got == expected
            cursor += POINT.size
        assert struct.unpack_from('<Q', payload, cursor)[0] == 0
        cursor += 8
    assert cursor == len(payload)

    OUT.mkdir(parents=True, exist_ok=True)
    BINARY.write_bytes(payload)
    inputs = {
        str(INVENTORY): {'bytes': INVENTORY.stat().st_size, 'sha256': sha(INVENTORY)},
        str(pins_file): {'bytes': pins_file.stat().st_size, 'sha256': sha(pins_file)},
        str(Path(inventory['inventory_source'])): {'bytes': Path(inventory['inventory_source']).stat().st_size,
                                                     'sha256': sha(Path(inventory['inventory_source']))},
    }
    for row in inventory['input_shards']:
        p = Path(row['owned_path'])
        inputs[str(p)] = {'bytes': p.stat().st_size, 'sha256': sha(p)}
    provenance = {
        'schema': 'residual472_parent_face_query_v2_preparation',
        'scope': 'Four original parent physical faces only; prepared for the existing C++ v2 reader. C++ must verify acceptance; no query was run here.',
        'serializer_reference': 'prepare_geometry225_queries.py: HEADER <QQQQdQ, FACE <QQQQ, POINT <QdddQQQdd; each face followed by support_count=0.',
        'magic': hex(MAGIC), 'version': VERSION, 'geometry_version': GEOMETRY_VERSION,
        'strict_tolerance': TOLERANCE, 'quota': QUOTA, 'face_count': len(selected),
        'binary': {'path': str(BINARY), 'bytes': len(payload), 'sha256': sha(BINARY)},
        'preparation_script': {'path': str(Path(__file__).resolve()), 'sha256': sha(Path(__file__).resolve())},
        'inputs': inputs,
        'faces': selected,
        'assertions': {'input_shards_match_inventory': True, 'native_ids_and_xyz_from_original_inventory': True,
                       'binding_dimension_zero_feature_node_id_version_18_uv_zero': True,
                       'four_faces_exactly_rank4_cells_215614_222691_and_rank5_cells_275002_297126': True,
                       'binary_roundtrip_and_size': True, 'support_count_zero_for_native_discovery': True},
        'limits': ['Input acceptance, facet binding and query outcome remain for the existing C++ helper.',
                   'No publication, geometry closure, or gate change is claimed.']
    }
    PROVENANCE.write_text(json.dumps(provenance, indent=2) + '\n')
    print(json.dumps({'binary': str(BINARY), 'bytes': len(payload), 'sha256': sha(BINARY),
                      'provenance': str(PROVENANCE), 'faces': [(x['context'], x['cell'], x['facet']) for x in selected]}))


if __name__ == '__main__':
    main()
