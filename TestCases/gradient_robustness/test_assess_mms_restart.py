import json
from pathlib import Path
import tempfile
import unittest

from assess_mms_restart import assess


class RestartErrorTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.config = Path(self.directory.name) / 'case.cfg'
        self.config.write_text('SOLVER= NAVIER_STOKES\nKIND_VERIFICATION_SOLUTION= MMS_NS_UNIT_QUAD\n'
                               'REF_DIMENSIONALIZATION= DIMENSIONAL\nMGLEVEL= 0\n')
        self.restart = Path(self.directory.name) / 'fields.csv'
        self.header = 'PointID,x,y,Density,Momentum_x,Momentum_y,Energy\n'

    def test_known_origin_state_and_density_error(self):
        # Native exact boundary state at (0,0): rho=1.23, u=65, v=59, p=70000.
        self.restart.write_text(self.header + '0,0,0,1.23,79.95,72.57,179739.19\n')
        result = assess(self.config, self.restart)
        self.assertEqual(result['points'], 1)
        self.assertLess(max(result['max_error'].values()), 1e-9)
        self.restart.write_text(self.header + '0,0,0,2.23,79.95,72.57,179739.19\n')
        self.assertAlmostEqual(assess(self.config, self.restart)['rms_error']['Density'], 1)
        json.dumps(result, allow_nan=False)

    def test_invalid_states_and_empty_restart(self):
        for row in ('', '0,0,0,nan,0,0,1\n', '0,0,0,0,0,0,1\n', '0,0,0,1,2,0,1\n'):
            with self.subTest(row=row):
                self.restart.write_text(self.header + row)
                with self.assertRaises(ValueError):
                    assess(self.config, self.restart)

    def test_unsupported_units_and_dimension(self):
        self.restart.write_text(self.header + '0,0,0,1.23,79.95,72.57,179739.19\n')
        self.config.write_text(self.config.read_text().replace('DIMENSIONAL', 'FREESTREAM_VEL_EQ_MACH'))
        with self.assertRaises(ValueError):
            assess(self.config, self.restart)
        self.config.write_text(self.config.read_text().replace('FREESTREAM_VEL_EQ_MACH', 'DIMENSIONAL'))
        self.restart.write_text(self.header.replace('x,y', 'x,y,z') + '0,0,0,0,1.23,79.95,72.57,179739.19\n')
        with self.assertRaises(ValueError):
            assess(self.config, self.restart)


if __name__ == '__main__':
    unittest.main()
