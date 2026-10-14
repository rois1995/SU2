#!/usr/bin/env python3
"""Analytic curved-wall/first-height compatibility experiment, not native output."""
import csv
import hashlib
import json
import math
from pathlib import Path
HERE = Path(__file__).resolve().parent

def right_tet(t):
    # Unit normal metric leg and two equal tangential metric legs t.
    q = 4 * (t*t/2)**(2/3) / (2*t*t+1)
    j = math.sqrt(2)*min(1, 1/math.sqrt(2*(t*t+1)), t*t/(t*t+1))
    length = max(math.sqrt(2)*t, math.hypot(t,1))
    return q,j,length

def write_sphere(path, radius, theta, h):
    z=radius*math.cos(theta);rho=radius*math.sin(theta)
    points=[(rho*math.cos(a),rho*math.sin(a),z) for a in (0,2*math.pi/3,4*math.pi/3)]+[(0.,0.,z+h)]
    # CCW wall triangle and outward apex give a positive coordinate determinant.
    with path.open('w') as f:
        f.write('NDIME= 3\nNELEM= 1\n10 0 1 2 3 0\nNPOIN= 4\n')
        for i,p in enumerate(points):f.write(' '.join(format(v,'.17g') for v in p)+f' {i}\n')
        f.write('NMARK= 2\nMARKER_TAG= wall\nMARKER_ELEMS= 1\n5 0 2 1\nMARKER_TAG= artificial\nMARKER_ELEMS= 3\n5 1 2 3\n5 0 3 2\n5 0 1 3\n')

def main():
    assert right_tet(1)[0]>.2 and right_tet(1)[1]>.05 and right_tet(1)[2]<1.8
    rows=[]
    for radius in (.1,1.,1000.):
        for ratio in (1e-2,1e-3,1e-4,1e-5):
            h=radius*ratio;epsilon=.1*h
            # Exact circular chord defect; this size is a surface-resolution bound,
            # not an imposed new production target or a claimed complete mesher.
            chord=2*math.sqrt(epsilon*(2*radius-epsilon))
            theta=math.pi/6;defect=2*radius*math.sin(theta/2)**2
            t=chord/radius;q,j,length=right_tet(t)
            # For a symmetric spherical triangle, the normal-centroid apex has
            # exact facet altitude h but signed analytic clearance h-defect.
            assert h-defect<0
            rows.append(dict(radius=radius,h0=h,coarse_angle_deg=30.,coarse_surface_defect=defect,
                facet_height_over_h0=1.,foot_barycentric=1/3,coarse_signed_smooth_clearance=h-defect,
                coarse_clearance_over_h0=(h-defect)/h,surface_error_budget=epsilon,
                resolving_chord=chord,tangent_length_over_reference_size=t,
                unchanged_tangent_metric_q=q,unchanged_tangent_metric_j=j,unchanged_tangent_metric_L=length,
                geometry_scaled_proxy_q=right_tet(1)[0],geometry_scaled_proxy_j=right_tet(1)[1],geometry_scaled_proxy_L=right_tet(1)[2]))
    assert all(r['unchanged_tangent_metric_j']<.05 for r in rows)
    # Dimensionless conclusions must be independent of the chosen length unit.
    for k in range(4):
        reference=rows[k]
        for r in (rows[4+k],rows[8+k]):
            for key in ['coarse_clearance_over_h0','unchanged_tangent_metric_q','unchanged_tangent_metric_j']:
                assert math.isclose(r[key],reference[key],rel_tol=2e-12)
    with (HERE/'sweep.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
    write_sphere(HERE/'coarse_spherical_normal_row_rejected.su2',1.,math.pi/6,.001)
    write_sphere(HERE/'resolved_spherical_normal_row_geometry_only.su2',1.,math.acos(1-.0001),.001)
    result=dict(status='PASS_DIAGNOSTIC_ONLY',experiment_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=len(rows),
        native_measure_source_sha256=hashlib.sha256((HERE.parents[2]/'Common/src/adaptation/CNativePredicates3D.cpp').read_bytes()).hexdigest(),
        statements=[
            'Exact facet altitude and normal-foot containment alone do not establish positive clearance to a smooth curved wall.',
            'Normal-centroid construction on the declared coarse sphere lies inside the analytic wall in all12 cases; this rejects that construction, not the adaptive problem.',
            'Refining to resolve a surface error of0.1*h0 can violate scaled-J and quality in an unchanged coarse tangential metric; geometry resolution, the frozen target and reconstruction must be coordinated.',
            'A curvature-scaled tangential proxy admits the illustrative right tet with q/J/L gates; this is a mathematical proxy, not a production metric implementation or a remeshing certificate.'
        ],limits='Circular/symmetric-sphere geometry and right-tet metric proxy. No native construction, complete geometry, curved integration, layer stack, CFD, MPI or performance qualification. No production target/fade/height change.',
        artifacts={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.iterdir() if p.suffix in ('.csv','.su2')})
    (HERE/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS diagnostic:',len(rows),'scale/height cases; native curved BL remains unsupported')
if __name__=='__main__':main()
