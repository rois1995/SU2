#!/usr/bin/env python3
"""Export an independently audited offline witness; this is not an accepted native event."""
import hashlib,json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    source=HERE/'candidate_v4_0.0625.json'
    audit=json.loads((HERE/'independent_full_mesh.json').read_text())
    assert audit['status']=='INDEPENDENT_COMPLETE_FROZEN_MESH_PASS_OFFLINE_PROTOTYPE'
    assert sha(source)==audit['candidate_sha256']
    assert sha(HERE/'probe_v4.py')==audit['constructor_sha256']
    data=json.loads(source.read_text());nodes=data['nodes'];ids={n[0]:i for i,n in enumerate(nodes)}
    assert len(ids)==len(nodes)
    facets={f['id']:f['marker'] for f in data['facets']}
    markers={name:[] for name in data['marker_names']}
    for face in data['surface']:
        markers[data['marker_names'][facets[face['facet']]]].append([ids[n] for n in face['nodes']])
    cells=[[ids[n] for n in c['nodes']] for c in data['cells']]
    lines=['% Offline prototype witness; NOT an accepted native adaptation or restart.','NDIME= 3','NELEM= '+str(len(cells))]
    lines+=['10 '+' '.join(map(str,c))+' '+str(i) for i,c in enumerate(cells)]
    lines+=['NPOIN= '+str(len(nodes))]+[' '.join(format(x,'.17g') for x in n[1:])+' '+str(i) for i,n in enumerate(nodes)]
    lines+=['NMARK= '+str(len(markers))]
    for name,faces in markers.items():
        lines+=['MARKER_TAG= '+name,'MARKER_ELEMS= '+str(len(faces))]+['5 '+' '.join(map(str,f)) for f in faces]
    output=HERE/'annulus_frozen_prototype_witness.su2';output.write_text('\n'.join(lines)+'\n')
    mesh=read_su2(output)
    assert mesh.dim==3 and np.array_equal(mesh.P,np.array([n[1:] for n in nodes])) and np.array_equal(mesh.E,cells)
    assert set(mesh.M)==set(markers) and all(np.array_equal(mesh.M[k],v) for k,v in markers.items())
    receipt=dict(status='AUDITED_OFFLINE_WITNESS_EXPORTED_ROUNDTRIP_PASS',accepted_native_event=False,
        cells=len(cells),nodes=len(nodes),markers={k:len(v) for k,v in markers.items()},
        sha256={p.name:sha(p) for p in (source,HERE/'independent_full_mesh.json',Path(__file__),output)},
        scope='Inspectable complete geometry only; no native integration, solution transfer, restart or MPI qualification')
    (HERE/'export.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
if __name__=='__main__':main()
