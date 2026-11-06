#!/usr/bin/env python3
"""Offline infrastructure tests with synthetic CSV data; no CFD is executed."""
import json
from pathlib import Path
import tempfile
import unittest
from campaign import compare, config_options, digest, extract, prepare, save, stress_budget
from generate_field import make_seed
import numpy as np
from prepare_example import prepare as example

class CampaignTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def test_owned_stress_budget_and_source_limiter_gap(self):
        data={"Stress_Volume":np.array([1.,3.,0.]),"Stress_Work":np.array([2.,-1.,100.])}
        for name in ("XX","YY","ZZ","XY","XZ","YZ"):data["Stress_E_"+name]=np.zeros(3)
        for name in ("PARENT","RAW","LIMITED","OMEGA","CAP_GAP"):data["Stress_"+name]=np.array([2.,1.,100.])
        for name in ("LOWER_CAP","UPPER_CAP","K_FLOOR","OMEGA_FLOOR"):data["Stress_"+name]=np.array([1.,0.,1.])
        data["Stress_COV_MIN"]=np.array([-.2,.1,-100.])
        budget=stress_budget(data,.01)
        self.assertEqual(budget["owned_volume"],4)
        self.assertEqual(budget["stress_work_integral"],-1)
        self.assertEqual(budget["negative_covariance_volume_fraction"],.25)
        self.assertEqual(budget["minimum_covariance_eigenvalue"],-.2)
        self.assertEqual(budget["k_floor_volume_fraction"],.25)
        with self.assertRaises(ValueError):stress_budget(data,-1)

    def test_localized_seed_combinations_and_physical_queries(self):
        nodes=np.array([[0.,0.],[.5,.5],[1.,1.]])
        positive={"center":[.5,.5],"width":[.2,.4],"coefficients":[1,0,2,0,0]}
        definition={"type":"LOCALIZED_PHYSICAL_V1","terms":[positive]}
        seed=make_seed(nodes,definition)
        np.testing.assert_array_equal(seed[1],[1,0,2,0,0])
        np.testing.assert_array_equal(make_seed(nodes[[1]],definition)[0],seed[1])
        negative={**positive,"coefficients":[-1,0,-2,0,0]}
        np.testing.assert_array_equal(make_seed(nodes,{**definition,"terms":[positive,negative]}),0)
        with self.assertRaises(ValueError):make_seed(nodes,{**definition,"terms":[{**positive,"width":[0,1]}]})
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
