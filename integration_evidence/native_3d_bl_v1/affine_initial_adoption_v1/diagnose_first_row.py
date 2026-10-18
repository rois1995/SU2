#!/usr/bin/env python3
"""Independent saved-state placement diagnosis; proves no metric-quality acceptance."""
import hashlib,importlib.util,itertools,json,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
spec=importlib.util.spec_from_file_location('exact',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py')
exact=importlib.util.module_from_spec(spec);spec.loader.exec_module(exact)
def rounded(p):return tuple(float(x) for x in p)
def determinant(t):return exact.determinant([tuple(F(x) for x in rounded(p)) for p in t])
def weights(p,t):
    total=determinant(t);return [determinant(t[:i]+[p]+t[i+1:])/total for i in range(4)]
def main():
    folder=HERE/(sys.argv[1] if len(sys.argv)>1 else 'annulus_v1_n1');source=folder/'affine_annulus_bl_budget_20_m1_initial.json'
    data=json.loads(source.read_text());nodes={n[0]:np.array(n[1:],dtype=np.longdouble) for n in data['nodes']};facets={f['id']:f for f in data['facets']}
    wall={tuple(sorted(f['nodes'])):f for f in data['surface'] if data['marker_names'][facets[f['facet']]['marker']]=='wall'};reports=[]
    for cell in data['cells']:
        t=[nodes[n] for n in cell['nodes']]
        for key,face in wall.items():
            if not set(key).issubset(cell['nodes']):continue
            a,b,c=[nodes[n] for n in face['nodes']];normal=np.cross(b-a,c-a);normal/=np.sqrt(sum(normal*normal))
            center=np.array(rounded((a+b+c)/3),dtype=np.longdouble);apex=np.array(rounded(center-np.longdouble(.1)*normal),dtype=np.longdouble)
            backgrounds=[('cell_centroid',np.array(rounded(sum((p/4 for p in t),np.zeros(3,dtype=np.longdouble))),dtype=np.longdouble))]
            backgrounds += [(str(f),np.array(rounded((a+b+c)/3-np.longdouble(f)*np.longdouble(.1)*normal),dtype=np.longdouble)) for f in (1.25,1.5,2.)]
            trials=[]
            for label,p in backgrounds:
                own=weights(p,t);fan=weights(apex,[p,a,b,c]);trials.append(dict(proposal=label,point=rounded(p),minimum_background_weight=float(min(own)),minimum_apex_fan_weight=float(min(fan)),inside_cell=all(x>0 for x in own),apex_in_wall_fan=all(x>0 for x in fan)))
            direct=weights(apex,t);assert all(x>0 for x in direct)
            # Stellar insertion gives four positive cells and exact signed-volume closure.
            children=[t[:i]+[apex]+t[i+1:] for i in range(4)]
            assert all(determinant(tet)>0 for tet in children) and sum(determinant(tet) for tet in children)==determinant(t)
            reports.append(dict(cell=cell['id'],wall_facet=face['facet'],wall_nodes=face['nodes'],normal_point=rounded(apex),normal_point_barycentric=list(map(float,direct)),old_backgrounds=trials,direct_insertion_positive_and_exact_volume=True))
    assert len(reports)==2 and not any(t['inside_cell'] and t['apex_in_wall_fan'] for t in reports[0]['old_backgrounds'])
    output=dict(status='EXACT_GEOMETRIC_INSERTION_WITNESS_NOT_ACCEPTED_MESH',cells=reports,
        reason='A mandatory normal point lies strictly inside the retained wall tetrahedron, but all four pre-existing background proposals fail its cell/fan containment. An additional background is not required for positive stellar insertion.',
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),diagnostic_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        limitations=['Rounded long-double proposals reconstructed independently; no assertion of bitwise identity to C++ normal arithmetic','Exact predicates prove positive insertion/volume only; quality, full composed metric, coupled constraints and complete remeshing/restart remain unproved','Direct insertion near a cell face can create poor shapes; subsequent construction may still fail under unchanged final gates'])
    (folder/'first_row_diagnosis.json').write_text(json.dumps(output,indent=2)+'\n');print(output['status'])
if __name__=='__main__':main()
