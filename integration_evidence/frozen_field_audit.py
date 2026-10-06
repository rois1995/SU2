"""Independent AABB locator for the *saved original connectivity*, not a retriangulation."""
import math
import numpy as np


class FrozenField:
    def __init__(self, points, cells, tensors, markers, extension_limit=0):
        self.points = np.asarray(points, dtype=np.longdouble)
        self.cells = np.asarray(cells, dtype=np.int64)
        self.tensors = np.asarray(tensors, dtype=np.longdouble)
        xyz = self.points[self.cells]
        self.lo, self.hi = xyz.min(axis=1), xyz.max(axis=1)
        self.centers = (self.lo+self.hi)/2
        self.keys = [tuple(sorted(cell)) for cell in cells]
        self.tree = self.build(np.arange(len(cells)))
        self.cache, self.roundoff, self.extensions = {}, 0, 0
        self.extension_limit = extension_limit
        self.edges = [(tag,edge) for tag,group in sorted(markers.items()) for edge in group]
        self.max_extension = 0

    def build(self, ids):
        lo, hi = self.lo[ids].min(axis=0), self.hi[ids].max(axis=0)
        if len(ids) <= 8:
            return lo,hi,ids,None
        axis = int(np.argmax(np.ptp(self.centers[ids],axis=0)))
        ids = ids[np.argsort(self.centers[ids,axis],kind='stable')]
        middle = len(ids)//2
        return lo,hi,self.build(ids[:middle]),self.build(ids[middle:])

    def candidates(self, p):
        todo, found = [self.tree], []
        # Bounding-box padding is only broad-phase admission. Every actual
        # interpolation/extension below has a separate geometric distance test.
        pad = 32*np.finfo(float).eps*max(1,float(np.max(np.abs(p))))
        while todo:
            lo,hi,left,right = todo.pop()
            if (p < lo-pad).any() or (p > hi+pad).any():
                continue
            if right is None:
                found.extend(left.tolist())
            else:
                todo.extend((left,right))
        return sorted(found,key=lambda i:self.keys[i])

    @staticmethod
    def cross(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])

    def __call__(self, point):
        if point in self.cache:
            return self.cache[point]
        p = np.asarray(point,dtype=np.longdouble)
        candidates = self.candidates(p)
        rounded = []
        for cell in candidates:
            a,b,c = self.points[self.cells[cell]]
            signs = np.asarray([self.cross(p,b,c),self.cross(a,p,c),self.cross(a,b,p)])
            if (signs >= 0).all():
                value = signs @ self.tensors[self.cells[cell]]/signs.sum()
                self.cache[point] = tuple(map(float,value))
                return self.cache[point]
            xyz = (a,b,c)
            outside = 0
            for k in range(3):
                if signs[k] < 0:
                    u,v = xyz[(k+1)%3],xyz[(k+2)%3]
                    outside = max(outside,float(-signs[k]/np.sqrt(np.sum((v-u)**2))))
            scale = max(float(np.max(np.abs(p))),float(np.max(np.abs(xyz))))
            if outside <= 4*np.finfo(float).eps*scale:
                rounded.append((outside,self.keys[cell],cell,signs))
        if rounded:
            _,_,cell,signs = min(rounded,key=lambda row:row[:2])
            weights = np.maximum(signs,0)
            value = weights @ self.tensors[self.cells[cell]]/weights.sum()
            self.cache[point] = tuple(map(float,value))
            self.roundoff += 1
            return self.cache[point]
        nearest = None
        if self.extension_limit:
            for tag,edge in self.edges:
                a,b = self.points[list(edge)]
                delta = b-a
                u = float(np.clip(np.sum((p-a)*delta)/np.sum(delta*delta),0,1))
                distance = float(np.sqrt(np.sum((p-a-u*delta)**2)))
                key = distance,tag,tuple(sorted(edge))
                if nearest is None or key < nearest[0]:
                    nearest = key,edge,u
            if nearest and nearest[0][0] <= self.extension_limit:
                key,edge,u = nearest
                value = (1-u)*self.tensors[edge[0]]+u*self.tensors[edge[1]]
                self.extensions += 1
                self.max_extension = max(self.max_extension,key[0])
                self.cache[point] = tuple(map(float,value))
                return self.cache[point]
        raise ValueError(f'Query outside saved donor domain and permitted physical-face extension: {point}')
