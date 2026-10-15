"""Geometry-derived spacing proxy for construction design; never a mesh-acceptance field."""
import collections,math
import numpy as np
class Design:
 def __init__(self,geometry,h,growth,thickness):
  self.g=geometry;self.h=h;self.growth=growth;self.thickness=thickness
  self.cache={};self.patterns=collections.Counter();self.margins=[]
  self.gauss,self.weights=np.polynomial.legendre.leggauss(32)
 def metric(self,points):
  kind,origin,extent,_,axis=self.g;v=points-origin;axis=axis/np.linalg.norm(axis)
  if kind==3:
   radial=v-(v@axis)[:,None]*axis;r=np.linalg.norm(radial,axis=1);n=radial/np.maximum(r[:,None],1e-100);t=np.cross(axis,n)
  else:
   assert kind==1;r=np.linalg.norm(v,axis=1);n=v/np.maximum(r[:,None],1e-100)
  d=np.abs(r-extent[0]);s=np.clip((d-.9*self.thickness)/(.1*self.thickness),0,1);fade=1-s*s*(3-2*s)
  hn=np.maximum(self.h,2*(self.h+(self.growth-1)*d)/(self.growth+1))
  epsilon=min(.04,.5*self.h,extent[0]);curvature=(1.8/(2*math.sqrt(epsilon*(2*extent[0]-epsilon))))**2
  normal=np.maximum(1,hn**(-2*fade));tangent=max(1,curvature)**fade
  eye=np.broadcast_to(np.eye(3),(len(points),3,3)).copy()
  result=eye+(normal-1)[:,None,None]*n[:,:,None]*n[:,None,:]
  if kind==3:result+=(tangent-1)[:,None,None]*t[:,:,None]*t[:,None,:]
  else:result+=(tangent-1)[:,None,None]*(eye-n[:,:,None]*n[:,None,:])
  return result
 def length(self,a,b,nodes):
  key=tuple(sorted((a,b)))
  if key not in self.cache:
   x,y=nodes[a],nodes[b];d=y-x;points=.5*((1-self.gauss[:,None])*x+(1+self.gauss[:,None])*y)
   m=self.metric(points);self.cache[key]=.5*np.sum(self.weights*np.sqrt(np.einsum('i,nij,j->n',d,m,d)))
  return self.cache[key]
 def score(self,cells,nodes):
  p=np.array([[nodes[n] for n in c] for c in cells]);det=np.linalg.det(p[:,1:]-p[:,:1])
  if np.any(det<=0):return -math.inf
  m=self.metric(p.mean(axis=1));e=p[:,None,:,:]-p[:,:,None,:];s=np.einsum('nabi,nij,nabj->nab',e,m,e)
  weighted=det*np.sqrt(np.linalg.det(m));i,j=np.triu_indices(4,1)
  q=12*(weighted/2)**(2/3)/s[:,i,j].sum(axis=1)
  products=np.array([np.prod(np.delete(s[:,k,:],k,axis=1),axis=1) for k in range(4)]).T
  jac=np.min(math.sqrt(2)*weighted[:,None]/np.sqrt(products),axis=1)
  length=max(self.length(c[i],c[j],nodes) for c in cells for i in range(4) for j in range(i+1,4))
  return min(float(q.min()/.22),float(jac.min()/.055),.95*1.8/length)
