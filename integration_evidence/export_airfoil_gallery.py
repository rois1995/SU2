"""Export the independently audited prior native NACA run without modifying it."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import xml.etree.ElementTree as ET
from audit_native_bl import mesh

root = Path(__file__).resolve().parent
source = Path('/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/BL_NATIVE_INTEGRATION_WORK/native_airfoil_wall_metric_guard_runtime_v1')
verification = json.loads((source/'independent_full_v1/evidence.json').read_text())
assert verification['verified'] and verification['terminal'] and len(verification['runs']) == 9
assert all(row['verified'] for row in verification['runs'])
output = root/'real_airfoil_gallery_v1'
output.mkdir()
provenance = {'scope': 'Earlier native baseline, not current integrated-branch runtime evidence; mesh/transfer and short viscous tests, not converged CFD accuracy',
              'source': str(source), 'independent_evidence_sha256': hashlib.sha256((source/'independent_full_v1/evidence.json').read_bytes()).hexdigest(), 'exports': []}
def export(input_path, name):
    points, triangles, markers = mesh(input_path)
    original_hash = hashlib.sha256(input_path.read_bytes()).hexdigest()
    shutil.copy2(input_path, output/(name+'.su2'))
    names = sorted(markers)
    lines = [edge for tag in names for edge in markers[tag]]
    ids = [index+1 for index,tag in enumerate(names) for edge in markers[tag]]
    cells = triangles+lines
    vtk = ET.Element('VTKFile', type='UnstructuredGrid', version='0.1', byte_order='LittleEndian')
    piece = ET.SubElement(ET.SubElement(vtk, 'UnstructuredGrid'), 'Piece', NumberOfPoints=str(len(points)), NumberOfCells=str(len(cells)))
    def array(parent, name, kind, values, components=None):
        attributes = dict(type=kind, Name=name, format='ascii')
        if components: attributes['NumberOfComponents'] = str(components)
        ET.SubElement(parent, 'DataArray', **attributes).text = ' '.join(map(str,values))
    array(ET.SubElement(piece,'Points'), 'Points', 'Float64', (f'{v:.17g}' for x,y in points for v in (x,y,0)), 3)
    node = ET.SubElement(piece,'Cells'); offsets=[]; offset=0
    for cell in cells: offset+=len(cell); offsets.append(offset)
    array(node,'connectivity','Int64',(i for cell in cells for i in cell))
    array(node,'offsets','Int64',offsets); array(node,'types','UInt8',[5]*len(triangles)+[3]*len(lines))
    array(ET.SubElement(piece,'CellData'),'physical_marker','Int32',[0]*len(triangles)+ids)
    path=output/(name+'.vtu');ET.ElementTree(vtk).write(path,encoding='utf-8',xml_declaration=True)
    parsed=ET.parse(path)
    values=list(map(float,parsed.find('.//Points/DataArray').text.split()))
    assert list(zip(values[::3],values[1::3]))==points and all(v==0 for v in values[2::3])
    connectivity=list(map(int,parsed.find('.//Cells/DataArray[@Name="connectivity"]').text.split()))
    assert connectivity==[i for cell in cells for i in cell]
    assert original_hash==hashlib.sha256(input_path.read_bytes()).hexdigest()==hashlib.sha256((output/(name+'.su2')).read_bytes()).hexdigest()
    provenance['exports'].append(dict(name=name,input=str(input_path),input_sha256=original_hash,
        vtu_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),points=len(points),triangles=len(triangles),
        physical_marker={index+1:tag for index,tag in enumerate(names)}))
    return points, triangles, markers
seed=export(source/'airfoil_input.su2','initial')
final=None
for cycle in range(3):
    result=export(source/f'audit_np4/native_airfoil_cycle_{cycle}_adapted.su2',f'np4_cycle{cycle}')
    if cycle==2:final=result
os.environ['MPLCONFIGDIR']=str(output/'mpl_cache')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
fig,axes=plt.subplots(2,2,figsize=(12,8))
def draw(ax,data,bounds,title):
    points,triangles,markers=data
    edges={tuple(sorted((cell[k],cell[(k+1)%3]))) for cell in triangles for k in range(3)}
    xmin,xmax,ymin,ymax=bounds
    segments=[[points[a],points[b]] for a,b in edges if max(points[a][0],points[b][0])>=xmin and min(points[a][0],points[b][0])<=xmax and max(points[a][1],points[b][1])>=ymin and min(points[a][1],points[b][1])<=ymax]
    ax.add_collection(LineCollection(segments,colors='#5c6673',linewidths=.35))
    ax.add_collection(LineCollection([[points[a],points[b]] for a,b in markers['airfoil']],colors='black',linewidths=.8))
    ax.set(xlim=(xmin,xmax),ylim=(ymin,ymax),aspect='equal',title=title,xlabel='x / chord',ylabel='y / chord')
    ax.ticklabel_format(axis='both',style='plain',useOffset=False)
upper=min((p for i in {n for edge in final[2]['airfoil'] for n in edge} for p in [final[0][i]] if p[1]>0),key=lambda p:abs(p[0]-.5))
draw(axes[0,0],seed,(-.08,1.08,-.18,.18),f'Initial: {len(seed[1]):,} triangles')
draw(axes[0,1],final,(-.08,1.08,-.18,.18),f'Native adaptation, 4 ranks: {len(final[1]):,} triangles')
draw(axes[1,0],final,(-.015,.075,-.055,.055),'Adapted leading edge')
draw(axes[1,1],final,(upper[0]-.01,upper[0]+.01,upper[1]-.002,upper[1]+.004),'Adapted upper wall near mid-chord')
fig.suptitle('NACA0012: actual anisotropic triangular mesh and boundary adaptation')
fig.text(.5,.02,'Physical aspect ratios preserved. First wall altitude 0.0002 chord. Earlier native baseline; short viscous/transfer tests passed.',ha='center',fontsize=10)
fig.tight_layout(rect=(0,.045,1,.96));fig.savefig(output/'naca0012_preview.png',dpi=180);fig.savefig(output/'naca0012_preview.svg')
(output/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print(json.dumps({'directory':str(output),'meshes':[{k:row[k] for k in ['name','points','triangles']} for row in provenance['exports']]},indent=2))
