"""Stream the existing exported mesh; preserve exact failing cell/face/node identities."""
from pathlib import Path
import json, math, sys
root=Path(__file__).resolve().parents[3]
source=Path(sys.argv[1]) if len(sys.argv)>1 else root/'integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc17/chain_c9/event1/event1_final.mesh'
wanted={1203130,1217351,1217329}
rows={x:{'cell':x,'faces':[]} for x in wanted}
def blocks(path):
    with path.open() as f:
        for header in f:
            words=header.split()
            if not words:continue
            kind=words[0]
            if kind=='h0':continue
            for _ in range(int(words[1])):yield kind,f.readline().split()
for kind,v in blocks(source):
    if kind in ('cells','faces','cell_gates') and int(v[0]) in wanted:
        r=rows[int(v[0])]
        if kind=='cells':r['nodes']=list(map(int,v[1:5]))
        elif kind=='faces':r['faces'].append(list(map(int,v)))
        else:r['gates']=list(map(float,v[1:]))
missing=[k for k,r in rows.items() if 'nodes' not in r]
rows={k:r for k,r in rows.items() if 'nodes' in r}
assert rows, 'No witness cells remain in the supplied mesh'
print('Witness cells removed by reconstruction:',missing)
ids={n for r in rows.values() for n in r['nodes']};points={}
for kind,v in blocks(source):
    if kind!='nodes':break
    if int(v[0]) in ids:points[int(v[0])]=list(map(float,v[1:4]))
def sub(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
for r in rows.values():
    r['points']={n:points[n] for n in r['nodes']}
    for f in r['faces']:
        if f[5]!=1:continue
        a,b,c=[points[n] for n in f[1:4]];n=cross(sub(b,a),sub(c,a));length=math.sqrt(dot(n,n));n=[x/length for x in n]
        apex=next(n for n in r['nodes'] if n not in f[1:4]);h=abs(dot(sub(points[apex],a),n))
        r['apex']=apex;r['height']=h;r['height_over_requested']=h/3.2e-5
out=Path(sys.argv[2]) if len(sys.argv)>2 else Path(__file__).with_name('height_witnesses_c9.json');out.write_text(json.dumps(list(rows.values()),indent=2)+'\n')
for r in rows.values():print(r)
