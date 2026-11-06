import unittest
import numpy as np
from run_recovery_checks import boundary_summary


class RecoveryTests(unittest.TestCase):
    def test_boundary_classification_and_invalid_fixture(self):
        nodes=np.array([(x,y) for y in np.linspace(0,1,9) for x in np.linspace(0,1,9)])
        data={"PointID":np.arange(81),"x":nodes[:,0],"y":nodes[:,1]}
        data.update({"Stress_E_"+key:np.zeros(81) for key in ("XX","YY","ZZ","XY","XZ","YZ")})
        data["Stress_E_XY"][44]=.2
        report=boundary_summary(data)
        self.assertEqual(report["outlet"]["worst_point_id"],44)
        self.assertEqual(report["outlet"]["maximum"],.2)
        self.assertEqual(report["wall"]["nonzero_nodes"],0)
        data["Stress_E_XX"][0]=np.nan
        with self.assertRaisesRegex(ValueError,"nonfinite"):boundary_summary(data)
        data["PointID"][1]=0
        with self.assertRaisesRegex(ValueError,"unique"):boundary_summary(data)


if __name__=="__main__":unittest.main()
