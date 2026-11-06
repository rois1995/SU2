#!/usr/bin/env python3
"""Offline infrastructure tests with synthetic CSV data; no CFD is executed."""
import json
from pathlib import Path
import tempfile
import unittest
from campaign import compare, config_options, digest, extract, prepare, save
from prepare_example import prepare as example

class CampaignTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def test_prepare_preserves_inputs_and_refuses_stale_output(self):
        plan=example(self.root/"inputs");out=self.root/"runs"
        self.assertEqual(prepare(plan,out),{"prepared":13,"executed":0})
        record=json.loads((out/"SA_positive/run.json").read_text())
        self.assertEqual(record["status"],"PREPARED_NOT_RUN")
        self.assertEqual(record["inputs"]["field.suf"]["sha256"],digest(out/"SA_positive/field.suf"))
        with self.assertRaises(ValueError):prepare(plan,out)
        self.assertEqual(config_options("A= (1,\n2) % test\nB= YES"),{"A":"(1, 2)","B":"YES"})
        with self.assertRaises(ValueError):config_options("A=1\nA=2")
    def test_pairing_requires_ids_and_fixed_scales(self):
        a=self.root/"a.csv";b=self.root/"b.csv";out=self.root/"pair.json"
        a.write_text('PointID,R\n0,1\n1,2\n');b.write_text('PointID,R\n1,2\n0,1\n')
        self.assertTrue(compare(a,b,["R"],[1],1e-10,out,"PointID")["pass"])
        b.write_text('PointID,R\n0,1.5\n1,2\n')
        self.assertFalse(compare(a,b,["R"],[1],.1,out,"PointID")["pass"])
        b.write_text('PointID,R\n0,1\n2,2\n')
        with self.assertRaises(ValueError):compare(a,b,["R"],[1],.1,out,"PointID")
    def test_extract_rejects_missing_and_stale_evidence(self):
        plan=example(self.root/"inputs");prepare(plan,self.root/"runs");run=self.root/"runs/SA_base"
        schema=self.root/"schema.json";out=self.root/"metrics.json"
        save(schema,{"files":{"history":{"path":"history.csv","required":["Iter","Drag"],
                                           "metrics":{"drag":{"column":"Drag","units":"1","window":2}}}}})
        with self.assertRaises(FileNotFoundError):extract(run,schema,out)
        (run/"history.csv").write_text('Iter,Drag\n0,1\n1,1.1\n')
        record=json.loads((run/"run.json").read_text())
        save(run/"execution.json",{"returncode":0,"command":["SYNTHETIC_TEST_ONLY"],
             "inputs":{k:v["sha256"] for k,v in record["inputs"].items()},"outputs":{"history.csv":digest(run/"history.csv")}})
        result=extract(run,schema,out);self.assertAlmostEqual(result["fields"]["drag"]["window_range"],.1)
        (run/"history.csv").write_text('Iter,Drag\n0,2\n')
        with self.assertRaises(ValueError):extract(run,schema,out)

if __name__=="__main__":unittest.main()
