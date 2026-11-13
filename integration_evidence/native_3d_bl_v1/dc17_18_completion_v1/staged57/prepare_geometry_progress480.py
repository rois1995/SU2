"""One exact dominating query answers a geometry-progress construction decision; no policy edit."""
import gzip,hashlib,importlib.util,json,math,struct
from pathlib import Path
R=Path(__file__).resolve().parent
bits=lambda x:struct.pack('>d',x).hex()
decode=lambda x:struct.unpack('>d',bytes.fromhex(x))[0]
canon=lambda points:tuple(sorted(tuple(p) for p in points))
def main():
    a=json.loads((R/'current_trajectory476_assessment.json').read_text());source=Path(a['case'])/'trajectory_working_owned.bin'
    assert hashlib.file_digest(source.open('rb'),'sha256').hexdigest()==a['working_mesh_sha256']
    op=a['contexts'][0]['operations'][-1];edge=set(op['edge']);assert edge=={14983,24439} and not op['published']
    spec=importlib.util.spec_from_file_location('inspect_native',R/'inspect_native_failure3d.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    points={};parents=[];faces=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
    for row in m._read_owned(source,304521,2_000_000_000):
        ids={n['id'] for n in row['nodes']}
        for n in row['nodes']:points[n['id']]=tuple(bits(v) for v in n['xyz'])
        if edge.issubset(ids):
            for k,face in enumerate(row['physical']):
                if face['present']:
                    nodes=[row['nodes'][i]['id'] for i in faces[k]]
                    assert edge.issubset(nodes),'non-edge physical face needs independent accounting'
                    parents.append((face['facet'],nodes))
    assert len(parents)==2
    known=set(points.values());queries=json.loads(gzip.decompress((R/'geometry_refusals477_queries.json.gz').read_bytes()))
    candidates={}
    for q in queries:
        if len(q['triangles_bits'])!=1:continue
        t=q['triangles_bits'][0];coords=[tuple(t[k:k+3]) for k in range(0,9,3)];unknown=[p for p in coords if p not in known]
        if len(unknown)==1:candidates.setdefault(unknown[0],[]).append(q)
    selected=[]
    for p,qs in candidates.items():
        expected=[]
        for _,ids in parents:
            for removed in edge:expected.append(canon([p if n==removed else points[n] for n in ids]))
        if all(canon([tuple(q['triangles_bits'][0][k:k+3]) for k in range(0,9,3)]) in expected for q in qs):selected.append((p,qs,expected))
    assert len(selected)==1
    point,qs,expected=selected[0];children=[]
    for triangle in expected:
        matches=[q for q in qs if canon([tuple(q['triangles_bits'][0][k:k+3]) for k in range(0,9,3)])==triangle]
        if matches:
            q=min(matches,key=lambda q:q['bound']);children.append(dict(triangle=triangle,upper=q['bound'],query=q))
        else:children.append(dict(triangle=triangle,upper=1e-4,query=None))
    # These WING faces have no affine floor; a missing BoundPhysical query means direct error<=tau.
    ref=R/'prepared_reference441.native_ref'
    with ref.open() as f:
        f.readline();h=f.readline().split();markers=[f.readline() for _ in range(int(h[0]))]
        parent_markers={}
        for _ in range(int(h[1])):
            row=f.readline().split()
            if int(row[0]) in {p[0] for p in parents}:parent_markers[int(row[0])]=int(row[1])
        for _ in range(int(h[2])):f.readline()
        affine={int(f.readline().split()[0]) for _ in range(int(h[3]))}
    assert all(facet not in affine and markers[parent_markers[facet]].strip()=='"WING3D"' for facet,_ in parents)
    current=sum(max(0.,c['upper']-1e-4)**2 for c in children)
    assert abs(current-op['new_geometry_excess'])<1e-16,(current,op['new_geometry_excess'])
    worst=max(children,key=lambda c:c['upper']);others=current-max(0.,worst['upper']-1e-4)**2
    allowance=op['old_geometry_excess']*(1-1e-9)-others;assert allowance>0
    ceiling=math.nextafter(1e-4+math.sqrt(allowance),0.)
    q=worst['query'];assert q and q['support']
    output=R/'geometry_progress480_query.dat';output.write_text(f"{len(q['support'])} 1 {ceiling:.17g} 200000\n"+' '.join(map(str,q['support']))+'\n'+'\n'.join(' '.join(format(decode(x),'.17g') for x in p) for p in worst['triangle'])+'\n')
    manifest=dict(construction_question='Can exact existing rejected split be certified to reduce complete patch geometry upper-bound merit, without changing geometry, support or any acceptance gate?',source_state=str(source),edge=sorted(edge),parents=parents,projected_point=[decode(x) for x in point],old_complete_excess=op['old_geometry_excess'],current_fresh_excess=current,other_children_upper_excess=others,dominating_old_upper=worst['upper'],decision_ceiling=ceiling,physical_geometry_gate=1e-4,query_input=str(output),query_sha256=hashlib.file_digest(output.open('rb'),'sha256').hexdigest(),exact_support=q['support'],triangle_bits=worst['triangle'],children=children,work_limit=200000,total_seconds_limit=300,support_result='New valid whole-face upper makes new complete summed excess<old; all other native gates already passed for this same candidate.',reject_or_unresolved='Failure to obtain a sufficient upper leaves progress unproven; a genuine distance witness may reject this budget, resource exhaustion is unresolved.',downstream='Choose purpose-correct geometry certification versus physical candidate correction. No production ceiling/constraint/config edit authorized solely by a partial query.',production_changed=False,goal_complete=False)
    (R/'geometry_progress480_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:manifest[k] for k in ('old_complete_excess','current_fresh_excess','other_children_upper_excess','dominating_old_upper','decision_ceiling')},indent=2))
if __name__=='__main__':main()
