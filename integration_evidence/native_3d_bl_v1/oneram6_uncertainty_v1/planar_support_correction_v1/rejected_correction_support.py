#!/usr/bin/env python3
"""Faceted-wall support proposal; sums overlapping component PDFs, never approximates the target metric."""
import json,math,time
from pathlib import Path
import numpy as np
from scipy.spatial import cKDTree
from run import HERE,ROOT,OLD,sha
from support_bound import wall_geometry,inverse_height_moments


def unit(x):
    norm=np.sqrt(np.sum(x*x,axis=-1));assert np.all(norm>0)
    return x/norm[...,None]


def radial(u,k,h0,growth,radius):
    b=2*(growth-1)/(growth+1);split=min(radius,h0/b) if b else radius
    total=float(inverse_height_moments(h0,growth,radius)[k]);low=split**(k+1)/(h0*(k+1))
    mass=np.asarray(u)*total;value=(mass*h0*(k+1))**(1/(k+1));upper=mass>low
    if np.any(upper):
        rest=mass[upper]-low
        if k==0:value[upper]=split*np.exp(b*rest)
        else:value[upper]=(split**k+k*b*rest)**(1/k)
    return np.clip(value,0.,radius)


class Proposal:
    def __init__(self,fixture,selection=None):
        start=time.monotonic();self.g=wall_geometry(fixture);g=self.g
        self.p=np.asarray(g['p'],float).astype(np.longdouble);self.f=g['f'];self.x=self.p[self.f]
        cross=np.cross(self.x[:,1]-self.x[:,0],self.x[:,2]-self.x[:,0]);norm=np.sqrt(np.sum(cross*cross,axis=1))
        self.n=cross/norm[:,None];self.area=norm/2;self.edges=g['edges'];e=self.edges
        self.edge_a=self.p[e[:,0]];self.edge_vector=self.p[e[:,1]]-self.edge_a
        self.length=np.sqrt(np.sum(self.edge_vector*self.edge_vector,axis=1));self.axis=self.edge_vector/self.length[:,None]
        first=np.full(len(e),-1,dtype=int);second=first.copy()
        for face,ids in enumerate(g['face_edges']):
            for edge in ids:
                if first[edge]<0:first[edge]=face
                else:assert second[edge]<0;second[edge]=face
        def inward(faces):
            opposite=np.array([next(int(k) for k in self.f[face] if k not in set(e[edge])) for edge,face in enumerate(faces)])
            v=self.p[opposite]-self.edge_a;v-=np.sum(v*self.axis,axis=1)[:,None]*self.axis
            return unit(v)
        self.inward1=inward(first);self.inward2=inward(np.where(second>=0,second,first))
        cross=np.cross(self.inward1,self.inward2);sine=np.sqrt(np.sum(cross*cross,axis=1));cosine=np.sum(self.inward1*self.inward2,axis=1)
        self.edge_width=np.arctan2(sine,-cosine)
        valid=self.edge_width>0;self.edge_center=np.zeros_like(self.axis);self.edge_side=np.zeros_like(self.axis)
        self.edge_center[valid]=-unit((self.inward1+self.inward2)[valid]);self.edge_side[valid]=unit(np.cross(self.axis[valid],self.edge_center[valid]))
        neighbors=[[] for _ in self.p]
        for a,b in e:neighbors[a].append(b);neighbors[b].append(a)
        self.va=np.zeros_like(self.p);self.vb=self.va.copy();self.vp=self.va.copy();self.vq=self.va.copy();self.vz=self.va.copy();self.vertex_width=np.zeros(len(self.p),dtype=np.longdouble)
        for i,adj in enumerate(neighbors):
            v=unit(self.p[adj]-self.p[i]);dot=v@v.T;a,b=np.unravel_index(np.argmin(dot),dot.shape)
            self.va[i]=v[a];self.vb[i]=v[b];axis=np.cross(v[a],v[b]);sine=np.sqrt(np.sum(axis*axis));cosine=np.sum(v[a]*v[b])
            width=np.arctan2(sine,-cosine);self.vertex_width[i]=width
            if width==0:continue
            if sine==0:
                # Equal halfspaces form a hemisphere; choose any stable perpendicular axis.
                candidate=np.eye(3,dtype=np.longdouble)[np.argmin(abs(v[a]))];axis=unit(np.cross(v[a],candidate))
            else:axis/=sine
            self.vp[i]=unit(v[a]+v[b]);self.vz[i]=axis;self.vq[i]=unit(np.cross(axis,self.vp[i]))
        # Tangent determinant controls importance only. It is not used in the physical density callback.
        edges=np.roll(self.x,-1,axis=1)-self.x;length=np.sqrt(np.sum(edges*edges,axis=2))
        trace=np.zeros(len(self.p),dtype=np.longdouble);weight=np.zeros_like(trace);face_trace=np.log(np.longdouble(4)/3*norm**2)
        for k in range(3):
            angle=np.arccos(np.clip(-np.sum(edges[:,(k+2)%3]*edges[:,k],axis=1)/(length[:,(k+2)%3]*length[:,k]),-1,1))
            np.add.at(trace,self.f[:,k],angle*face_trace);np.add.at(weight,self.f[:,k],angle)
        assert np.all(weight>0);density=np.exp(-.5*trace/weight)
        coefficients=[np.max(density[self.f],axis=1),np.max(density[e],axis=1),density]
        self.radius=g['thickness'];moments=inverse_height_moments(g['h0'],g['growth'],self.radius)
        self.masses=[2*self.area*moments[0],self.edge_width*self.length*moments[1],2*self.vertex_width*moments[2]]
        enabled=[np.ones(len(a),dtype=bool) for a in self.masses]
        if selection is not None:
            enabled=[np.zeros(len(a),dtype=bool) for a in self.masses];enabled[0][selection]=True
            enabled[1][np.unique(g['face_edges'][selection])]=True;enabled[2][np.unique(self.f[selection])]=True
        weights=[np.asarray(a*b*c,float) for a,b,c in zip(self.masses,coefficients,enabled)]
        raw=np.concatenate(weights);assert np.all(np.isfinite(raw)) and np.all(raw>=0)
        self.offset=np.cumsum([0]+[len(a) for a in self.masses]);active=np.flatnonzero(raw>0)
        self.order=active[np.argsort(raw[active],kind='stable')];total=np.sum(raw,dtype=np.longdouble)
        self.cdf=np.cumsum(raw[self.order]/float(total));self.cdf[-1]=1.
        probability=np.diff(np.r_[0.,self.cdf]);assert np.all(probability>0),'No positive component may be silently dropped.'
        effective=np.zeros_like(raw);effective[self.order]=probability
        self.pdf_coeff=[]
        for k,m in enumerate(self.masses):
            value=np.zeros(len(m));positive=m>0;value[positive]=effective[self.offset[k]:self.offset[k+1]][positive]/np.asarray(m[positive],float);self.pdf_coeff.append(value)
        centers=[self.x.mean(axis=1),(self.edge_a+self.p[e[:,1]])/2,self.p]
        self.bounds=[np.max(np.sqrt(np.sum((self.x-centers[0][:,None])**2,axis=2)),axis=1),self.length/2,np.zeros(len(self.p))]
        self.centers=centers;self.tree_ids=[np.flatnonzero(a>0) for a in self.pdf_coeff]
        self.trees=[cKDTree(np.asarray(a[ids],float)) for a,ids in zip(centers,self.tree_ids)]
        self.preparation_seconds=time.monotonic()-start;self.checked=0
        self.metadata=dict(wall_faces=len(self.f),wall_edges=len(e),wall_vertices=len(self.p),radius=float(self.radius),
            primitive_proposal_masses=[float(np.sum(w,dtype=np.longdouble)) for w in weights],positive_components=len(active),
            minimum_effective_component_probability=float(probability.min()),preparation_seconds=self.preparation_seconds)

    def draw(self,count,seed):
        draws=np.random.default_rng(seed).random((count,5));component=self.order[np.searchsorted(self.cdf,draws[:,0],side='right')]
        family=np.searchsorted(self.offset[1:],component,side='right');index=component-self.offset[family]
        point=np.empty((count,3),dtype=np.longdouble);self.selected_pdf=np.empty(count)
        for k in range(3):
            mask=family==k;ids=index[mask];u=draws[mask];r=radial(u[:,3],k,self.g['h0'],self.g['growth'],self.radius)
            b=2*(self.g['growth']-1)/(self.g['growth']+1)
            self.selected_pdf[mask]=self.pdf_coeff[k][ids]/np.maximum(self.g['h0'],b*r)
            if k==0:
                a=np.sqrt(u[:,1]);bary=np.column_stack([1-a,a*(1-u[:,2]),a*u[:,2]])
                point[mask]=np.sum(self.x[ids]*bary[:,:,None],axis=1)+np.where(u[:,4]<.5,-r,r)[:,None]*self.n[ids]
            elif k==1:
                phi=(u[:,2]-.5)*self.edge_width[ids];direction=np.cos(phi)[:,None]*self.edge_center[ids]+np.sin(phi)[:,None]*self.edge_side[ids]
                point[mask]=self.edge_a[ids]+u[:,1,None]*self.edge_vector[ids]+r[:,None]*direction
            else:
                z=2*u[:,1]-1;phi=(u[:,2]-.5)*self.vertex_width[ids];radial_part=np.sqrt(np.maximum(0,1-z*z))
                direction=radial_part[:,None]*(-np.cos(phi)[:,None]*self.vp[ids]+np.sin(phi)[:,None]*self.vq[ids])+z[:,None]*self.vz[ids]
                point[mask]=self.p[ids]+r[:,None]*direction
        return np.asarray(point,float),family,index

    def pdf(self,points,maximum_checks=80000000,maximum_seconds=90.):
        started=time.monotonic();result=np.zeros(len(points));counts=np.zeros(3,dtype=np.int64)
        scale=max(1.,float(abs(self.p).max()));padding=256*np.finfo(float).eps*scale
        for begin in range(0,len(points),128):
            part=np.asarray(points[begin:begin+128],dtype=np.longdouble);values=np.zeros(len(part),dtype=np.longdouble)
            for k in range(3):
                candidates=self.trees[k].query_ball_point(np.asarray(part,float),float(np.max(self.bounds[k])+self.radius+padding))
                for row,ids in enumerate(candidates):
                    if len(ids)>32768:raise RuntimeError('Proposal PDF candidate ceiling reached; do not omit candidates.')
                    counts[k]+=len(ids)
                    if counts.sum()>maximum_checks or time.monotonic()-started>maximum_seconds:raise RuntimeError('Proposal PDF work limit reached; retain failure, no truncated PDF.')
                    ids=self.tree_ids[k][np.array(ids,dtype=np.int64)]
                    if not len(ids):continue
                    delta=part[row]-self.centers[k][ids];keep=np.sum(delta*delta,axis=1)<=(self.bounds[k][ids]+self.radius+padding)**2;ids=ids[keep]
                    if not len(ids):continue
                    if k==0:
                        u=self.x[ids,1]-self.x[ids,0];v=self.x[ids,2]-self.x[ids,0];w=part[row]-self.x[ids,0]
                        cross=np.cross(u,v);square=np.sum(cross*cross,axis=1)
                        a=np.sum(np.cross(w,v)*cross,axis=1)/square;b=np.sum(np.cross(u,w)*cross,axis=1)/square
                        distance=abs(np.sum(w*self.n[ids],axis=1));inside=(a>=0)&(b>=0)&(a+b<=1)
                    elif k==1:
                        w=part[row]-self.edge_a[ids];z=np.sum(w*self.axis[ids],axis=1);rad=w-z[:,None]*self.axis[ids]
                        distance=np.sqrt(np.sum(rad*rad,axis=1));inside=(z>=0)&(z<=self.length[ids])&(np.sum(rad*self.inward1[ids],axis=1)<=0)&(np.sum(rad*self.inward2[ids],axis=1)<=0)
                    else:
                        w=part[row]-self.p[ids];distance=np.sqrt(np.sum(w*w,axis=1));inside=(np.sum(w*self.va[ids],axis=1)<=0)&(np.sum(w*self.vb[ids],axis=1)<=0)
                    inside&=distance<=self.radius
                    d=distance[inside];b=2*(self.g['growth']-1)/(self.g['growth']+1)
                    profile=1/np.maximum(self.g['h0'],b*d)
                    values[row]+=np.sum(self.pdf_coeff[k][ids[inside]]*profile,dtype=np.longdouble)
            result[begin:begin+len(part)]=values
        if not np.all(np.isfinite(result)) or not np.all(result>0):raise RuntimeError('Generated point has no finite full-mixture PDF; do not discard it.')
        if hasattr(self,'selected_pdf') and len(self.selected_pdf)==len(result):
            self.maximum_selected_pdf_excess=float(np.max(self.selected_pdf/result-1))
            if self.maximum_selected_pdf_excess>1e-8:raise RuntimeError('Full mixture PDF omits a sampled component; do not ignore it.')
        self.pdf_checks=counts.tolist();self.pdf_seconds=time.monotonic()-started
        return result


def selfcheck():
    for growth in (1.,1.+1e-12,1.2,2.):
        h0=2e-6;R=.01;b=2*(growth-1)/(growth+1);split=min(R,h0/b) if b else R
        for k in range(3):
            u=np.array([0.,.1,.5,.9,1.]);r=radial(u,k,h0,growth,R);mass=np.minimum(r,split)**(k+1)/(h0*(k+1));above=r>split
            if np.any(above):mass[above]+=(np.log(r[above]/split)/b if k==0 else (r[above]**k-split**k)/(b*k))
            assert np.allclose(mass/float(inverse_height_moments(h0,growth,R)[k]),u,rtol=3e-13,atol=3e-13)
    assert abs(float(inverse_height_moments(1,1,1)[0])-1)<1e-15


def geometry_selfcheck(folder):
    folder.mkdir(exist_ok=True);fixture=folder/'two_triangles.dat'
    body='NATIVE_BL_REPLAY_3D 2\n6 2\n0.01 1 0.1\n0 0 0\n1 0 0\n0 1 0\n0 0 0.05\n1 0 0.05\n0 1 0.05\n0 1 2\n3 4 5\n'
    if fixture.exists():assert fixture.read_text()==body
    else:fixture.write_text(body)
    proposal=Proposal(fixture)
    point=np.array([[.25,.25,.025],[.25,.25,.12]])
    actual=proposal.pdf(point)
    expected=np.array([sum(proposal.pdf_coeff[0])/.01,proposal.pdf_coeff[0][1]/.01])
    assert np.allclose(actual,expected,rtol=1e-13,atol=1e-13),('overlapping face PDFs must sum',actual,expected)
    points,family,index=proposal.draw(512,123);density=proposal.pdf(points)
    assert set(family)=={0,1,2} and np.all(density>0)
    result=dict(status='PASS',radial_inverse_selfchecks=True,overlapping_face_PDF=True,all_feature_families_sampled=True,
        maximum_selected_component_pdf_excess=proposal.maximum_selected_pdf_excess,fixture_sha256=sha(fixture),producer_sha256=sha(Path(__file__)))
    (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');return result


if __name__=='__main__':selfcheck()
