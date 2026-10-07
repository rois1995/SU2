"""Independent saved original-P1 sensor plus geometric 2D BL metric audit.

Only RAE Euler without BL or one AIRFOIL wall with METRIC BL (no tangential
coarsening or experimental corner restriction) is supported.
Geometry/topology/solution checks remain in inspect_rae2822_solution.py.
"""
import argparse, hashlib, json, math, re, sys, time
from pathlib import Path
import numpy as np
from frozen_field_audit import FrozenField
from airfoil_reference_audit import loops
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'TestCases/adaptation/capability'))
import capcheck


def intersection(a,b):
    val,vec=np.linalg.eigh(a)
    assert val[0]>0 and np.isfinite(val).all()
    sqrt=(vec*np.sqrt(np.maximum(val,1e-16)))@vec.T
    inverse=(vec/np.sqrt(np.maximum(val,1e-16)))@vec.T
    val,vec=np.linalg.eigh(inverse@b@inverse)
    out=sqrt@((vec*np.maximum(val,1))@vec.T)@sqrt
    return (out+out.T)/2


class GeometricWall:
    def __init__(self,points,edges,h,g,thickness,angle,core):
        self.points=np.asarray(points);self.edges=np.asarray(edges);self.h=h;self.g=g
        self.thickness=thickness;self.core=core
        self.a=self.points[self.edges[:,0]];self.b=self.points[self.edges[:,1]]
        self.delta=self.b-self.a;self.length=np.linalg.norm(self.delta,axis=1)
        self.rays={};self.incident={}
        for i,(u,v) in enumerate(self.edges):
            self.rays.setdefault(u,[]).append(self.delta[i]/self.length[i])
            self.rays.setdefault(v,[]).append(-self.delta[i]/self.length[i])
            self.incident.setdefault(u,[]).append(i);self.incident.setdefault(v,[]).append(i)
        self.normals={};self.corners=[]
        threshold=max(1e-10,2*math.sin(math.radians(angle)/2))
        for node,rays in self.rays.items():
            assert len(rays)==2
            bisector=sum(rays);size=np.linalg.norm(bisector)
            self.normals[node]=bisector/size if size>1e-10 else np.array([-rays[0][1],rays[0][0]])
            if size>threshold:self.corners.append((self.points[node],size/2))
        self.lo=self.a.min(0)-thickness;self.hi=self.a.max(0)+thickness

    def __call__(self,p,sensor):
        p=np.asarray(p)
        if (p<self.lo).any() or (p>self.hi).any():return sensor
        along=np.clip(np.sum((p-self.a)*self.delta,axis=1)/self.length**2,0,1)
        radial=p-self.a-along[:,None]*self.delta
        distances=np.linalg.norm(radial,axis=1);i=int(np.argmin(distances));distance=distances[i]
        if along[i] in (0,1):
            node=self.edges[i,int(along[i])]
            for other in self.incident[node]:
                tie=abs(distances[other]-distance)<=1e-12*distance
                nearer=np.sum((p-(self.a[other]+self.b[other])/2)**2)<np.sum((p-(self.a[i]+self.b[i])/2)**2)
                if (distances[other]<distance and not tie) or (tie and nearer):
                    i=other;distance=min(distance,distances[other])
        full=max(self.h,.9*self.thickness);width=self.thickness-full
        fade=float(np.clip((distance-full)/width,0,1)) if width else float(distance>full)
        weight=1-fade*fade*(3-2*fade)
        if weight<=0:return sensor
        tangent=self.delta[i]/self.length[i];normal=np.array([-tangent[1],tangent[0]])
        if along[i] in (0,1):
            radius=np.linalg.norm(radial[i]);node=self.edges[i,int(along[i])]
            normal=radial[i]/radius if radius>1e-12*self.length[i] else self.normals[node]
        tangent=np.array([-normal[1],normal[0]])
        ht=self.length[i]
        for corner,turn in self.corners:
            r=np.linalg.norm(p-corner)
            if r<=.5*self.length[i]:ht=min(ht,max(self.h,2*r/turn))
        hn=max(self.h,2*(self.h+(self.g-1)*distance)/(self.g+1))
        # Interpolate eigenvalues in log space during the outer fade.
        lt=math.exp(weight*math.log(1/ht**2)+(1-weight)*math.log(self.core))
        ln=math.exp(weight*math.log(1/hn**2)+(1-weight)*math.log(self.core))
        wall=lt*np.outer(tangent,tangent)+ln*np.outer(normal,normal)
        a=np.array([[sensor[0],sensor[1]],[sensor[1],sensor[2]]])
        out=intersection(a,wall)
        return float(out[0,0]),float(out[0,1]),float(out[1,1])


def check_math():
    assert np.allclose(intersection(np.diag([4.,1.]),np.diag([1.,9.])),np.diag([4.,9.]))
    wall=GeometricWall([[0,0],[1,0],[1,1],[0,1]],[[0,1],[1,2],[2,3],[3,0]],.01,1.2,.1,45,1.)
    xx,xy,yy=wall([.5,.005],(4.,0.,1.));assert abs(xx-4)<1e-10 and abs(xy)<1e-10 and abs(yy-10000)<1e-6
    assert wall([.5,.2],(4.,0.,1.))==(4.,0.,1.)
    # Oblique intersection must dominate each input in every direction.
    a=np.array([[10.,2.],[2.,1.]]);b=np.array([[2.,-3.],[-3.,10.]])
    c=intersection(a,b);assert min(np.linalg.eigvalsh(c-a))>-1e-12 and min(np.linalg.eigvalsh(c-b))>-1e-12


def audit(wd,cycle,donorpath=None,candidatepath=None,restart=None):
    cfg=(wd/'run.cfg').read_text()
    def setting(name):return re.search(r'^'+name+r'\s*=\s*(.*)',cfg,re.M)[1].strip()
    assert setting('ADAP_REMESHER')=='NATIVE_CAVITY'
    has_layer=bool(re.search(r'^ADAP_BL_MARKER\s*=',cfg,re.M))
    if has_layer:
        assert setting('ADAP_BL_METHOD')=='METRIC' and setting('ADAP_BL_MARKER').strip('( )')=='AIRFOIL'
        h=float(setting('ADAP_BL_FIRST_HEIGHT').strip('( )'));g=float(setting('ADAP_BL_GROWTH'));thickness=float(setting('ADAP_BL_THICKNESS'))
    angle=float(setting('ADAP_ANGLE'));extension=float(setting('ADAP_HAUSD'))
    if donorpath is None:donorpath=wd/('input.su2' if cycle==1 else f'mesh_adap_{cycle-1:05d}.su2')
    if candidatepath is None:candidatepath=wd/f'mesh_adap_{cycle:05d}.su2'
    if restart is None:restart=wd/f'solution_adap_{cycle-1:05d}.dat'
    donor=capcheck.read_su2(donorpath);candidate=capcheck.read_su2(candidatepath);original=capcheck.read_su2(wd/'input.su2')
    metric,_=capcheck.metric_of(donor,restart)
    xx=metric[:,0,0].astype(np.longdouble);yy=metric[:,1,1].astype(np.longdouble);xy=metric[:,0,1].astype(np.longdouble)
    largest=(xx+yy+np.hypot(xx-yy,2*xy))/2;core=float(np.min((xx*yy-xy*xy)/largest));assert core>0
    line=loops(original.P,original.E,original.M)['AIRFOIL'][0];edges=list(zip(line,line[1:]+line[:1]))
    wall=GeometricWall(original.P,edges,h,g,thickness,angle,core) if has_layer else None
    frozen=FrozenField(donor.P,donor.E,np.stack([xx,xy,yy],axis=1),donor.M,extension)
    cache={}
    def target(p):
        key=tuple(map(float,p))
        if key not in cache:cache[key]=wall(p,frozen(key)) if wall is not None else frozen(key)
        return cache[key]
    minq=1.;maxl=0.;badq=[];badl=[]
    for i,ids in enumerate(candidate.E):
        p=candidate.P[ids];xx,xy,yy=target(p.mean(0));delta=np.roll(p,-1,axis=0)-p
        denom=np.sum(xx*delta[:,0]**2+2*xy*delta[:,0]*delta[:,1]+yy*delta[:,1]**2)
        area2=np.cross(p[1]-p[0],p[2]-p[0]);assert area2>0
        q=float(2*math.sqrt(3)*area2*math.sqrt(xx*yy-xy*xy)/denom);minq=min(minq,q)
        if q<.18-1e-8:badq.append(i)
    edges=np.unique(np.sort(np.vstack([candidate.E[:,[0,1]],candidate.E[:,[1,2]],candidate.E[:,[2,0]]]),axis=1),axis=0)
    for edge in edges:
        a,b=candidate.P[edge];d=b-a;values=[]
        for p in [a,(a+b)/2,b]:
            xx,xy,yy=target(p);values.append(math.sqrt(xx*d[0]**2+2*xy*d[0]*d[1]+yy*d[1]**2))
        length=(values[0]+4*values[1]+values[2])/6;maxl=max(maxl,length)
        if length>1.8+1e-8:badl.append(edge.tolist())
    paths=[donorpath,candidatepath,restart,wd/'input.su2',wd/'run.cfg',Path(__file__),Path(__file__).with_name('frozen_field_audit.py'),Path(__file__).with_name('airfoil_reference_audit.py')]
    return dict(cycle=cycle,points=len(candidate.P),triangles=len(candidate.E),min_quality=minq,max_simpson_length=maxl,
      bad_quality_cells=badq,bad_length_edges=badl,numerical_gate_tolerance=1e-8,core_eigenvalue=core,
      reference_corners=len(wall.corners) if wall is not None else 0,unique_target_queries=len(cache),boundary_extensions=frozen.extensions,
      maximum_extension=frozen.max_extension,roundoff_queries=frozen.roundoff,
      input_sha256={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
      scope='Independent original-connectivity P1 sensor, original geometric wall constraints, centroid q and Simpson edge lengths. Does not validate CFD convergence or arbitrary configurations.')


if __name__=='__main__':
    check_math()
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('case',type=Path);parser.add_argument('--cycles',nargs='+',type=int,default=[1,2]);args=parser.parse_args()
    for cycle in args.cycles:
        assert cycle in (1,2)
        output=args.case/f'independent_composite_metric_{cycle:05d}.json';assert not output.exists(),'Preserve existing audit evidence'
        start=time.monotonic();row=audit(args.case.resolve(),cycle);row['elapsed_seconds']=time.monotonic()-start
        output.write_text(json.dumps(row,indent=2)+'\n');print(cycle,row['min_quality'],row['max_simpson_length'],len(row['bad_quality_cells']),len(row['bad_length_edges']),flush=True)
        assert not row['bad_quality_cells'] and not row['bad_length_edges'],str(output)
