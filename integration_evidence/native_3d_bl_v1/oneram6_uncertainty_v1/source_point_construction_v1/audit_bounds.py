#!/usr/bin/env python3
"""Independent exact affine-bound oracle, including original binary64 parameters and subnormals."""
import csv,hashlib,json,math,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
folder=Path(sys.argv[1]).resolve();reports=[]
for path in sorted(folder.glob('affine_rounding_rank*.csv')):
 rows=list(csv.DictReader(path.open()));assert len(rows)==42
 for row in rows:
  t=F(float(row['t']));assert 0<=t<=1
  for axis in 'xyz':
   a,b=F(float(row['a'+axis])),F(float(row['b'+axis]));value=(1-t)*a+t*b
   lo,hi=float(row['l'+axis]),float(row['u'+axis]);assert math.isfinite(lo) and math.isfinite(hi)
   assert F(lo)<=value<=F(hi),(row,axis,str(value))
   if lo!=hi:assert float(np.nextafter(lo,math.inf))==hi
   if F(lo)==value or F(hi)==value:assert lo==hi,'Exact representable values must have a singleton bound'
 reports.append({'file':path.name,'rows':len(rows),'exact_coordinate_checks':len(rows)*3,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
assert reports,'Missing native affine-bound outputs'
result={'passed':True,'files':reports,'scope':'Exact Fraction arithmetic at actual binary64 input coordinates and t; adjacent/minimal bounds, cancellation and subnormals. Not mesh acceptance.',
 'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(folder/'independent_affine_bounds.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
