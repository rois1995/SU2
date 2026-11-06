"""Diagnostic original-wing surface with flow and actual/candidate metrics."""
import base64
import hashlib
import json
import struct
import xml.etree.ElementTree as ET
import numpy as np
import metric_floor_diagnostic as d

mesh=d.capcheck.read_su2(d.RUN/'m6.su2')
P,f,_=d.capcheck.read_restart(d.RUN/'restart_flow_adap_00000.dat')
T=mesh.M['WING3D'];ids=np.unique(T);triangles=np.searchsorted(ids,T)
old=np.zeros((len(P),3,3))
for a,b,k in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:
    old[:,a,b]=old[:,b,a]=f['Metric_'+k]
new=np.fromfile(d.OUT/'local_graded_scale04236855.bin',dtype='<f8').reshape(-1,3,3)
V=P[T];u=V[:,1]-V[:,0];v=V[:,2]-V[:,0];G=np.stack([u,v],axis=1)
s=f['Mach'][T]
gradient=np.einsum('ni,nij->nj',np.linalg.solve(G@G.transpose(0,2,1),(s[:,1:]-s[:,0,None])[...,None])[...,0],G)
length=np.linalg.norm(gradient,axis=1)
direction=gradient/np.maximum(length[:,None],1e-300)
normal=np.cross(u,v);normal/=np.linalg.norm(normal,axis=1)[:,None]
b=u/np.linalg.norm(u,axis=1)[:,None];basis=np.stack([b,np.cross(normal,b)],axis=1)
root=ET.Element('VTKFile',type='UnstructuredGrid',version='1.0',byte_order='LittleEndian',header_type='UInt64')
grid=ET.SubElement(root,'UnstructuredGrid');field=ET.SubElement(grid,'FieldData')
piece=ET.SubElement(grid,'Piece',NumberOfPoints=str(len(ids)),NumberOfCells=str(len(T)))
checks=[]
def array(parent,name,value,kind='Float64',components=1):
    dtype={'Float64':'<f8','Int64':'<i8','UInt8':'u1'}[kind]
    a=np.ascontiguousarray(value,dtype=dtype)
    assert np.isfinite(a).all()
    node=ET.SubElement(parent,'DataArray',Name=name,type=kind,NumberOfComponents=str(components),format='binary')
    raw=a.tobytes();node.text=base64.b64encode(struct.pack('<Q',len(raw))+raw).decode()
    checks.append((a,dtype))
array(field,'unqualified_metric_candidate',np.array([1]),'UInt8')
pd=ET.SubElement(piece,'PointData',Scalars='Mach')
for name in ['Mach','Pressure','Pressure_Coefficient']:
    array(pd,name,f[name][ids])
for name,M in [('actual',old),('candidate',new)]:
    eig=np.linalg.eigvalsh(M[ids]);assert (eig>0).all()
    array(pd,name+'_metric',M[ids],components=9)
    for k,label in [(2,'h_min'),(1,'h_mid'),(0,'h_max')]:
        array(pd,name+'_'+label,1/np.sqrt(eig[:,k]))
cd=ET.SubElement(piece,'CellData')
array(cd,'Mach_surface_gradient_magnitude',length)
for name,M in [('actual',old),('candidate',new)]:
    A=M[T].mean(axis=1)
    array(cd,name+'_h_across_Mach_gradient',1/np.sqrt(np.einsum('ni,nij,nj->n',direction,A,direction)))
    array(cd,name+'_h_normal',1/np.sqrt(np.einsum('ni,nij,nj->n',normal,A,normal)))
    vals=np.linalg.eigvalsh(basis@A@basis.transpose(0,2,1))
    array(cd,name+'_h_surface_min',1/np.sqrt(vals[:,-1]))
    array(cd,name+'_h_surface_max',1/np.sqrt(vals[:,0]))
array(ET.SubElement(piece,'Points'),'Points',P[ids],components=3)
cells=ET.SubElement(piece,'Cells')
array(cells,'connectivity',triangles,'Int64')
array(cells,'offsets',np.arange(1,len(T)+1)*3,'Int64')
array(cells,'types',np.full(len(T),5),'UInt8')
path=d.OUT/'M6_original_wing_metric_comparison.vtu'
ET.ElementTree(root).write(path,encoding='utf-8',xml_declaration=True)
parsed=ET.parse(path);nodes=parsed.findall('.//DataArray');assert len(nodes)==len(checks)
for node,(expected,dtype) in zip(nodes,checks):
    raw=base64.b64decode(node.text,validate=True);assert struct.unpack_from('<Q',raw)[0]==len(raw)-8
    actual=np.frombuffer(raw,dtype=dtype,offset=8).reshape(expected.shape)
    assert np.array_equal(actual,expected),node.get('Name')
report=dict(scope='Original-grid wing surface and flow; candidate target from isolated serial diagnostic; NOT an adapted or accepted mesh',points=len(ids),triangles=len(T),all_arrays_roundtrip_exact=True,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),bytes=path.stat().st_size)
(d.OUT/'surface_comparison_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
