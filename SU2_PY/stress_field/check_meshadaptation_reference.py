#!/usr/bin/env python3
"""Compare saved-field interpolation with MeshAdaptation's existing weight kernel."""
import argparse
import ast
import hashlib
from pathlib import Path
import subprocess
import tempfile
import numpy as np
from field_io import write_field


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--geometry-helper",required=True);p.add_argument("--probe",required=True);args=p.parse_args()
    source=Path(args.geometry_helper).read_text()
    # Load only the pure function, without importing unrelated adaptation code or invoking Numba.
    function=next(n for n in ast.parse(source).body if isinstance(n,ast.FunctionDef) and n.name=="get_barycentric_weights")
    function.decorator_list=[];namespace={"np":np}
    exec(compile(ast.Module(body=[function],type_ignores=[]),args.geometry_helper,"exec"),namespace)
    barycentric=namespace[function.name];rng=np.random.default_rng(371)
    definition={"scenario":"mesh_adaptation_reference","U_ref":1,"L_ref":1,"epsilon":10,"ell":.1,
                "ell_w":0,"geometry":{"type":"NONE"},"support":{"type":"NONE"}}
    maximum=0.;count=0
    with tempfile.TemporaryDirectory() as temp:
        for dim in (2,3):
            for _ in range(10):
                transform=rng.normal(size=(dim,dim))
                if abs(np.linalg.det(transform))<.05:transform+=2*np.eye(dim)
                if np.linalg.det(transform)<0:transform[0]*=-1
                nodes=np.vstack((np.zeros(dim),np.eye(dim)))@transform+rng.normal(size=dim)
                raw=rng.normal(size=(dim+1,5))*.1
                if dim==2:raw[:,3:]=0
                weights=rng.dirichlet(np.ones(dim+1),10);points=weights@nodes
                coefficients=np.array([barycentric(nodes,x,dim)@raw for x in points])
                expected=np.column_stack((coefficients[:,0]/np.sqrt(2)+coefficients[:,1]/np.sqrt(6),
                                          -coefficients[:,0]/np.sqrt(2)+coefficients[:,1]/np.sqrt(6),
                                          -2*coefficients[:,1]/np.sqrt(6),coefficients[:,2:]/np.sqrt(2)))
                artifact=Path(temp)/"test.suf";write_field(artifact,nodes,np.arange(dim+1)[None,:],raw,definition)
                xyz=points if dim==3 else np.column_stack((points,np.zeros(len(points))))
                result=subprocess.run([args.probe,str(artifact)],input="\n".join(" ".join(map(str,x)) for x in xyz)+"\n",
                                      text=True,capture_output=True,check=True)
                actual=np.array([list(map(float,row.split())) for row in result.stdout.splitlines()])
                np.testing.assert_allclose(actual,expected,rtol=1e-11,atol=1e-12)
                maximum=max(maximum,float(np.max(abs(actual-expected))));count+=len(points)
    print(f"PASS: {count} queries; max_abs={maximum:.17g}; source_sha256={hashlib.sha256(source.encode()).hexdigest()}")

if __name__=="__main__":main()
