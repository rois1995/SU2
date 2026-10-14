#!/usr/bin/env python3
"""Independent arithmetic check of the directional tangent constraint, not mesh qualification."""
from pathlib import Path
import hashlib,json
import numpy as np

def main():
    random=np.random.default_rng(73);maximum=0.
    for _ in range(100):
        angle=random.uniform(-3,3);c,s=np.cos(angle),np.sin(angle);rotation=np.array([[c,-s],[s,c]])
        covariance=rotation@np.diag([1.,10**random.uniform(-6,0)])@rotation.T;old=np.linalg.inv(covariance)
        direction=random.normal(size=2);direction/=np.linalg.norm(direction);curvature=10**random.uniform(-1,6)
        alpha=max(0.,curvature-1/(direction@covariance@direction));new=old+alpha*np.outer(direction,direction)
        assert np.linalg.eigvalsh(new-old)[0]>=-1e-8*np.linalg.norm(new)
        ratio=curvature*(direction@np.linalg.solve(new,direction));maximum=max(maximum,ratio);assert ratio<=1+2e-10
    path=Path(__file__);result=dict(status='PASS_ARITHMETIC_ONLY',cases=100,seed=73,maximum_inverse_demand_ratio=maximum,
        numpy_version=np.__version__,helper_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        scope='Positive2D covariances, preserved old tensor and directional bound; not C++ runtime or full curved mesh evidence.')
    path.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
