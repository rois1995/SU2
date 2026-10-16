#!/usr/bin/env python3
"""Export original M6 cells22/32 and their vertex neighborhoods; no candidate mesh."""
from pathlib import Path
import json,sys,hashlib,csv
import numpy as np
import xml.etree.ElementTree as ET
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
mesh=read_su2(HERE/'Coarse.su2')
for gid in (22,32):
 vertices=mesh.E[gid];chosen=np.flatnonzero(np.isin(mesh.E,vertices).any(axis=1));nodes=np.unique(mesh.E[chosen]);assert gid in chosen
 vtk=ET.Element('VTKFile',type='UnstructuredGrid',version='0.1',byte_order='LittleEndian');grid=ET.SubElement(vtk,'UnstructuredGrid');part=ET.SubElement(grid,'Piece',NumberOfPoints=str(len(nodes)),NumberOfCells=str(len(chosen)))
 def array(parent,name,values,kind='Float64',components=1):
  item=ET.SubElement(parent,'DataArray',type=kind,Name=name,format='ascii',NumberOfComponents=str(components))
  item.text=' '.join(format(float(x),'.17g') if kind=='Float64' else str(int(x)) for x in np.asarray(values).ravel())
 array(ET.SubElement(part,'Points'),'Points',mesh.P[nodes],components=3)
 array(ET.SubElement(part,'PointData'),'OriginalPointID',nodes,'Int64')
 cells=ET.SubElement(part,'Cells');array(cells,'connectivity',np.searchsorted(nodes,mesh.E[chosen]),'Int64');array(cells,'offsets',4*np.arange(1,len(chosen)+1),'Int64');array(cells,'types',np.full(len(chosen),10),'UInt8')
 data=ET.SubElement(part,'CellData');array(data,'OriginalCellID',chosen,'Int64');array(data,'FailureCell',chosen==gid,'UInt8')
 path=HERE/('failure_cell_'+str(gid)+'_input.vtu');ET.ElementTree(vtk).write(path,encoding='utf-8',xml_declaration=True)
 parsed=ET.parse(path).getroot().find('UnstructuredGrid/Piece');assert int(parsed.attrib['NumberOfCells'])==len(chosen)
 assert sum(map(int,parsed.find("CellData/DataArray[@Name='FailureCell']").text.split()))==1
 assert np.array_equal(np.fromstring(parsed.find("Points/DataArray").text,sep=' ').reshape(-1,3),mesh.P[nodes])
 physical={k:v[np.isin(v,vertices).all(axis=1)].tolist() for k,v in mesh.M.items()};physical={k:v for k,v in physical.items() if v}
 sample=ROOT/'integration_evidence/native_3d_bl_v1/oneram6_quadrature_v1'/('cell'+str(gid)+'_probe/replay_samples.csv')
 with sample.open() as f:centroid_sample=next(row for row in csv.DictReader(f) if int(row['mask'])==15)
 report={'global_cell':gid,'source_point_ids':vertices.tolist(),'coordinates':mesh.P[vertices].tolist(),'centroid':mesh.P[vertices].mean(0).tolist(),'centroid_wall_distance':float(centroid_sample['actual_distance']),'physical_faces':physical,'neighborhood_cells':len(chosen),'scope':'ORIGINAL INPUT neighborhood only; metric integration failed before remeshing. No rejected candidate/adapted mesh. FailureCell flags the reported source element.','sha256':{'input_mesh':hashlib.sha256((HERE/'Coarse.su2').read_bytes()).hexdigest(),'vtu':hashlib.sha256(path.read_bytes()).hexdigest(),'helper':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
 path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print('EXPORTED_INPUT_FAILURE',gid,'centroid',report['centroid'],'cells',len(chosen),flush=True)
