"""Independent original-polyline correspondence and feature audit for the airfoil gate."""
import math
from audit_native_bl import mesh, cross


def loops(points, cells, markers):
    physical={tuple(sorted(e)):tag for tag,group in markers.items() for e in group}
    directed={tag:{} for tag in markers}
    for ids in cells:
        for k in range(3):
            a,b=ids[k],ids[(k+1)%3]
            tag=physical.get(tuple(sorted((a,b))))
            if tag is not None:
                assert a not in directed[tag], 'Multiple successors on physical boundary'
                directed[tag][a]=b
    groups={}
    for tag,successor in directed.items():
        remaining=set(successor)
        groups[tag]=[]
        while remaining:
            start=min(remaining); node=start; line=[]
            while node in remaining:
                line.append(node); remaining.remove(node); node=successor[node]
            assert node==start, 'Open/nonmanifold physical component'
            groups[tag].append(line)
    return groups


def reference_audit(original_path, candidate_path):
    original,oc,om=mesh(original_path)
    points,cells,markers=mesh(candidate_path)
    old_groups=loops(original,oc,om); new_groups=loops(points,cells,markers)
    assert set(old_groups)==set(new_groups), 'Marker identity changed'
    rows=[]
    for tag,groups in old_groups.items():
        # This gate has one airfoil and one outer component. General same-marker
        # multi-component behavior has separate native kernel controls.
        assert len(groups)==len(new_groups[tag])==1
        ids=groups[0]; line=[original[i] for i in ids]; line.append(line[0])
        arc=[0.]
        for a,b in zip(line,line[1:]): arc.append(arc[-1]+math.dist(a,b))
        period=arc[-1]
        def locate(p):
            chosen=None
            for k,(a,b) in enumerate(zip(line,line[1:])):
                dx,dy=b[0]-a[0],b[1]-a[1]
                u=max(0.,min(1.,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/(dx*dx+dy*dy)))
                q=(a[0]+u*dx,a[1]+u*dy)
                key=(math.dist(p,q),arc[k]+u*(arc[k+1]-arc[k]),k)
                if chosen is None or key<chosen: chosen=key
            return chosen[:2]
        def at(u):
            u%=period
            for k in range(len(arc)-1):
                if u<=arc[k+1]:
                    t=(u-arc[k])/(arc[k+1]-arc[k]);a,b=line[k:k+2]
                    return (a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1]))
            raise AssertionError('Invalid polyline parameter')
        feature_points=[]
        for k,p in enumerate(line[:-1]):
            previous=line[(k-1)%len(ids)];following=line[(k+1)%len(ids)]
            a=(p[0]-previous[0],p[1]-previous[1]);b=(following[0]-p[0],following[1]-p[1])
            turning=math.atan2(abs(a[0]*b[1]-a[1]*b[0]),a[0]*b[0]+a[1]*b[1])
            if turning>=math.pi/4: feature_points.append(p)
        new_ids=new_groups[tag][0]; coords={points[i] for i in new_ids}
        parameters={i:locate(points[i]) for i in new_ids}
        deviation=0.;covered_arc=0.
        for k,i in enumerate(new_ids):
            j=new_ids[(k+1)%len(new_ids)];a,b=points[i],points[j]
            first,last=parameters[i][1],parameters[j][1]
            if last<=first: last+=period
            covered_arc+=last-first
            samples=[first,last]+[u+wrap*period for u in arc for wrap in (0,1)
                                      if first<u+wrap*period<last]
            for u in samples:
                t=(u-first)/(last-first)
                segment=(a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1]))
                deviation=max(deviation,math.dist(at(u),segment))
        rows.append({'marker':tag,'original_components':len(groups),
                     'candidate_components':len(new_groups[tag]),'original_faces':len(ids),
                     'candidate_faces':len(new_ids),'covered_arc_over_period':covered_arc/period,
                     'max_corresponding_parameter_deviation':deviation,
                     'max_vertex_projection_distance':max(v[0] for v in parameters.values()),
                     'declared_feature_coordinates':feature_points,
                     'all_declared_features_exactly_retained':all(p in coords for p in feature_points),
                     'leading_extremum_shift':min(p[0] for p in coords)-min(p[0] for p in line)
                                            if tag=='airfoil' else None})
    return rows
