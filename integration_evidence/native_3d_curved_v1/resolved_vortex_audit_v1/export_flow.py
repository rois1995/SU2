"""Export actual CFD donor/accepted fields; retain native result status separately."""
from pathlib import Path
import json,math,hashlib,sys
HERE=Path(__file__).resolve().parent;case=HERE.parent/sys.argv[1]
report={}
for work in sorted(case.glob('mpi*')):
 for meshfile in sorted(work.glob('resolved_vortex_m*_s*_initial.json'))+sorted(work.glob('resolved_vortex_m*_s*_accepted.json')):
  name=meshfile.stem.rsplit('_',1)[0];accepted=meshfile.stem.endswith('_accepted')
  statefile=work/(name+'_states.json' if accepted else name+'_evolved_states.json')
  if not statefile.exists():continue
  mesh=json.loads(meshfile.read_text());states=json.loads(statefile.read_text())['accepted']
  physics=json.loads((work/(name.rsplit('_s',1)[0]+'_physics.json')).read_text())
  byid={r['id']:r for r in states};ids=[n[0] for n in mesh['nodes']];index={n:i for i,n in enumerate(ids)}
  assert set(ids)==set(byid),'Geometry and actual flow ownership differ'
  values={'Density':[],'Pressure':[],'PressureRelative':[],'Mach':[]}
  for n in mesh['nodes']:
   r=byid[n[0]];assert r['point']==n[1:]
   rho,mx,my,mz,energy=r['states'][:5];pressure=(physics['gamma']-1)*(energy-(mx*mx+my*my+mz*mz)/(2*rho));assert rho>0 and pressure>0
   values['Density'].append(rho);values['Pressure'].append(pressure);values['PressureRelative'].append(pressure/physics['p0']);values['Mach'].append(math.sqrt((mx*mx+my*my+mz*mz)/(rho*physics['gamma']*pressure)))
  dest=HERE/work.name;dest.mkdir(exist_ok=True);target=dest/(meshfile.stem+'_flow.vtu')
  out=['<?xml version="1.0"?>','<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid>',f'<Piece NumberOfPoints="{len(ids)}" NumberOfCells="{len(mesh["cells"])}">','<Points><DataArray type="Float64" NumberOfComponents="3" format="ascii">',' '.join(format(x,'.17g') for n in mesh['nodes'] for x in n[1:]),'</DataArray></Points><Cells>']
  for name0,type0,data in [('connectivity','Int64',[index[n] for c in mesh['cells'] for n in c['nodes']]),('offsets','Int64',[4*(i+1) for i in range(len(mesh['cells']))]),('types','UInt8',[10]*len(mesh['cells']))]:out.extend([f'<DataArray type="{type0}" Name="{name0}" format="ascii">',' '.join(map(str,data)),'</DataArray>'])
  out+=['</Cells><PointData Scalars="PressureRelative">']
  for name0,data in values.items():out.extend([f'<DataArray type="Float64" Name="{name0}" format="ascii">',' '.join(format(x,'.17g') for x in data),'</DataArray>'])
  out+=['</PointData></Piece></UnstructuredGrid></VTKFile>'];target.write_text('\n'.join(out)+'\n')
  report[str(meshfile.relative_to(case))]={'export':str(target.relative_to(HERE)),'mesh_sha256':hashlib.sha256(meshfile.read_bytes()).hexdigest(),'state_sha256':hashlib.sha256(statefile.read_bytes()).hexdigest(),'export_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'scope':'Actual CFD flow on its matching donor or accepted mesh; accepted status must be checked in the native runtime receipt.'}
(HERE/'flow_exports.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'exports':len(report),'folder':str(HERE)},indent=2))
