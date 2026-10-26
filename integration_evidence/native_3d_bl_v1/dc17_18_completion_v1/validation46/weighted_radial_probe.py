"""Check the derived two-node positive rule against exact normalized moments."""
import math,json
from pathlib import Path
rows=[]
for r in [0.,1e-12,.01,.1,.5,.9,1.]:
 m0=2+2*r*r/3;m1=-4*r/3;m2=2/3+2*r*r/5;m3=-4*r/5
 alpha=-(m3-m1*m2/m0)/(m2-m1*m1/m0);beta=-(m2+alpha*m1)/m0
 root=math.sqrt(alpha*alpha-4*beta);nodes=[(-alpha-root)/2,(-alpha+root)/2]
 weights=[(m0*nodes[1]-m1)/(nodes[1]-nodes[0]),0.];weights[1]=m0-weights[0]
 assert all(-1<x<1 for x in nodes) and all(w>0 for w in weights)
 residuals=[]
 for k in range(4):
  exact=(1+(-1)**k)/(k+1)-2*r*(1+(-1)**(k+1))/(k+2)+r*r*(1+(-1)**(k+2))/(k+3)
  error=abs(sum(w*x**k for x,w in zip(nodes,weights))-exact)
  assert error<2e-15,(r,k,error)
  residuals.append(error)
 rows.append(dict(ratio=r,nodes=nodes,weights=weights,moment_residuals=residuals))
Path(__file__).with_suffix('.json').write_text(json.dumps(rows,indent=2)+'\n')
print('7 ratios, positive interior nodes and weights, all moments through cubic degree verified')
