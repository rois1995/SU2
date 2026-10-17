"""Strict source/container invariance; rejected pre-canonical receipts remain separate."""
import hashlib,json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
prefix=sys.argv[1] if len(sys.argv)>1 else 'canonical_';assert prefix in ('','canonical_')
reports=[];hashes={}
for rank in range(4):
 paths=[HERE/(prefix+mode)/('query_trace_rank%d.csv'%rank) for mode in ('frozen','aliased')]
 a,b=[np.loadtxt(p,delimiter=',') for p in paths]
 reports.append(dict(rank=rank,fast_rows=len(a),fallback_rows=len(b),same_shape=a.shape==b.shape,numeric_equal=bool(np.array_equal(a,b)),bits_equal=bool(a.shape==b.shape and np.array_equal(a.view('u8'),b.view('u8')))))
 for p in paths:hashes[str(p.relative_to(HERE))]=hashlib.sha256(p.read_bytes()).hexdigest()
p=HERE/(prefix+'frozen')/'first_row_rejected.dat';q=HERE/(prefix+'aliased')/'first_row_rejected.dat';legacy=HERE.parent/'first_row_shape_ranking_v1/native_np4_b8/first_row_rejected.dat'
rows,other,old=[p.read_bytes() for p in (p,q,legacy)]
report=dict(query_rows=reports,actual_geometry_fast_vs_fallback=rows==other,actual_geometry_vs_saved_baseline=rows==old,CFD_steps=0,hashes=hashes,helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(HERE/(prefix+'frozen_comparison.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
assert all(x['bits_equal'] for x in reports) and rows==other and rows==old
