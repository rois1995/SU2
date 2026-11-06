#!/usr/bin/env python3
"""Parser and sidecar checks. These are not disk-checkpoint continuation tests."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import unittest

class ConfigTests(unittest.TestCase):
    probe=None

    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
        fixture=Path(__file__).resolve().parents[2]/"UnitTests/fixtures/stress/zero.suf"
        self.base=f"""SOLVER=RANS
MACH_NUMBER=0.1
REYNOLDS_NUMBER=1000
KIND_TURB_MODEL=SST
SST_OPTIONS=V2003m
MGLEVEL=0
STRESS_DISCREPANCY=PRESCRIBED
STRESS_DISCREPANCY_FILE={fixture}
"""

    def tearDown(self):
        self.tmp.cleanup()

    def invoke(self,text,action="PARSE",success=True,reason=None):
        cfg=self.root/"test.cfg";cfg.write_text(text)
        result=subprocess.run([self.probe,str(cfg),action,str(self.root/"restart")],capture_output=True,text=True)
        self.assertEqual(result.returncode==0,success,result.stdout+result.stderr)
        if reason:self.assertIn(reason,result.stdout+result.stderr)

    def test_supported_and_explicit_rejections(self):
        self.invoke(self.base)
        for text,reason in [
            (self.base.replace("MGLEVEL=0","MGLEVEL=1"),"MGLEVEL=0"),
            (self.base.replace("SST_OPTIONS=V2003m","SST_OPTIONS=V1994m"),"V2003m"),
            (self.base+"STRESS_DISCREPANCY_SCALE=-1\n","[0,1]"),
            (self.base+"STRESS_DISCREPANCY_PURPOSE=COUPLING_CONTROL\n","purposes differ"),
            (self.base.replace("STRESS_DISCREPANCY=PRESCRIBED","STRESS_DISCREPANCY=EPM_MATCHED"),"must not also supply"),
            (self.base.replace("KIND_TURB_MODEL=SST","KIND_TURB_MODEL=SA")+"SA_OPTIONS=QCR2000\n","linear SA"),
            (self.base.replace("STRESS_DISCREPANCY=PRESCRIBED","STRESS_DISCREPANCY=UNKNOWN"),"choose NONE"),
        ]:
            with self.subTest(reason=reason):self.invoke(text,success=False,reason=reason)

    def test_restart_identity_missing_same_changed_and_explicit_segment(self):
        self.invoke(self.base,"CHECK",False,"Missing or mismatched")
        self.invoke(self.base,"WRITE")
        self.invoke(self.base,"CHECK")
        self.invoke(self.base+"STRESS_DISCREPANCY_SCALE=0.5\n","CHECK",False,"Missing or mismatched")
        self.invoke(self.base.replace("KIND_TURB_MODEL=SST","KIND_TURB_MODEL=SA"),"CHECK",False,"Missing or mismatched")
        self.invoke(self.base+"STRESS_DISCREPANCY_SCALE=0.5\nSTRESS_DISCREPANCY_RESTART=NEW_SEGMENT\n","CHECK")

    def test_diagnostic_scope_and_replay_contract(self):
        self.invoke(self.base+"STRESS_DIAGNOSTIC=CAPTURE\n",success=False,reason="ITER=1")
        self.invoke(self.base+"ITER=1\nSTRESS_DIAGNOSTIC=CAPTURE\n")
        replay="\n".join(line for line in self.base.replace("STRESS_DISCREPANCY=PRESCRIBED","STRESS_DISCREPANCY=REPLAY").splitlines()
                           if not line.startswith("STRESS_DISCREPANCY_FILE="))+"\nITER=1\n"
        self.invoke(replay,success=False,reason="REPLAY requires")
        replay+="STRESS_DIAGNOSTIC=RESIDUAL\nSTRESS_DISCREPANCY_PURPOSE=COUPLING_CONTROL\n"
        self.invoke(replay)
        self.invoke(replay+"STRESS_DISCREPANCY_SCALE=0.5\n",success=False,reason="scale=1")

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--probe",required=True)
    args,rest=parser.parse_known_args();ConfigTests.probe=str(Path(args.probe).resolve())
    unittest.main(argv=[__file__,*rest])
