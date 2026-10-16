#!/usr/bin/env python3
"""Export the reported original M6 cell and its vertex neighborhood; no remeshing."""
from pathlib import Path
import sys, json, hashlib
import xml.etree.ElementTree as ET
import numpy as np
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'TestCases/adaptation/capability'))
from capcheck import read_su2

def main():
    case = Path(__file__).resolve().parent
    mesh = read_su2(case / 'Coarse.su2')
    cell = 13
    vertices = mesh.E[cell]
    chosen = np.flatnonzero(np.isin(mesh.E, vertices).any(axis=1))
    nodes = np.unique(mesh.E[chosen])
    coordinates = mesh.P[vertices]
    faces = {k: v[np.isin(v, vertices).all(axis=1)] for k,v in mesh.M.items()}
    assert len(faces['WING3D']) == 1 and cell in chosen
    a,b,c = mesh.P[faces['WING3D'][0]]
    normal = np.cross(b-a,c-a); normal /= np.linalg.norm(normal)
    apex = mesh.P[next(v for v in vertices if v not in faces['WING3D'][0])]
    info = {'global_cell': cell, 'source_element_ordinal_one_based': cell+1,
            'source_numbering': 'CSU2 ASCII element ordinal; CPhysicalGeometry preserves global element IDs across partitioning.',
            'point_ids': vertices.tolist(), 'coordinates': coordinates.tolist(),
            'centroid': coordinates.mean(0).tolist(),
            'bounds': [coordinates.min(0).tolist(), coordinates.max(0).tolist()],
            'physical_faces': {k:v.tolist() for k,v in faces.items() if len(v)},
            'existing_wall_altitude': float(abs((apex-a) @ normal)),
            'requested_wall_altitude': 2e-6,
            'neighborhood_cells': len(chosen),
            'scope': 'Original input cell neighborhood, NOT a rejected remeshing proposal or an adapted grid.'}
    vtk = ET.Element('VTKFile',type='UnstructuredGrid',version='0.1',byte_order='LittleEndian')
    grid = ET.SubElement(vtk,'UnstructuredGrid')
    part = ET.SubElement(grid,'Piece',NumberOfPoints=str(len(nodes)),NumberOfCells=str(len(chosen)))
    def array(parent,name,values,kind='Float64',components=1):
        attr = dict(type=kind,Name=name,format='ascii',NumberOfComponents=str(components))
        item = ET.SubElement(parent,'DataArray',**attr)
        item.text = ' '.join(format(float(x),'.17g') if kind=='Float64' else str(int(x)) for x in np.asarray(values).ravel())
    p = ET.SubElement(part,'Points'); array(p,'Points',mesh.P[nodes],components=3)
    p = ET.SubElement(part,'PointData'); array(p,'OriginalPointID',nodes,'Int64')
    p = ET.SubElement(part,'Cells'); array(p,'connectivity',np.searchsorted(nodes,mesh.E[chosen]),'Int64')
    array(p,'offsets',4*np.arange(1,len(chosen)+1),'Int64'); array(p,'types',np.full(len(chosen),10),'UInt8')
    p = ET.SubElement(part,'CellData'); array(p,'OriginalCellID',chosen,'Int64'); array(p,'FailureCell',chosen==cell,'UInt8')
    path = case/'failure_cell_13.vtu'; ET.ElementTree(vtk).write(path,encoding='utf-8',xml_declaration=True)
    parsed = ET.parse(path).getroot().find('UnstructuredGrid/Piece')
    assert int(parsed.attrib['NumberOfCells']) == len(chosen)
    flagged = parsed.find("CellData/DataArray[@Name='FailureCell']").text.split()
    assert sum(map(int,flagged)) == 1
    info['diagnostic_vtu_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
    (case/'failure_cell_13.json').write_text(json.dumps(info,indent=2)+'\n')
    print(json.dumps(info,indent=2))
if __name__ == '__main__': main()
