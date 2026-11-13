"""Reconstruct one preserved M6 pre-inversion geometry/topology checkpoint, not a mesh acceptance."""
import hashlib,json,struct
from pathlib import Path
R=Path(__file__).resolve().parent
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    assessment=R/'surface_patch466_assessment.json';a=json.loads(assessment.read_text())
    assert a['all_actual_handles_terminal'] and a['owned_records_byte_identical_to_saved451']==303622
    pins=json.loads((Path(a['case'])/'pins.json').read_text())
    base=R/'cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424'
    cells=[];points={};inputs={}
    for rank in range(6):
        p=base/f'stage_surface_rank{rank}_owned.bin';digest=sha(p);assert pins[str(p)]==digest;inputs[str(p)]=digest
        with p.open('rb') as stream:
            count=struct.unpack('<Q',stream.read(8))[0]
            for _ in range(count):
                row=stream.read(416);assert len(row)==416 and struct.unpack_from('<Q',row)[0]==0x4e334443454c4c02
                ids=[]
                for k in range(4):
                    node=struct.unpack_from('<Qddd',row,16+32*k);ids.append(node[0]);old=points.setdefault(node[0],node[1:]);assert old==node[1:]
                cells.append(tuple(ids))
            assert not stream.read(1)
    assert len(cells)==303622
    cursor=max(points)+1
    def split(edge,identity):
        nonlocal cells
        x,y=edge;next_cells=[]
        for cell in cells:
            if x in cell and y in cell:next_cells.extend(tuple(identity if n==u else n for n in cell) for u in (x,y))
            else:next_cells.append(cell)
        cells=next_cells
    for k,edge in enumerate(((4218,8070),(4218,18565),(4219,8070),(4219,18565))):split(edge,cursor+k)
    for f in a['records']:
        if f['stage']=='residual_face':
            for node in f['vertices']:points[node[0]]=tuple(node[1:4])
    cursor+=4;recipe=[]
    operations=a['operations']
    assert all(x['admitted'] for x in operations[:16])
    assert not operations[16]['constructed'] and 'inverts inherited volume connectivity' in operations[16]['reason']
    for k,op in enumerate(operations[:16]):
        found={}
        for row in a['records']:
            if row['stage']=='trajectory_edge' and row['pass']==k+1:
                for identity,position in zip(row['edge'],(row['a'],row['b'])):
                    old=found.setdefault(identity,tuple(position));assert old==tuple(position)
        assert cursor in found;points[cursor]=found[cursor];recipe.append(dict(edge=op['edge'],point=[cursor,*found[cursor]]));split(op['edge'],cursor);cursor+=1
    edge=operations[16]['edge'];star=sorted(tuple(sorted(c)) for c in cells if all(n in c for n in edge))
    assert 0<len(star)<=64 and len(set(star))==len(star)
    nodes=sorted({n for c in star for n in c})
    physical=[f for f in a['records'] if f['stage']=='trajectory_face' and f['pass']==16 and any(set(f['vertices']).issubset(c) for c in star)]
    lines=[str(len(recipe))]
    for row in recipe:lines.append(' '.join(map(str,(*row['edge'],*row['point']))))
    lines.extend((' '.join(map(str,edge)),str(len(star))))
    lines.extend(' '.join(map(str,c)) for c in star);lines.append(str(len(nodes)))
    lines.extend(' '.join(map(str,(n,*points[n]))) for n in nodes)
    lines.append(str(len(physical)));lines.extend(' '.join(map(str,(f['facet'],*sorted(f['vertices'])))) for f in physical)
    target=R/'surface_replay468.dat';target.write_text('\n'.join(lines)+'\n')
    report=dict(assessment=str(assessment),assessment_sha256=sha(assessment),script_sha256=sha(Path(__file__)),source_owned=inputs,
        replay=recipe,failed_edge=edge,complete_edge_star=star,complete_nodes={n:points[n] for n in nodes},physical_faces=physical,recipe_file=str(target),recipe_sha256=sha(target),
        limits=['Diagnostic reconstruction of saved geometry/topology only. Current nodal metrics are transported afresh per publication.',
                'All16prefix constructor calls count inside the same24-call diagnostic budget.',
                'No new production persistence/configuration or stage acceptance.'])
    (R/'surface_replay468_provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(prefix=len(recipe),failed_edge=edge,star_cells=len(star),nodes=len(nodes),sha256=report['recipe_sha256']),indent=2))
if __name__=='__main__':main()
