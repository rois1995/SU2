"""Bounded exact surface association and geometric wing-section diagnostics."""
from pathlib import Path
import csv
import hashlib
import json
import sys
import numpy as np

R = Path(__file__).resolve().parent
sys.path.insert(0, str(R.parents[3] / 'TestCases/adaptation/capability'))
import capcheck


def summary(a):
    a = np.asarray(a)
    assert a.size and np.isfinite(a).all()
    return dict(min=float(a.min()), max=float(a.max()),
        quantiles_0_10_50_90_99_100=np.quantile(a, [0,.1,.5,.9,.99,1]).tolist())


def match_wall(wall, surface):
    wk = [tuple(row) for row in wall.astype(np.float32)]
    sk = [tuple(row) for row in surface.astype(np.float32)]
    assert len(set(wk)) == len(wk) and len(set(sk)) == len(sk), 'Float32 coordinate collision'
    index = dict(zip(sk, range(len(sk))))
    assert set(wk) == set(sk), 'surface/wall coordinates differ in Float32 representation'
    return np.array([index[key] for key in wk], dtype=int)


def analyze():
    folder = R / 'm6_runs/rans_seed_continuation303_v1'
    assert json.loads((folder/'status.json').read_text())['state'] == 'terminal'
    assert json.loads((folder/'terminal_handles.json').read_text())['all_owned_handles_terminal']
    mesh = capcheck.read_su2(folder/'Coarse.su2')
    points, fields, precision = capcheck.read_vtu(folder/'surface_flow.vtu')
    assert precision == 'Float32' and all(np.isfinite(v).all() for v in fields.values())
    faces = mesh.M['WING3D']; nodes = np.unique(faces)
    association = match_wall(mesh.P[nodes], points)
    names = ['Pressure_Coefficient','Pressure','Y_Plus','Mach']
    values = np.zeros((len(mesh.P), len(names)))
    values[nodes] = np.column_stack([fields[name][association] for name in names])
    out = folder/'spatial303'; out.mkdir(exist_ok=False)
    reports = []
    for y in [.2,.4,.6,.8,1.0]:
        segments = []
        for face in faces:
            xyz = mesh.P[face]; dy = xyz[:,1]-y
            if dy.min() >= 0 or dy.max() <= 0:
                continue
            hits=[]
            for i,j in [(0,1),(1,2),(2,0)]:
                if dy[i]*dy[j]<0:
                    t=-dy[i]/(dy[j]-dy[i])
                    hits.append(np.r_[xyz[i]+t*(xyz[j]-xyz[i]), values[face[i]]+t*(values[face[j]]-values[face[i]])])
                elif dy[i] == 0:
                    hits.append(np.r_[xyz[i], values[face[i]]])
            assert len(hits)==2, (y,face,hits)
            segments.append(hits)
        segments=np.array(segments)
        assert len(segments) and np.isfinite(segments).all()
        leading=float(segments[:,:,0].min()); trailing=float(segments[:,:,0].max())
        rows=[]
        for x in np.linspace(leading,trailing,203)[1:-1]:
            hits=[]
            for a,b in segments:
                if min(a[0],b[0]) <= x <= max(a[0],b[0]) and a[0]!=b[0]:
                    t=(x-a[0])/(b[0]-a[0]); hits.append(a+t*(b-a))
            assert len(hits)>=2,(y,x,len(hits))
            hits.sort(key=lambda row:row[2])
            for side,h in [('lower',hits[0]),('upper',hits[-1])]:
                rows.append([y,side,x,(x-leading)/(trailing-leading),h[2],*h[3:]])
        with (out/f'section_y{y:.1f}.csv').open('w') as stream:
            writer=csv.writer(stream);writer.writerow(['y','side','x','x_over_local_chord','z',*names]);writer.writerows(rows)
        sides={}
        for side in ['upper','lower']:
            a=np.array([row[2:] for row in rows if row[1]==side],dtype=float)
            cp=a[:,3];dp=np.diff(cp);k=int(np.argmax(dp))
            sides[side]=dict(pressure_coefficient=summary(cp),pressure=summary(a[:,4]),
                y_plus=summary(a[:,5]),mach=summary(a[:,6]),
                largest_positive_adjacent_cp_increment=float(max(0,dp[k])),
                increment_local_chord_interval=a[k:k+2,1].tolist())
        reports.append(dict(y=y,intersected_wall_triangles=len(segments),
            leading_x=leading,trailing_x=trailing,chord=trailing-leading,
            samples_per_side=201,sides=sides))
    report=dict(status='SPATIAL_DIAGNOSTICS_ONLY_UNQUALIFIED',
        surface_nodes=len(points),wall_nodes=len(nodes),all_wall_nodes_uniquely_matched=True,
        coordinate_matching='Exact equality after conversion to Float32, matching existing VTU representation; mesh cuts use original double coordinates.',
        interpolation='Actual triangular wall intersections at fixed y; linear interpolation of nodal fields along cut segments. Upper/lower are maximum/minimum geometric z intersections at each x.',
        wall_y_plus=summary(fields['Y_Plus']),wall_pressure_coefficient=summary(fields['Pressure_Coefficient']),
        wall_pressure=summary(fields['Pressure']),wall_mach=summary(fields['Mach']),sections=reports,
        qualification='No resolved-shock, wall-resolution, physical-convergence or experimental agreement claim. Positive adjacent Cp increments describe sampled chordwise compression only. Parent determines initial adaptation-seed adequacy.',
        inputs_sha256={name:hashlib.sha256((folder/name).read_bytes()).hexdigest() for name in ['Coarse.su2','surface_flow.vtu','restart_flow.dat']})
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    analyze()
