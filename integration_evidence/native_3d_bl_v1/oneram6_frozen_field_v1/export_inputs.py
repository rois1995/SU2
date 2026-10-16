#!/usr/bin/env python3
"""Inspectable original-cell neighborhoods with the captured sensor-only field; no adapted grid."""
import hashlib,json,sys
from pathlib import Path
import xml.etree.ElementTree as ET
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
mesh=read_su2(HERE/'Coarse.su2');field=np.load(HERE/'frozen_trial.npz',allow_pickle=False)
audit=np.load(HERE/'sensor_mass_audit.npz',allow_pickle=False)
for cell in (88,157,329):
 chosen=np.flatnonzero(np.isin(mesh.E,mesh.E[cell]).any(axis=1));nodes=np.unique(mesh.E[chosen]);assert cell in chosen
 root=ET.Element('VTKFile',type='UnstructuredGrid',version='0.1',byte_order='LittleEndian')
 piece=ET.SubElement(ET.SubElement(root,'UnstructuredGrid'),'Piece',NumberOfPoints=str(len(nodes)),NumberOfCells=str(len(chosen)))
 def array(parent,name,values,kind='Float64',components=1):
  out=ET.SubElement(parent,'DataArray',type=kind,Name=name,format='ascii',NumberOfComponents=str(components))
  out.text=' '.join(format(float(x),'.17g') if kind=='Float64' else str(int(x)) for x in np.asarray(values).ravel())
 array(ET.SubElement(piece,'Points'),'Points',mesh.P[nodes],components=3)
 p=ET.SubElement(piece,'PointData');array(p,'OriginalPointID',nodes,'Int64');array(p,'PointOwnerRank',field['node_owner'][nodes],'UInt8')
 values=field['sensor_owner'][nodes];array(p,'OwnerSensorOnlyTrial1',values,components=9)
 eigen=np.linalg.eigvalsh(values);assert np.all(eigen>0)
 array(p,'SensorMinimumSize',1/np.sqrt(eigen[:,-1]));array(p,'SensorMaximumSize',1/np.sqrt(eigen[:,0]))
 cells=ET.SubElement(piece,'Cells');array(cells,'connectivity',np.searchsorted(nodes,mesh.E[chosen]),'Int64')
 array(cells,'offsets',4*np.arange(1,len(chosen)+1),'Int64');array(cells,'types',np.full(len(chosen),10),'UInt8')
 c=ET.SubElement(piece,'CellData');array(c,'OriginalCellID',chosen,'Int64');array(c,'SelectedFailureCell',chosen==cell,'UInt8')
 array(c,'CellOwnerRank',field['cell_owner'][chosen],'UInt8')
 for key in ('lower','upper','q18'):array(c,'SensorMass_'+key,audit[key][chosen])
 array(c,'WallBoxPossible',audit['wall_box_possible'][chosen],'UInt8')
 path=HERE/('frozen_cell%d_sensor_input.vtu'%cell);ET.ElementTree(root).write(path,encoding='utf-8',xml_declaration=True)
 parsed=ET.parse(path).getroot().find('UnstructuredGrid/Piece')
 assert np.array_equal(np.fromstring(parsed.find('Points/DataArray').text,sep=' ').reshape(-1,3),mesh.P[nodes])
 assert np.array_equal(np.fromstring(parsed.find("PointData/DataArray[@Name='OwnerSensorOnlyTrial1']").text,sep=' ').reshape(-1,3,3),values)
 assert sum(map(int,parsed.find("CellData/DataArray[@Name='SelectedFailureCell']").text.split()))==1
 report={'cell':cell,'points':len(nodes),'cells':len(chosen),'scope':'Original input neighborhood only, first-trial sensor-only owner matrices. Not composed BL, final normalized target, rejected candidate or adapted grid. Exact host-rank halo coefficients remain in frozen_trial.npz; owner/halo roundoff is separately audited.',
         'sha256':{'checkpoint':hashlib.sha256((HERE/'frozen_trial.npz').read_bytes()).hexdigest(),'vtu':hashlib.sha256(path.read_bytes()).hexdigest(),'helper':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
 path.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print('FROZEN_INPUT_EXPORTED',cell,len(chosen),flush=True)
