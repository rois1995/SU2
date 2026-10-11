"""Exact necessary binary64 placement certificates for retained rejected edges; no meshing claim."""
from fractions import Fraction as F
from pathlib import Path
import hashlib,itertools,json,math,sys
from collections import defaultdict

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def certificate(a,b):
    d=[y-x for x,y in zip(a,b)]
    for axis in range(3):
        lo,hi=sorted((a[axis],b[axis]))
        if lo<=0 or lo==hi:continue
        exponent=math.frexp(float(lo))[1]-1
        if exponent < -1022:continue
        lower=F(2)**exponent
        if not lower<=lo<hi<=2*lower:continue
        # Every binary64 interior coordinate in this binade is an integer multiple of this spacing.
        spacing=F(2)**(exponent-52);count=(hi-lo)/spacing
        if count.denominator!=1:continue
        count=count.numerator;odd=count
        while odd%2==0:odd//=2
        step=1;constraints=[]
        for i in range(3):
            if i==axis:continue
            divisor=odd//math.gcd(odd,abs(d[i].numerator))
            step=step*divisor//math.gcd(step,divisor)
            constraints.append(dict(axis=i,direction=str(d[i]),necessary_index_divisor=divisor))
        # An odd factor in t=k/count must cancel in every direction component for coordinates to be dyadic.
        if step>=count:
            return dict(axis=axis,binade=[str(lower),str(2*lower)],spacing=str(spacing),
                        grid_intervals=count,odd_intervals=odd,necessary_index_multiple=step,
                        other_coordinate_constraints=constraints,
                        proof='All binary64 interior points need an integer grid index 0<k<grid_intervals. Dyadicity of every other coordinate requires k divisible by necessary_index_multiple. No such k exists.')
    return None

def self_check():
    assert certificate(tuple(map(F,(0.,0.,0.))),tuple(map(F,(1.,1.,1.)))) is None
    assert certificate(tuple(map(F,(.5,0.,0.))),tuple(map(F,(1.,.5,.5)))) is None
    assert certificate(tuple(map(F,(2/3,0.,0.))),tuple(map(F,(1.,1/3,1/3)))) is not None
    assert certificate(tuple(map(F,(.75,.25,0.))),tuple(map(F,(1.,.5,.25)))) is None

def main(source,folder):
    self_check();source=source.resolve();folder=folder.resolve();folder.mkdir(exist_ok=False)
    print('Case working and retained folder:',folder,flush=True)
    audit=json.loads((source/'independent_audit.json').read_text());assert audit['status']=='PASS'
    initial=json.loads((source/'replay_initial.json').read_text())
    markers={f['id']:f['marker'] for f in initial['facets']}
    junctions=defaultdict(list)
    for face in initial['surface']:
        for edge in itertools.combinations(face['nodes'],2):
            junctions[tuple(sorted(edge))].append(dict(facet=face['facet'],marker=markers[face['facet']]))
    junctions={edge:uses for edge,uses in junctions.items() if len({r['marker'] for r in uses})>1}
    records=[]
    for row in audit['oversized_edges']:
        a,b=[tuple(F(float(x)) for x in p) for p in row['points']]
        d=[y-x for x,y in zip(a,b)]
        m=tuple(F(float((x+y)/2)) for x,y in zip(a,b));v=[x-y for x,y in zip(m,a)]
        residuals=[d[i]*v[j]-d[j]*v[i] for i in range(3) for j in range(i+1,3)]
        records.append(dict(**row,rounded_midpoint=[float(x) for x in m],
                            exact_cross_residuals=list(map(str,residuals)),midpoint_on_exact_edge=not any(residuals),
                            no_binary64_interior_certificate=certificate(a,b),
                            original_marker_junction=junctions.get(tuple(sorted(row['vertices'])),[])))
    result=dict(status='PASS',source_folder=str(source),source_audit_sha256=sha(source/'independent_audit.json'),
                source_private_mesh_sha256=sha(source/'replay_rejected.json'),auditor_sha256=sha(Path(__file__)),
                oversized_edges=len(records),off_edge_midpoints=sum(not r['midpoint_on_exact_edge'] for r in records),
                certified_no_binary64_interior=sum(r['no_binary64_interior_certificate'] is not None for r in records),
                source_initial_mesh_sha256=sha(source/'replay_initial.json'),
                certified_oversized_original_junctions=[r['vertices'] for r in records if r['no_binary64_interior_certificate'] and r['original_marker_junction']],
                edges=records,scope='Exact arithmetic necessary-condition certificates for these retained edges. Absence of a certificate does not prove placement possible. Only an oversized, immutable original marker-junction segment with no binary64 interior coordinate establishes a geometry/length incompatibility for that segment. Interior-edge certificates do not prove target infeasibility or justify moving physical geometry. Private retriangulation can remove an obstructing interior edge while preserving the cavity skin.')
    (folder/'placement_certificates.json').write_text(json.dumps(result,indent=2)+'\n')
    (folder/'audit_placement.py').write_bytes(Path(__file__).read_bytes())
    print('PASS',result['off_edge_midpoints'],'off-edge midpoints;',result['certified_no_binary64_interior'],'proved unsplittable binary64 edges',flush=True)
if __name__=='__main__':main(Path(sys.argv[1]),Path(sys.argv[2]))
