"""Offline repeated-point count from preserved source coordinates; no mesh/flow run."""
import csv,json,re,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import run
BASE=Path(__file__).resolve().parent
source=run.HERE/'construction_first_trial_v1/sensor_original.bin'
folder=run.HERE/'point_preparation_reuse_v1/m6_np1'
rowsfile=folder/'preparation_weights_rank0.csv'
proof=json.loads((folder/'input_provenance.json').read_text())
assert run.sha(source)==proof[str((folder/'sensor_original.bin').relative_to(run.ROOT))]
raw=source.read_bytes();count=struct.unpack_from('<Q',raw)[0];assert len(raw)==8+104*count
coords={r[0]:r[1:4] for r in struct.iter_unpack('<Q12d',raw[8:])};assert len(coords)==count
with rowsfile.open() as f:rows=list(csv.DictReader(f))
ids={int(r['n%d'%j]) for r in rows for j in range(4)};points={coords[i] for i in ids}
header=run.ROOT/'Common/include/adaptation/CNativeField3D.hpp'
capacity=int(re.search(r'MaximumSensorSamples = (\d+)',header.read_text())[1])
result={'cells':len(rows),'nodal_queries':4*len(rows),'unique_original_node_ids':len(ids),
 'unique_nodal_coordinates':len(points),'all_point_queries':5*len(rows),
 'upper_bound_unique_queries_including_centroids':len(points)+len(rows),
 'lower_bound_repeated_point_queries':4*len(rows)-len(points),'cache_capacity':capacity,
 'inputs':{str(p.relative_to(run.ROOT)):run.sha(p) for p in (source,rowsfile,header)},
 'helper_sha256':run.sha(Path(__file__)),
 'scope':'Same selected M6 cells; nodal coordinate count is exact. Distinct centroids not assumed; unique-query total is an upper bound. No runtime speedup follows.'}
(BASE/'point_reuse_opportunity.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result),flush=True)
