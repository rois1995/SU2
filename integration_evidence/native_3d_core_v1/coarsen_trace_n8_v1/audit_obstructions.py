"""Independent exact witnesses for early geometry rejection, without a target or acceptance change."""
from collections import Counter
from fractions import Fraction as F
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
folder=Path(__file__).resolve().parent;sys.path.insert(0,str(folder.parent))
from audit_cavities import determinant
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
review=json.loads((folder/'analysis.json').read_text());assert review['status']=='PASS'
assert sha(folder/'attempts_rank0.jsonl')==review['artifact_sha256']['attempts_rank0.jsonl']
rows=[];signs=Counter()
for event in map(json.loads,(folder/'attempts_rank0.jsonl').read_text().splitlines()):
 m=re.fullmatch(r'Native 3D collapse obstructed at cell (\d+)\.',event['reason'])
 if event['approved'] or not m:continue
 cell=next(c for c in event['old'] if c['id']==int(m[1]))
 retained,removed=event['edge'];coordinates={n[0]:tuple(F(float(x)) for x in n[1:]) for c in event['old'] for n in c['nodes']}
 ids=[n[0] for n in cell['nodes']];assert removed in ids and retained not in ids
 proposal=[coordinates[retained if id==removed else id] for id in ids]
 det=determinant(proposal);assert det<=0
 # The same witness accepts an interior replacement point: test the rejection boundary, not just the trace text.
 centroid=tuple(sum(coordinates[id][j] for id in ids)/4 for j in range(3))
 control=[centroid if id==removed else coordinates[id] for id in ids]
 assert determinant(control)>0
 sign='zero' if det==0 else 'negative';signs[sign]+=1
 rows.append(dict(round=event['round'],edge=event['edge'],cell=cell['id'],version=cell['version'],exact_determinant=str(det)))
assert len(rows)==183
result=dict(status='PASS',orientation_rejections=len(rows),sign_counts=dict(signs),interior_positive_controls=len(rows),witnesses=rows,trace_analysis_sha256=sha(folder/'analysis.json'),source_sha256={'audit_obstructions.py':sha(Path(__file__)),'../audit_cavities.py':sha(folder.parent/'audit_cavities.py')},scope='Exact rational witnesses on actual imported serial n8 stars: each surviving-cell replacement has nonpositive determinant and an interior-point positive control. These 183 rejections could be known before private reconstruction when a local witness and retained point are available. No implementation, remote witness discovery, dependency invalidation, measured speedup or n8 completion claim.')
(folder/'orientation_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='witnesses'},indent=2))
