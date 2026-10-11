from pathlib import Path
import importlib.util,json,sys
root=Path.cwd();suite=root/'integration_evidence/native_cluster_3d_query_comparison_v1'
sys.path[:0]=[str(suite),str(root/'integration_evidence/native_cluster_3d_core_v1'),str(root/'integration_evidence/native_3d_core_v1')]
spec=importlib.util.spec_from_file_location('query_campaign',suite/'campaign.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
records=[]
for name in ('query_routing_n8_n1_m1_rounds1024_v1','query_routing_n4_n4_m2_v1'):
 case=root/'integration_evidence/native_3d_core_v1'/name
 d=m.runtime(case,json.loads((case/'parameters.json').read_text()))
 records.append(dict(folder=name,event_samples=d['samples'],profile_keys=list(d['profiles'])))
print(json.dumps(records,indent=2))
