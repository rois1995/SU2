"""Materialize tuple-selected actual original-M6 cavities; preserve all source payloads."""
from pathlib import Path
import hashlib,json,re,sys
R=Path(__file__).resolve().parent;repo=R.parents[3]
sys.path.insert(0,str(repo/'TestCases/adaptation/capability'));import capcheck
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
source=R/'m6_bl_inputs/rans_curved303_v1';manifest=R/'m6_bl_inputs/chord331_from330_v1/actual_contexts.json'
x=json.loads(manifest.read_text())
for name,h in x['pins'].items():assert sha(name)==h,name
mesh=capcheck.read_su2(source/'m6.su2');rows=[str(len(x['contexts']))]
for c in x['contexts']:
    a,b=c['edge'];ids=c['complete_endpoint_star_cell_ordinals'];assert 0<len(ids)<=64
    truth=[i for i,t in enumerate(mesh.E) if a in t or b in t];assert ids==truth
    rows.append(f"{c['label']} {a} {b} {len(ids)}")
    rows.extend(' '.join(str(int(n)) for n in mesh.E[i]) for i in ids)
out=R/'m6_bl_inputs/surface_cavities335_v1';out.mkdir(exist_ok=False)
(out/'contexts.dat').write_text('\n'.join(rows)+'\n')
cfg=(source/'run.cfg').read_text();cfg,n=re.subn(r'^MESH_FILENAME\s*=.*$',f"MESH_FILENAME= {(source/'m6.su2').resolve()}",cfg,flags=re.M);assert n==1
(out/'run.cfg').write_text(cfg)
pins={str(p.resolve()):sha(p) for p in (manifest,source/'m6.su2',source/'m6.su2.native_ref',source/'run.cfg',Path(__file__),repo/'TestCases/adaptation/capability/capcheck.py',out/'contexts.dat',out/'run.cfg')}
(out/'provenance.json').write_text(json.dumps(dict(scope='Exact original geometry and connectivity; no exact same303 sensor snapshot available. Production metric certification remains unproven.',input_mesh_points=len(mesh.P),input_mesh_cells=len(mesh.E),contexts=len(x['contexts']),pins=pins),indent=2)+'\n')
print(out)
