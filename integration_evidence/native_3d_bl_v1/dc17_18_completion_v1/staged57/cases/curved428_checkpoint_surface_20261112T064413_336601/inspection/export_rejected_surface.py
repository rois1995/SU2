"""One rejected428 volume visualization; saved nodal witnesses are not a current-field certificate."""
from pathlib import Path
import base64, hashlib, importlib.util, json, os, struct, sys, time
import xml.etree.ElementTree as ET
import numpy as np

HERE = Path(__file__).resolve().parent
CASE = HERE.parent
R = CASE.parent.parent
ROOT = R.parents[3]
sys.path.insert(0, str(R))
import inspect_native_failure3d as vtk_helpers
import assess_target_directions396 as checkpoint

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b''): h.update(chunk)
    return h.hexdigest()

def main():
    start = time.monotonic()
    output = HERE / 'rejected_surface428_nodal_witness.vtu'
    manifest_path = HERE / 'manifest.json'
    assert not output.exists() and not manifest_path.exists(), 'Preserve existing artifacts.'
    inputs = {}
    def read_json(path):
        inputs[str(path)] = sha(path)
        return json.loads(path.read_text())
    terminal = read_json(CASE / 'terminal_handles_actual.json')
    assert terminal['all_owned_handles_terminal'] and not terminal['live_owned_handles']
    status = read_json(CASE / 'status.json')
    assert status['pins_verified'] and not status['changed']
    pins = read_json(CASE / 'pins.json')
    # Full frozen run provenance includes the loaded checkpoint, core, authority, config and binary.
    for name, expected in pins.items():
        path = Path(name)
        if not path.is_absolute(): path = ROOT / path
        assert sha(path) == expected, str(path)
    stage = read_json(CASE / 'actual_bl_surface_stage.json')
    captured = read_json(CASE / 'stage_surface_rank0_manifest.json')
    assert stage['target_policy_unchanged'] and not stage['surface_prepared'] and not stage['final_mesh_accepted']
    assert captured['complete_shard'] and captured['private_capture'] and not captured['final_mesh_accepted']
    p396 = read_json(R / 'build396_source_pins.json')
    p428 = read_json(R / 'build428_source_pins.json')
    unchanged = []
    for suffix in ('SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
                   'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
                   'SU2_CFD/include/adaptation/CNativeBLTarget3D.hpp'):
        name = str(ROOT / suffix)
        assert p396[name] == p428[name] == sha(name)
        unchanged.append(name)
    saved = read_json(R / 'target_checkpoint396_producer.json')
    qualified = read_json(R / 'target_checkpoint396_qualification.json')
    assert qualified['actual_target_checkpoint_qualified'] and qualified['owned_bit_identical'] == 61330
    producer = ROOT / saved['producer']
    original = {}; headers = []
    for rank in range(6):
        path = producer / f'target_rank{rank}.bin'
        rows, header = checkpoint.inspect(path, saved['files'][path.name]['sha256'])
        inputs[str(path)] = saved['files'][path.name]['sha256']
        assert not original.keys() & rows.keys()
        original.update(rows); headers.append(header)
    assert len(original) == 61330 and len({x['reference_sha256'] for x in headers}) == 1
    metadata_path = producer / 'saved_cfd_metric_rank0.jsonl'
    inputs[str(metadata_path)] = sha(metadata_path)
    metadata = json.loads(metadata_path.open().readline())
    assert all(h['core'] == metadata['core'] and h['tangent_scale'] == metadata['tangential_scale'] for h in headers)
    allocation = read_json(producer / 'core_allocation_assessment.json')
    core_path = Path(allocation['core_native_export']['path'])
    if not core_path.is_absolute(): core_path = ROOT / core_path
    assert sha(core_path) == allocation['core_native_export']['sha256']
    inputs[str(core_path)] = sha(core_path)
    core_dtype = np.dtype([('id', '<u8'), ('x', '<f8', (3,)), ('m', '<f8', (9,))])
    core_count = struct.unpack('<Q', core_path.open('rb').read(8))[0]
    core = np.memmap(core_path, core_dtype, 'r', offset=8, shape=(core_count,))
    assert core_count == len(original) and np.array_equal(core['id'], np.arange(core_count))
    node_dtype = np.dtype([('id', '<u8'), ('x', '<f8', (3,))])
    owned_dtype = np.dtype([('format', '<u8'), ('id', '<u8'), ('nodes', node_dtype, (4,)),
        ('version', '<u8'), ('physical', '<u8', (4, 2)), ('present', '<u8'), ('sensor', '<f8', (4, 6))])
    assert owned_dtype.itemsize == vtk_helpers.OWNED_RECORD_BYTES == 416
    owned_path = CASE / captured['files']['owned']
    count = struct.unpack('<Q', owned_path.open('rb').read(8))[0]
    assert count == captured['working_rows'] == stage['tets'] and owned_path.stat().st_size == 8 + 416 * count
    inputs[str(owned_path)] = sha(owned_path)
    owned = np.memmap(owned_path, owned_dtype, 'r', offset=8, shape=(count,))
    assert np.all(owned['format'] == vtk_helpers.OWNED_FORMAT) and np.all(owned['present'] == 1)
    ids = owned['nodes']['id']; node_ids = np.unique(ids)
    assert len(node_ids) == stage['nodes'] and np.all(ids < core_count)
    core9 = core['m'].reshape(core_count, 3, 3)
    core6 = core9[:, [0, 0, 0, 1, 1, 2], [0, 1, 2, 1, 2, 2]]
    assert np.array_equal(owned['nodes']['x'], core['x'][ids])
    assert np.array_equal(owned['sensor'], core6[ids])
    points = core['x'][node_ids].copy(); transported = core9[node_ids].copy()
    witness = np.empty_like(transported)
    for i, node in enumerate(node_ids):
        point, sensor, tensor = original[int(node)]
        assert np.array_equal(points[i], point) and np.array_equal(transported[i], sensor)
        witness[i] = tensor
    assert np.isfinite(points).all()
    for tensors in (transported, witness):
        assert np.isfinite(tensors).all() and np.array_equal(tensors, tensors.transpose(0, 2, 1))
        assert np.all(np.linalg.eigvalsh(tensors) > 0)
    index = np.full(core_count, -1, dtype=np.int64); index[node_ids] = np.arange(len(node_ids))
    connectivity = index[ids]; assert np.all(connectivity >= 0)
    stream = vtk_helpers._CappedText(output, vtk_helpers.MAX_FULL_VTU_BYTES)
    emit = lambda name, kind, values, components=1: vtk_helpers._stream_data_array(stream, name, kind, iter(values), components)
    try:
        stream.write('<?xml version="1.0"?><VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid><FieldData>')
        for name, value in (('visualization_only', 1), ('surface_prepared', 0), ('final_mesh_accepted', 0), ('arbitrary_point_target_reconstructed', 0)):
            emit(name, 'UInt8', [value])
        stream.write(f'</FieldData><Piece NumberOfPoints="{len(node_ids)}" NumberOfCells="{count}"><PointData Tensors="saved_composed_nodal_witness">')
        emit('global_node_id', 'UInt64', node_ids)
        emit('saved_composed_nodal_witness', 'Float64', witness.flat, 9)
        emit('transported_core', 'Float64', transported.flat, 9)
        emit('saved_witness_available', 'UInt8', np.ones(len(node_ids), dtype=np.uint8))
        stream.write('</PointData><CellData>')
        emit('global_cell_id', 'UInt64', owned['id'])
        for name in ('q', 'J', 'geometry_excess_l2'):
            emit(name, 'Float64', (float('nan') for _ in range(count)))
        for name in ('q_known', 'J_known', 'geometry_known'):
            emit(name, 'UInt8', np.zeros(count, dtype=np.uint8))
        stream.write('</CellData><Points>'); emit('Points', 'Float64', points.flat, 3)
        stream.write('</Points><Cells>'); emit('connectivity', 'Int64', connectivity.flat)
        emit('offsets', 'Int64', np.arange(1, count + 1, dtype=np.int64) * 4)
        emit('types', 'UInt8', np.full(count, 10, dtype=np.uint8))
        stream.write('</Cells></Piece></UnstructuredGrid></VTKFile>')
    finally: stream.close()
    # Validate serialized tuple counts, topology, IEEE unknowns and exact tensor/coordinate round trip.
    tree = ET.parse(output); piece = tree.find('./UnstructuredGrid/Piece')
    assert int(piece.attrib['NumberOfPoints']) == len(node_ids) and int(piece.attrib['NumberOfCells']) == count
    arrays = {}
    for section, tuples in (('PointData', len(node_ids)), ('CellData', count), ('Points', len(node_ids)), ('Cells', None)):
        for array in piece.find(section):
            components = int(array.attrib.get('NumberOfComponents', 1))
            if array.attrib['format'] == 'binary':
                raw = base64.b64decode(array.text); size = struct.unpack_from('<I', raw)[0]
                assert size == len(raw) - 4
                values = np.frombuffer(raw, '<f8', offset=4)
            else: values = np.fromstring(array.text or '', dtype=np.int64, sep=' ')
            if tuples is not None: assert len(values) == tuples * components, array.attrib['Name']
            arrays[array.attrib['Name']] = values
    assert np.array_equal(arrays['saved_composed_nodal_witness'], witness.ravel())
    assert np.array_equal(arrays['transported_core'], transported.ravel()) and np.array_equal(arrays['Points'], points.ravel())
    assert np.array_equal(arrays['connectivity'], connectivity.ravel()) and np.array_equal(arrays['offsets'], np.arange(1, count + 1) * 4)
    assert len(arrays['types']) == count and np.all(arrays['types'] == 10)
    assert all(np.isnan(arrays[name]).all() for name in ('q', 'J', 'geometry_excess_l2'))
    assert all(not arrays[name].any() for name in ('q_known', 'J_known', 'geometry_known'))
    inputs[str(Path(vtk_helpers.__file__))] = sha(vtk_helpers.__file__)
    inputs[str(Path(checkpoint.__file__))] = sha(checkpoint.__file__)
    report = dict(scope=__doc__, surface_prepared=False, final_mesh_accepted=False,
        nodes=len(node_ids), tetrahedra=count, exact_coordinate_and_core_join=True,
        source_functions_unchanged_since396=unchanged, target_policy_unchanged=True,
        checkpoint_profile=headers[0], run_pins_verified=len(pins), inputs_sha256=inputs,
        frozen_run_pins=str(CASE/'pins.json'), script_sha256=sha(__file__),
        output=dict(path=str(output), bytes=output.stat().st_size, sha256=sha(output)),
        validation='Serialized XML tuple lengths/topology, binary round trip, finite symmetric positive definite nodal tensors; q/J/geometry IEEE NaN and known=0. No VTK/pvpython installed.',
        limitations=['Rejected private surface-stage output; not an accepted mesh or complete geometry/height/quality census.',
            'Saved original composed nodal witnesses only; CavityMetricSource remains current field authority. No arbitrary-point target reconstruction.',
            'q/J/geometry values are unknown; no new geometry, exact positivity or shape certification performed.'],
        seconds=time.monotonic()-start)
    manifest_path.write_text(json.dumps(report, indent=2, allow_nan=False)+'\n')
    print(json.dumps({key: report[key] for key in ('nodes', 'tetrahedra', 'output', 'seconds')}, indent=2))

if __name__ == '__main__': main()
