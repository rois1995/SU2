#!/usr/bin/env python3
"""Derived inspection files only; source receipts and native mesh acceptance are unchanged."""
import csv
import hashlib
import json
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path
from construct import write_su2

HERE = Path(__file__).resolve().parent

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    case = HERE / 'native_complete_v1'
    field = case / 'native_field'
    receipt = json.loads((case / 'native_field.json').read_text())
    assert receipt['status'] == 'PASS_NATIVE_FIELD_ONLY'
    for name, expected in receipt['artifacts'].items():
        assert digest(field / name) == expected, name
    data = json.loads((field / 'audited_grid.json').read_text())
    nodes = {n[0]: n[1:] for n in data['nodes']}
    ids = {node: i for i, node in enumerate(nodes)}
    samples = {int(row['cell']): row for row in csv.DictReader((field / 'grid_samples.csv').open())}
    grid = ET.Element('VTKFile', type='UnstructuredGrid', version='0.1', byte_order='LittleEndian')
    piece = ET.SubElement(ET.SubElement(grid, 'UnstructuredGrid'), 'Piece',
                          NumberOfPoints=str(len(nodes)), NumberOfCells=str(len(data['cells'])))
    def array(parent, name, values, kind='Float64', components=1):
        out = ET.SubElement(parent, 'DataArray', type=kind, Name=name,
                            NumberOfComponents=str(components), format='ascii')
        out.text = ' '.join(str(value) for value in values)
    array(ET.SubElement(piece, 'Points'), 'Points', (x for p in nodes.values() for x in p), components=3)
    cells = ET.SubElement(piece, 'Cells')
    array(cells, 'connectivity', (ids[n] for c in data['cells'] for n in c['nodes']), 'Int64')
    array(cells, 'offsets', (4 * (i + 1) for i in range(len(data['cells']))), 'Int64')
    array(cells, 'types', (10 for _ in data['cells']), 'UInt8')
    values = ET.SubElement(piece, 'CellData')
    for key, label in [('q', 'native_mean_ratio'), ('j', 'native_scaled_jacobian')]:
        array(values, label, (float(samples[c['id']][key]) for c in data['cells']))
    output = case / 'accepted_metrics.vtu'
    ET.ElementTree(grid).write(output, encoding='utf-8', xml_declaration=True)
    check = ET.parse(output).getroot().find('UnstructuredGrid/Piece')
    assert int(check.attrib['NumberOfCells']) == 1368
    assert len(check.find("Cells/DataArray[@Name='connectivity']").text.split()) == 4 * 1368
    for rejected in HERE.glob('native_complete_v1_previous_*'):
        if not rejected.is_dir():
            continue
        proposal = json.loads((rejected / 'saved_grid_last_rejected_proposal.json').read_text())
        out = rejected / 'region_proposal_rejected.su2'
        write_su2(out, {n[0]: n[1:] for n in proposal['nodes']},
                  [c['nodes'] for c in proposal['cells']], proposal['surface'],
                  {f['id']: f['marker'] for f in proposal['facets']}, proposal['marker_names'])
        shutil.copyfile(rejected / 'curved_cylinder_initial_euler.su2.native_ref',
                        Path(str(out) + '.native_ref'))
    print(output)

if __name__ == '__main__':
    main()
