"""Offline corruption controls for the source-pinned MPI coverage lower bound."""
import unittest
from audit_mpi_coverage import counts,records
class CoverageChecks(unittest.TestCase):
 def row(self):
  return dict(case='fixture',rank=0,ranks=1,accepted=2,tracked_calls=10,directory_elections=5,
              total_s=1,directory_election_nested_s=.1,tracked_collectives_nested_s=.2)
 def test_valid_counts_and_zero_work(self):
  out=counts(self.row());self.assertEqual(out['minimum_mpi_collectives'],15)
  self.assertEqual(out['minimum_collectives_per_accepted_edit'],7.5)
  self.assertAlmostEqual(out['minimum_untracked_fraction'],1/3)
  row=self.row();row.update(accepted=0,tracked_calls=0,directory_elections=0)
  out=counts(row);self.assertIsNone(out['minimum_collectives_per_accepted_edit'])
  self.assertIsNone(out['minimum_untracked_fraction'])
 def test_invalid_or_incomplete_counts(self):
  for key in ('rank','ranks','accepted','tracked_calls','directory_elections'):
   for value in (-1,.5,float('nan'),float('inf')):
    row=self.row();row[key]=value
    with self.assertRaises(ValueError):counts(row)
   row=self.row();del row[key]
   with self.assertRaises(KeyError):counts(row)
  for key in ('total_s','directory_election_nested_s','tracked_collectives_nested_s'):
   for value in (-1,float('nan'),float('inf')):
    row=self.row();row[key]=value
    with self.assertRaises(ValueError):counts(row)
  row=self.row();row['rank']=1
  with self.assertRaises(ValueError):counts(row)
  row=self.row();row['ranks']=0
  with self.assertRaises(ValueError):counts(row)
  row=self.row();row['directory_election_nested_s']=2
  with self.assertRaises(ValueError):counts(row)
 def test_log_requires_current_schema(self):
  line='ENGINE '+' '.join(str(k)+'='+str(v) for k,v in self.row().items())
  self.assertEqual(len(records('ignored\n'+line+'\n')),1)
  with self.assertRaises(ValueError):records('no ENGINE records')
  with self.assertRaises(KeyError):records(line.replace(' tracked_calls=10',''))
if __name__=='__main__':unittest.main()
