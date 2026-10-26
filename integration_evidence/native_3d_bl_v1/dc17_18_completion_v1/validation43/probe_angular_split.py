"""Independent angular-conditioning diagnosis; not production acceptance evidence."""
import math, heapq, json
from pathlib import Path
B7=[(1/3,)*3,(.059715871789770,.470142064105115,.470142064105115),(.470142064105115,.059715871789770,.470142064105115),(.470142064105115,.470142064105115,.059715871789770),(.797426985353087,.101286507323456,.101286507323456),(.101286507323456,.797426985353087,.101286507323456),(.101286507323456,.101286507323456,.797426985353087)]
W7=[.225]+[.132394152788506]*3+[.125939180544827]*3
B12=[(.501426509658179,.249286745170910,.249286745170910),(.249286745170910,.501426509658179,.249286745170910),(.249286745170910,.249286745170910,.501426509658179),(.873821971016996,.063089014491502,.063089014491502),(.063089014491502,.873821971016996,.063089014491502),(.063089014491502,.063089014491502,.873821971016996),(.053145049844817,.310352451033784,.636502499121399),(.053145049844817,.636502499121399,.310352451033784),(.310352451033784,.053145049844817,.636502499121399),(.310352451033784,.636502499121399,.053145049844817),(.636502499121399,.053145049844817,.310352451033784),(.636502499121399,.310352451033784,.053145049844817)]
W12=[.116786275726379]*3+[.050844906370207]*3+[.082851075618374]*6
def norm(p):return math.sqrt(sum(x*x for x in p))
def det(a,b,c):return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])
center=(.35,.35,.02);vertices=[(0,0,0),(1,0,0),(0,1,0),(0,0,1)]
charts=[]
for skip in range(4):
 face=[v for k,v in enumerate(vertices) if k!=skip]
 anchor=tuple(x+(1-sum(center))/3 for x in center) if skip==0 else tuple(0 if k==skip-1 else x for k,x in enumerate(center))
 for j in range(3):charts.append(tuple(tuple(x-c for x,c in zip(v,center)) for v in [anchor,face[j],face[(j+1)%3]]))
def run(angular,strategy="gradient"):
 def score(t,vs,i,j):
  if strategy=="gradient":return abs(vs[i]-vs[j])
  if strategy=="angle":return sum((t[i][d]/norm(t[i])-t[j][d]/norm(t[j]))**2 for d in range(3))
  length=sum((t[i][d]-t[j][d])**2 for d in range(3))
  return length/(norm(t[i])*norm(t[j])) if strategy=="relative" else length
 heap=[];total=error=0.;identity=0
 def add(tri):
  nonlocal total,error,identity
  area=abs(det(*tri))/2
  def rule(bs,ws):
   values=[norm(tuple(sum(b[k]*tri[k][d] for k in range(3)) for d in range(3)))**-3 for b in bs]
   return area*sum(w*v for w,v in zip(ws,values)),values
  v,vs=rule(B7,W7);low,_=rule(B12,W12);e=abs(v-low)
  total+=v;error+=e;heapq.heappush(heap,(-e,identity,tri,v,vs[-3:]));identity+=1
 for t in charts:add(t)
 while error>1e-4*total and len(heap)<4096:
  ne,_,t,v,vs=heapq.heappop(heap);total-=v;error+=ne
  a,b=max(((0,1),(0,2),(1,2)),key=lambda ij:score(t,vs,*ij))
  alpha=norm(t[a])/(norm(t[a])+norm(t[b])) if angular else .5
  mid=tuple((1-alpha)*x+alpha*y for x,y in zip(t[a],t[b]))
  for k in [a,b]:
   c=list(t);c[k]=mid;add(tuple(c))
 return dict(angular=angular,strategy=strategy,panels=len(heap),constructed=identity,integral=total,oracle=4*math.pi,relative_error=total/(4*math.pi)-1,estimated_relative_error=error/total)
results=[run(a,s) for s in ["gradient","angle","relative","length"] for a in [False,True]]
print(json.dumps(results,indent=2))
Path(__file__).with_suffix('.json').write_text(json.dumps(results,indent=2)+'\n')
