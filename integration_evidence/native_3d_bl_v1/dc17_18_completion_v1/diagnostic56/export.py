"""Readable selected trial/donor records; the large surrounding mesh remains hash-pinned locally."""
import json
from pathlib import Path
from analyze import Reader,read
from run import ROOT,save
folder=ROOT/'capture'
def patch(path):
    r=Reader(read(Path(path)));cells=[r.cell() for _ in range(r.count(128))]
    surface=[r.take('QQQQ') for _ in range(r.count(512))];parameters=[]
    for _ in range(r.count(512)):
        node=r.take('Qddd');binding=r.take('QQQdd');parameters.append(dict(node=node,binding=binding))
    r.end();return dict(cells=cells,surface=surface,parameters=parameters)
report=json.loads((folder/'transaction_rank2.json').read_text())
for i,event in enumerate(report['events']):
    stem=folder/('transaction_rank2_event%d'%i)
    for key in ['old','fresh']:event[key]=patch(str(stem)+'_'+key+'.bin')
    r=Reader(read(folder/('transaction_rank2_event%d_dependencies.bin'%i)))
    event['dependencies']=[r.take('QQq') for _ in range(r.count(64))];r.end()
header={k:v for k,v in report.items() if k!='events'}
encoded=json.dumps(header,indent=2)[:-2]+',\n  "events": [\n'+',\n'.join(json.dumps(e,separators=(',',':')) for e in report['events'])+'\n  ]\n}\n'
assert json.loads(encoded)==json.loads(json.dumps(report))
(ROOT/'selected_trials.json').write_text(encoded)
r=Reader(read(folder/'transaction_rank2_donors.bin'));donors=[]
for _ in range(r.count(65536)):
    cell=r.cell();sensor=[r.take('dddddd') for _ in range(4)];extension,parent=r.take('QQ')
    donors.append(dict(cell=cell,sensor=sensor,extension_component=extension,parent_cell=parent))
r.end();save(ROOT/'selected_donors.json',donors)
print('Exported',len(report['events']),'events and',len(donors),'donors.')
