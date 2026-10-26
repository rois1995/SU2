"""Independent full-patch centroid quality for geometrically constructed captured candidates."""
import importlib.util,json,sys
from pathlib import Path
import numpy as np
from analyze import Reader,read
from run import ROOT,save
sys.path.insert(0,str(ROOT.parent/'diagnostic55'))
spec=importlib.util.spec_from_file_location('witness55',ROOT.parent/'diagnostic55/witness_analysis.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
folder=ROOT/'capture';r=Reader(read(folder/'transaction_rank2_donors.bin'));nodes={};cells=[]
for _ in range(r.count(65536)):
    cid,vertices=r.cell();metrics=[r.take('dddddd') for _ in range(4)];extension,parent=r.take('QQ')
    cells.append([n[0] for n in vertices])
    for n,m in zip(vertices,metrics):
        xx,xy,xz,yy,yz,zz=m;value=(n[1:],(xx,xy,xz,xy,yy,yz,xz,yz,zz))
        assert n[0] not in nodes or nodes[n[0]]==value
        nodes[n[0]]=value
r.end();ids=sorted(nodes);index={v:i for i,v in enumerate(ids)}
field=object.__new__(module.Original3D)
field.points=np.asarray([nodes[n][0] for n in ids],dtype=np.longdouble)
field.tensors=np.asarray([nodes[n][1] for n in ids],dtype=np.longdouble)
field.cells=np.asarray([[index[n] for n in c] for c in cells]);field.cache={};field.candidates=lambda p:range(len(cells))
report=json.loads((folder/'transaction_rank2.json').read_text());rows=[]
for i,event in enumerate(report['events']):
    if event['stage'] not in ('SplitEdge','ReconstructSplitPatch') or not event['passed']:continue
    reader=Reader(read(folder/('transaction_rank2_event%d_fresh.bin'%i)));shapes=[]
    for _ in range(reader.count(128)):
        cid,vertices=reader.cell();p=np.array([n[1:] for n in vertices],dtype=np.longdouble)
        center=tuple(np.asarray(np.mean(p,axis=0),dtype=float));shapes.append(module.shape(p,field(center)))
    q=float(min(x[0] for x in shapes));j=float(min(x[1] for x in shapes))
    rows.append(dict(event=i,stage=event['stage'],cells=len(shapes),q=q,J=j,score=min(q/.2,j/.05)))
rows.sort(key=lambda r:-r['score']);save(ROOT/'candidate_ranking.json',rows)
for row in rows:print(row)
