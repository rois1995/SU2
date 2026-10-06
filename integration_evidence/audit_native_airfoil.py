"""One-rank rejected/accepted real-airfoil candidate, actual frozen-P1 target audit."""
import argparse
from collections import Counter
import json
from pathlib import Path
from audit_native_bl import audit

parser=argparse.ArgumentParser()
parser.add_argument('directory',type=Path)
parser.add_argument('--cycle',type=int,default=0)
parser.add_argument('--height',type=float,default=.002)
parser.add_argument('--suffix',default='rejected')
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--reference',type=Path)
args=parser.parse_args()
prefix=args.directory/f'native_airfoil_cycle_{args.cycle}'
row=audit(str(prefix)+'_donor.su2',str(prefix)+'_metric.csv',str(prefix)+'_'+args.suffix+'.su2',args.height,
          wall_tags=('airfoil',),fast=True,extension_limit=1e-6)
if args.reference:
    from airfoil_reference_audit import reference_audit
    row['original_reference_checks']=reference_audit(args.reference,str(prefix)+'_'+args.suffix+'.su2')
row['residual_regions']=dict(Counter('near_airfoil' if -0.1 < sum(p[0] for p in cell['coordinates'])/3 < 1.1 and
                                    abs(sum(p[1] for p in cell['coordinates'])/3) < .15 else 'far_field'
                                    for cell in row['bad_cells']))
args.output.write_text(json.dumps(row,indent=2)+'\n')
print(json.dumps({key:value for key,value in row.items() if key!='bad_cells'},indent=2))
