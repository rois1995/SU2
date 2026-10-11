"""Export qualified accepted tetrahedra and their actual transported synthetic CFD/BDF states for ParaView."""
from pathlib import Path
import hashlib,json,sys,xml.etree.ElementTree as ET

def main(folder):
    assert json.loads((folder/'first_cycle_audit_v2.json').read_text())['status']=='PASS'
    files=[]
    for cycle in range(1):
        mesh=folder/f'envelope_s{cycle}.json';state=folder/f'envelope_s{cycle}_states.json'
        data=json.loads(mesh.read_text());values=json.loads(state.read_text())
        nodes=sorted(data['nodes']);local={n[0]:i for i,n in enumerate(nodes)}
        flow={tuple(r['point']):r for r in values['accepted']};assert len(flow)==len(nodes)
        rows=[flow[tuple(n[1:])] for n in nodes]
        vtk=ET.Element('VTKFile',type='UnstructuredGrid',version='0.1',byte_order='LittleEndian')
        piece=ET.SubElement(ET.SubElement(vtk,'UnstructuredGrid'),'Piece',NumberOfPoints=str(len(nodes)),NumberOfCells=str(len(data['cells'])))
        def array(parent,name,kind,count,flat):
            node=ET.SubElement(parent,'DataArray',type=kind,Name=name,NumberOfComponents=str(count),format='ascii')
            flat=list(flat);node.text=' '.join(format(x,'.17g') if isinstance(x,float) else str(x) for x in flat)
            return flat
        array(ET.SubElement(piece,'Points'),'Coordinates','Float64',3,(x for n in nodes for x in n[1:]))
        cells=ET.SubElement(piece,'Cells')
        array(cells,'connectivity','Int64',1,(local[k] for c in data['cells'] for k in c['nodes']))
        array(cells,'offsets','Int64',1,(4*(i+1) for i in range(len(data['cells']))))
        array(cells,'types','UInt8',1,(10 for c in data['cells']))
        point=ET.SubElement(piece,'PointData')
        for time,label in enumerate(('current','time_n','time_n1')):
            for k,name in enumerate(('density','momentum_x','momentum_y','momentum_z','energy')):
                array(point,label+'_'+name,'Float64',1,(r['states'][5*time+k] for r in rows))
        array(point,'transported_sensor_upper','Float64',6,(x for r in rows for x in r['metric']))
        cell=ET.SubElement(piece,'CellData');array(cell,'native_cell_id','UInt64',1,(c['id'] for c in data['cells']))
        output=folder/f'envelope_s{cycle}.vtu';ET.ElementTree(vtk).write(output,encoding='utf-8',xml_declaration=True)
        parsed=ET.parse(output).getroot().find('UnstructuredGrid/Piece');assert parsed.attrib==piece.attrib
        for a in parsed.findall('PointData/DataArray'):
            assert len(a.text.split())==len(nodes)*int(a.attrib['NumberOfComponents'])
        assert len(parsed.find('Cells/DataArray[@Name="connectivity"]').text.split())==4*len(data['cells'])
        sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
        files.append(dict(file=output.name,sha256=sha(output),source_mesh_sha256=sha(mesh),source_states_sha256=sha(state),cells=len(data['cells']),points=len(nodes)))
    (folder/'vtu_export.json').write_text(json.dumps(dict(status='PASS',exporter_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),files=files,scope='Inspectable qualified tetrahedra plus actual transferred affine synthetic states/BDF histories and sensor tensors; first accepted event only; later campaign FAIL, no CFD iteration or physical-flow prediction.'),indent=2)+'\n')
    print('PASS',len(files),'VTU mesh/state exports in',folder)
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
