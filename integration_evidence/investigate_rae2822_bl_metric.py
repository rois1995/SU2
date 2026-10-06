"""Attribute saved RAE2822 BL metric complexity; no solver or metric changes."""
import argparse, hashlib, json, math, re, sys
from pathlib import Path
import numpy as np


def flat_wall(h0, growth, depth):
    """Complexity per wall edge: geometric size law versus its two-node P1 tensor."""
    hn=max(h0,2*(h0+(growth-1)*depth)/(growth+1))
    if growth==1 or depth<=h0/2:
        exact=depth/h0
    else:
        exact=.5+(growth+1)/(2*(growth-1))*math.log((h0+(growth-1)*depth)/(h0*(growth+1)/2))
    a,b=1/h0,1/hn
    p1=2*depth/3*(a*a+a*b+b*b)/(a+b)
    nodal=depth*(a+b)/2
    return dict(h0=h0,growth=growth,depth=depth,geometric_integral=exact,
                nodal_quadrature=nodal,p1_tensor_integral=p1,p1_inflation=p1/exact)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('case',type=Path,nargs='?');p.add_argument('--output',type=Path)
    p.add_argument('--self-check',action='store_true');args=p.parse_args()
    if args.self_check:
        for growth in (1,1.2):
            row=flat_wall(.01,growth,.001)
            assert math.isclose(row['geometric_integral'],.1) and math.isclose(row['p1_tensor_integral'],.1)
        row=flat_wall(1e-5,1.2,.01)
        assert 28<row['geometric_integral']<30 and 22<row['p1_inflation']<24
        print('Flat-wall integral checks PASS');return
    if not args.case or not args.output:p.error('case and --output required')
    if args.output.exists():raise RuntimeError('Preserve prior diagnosis; use a fresh output')
    root=Path(__file__).resolve().parent.parent
    sys.path.insert(0,str(root/'TestCases/adaptation/capability'));import capcheck
    wd=args.case.resolve();paths=[wd/'input.su2',wd/'solution_adap_00000.dat',wd/'solver.log',wd/'run.cfg',wd/'run_evidence.json']
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    hashes={str(f):digest(f) for f in paths}
    state=json.loads(paths[-1].read_text());assert state['phase']=='terminal'
    m=capcheck.read_su2(paths[0]);M,_=capcheck.metric_of(m,paths[1]);gates=capcheck.check_validity(m)
    assert all(v[0]=='PASS' for v in gates.values())
    V=capcheck.dual_volumes(m);density=np.sqrt(np.linalg.det(M));contributions=V*density
    total=float(contributions.sum());CM=M[m.E].mean(1);centers=m.P[m.E].mean(1)
    cells=capcheck.volumes(m.P,m.E,2)*np.sqrt(np.linalg.det(CM));centroid_total=float(cells.sum())
    log=paths[2].read_text();printed=float(re.search(r'Mesh complexity with the boundary-layer metric: ([\de.+-]+)',log)[1])
    assert math.isclose(total,printed,rel_tol=1e-6)
    printed_centroid=float(re.search(r'frozen P1 centroid complexity=([\de.+-]+)',log)[1])
    assert math.isclose(centroid_total,printed_centroid,rel_tol=5e-6)
    nodes=[]
    for i in np.argsort(contributions)[-8:][::-1]:
        nodes.append(dict(point_id=int(i),coordinates=m.P[i].tolist(),dual_area=float(V[i]),tensor=M[i].tolist(),
                          principal_sizes=(1/np.sqrt(np.linalg.eigvalsh(M[i]))).tolist(),
                          contribution=float(contributions[i]),fraction=float(contributions[i]/total),
                          incident_cells=np.flatnonzero((m.E==i).any(1)).tolist()))
    top_cells=[]
    for i in np.argsort(cells)[-8:][::-1]:
        top_cells.append(dict(input_cell_id=int(i),point_ids=m.E[i].tolist(),centroid=centers[i].tolist(),
                              coordinates=m.P[m.E[i]].tolist(),contribution=float(cells[i]),fraction=float(cells[i]/centroid_total)))
    config=paths[3].read_text()
    h0=float(re.search(r'ADAP_BL_FIRST_HEIGHT\s*=\s*\(\s*([\de.+-]+)',config)[1])
    growth=float(re.search(r'ADAP_BL_GROWTH\s*=\s*([\de.+-]+)',config)[1])
    record=dict(case=str(wd),input_files_sha256=hashes,checker_sha256=digest(Path(__file__)),
                nodal_complexity=total,centroid_complexity=centroid_total,
                estimated_unit_triangles=4*centroid_total/math.sqrt(3),top_nodes=nodes,top_cells=top_cells,
                top_five_cell_fraction=float(sum(c['fraction'] for c in top_cells[:5])),
                flat_wall_example=flat_wall(h0,growth,.01),
                interpretation='Coarse nodal quadrature and frozen P1 tensors smear a pointwise thin BL/sharp-corner field over large donor areas. Values are consistent with the stored field, but do not resolve the intended geometric size law.',
                limits='Flat-wall example isolates representation error; it is not an integral of the complete RAE2822 sensor-plus-BL field. No modified target or adapted RANS mesh is produced.')
    assert all(digest(Path(f))==h for f,h in hashes.items())
    args.output.write_text(json.dumps(record,indent=2)+'\n')
    print('Nodal complexity',total,'centroid complexity',centroid_total)
    print('Dominant point',nodes[0]['point_id'],nodes[0]['coordinates'],'fraction',nodes[0]['fraction'])
    print('Flat wall:',record['flat_wall_example'])

if __name__=='__main__':main()
