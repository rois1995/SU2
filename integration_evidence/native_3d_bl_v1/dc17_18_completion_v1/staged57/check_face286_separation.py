"""Bounded exact finite-segment/open-triangle SAT check against rational interval clipping."""
from fractions import Fraction
from itertools import combinations,product
from pathlib import Path
import hashlib,json,time
started=time.monotonic()
def orient(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
def clipped(t,a,b):
 if a==b:return False
 sign=1 if orient(*t)>0 else -1
 low,high=Fraction(0),Fraction(1)
 for i in range(3):
  x,y=(sign*orient(t[i],t[(i+1)%3],p) for p in (a,b));delta=y-x
  if delta==0:
   if x<=0:return False
  elif delta>0:low=max(low,Fraction(-x,delta))
  else:high=min(high,Fraction(x,-delta))
 return low<high
def separated(t,a,b):
 sign=1 if orient(*t)>0 else -1
 for i in range(3):
  if all(sign*orient(t[i],t[(i+1)%3],p)<=0 for p in (a,b)):return False
 sides=[orient(a,b,p) for p in t]
 return min(sides)<0<max(sides)
points=list(product(range(-1,3),repeat=2));segments=list(combinations(points,2))+[(p,p) for p in points]
count=0
for triangle in combinations(points,3):
 if not orient(*triangle):continue
 for a,b in segments:
  for t in (triangle,triangle[::-1]):
   expected=clipped(t,a,b);actual=separated(t,a,b)
   assert expected==actual,(t,a,b,expected,actual)
   count+=1
result={'checks':count,'mismatches':0,'seconds':time.monotonic()-started,'oracle':'Exact rational interval clipping against all open triangle halfplanes; finite segment, both triangle orientations, tangencies, degeneracies and zero-length segments.','helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'production_validated':False}
Path(__file__).with_name('face286_separation_oracle.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
