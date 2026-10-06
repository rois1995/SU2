"""Export independently audited engine CSV meshes as SU2 and ParaView VTU."""
import argparse, csv, hashlib, json
from collections import defaultdict
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('case',type=Path);p.add_argument('destination',type=Path);a=p.parse_args()
case=a.case.resolve(strict=True);a.destination.mkdir(parents=True,exist_ok=False)
report=json.loads((case.parent/'independent_audit.json').read_text())
row=next(r for r in report['cases'] if Path(r['directory']).resolve()==case)
if not row['structural_pass'] or not row['complete']:raise SystemExit('Export only independently verified complete cases')
for name,expected in row['files_sha256'].items():
 if hashlib.sha256((case/name).read_bytes()).hexdigest()!=expected:raise SystemExit('Raw evidence changed: '+name)
def read(name):
 with (case/name).open() as f:return list(csv.DictReader(f))
points=read('points.csv');cells=read('cells.csv');faces=read('faces.csv')
ids={int(r['id']):i for i,r in enumerate(points)}
coords=[(float(r['x']),float(r['y'])) for r in points]
triangles=[tuple(ids[int(r[k])] for k in ('a','b','c')) for r in cells]
lines=[tuple(ids[int(r[k])] for k in ('a','b')) for r in faces]
markers=defaultdict(list)
for f,line in zip(faces,lines):markers[int(f['marker'])].append(line)
su2=a.destination/'adapted.su2'
with su2.open('x') as f:
 f.write('NDIME= 2\nNELEM= '+str(len(triangles))+'\n')
 for i,t in enumerate(triangles):f.write('5 '+' '.join(map(str,t))+' '+str(i)+'\n')
 f.write('NPOIN= '+str(len(points))+'\n')
 for i,(x,y) in enumerate(coords):f.write(f'{x:.17g} {y:.17g} {i}\n')
 f.write('NMARK= '+str(len(markers))+'\n')
 for marker,edges in sorted(markers.items()):
  tag={10:'bottom',11:'sides',12:'top'}.get(marker,'marker_'+str(marker))
  f.write(f'MARKER_TAG= {tag}\nMARKER_ELEMS= {len(edges)}\n')
  for edge in edges:f.write('3 '+' '.join(map(str,edge))+'\n')
def array(name,kind,values,components=None):
 extra=f' NumberOfComponents="{components}"' if components else ''
 return f'<DataArray type="{kind}" Name="{name}" format="ascii"{extra}>'+' '.join(map(str,values))+'</DataArray>\n'
connectivity=[v for cell in triangles+lines for v in cell]
offsets=[3*(i+1) for i in range(len(triangles))]+[3*len(triangles)+2*(i+1) for i in range(len(lines))]
vtu=a.destination/'adapted.vtu'
with vtu.open('x') as f:
 f.write('<?xml version="1.0"?>\n<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid>\n')
 f.write(f'<Piece NumberOfPoints="{len(points)}" NumberOfCells="{len(triangles)+len(lines)}">\n<Points>\n')
 f.write(array('Points','Float64',(f'{v:.17g}' for x,y in coords for v in (x,y,0)),3))
 f.write('</Points><Cells>\n'+array('connectivity','Int64',connectivity)+array('offsets','Int64',offsets)+array('types','UInt8',[5]*len(triangles)+[3]*len(lines))+'</Cells>\n')
 f.write('<PointData>\n'+array('native_point_id','UInt64',(r['id'] for r in points))+'</PointData>\n<CellData>\n')
 f.write(array('physical_marker','Int32',[0]*len(triangles)+[int(r['marker']) for r in faces]))
 f.write(array('is_boundary','UInt8',[0]*len(triangles)+[1]*len(lines)))
 f.write('</CellData></Piece></UnstructuredGrid></VTKFile>\n')
(a.destination/'provenance.json').write_text(json.dumps({'source_case':str(case),'audit':row,'exports':{x.name:hashlib.sha256(x.read_bytes()).hexdigest() for x in (su2,vtu)}},indent=2)+'\n')
print(a.destination)
