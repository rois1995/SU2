"""Independently certify the recorded eight disjoint native coarsening cavities."""
from collections import defaultdict
from pathlib import Path
import hashlib,json,sys
from audit_coupled import frame

def main(log,destination):
    destination.mkdir(exist_ok=False)
    groups=defaultdict(list);global_nodes=defaultdict(dict);global_cells=defaultdict(set)
    for line in log.read_text().splitlines():
        row=line.split()
        if not row or row[0]!='MESH' or not row[1].startswith(('batched_coarsen_','batched_refine_')):continue
        assert len(row)==28
        vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(4)]
        component=int(vertices[0][1]//2)
        assert 0<=component<8 and all(2*component<=v[1]<=2*component+1 for v in vertices)
        assert int(row[2]) not in global_cells[row[1]]
        global_cells[row[1]].add(int(row[2]))
        for vertex in vertices:
            prior=global_nodes[row[1]].setdefault(vertex[0],vertex)
            assert prior==vertex
        groups[row[1],component].append(dict(id=int(row[2]),vertices=vertices,
            physical=[list(map(int,row[20+2*i:22+2*i])) for i in range(4)]))
    names=('batched_coarsen_initial','batched_coarsen_final','batched_refine_initial','batched_refine_final')
    assert set(groups)=={(name,i) for name in names for i in range(8)}
    def faces(v):
        a,b,c,d=v
        return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
    records=[]
    for i in range(8):
        old=groups['batched_coarsen_initial',i]
        assert len(old)==12
        facets={}
        for row in old:
            for face,(present,id) in zip(faces(row['vertices']),row['physical']):
                if present:
                    assert id not in facets
                    facets[id]=dict(id=id,marker=10+id%12,vertices=face)
        assert len(facets)==12
        for name in names:
            rows=groups[name,i]
            if name.startswith('batched_coarsen_'):assert len(rows)==(12 if name.endswith('initial') else 6)
            elif name.endswith('initial'):assert len(rows)==6
            else:assert 6<len(rows)<=128
            tensor=[4,0,0,4,0,4] if name.startswith('batched_refine_') else [1,0,0,1,0,1]
            nodes={};cells=[];physical=[]
            for row in rows:
                cells.append(dict(id=row['id'],nodes=[v[0] for v in row['vertices']]))
                for vertex in row['vertices']:
                    if vertex[0] in nodes:assert nodes[vertex[0]]==vertex
                    assert 2*i<=vertex[1]<=2*i+1 and 0<=vertex[2]<=1 and 0<=vertex[3]<=1
                    nodes[vertex[0]]=vertex
                for face,(present,id) in zip(faces(row['vertices']),row['physical']):
                    if present:physical.append(dict(facet=id,nodes=[v[0] for v in face]))
            data=dict(nodes=list(nodes.values()),cells=cells,surface=physical,facets=list(facets.values()),
                original=[dict(vertices=[v+[tensor] for v in row['vertices']]) for row in old])
            path=destination/(name+'_'+str(i)+'.json');path.write_text(json.dumps(data,indent=2)+'\n')
            measured,_=frame(path,name!='batched_refine_initial')
            assert measured['volume']=='1'
            if name!='batched_refine_initial':assert measured['maximum_length']<=1.8
            records.append(measured)
    # Each component is certified inside a distinct [2i,2i+1] slab, so cross-component overlap is impossible.
    result=dict(status='PASS',frames=records,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
        scope='Exact positive volume/no overlaps/gaps/manifold/finite faceted geometry and markers, uniform original-P1 quality/sliver/edge gates on eight independently bounded disjoint components. Recorded actual batched engine outputs; no CFD transfer or timing certificate.')
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
    (destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(records),'independently audited batch-engine frames',destination)

if __name__=='__main__':main(*map(Path,sys.argv[1:]))
