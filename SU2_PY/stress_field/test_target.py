#!/usr/bin/env python3
"""Synthetic donor-fit check. Does not use or claim a converged donor flow."""
import argparse
import json
from pathlib import Path
import tempfile
import unittest
import numpy as np
from export_target import fit

class TargetTests(unittest.TestCase):
    probe=None
    def test_nodal_fit_and_wall_incompatibility(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);nodes=np.array([[0.,0.],[1.,0.],[0.,1.]])
            np.savez(root/"mesh.npz",nodes=nodes,cells=np.array([[0,1,2]]))
            (root/"donor.csv").write_text('PointID,x,y,Stress_E_XX,Stress_E_YY,Stress_E_ZZ,Stress_E_XY,Stress_E_XZ,Stress_E_YZ\n'
                                          '0,0,0,0,0,0,.2,0,0\n1,1,0,0,0,0,.3,0,0\n2,0,1,0,0,0,.4,0,0\n')
            definition={"scenario":"synthetic_target","U_ref":2,"L_ref":1,"epsilon":.001,"ell":.1,"ell_w":.02,
                        "geometry":{"type":"NONE"},"support":{"type":"NONE"}}
            path=root/"definition.json";path.write_text(json.dumps(definition));out=root/"target.suf"
            report=fit(root/"donor.csv",root/"mesh.npz",path,self.probe,out,1,1,"QCR","SYNTHETIC")
            self.assertLess(report["max_nodal_error"],1e-14)
            self.assertFalse(report["raw_bounds_satisfied"])
            self.assertEqual(report["exact_face_replay"],"NOT_IMPLEMENTED")
            certificate=json.loads((root/"target.suf.json").read_text())
            self.assertEqual(certificate["definition"]["purpose"],"COUPLING_CONTROL")
            definition["geometry"]={"type":"PLANES","walls":[{"normal":[0,1,0],"offset":0}]}
            path.write_text(json.dumps(definition))
            with self.assertRaisesRegex(ValueError,"incompatible"):
                fit(root/"donor.csv",root/"mesh.npz",path,self.probe,root/"bad.suf",1,1,"QCR","SYNTHETIC")
            self.assertFalse((root/"bad.suf").exists())

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--probe",required=True)
    args,rest=parser.parse_known_args();TargetTests.probe=str(Path(args.probe).resolve())
    unittest.main(argv=[__file__,*rest])
