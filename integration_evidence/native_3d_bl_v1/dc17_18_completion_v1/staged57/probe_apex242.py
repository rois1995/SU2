"""Read-only floating feasibility proposals, never native projection or mesh acceptance."""
from pathlib import Path
import hashlib
import importlib.util
import itertools
import json
import math
import time
import numpy as np

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
spec = importlib.util.spec_from_file_location("boundary_preparation", ROOT / "prepare_boundary202.py")
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)


def main():
    started = time.time()
    owned = RUN / "current_witnesses_v1/geometry_bad_1_owned.bin"
    reference = RUN / "inspection/reference_records.jsonl"
    paths = [Path(__file__), ROOT / "prepare_boundary202.py", ROOT / "inspect_native_failure3d.py", owned, reference]
    before = {str(p): old.sha256(p) for p in paths}
    rows = list(old.reader._read_owned(owned, 52, 128 << 20))
    edge = {7868, 7876}
    ring = [c for c in rows if edge <= {n["id"] for n in c["nodes"]}]
    boundary = {c["nodes"][i]["id"] for c in rows for k, f in enumerate(c["physical"]) if f["present"] for i in old.FACES[k]}
    apices = {n["id"] for c in ring for n in c["nodes"]} - boundary - edge
    assert len(apices) == 1
    apex = apices.pop()
    star = [c for c in rows if apex in {n["id"] for n in c["nodes"]}]
    nodes = {n["id"]: np.array(n["xyz"]) for c in rows for n in c["nodes"]}
    origin = nodes[apex]
    scale = max(np.linalg.norm(p-origin) for c in star for n in c["nodes"] for p in [nodes[n["id"]]])
    assert scale > 0 and len(ring) == 2
    facet, = [json.loads(line) for line in reference.open() if '"id":16734,' in line or '"id": 16734,' in line]
    net = np.array(facet["controls"])
    midpoint = (nodes[7868]+nodes[7876])/2
    curve_half = (net[0]+net[1]+3*net[3]+3*net[4])/8
    powers = [(3,0,0),(0,3,0),(0,0,3),(2,1,0),(1,2,0),(0,2,1),(0,1,2),(1,0,2),(2,0,1),(1,1,1)]
    def surface(uv):
        w = [1-uv.sum(), uv[0], uv[1]]
        value = np.zeros(3)
        gradient = np.zeros((3,2))
        for p, exponent in zip(net,powers):
            factor = math.factorial(3)/math.prod(math.factorial(e) for e in exponent)
            value += factor*math.prod(w[k]**exponent[k] for k in range(3))*p
            for dim in range(2):
                derivative = 0.
                for k, sign in [(0,-1),(dim+1,1)]:
                    if exponent[k]:
                        derivative += sign*exponent[k]*math.prod(w[j]**(exponent[j]-int(j==k)) for j in range(3))
                gradient[:,dim] += factor*derivative*p
        return value, gradient
    uv = np.array([.5,0.])
    for _ in range(30):
        point, jac = surface(uv)
        delta = np.linalg.lstsq(jac,point-midpoint,rcond=None)[0]
        changed = False
        for step in [1.,.5,.25,.125,.0625]:
            trial = np.maximum(0.,uv-step*delta)
            if trial.sum()>1: trial /= trial.sum()
            if np.linalg.norm(surface(trial)[0]-midpoint) < np.linalg.norm(point-midpoint):
                uv=trial;changed=True;break
        if not changed:break
    closest_proxy = surface(uv)[0]
    def solve(point):
        children = []
        for cell in star:
            ids = [n["id"] for n in cell["nodes"]]
            if edge <= set(ids):
                for endpoint in sorted(edge):
                    children.append([(900000000,point) if n==endpoint else (n,nodes[n]) for n in ids])
            else:children.append([(n,nodes[n]) for n in ids])
        coefficients, offsets = [], []
        for cell in children:
            index, = [i for i,n in enumerate(cell) if n[0]==apex]
            face = [cell[i][1] for i in old.FACES[index]]
            normal = -np.cross(face[1]-face[0],face[2]-face[0])
            length = np.linalg.norm(normal)
            assert length>0
            normal /= length
            coefficients.append(normal)
            offsets.append(np.dot(normal,origin-face[0])/scale)
        # A generous fixed box bounds this diagnostic enumeration, never a production permission.
        for k in range(3):
            for sign in [-1,1]:
                normal = np.zeros(3);normal[k]=sign
                coefficients.append(normal);offsets.append(2.)
        matrix = np.column_stack([coefficients,-np.ones(len(coefficients))])
        offsets = np.array(offsets)
        best = None
        for active in itertools.combinations(range(len(offsets)),4):
            block = matrix[list(active)]
            if abs(np.linalg.det(block))<1e-13:continue
            proposal = np.linalg.solve(block,-offsets[list(active)])
            if best is not None and proposal[3]<=best[3]:continue
            if np.min(matrix@proposal+offsets)<-1e-10:continue
            best=proposal
        assert best is not None
        candidate = origin+scale*best[:3]
        represented = [[dict(id=n,xyz=(candidate if n==apex else p).tolist()) for n,p in c] for c in children]
        return {"point_proxy":point.tolist(),"apex_proposal":candidate.tolist(),
                "floating_margin":float(scale*best[3]),"represented_children_exact_positive":all(old.exact_positive(c) for c in represented),
                "child_cells":len(children),"box_half_width":2*scale}
    results={name:solve(point) for name,point in [("unprojected_midpoint_control",midpoint),("cubic_edge_half_proxy",curve_half),("single_facet_closest_proxy",closest_proxy)]}
    assert results["unprojected_midpoint_control"]["floating_margin"]>0
    assert results["unprojected_midpoint_control"]["represented_children_exact_positive"]
    assert before == {str(p):old.sha256(p) for p in paths}
    result={"seconds":time.time()-started,"numpy":np.__version__,"input_sha256":before,"apex":apex,"complete_apex_star_cells":len(star),"results":results,
            "limits":"Floating, bounded one-apex retained-connectivity feasibility PROPOSALS only. Cubic/closest proxies are not the native BindNewPoint projection. No infeasibility certificate or geometry/quality/admissible-mesh proof; exact positivity applies only to represented proxy children. No production change or dependency closure inference."}
    (RUN/"apex242_feasibility_proposals.json").write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))


if __name__ == "__main__":main()
