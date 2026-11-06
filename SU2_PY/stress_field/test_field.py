#!/usr/bin/env python3
"""Small cross-language tests; never starts a CFD executable."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

import numpy as np
from field_io import assemble, write_field


class FieldTests(unittest.TestCase):
    probe = None
    v07 = None

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.nodes = np.array([[0.,0.],[1.,0.],[1.,1.],[0.,1.]])
        self.cells = np.array([[0,1,2],[0,2,3]])
        self.definition = {"scenario":"test", "U_ref":2., "L_ref":1., "epsilon":2., "ell":0.1,
                           "ell_w":0.02,"geometry":{"type":"NONE"},"support":{"type":"NONE"}}

    def tearDown(self):
        self.temp.cleanup()

    def query(self, path, points, success=True):
        points = np.atleast_2d(points)
        if points.shape[1] == 2:
            points = np.column_stack((points, np.zeros(len(points))))
        data = "".join(" ".join(format(v, ".17g") for v in row)+"\n" for row in points)
        result = subprocess.run([self.probe, str(path)], input=data, text=True, capture_output=True)
        if not success:
            self.assertNotEqual(result.returncode, 0, result.stderr)
            return
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(hashlib.sha256(path.read_bytes()).hexdigest(), result.stderr)
        return np.array([list(map(float, row.split())) for row in result.stdout.splitlines()])

    def write(self, name="field.suf", c=None, definition=None, binary=True):
        c = np.tile([0.1,0.2,0.3,0,0], (len(self.nodes),1)) if c is None else c
        path = self.root/name
        write_field(path, self.nodes, self.cells, c, definition or self.definition, binary)
        return path

    def test_affine_triangles_and_tetrahedron(self):
        for dim in (2,3):
            if dim == 3:
                self.nodes = np.array([[0.,0.,0.],[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]])
                self.cells = np.array([[0,1,2,3]])
            c = np.zeros((len(self.nodes),5));c[:,0] = 0.1+self.nodes[:,0]*0.2;c[:,2]=0.15
            point = np.full((1,dim), 0.2)
            outputs = [self.query(self.write(c=c,binary=b), point) for b in (False,True)]
            np.testing.assert_allclose(outputs[0], outputs[1], atol=1e-14)
            expected = 4*np.array([[0.14/np.sqrt(2),-0.14/np.sqrt(2),0,0.15/np.sqrt(2),0,0]])
            np.testing.assert_allclose(outputs[0], expected, atol=1e-14)
            m,k,measure,_ = assemble(self.nodes,self.cells)
            self.assertAlmostEqual(float(np.ones(len(self.nodes))@m@np.ones(len(self.nodes))), measure)
            self.assertLess(np.linalg.norm(k@np.ones(len(self.nodes))),1e-14)

    def test_wall_map_and_duplicate_charts(self):
        definition = copy.deepcopy(self.definition)
        wall = {"normal":[0,1,0],"offset":0,"weight":1}
        definition["geometry"]={"type":"PLANES","walls":[wall]}
        points = np.array([[0.4,0.0],[0.4,1e-6],[0.4,1e-5],[0.4,0.02],[0.4,0.6]])
        actual=self.query(self.write(definition=definition),points)
        c=np.array([0.1,0.2,0.3,0,0]);a=c[0]/np.sqrt(2);b=c[1]/np.sqrt(6)
        raw=np.array([[a+b,c[2]/np.sqrt(2),0],[c[2]/np.sqrt(2),-a+b,0],[0,0,-2*b]])
        expected=[]
        for _,d in points:
            q=d/(d+0.02);l=np.diag([1,q,1]);e=l@raw@l;e-=np.trace(e)*np.eye(3)/3;e*=4*q*q
            expected.append([e[0,0],e[1,1],e[2,2],e[0,1],e[0,2],e[1,2]])
        np.testing.assert_allclose(actual,expected,rtol=1e-10,atol=1e-20)
        wall["weight"]=0.5;definition["geometry"]["walls"]=[wall,copy.deepcopy(wall)]
        duplicate=self.query(self.write(definition=definition),points)
        np.testing.assert_allclose(actual,duplicate,rtol=1e-9,atol=1e-20)
        self.assertTrue(np.array_equal(actual[0],np.zeros(6)))

    def test_support_and_outside_domain(self):
        path=self.write()
        self.query(path,[[1.1,0.5]],success=False)
        definition=copy.deepcopy(self.definition)
        definition["support"]={"type":"BOX","lower":[0,0,0],"upper":[1,1,0],"width":[0.1,0.1,0]}
        path=self.write(definition=definition)
        np.testing.assert_array_equal(self.query(path,[[-0.1,0.5],[0,0.5],[1,0.5]]),np.zeros((3,6)))
        # The two triangles share a diagonal: use the same physical P1 field on both sides.
        c=np.zeros((4,5));c[:,0]=self.nodes[:,0]+self.nodes[:,1]
        definition["epsilon"]=4
        path=self.write(c=c,definition=definition)
        p=np.array([[0.5,0.5-1e-10],[0.5,0.5],[0.5,0.5+1e-10]])
        np.testing.assert_allclose(self.query(path,p)[:,0],4*p.sum(axis=1)/np.sqrt(2),atol=1e-14)

    def test_invalid_input(self):
        path=self.write();data=bytearray(path.read_bytes());data[-1]^=1;path.write_bytes(data)
        self.query(path,[[0.5,0.5]],success=False)
        path=self.write();path.write_bytes(path.read_bytes().replace(b"SU2_STRESS_FIELD 1",b"SU2_STRESS_FIELD 2",1))
        self.query(path,[[0.5,0.5]],success=False)
        with self.assertRaises(ValueError):self.write(c=np.full((4,5),np.nan))
        with self.assertRaises(ValueError):self.write(c=np.full((4,5),1.0))
        self.cells=np.array([[0,2,1]])
        with self.assertRaises(ValueError):self.write()

    def test_rotated_three_dimensional_two_wall_field(self):
        self.nodes=np.array([[0.,0.,0.],[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]])
        self.cells=np.array([[0,1,2,3]])
        definition=copy.deepcopy(self.definition)
        definition["geometry"]={"type":"PLANES","walls":[
            {"normal":[1,0,0],"offset":0},{"normal":[0,1,0],"offset":0}]}
        c=np.tile([.1,.2,.3,.4,.5],(4,1));point=np.array([[.2,.3,.1]])
        values=self.query(self.write(c=c,definition=definition),point)[0]
        e=np.array([[values[0],values[3],values[4]],[values[3],values[1],values[5]],[values[4],values[5],values[2]]])
        a,b=c[0,0]/np.sqrt(2),c[0,1]/np.sqrt(6)
        raw=np.array([[a+b,c[0,2]/np.sqrt(2),c[0,3]/np.sqrt(2)],
                      [c[0,2]/np.sqrt(2),-a+b,c[0,4]/np.sqrt(2)],
                      [c[0,3]/np.sqrt(2),c[0,4]/np.sqrt(2),-2*b]])
        l=np.diag([1/(1+.02/.2),1/(1+.02/.3),1]);mapped=l@raw@l
        mapped-=np.trace(mapped)*np.eye(3)/3;mapped*=4/(1+.02/.2+.02/.3)**2
        np.testing.assert_allclose(e,mapped,atol=1e-14)
        q=np.array([[.36,-.8,.48],[.48,.6,.64],[-.8,0,.6]])
        shift=np.array([2.,-3.,1.]);self.nodes=self.nodes@q.T+shift
        rotated=q@raw@q.T
        coefficients=[(rotated[0,0]-rotated[1,1])/np.sqrt(2),
                      (rotated[0,0]+rotated[1,1]-2*rotated[2,2])/np.sqrt(6),
                      *(np.sqrt(2)*rotated[i,j] for i,j in ((0,1),(0,2),(1,2)))]
        for wall in definition["geometry"]["walls"]:
            n=q@wall["normal"];wall["normal"]=n.tolist();wall["offset"]=float(n@shift)
        actual=self.query(self.write(c=np.tile(coefficients,(4,1)),definition=definition),point@q.T+shift)[0]
        expected=q@e@q.T
        np.testing.assert_allclose(actual,expected[(0,1,2,0,0,1),(0,1,2,1,2,2)],atol=1e-14)

    def test_generator_is_deterministic_and_saves_admissible_field(self):
        mesh=self.root/"mesh.npz";np.savez(mesh,nodes=self.nodes,cells=self.cells)
        definition=copy.deepcopy(self.definition)
        definition["seed"]={"type":"SINE_PHYSICAL_V1","wave_vectors":[[1,2]]*5,"phases":[0,.1,.2,.3,.4]}
        path=self.root/"definition.json";path.write_text(json.dumps(definition))
        outputs=[]
        for i in range(2):
            out=self.root/f"generated{i}.suf"
            subprocess.run(["python3",str(Path(__file__).with_name("generate_field.py")),"--mesh",str(mesh),
                            "--definition",str(path),"--output",str(out)],check=True,capture_output=True)
            outputs.append(out.read_bytes())
            report=json.loads(out.with_suffix(".suf.json").read_text())
            self.assertLessEqual(max(report["norm_inf"],report["norm_H1"]),definition["epsilon"]*(1+1e-12))
            self.query(out,[[.3,.4]])
        self.assertEqual(*outputs)

    def test_saved_circle_oracle(self):
        if not self.v07:self.skipTest("--v07-root not supplied")
        source=Path(self.v07)/"results/v07"
        out=self.root/"circle.suf"
        subprocess.run(["python3",str(Path(__file__).with_name("export_field.py")),"--input",str(source/"curved_field.npz"),
                        "--v07-circle-manifest",str(source/"curved_field_manifest.json"),"--output",str(out)],check=True,capture_output=True)
        q=np.loadtxt(source/"example_queries.csv",delimiter=",",skiprows=1)
        actual=self.query(out,q[:,:2])
        np.testing.assert_allclose(actual,q[:,2:],rtol=1e-10,atol=1e-11)
        np.testing.assert_array_equal(self.query(out,[[0.5,0],[1.3,0]]),np.zeros((2,6)))
        self.query(out,[[0.1,0]],success=False)


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe",required=True)
    parser.add_argument("--v07-root")
    args,remaining=parser.parse_known_args()
    FieldTests.probe=str(Path(args.probe).resolve());FieldTests.v07=args.v07_root
    unittest.main(argv=[__file__,*remaining])
