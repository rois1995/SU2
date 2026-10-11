"""Source-pinned lower bound on MPI collectives; never a complete traffic or wait measurement."""
from pathlib import Path
import argparse,hashlib,json,math
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
PROOF={
 'Common/src/adaptation/CDistributedSearch.cpp':(
  'const unsigned long severity = CPassiveComm::AllreduceMax(local.severity, comm);',
  'if (severity == 0) return elected;'),
 'Common/src/adaptation/CNativeDistributed3D.cpp':(
  '++stats.elections;', 'const auto elected = world.elect(failure);'),
 'Common/include/adaptation/CNativeDistributed2D.hpp':(
  'CElectedFailure elect(const CLocalFailure& local) const { return ElectFailure(local, comm); }',
  'CPassiveComm::Alltoall(counts.data(), incoming.data(), sizeof(uint64_t), comm);',
  'CPassiveComm::AlltoallvRounds(bytes.data(), sendBytes, recvBytes, comm);',
  'CPassiveComm::Allreduce(value, op, comm);'),
 'Common/src/parallelization/CPassiveComm.cpp':(
  'MPI_Allreduce(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_MAX, comm);',
  'MPI_Alltoall(const_cast<void*>(send), count, MPI_BYTE, recv, count, MPI_BYTE, comm);',
  'Alltoall(sendCount64.data(), recvCount64.data(), sizeof(uint64_t), comm);'),
 'Common/include/parallelization/CPassiveComm.hpp':(
  'MPI_Allreduce(const_cast<T*>(send), recv, CheckedInt(n, "Allreduce count"), NativeType<T>(), NativeOp(op),',),
}
def sha(path):
 return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def counts(row):
 keys=('rank','ranks','accepted','tracked_calls','directory_elections')
 for k in keys:
  v=row[k]
  if not math.isfinite(v) or v<0 or int(v)!=v:raise ValueError('Invalid integer '+k)
 if row['ranks']<1 or row['rank']>=row['ranks']:raise ValueError('Invalid rank')
 for k in ('total_s','directory_election_nested_s','tracked_collectives_nested_s'):
  if not math.isfinite(row[k]) or row[k]<0:raise ValueError('Invalid time '+k)
 if row['directory_election_nested_s']>row['total_s']+1e-10:raise ValueError('Election time leaves engine envelope')
 known=row['tracked_calls']+row['directory_elections']
 return dict(case=row['case'],rank=int(row['rank']),ranks=int(row['ranks']),
  accepted=int(row['accepted']),tracked_wrapper_calls=int(row['tracked_calls']),
  directory_elections=int(row['directory_elections']),minimum_mpi_collectives=int(known),
  minimum_untracked_fraction=row['directory_elections']/known if known else None,
  minimum_collectives_per_accepted_edit=known/row['accepted'] if row['accepted'] else None,
  engine_seconds=row['total_s'],nested_wrapper_seconds=row['tracked_collectives_nested_s'],
  nested_directory_election_seconds=row['directory_election_nested_s'])

def records(text):
 rows=[]
 for line in text.splitlines():
  if not line.startswith('ENGINE '):continue
  r={k:v if k=='case' else float(v) for k,v in (s.split('=') for s in line.split()[1:])}
  rows.append(counts(r))
 if not rows:raise ValueError('No current engine records')
 return rows

def main(output):
 # A serial application without HAVE_MPI cannot establish this MPI API lower bound.
 build_path=HERE/'transport_build_v1/validation.json';build=json.loads(build_path.read_text())
 run_path=HERE/'transport_serial_v1/validation.json';run=json.loads(run_path.read_text())
 recovery_path=HERE/'transport_serial_audit_v2/validation.json';recovery=json.loads(recovery_path.read_text())
 assert build['status']=='PASS' and recovery['status']=='SERIAL_AUDIT_PASS'
 assert sha(build_path)==recovery['build_validation_sha256'] and sha(run_path)==recovery['original_run_sha256']
 assert run['binary_sha256']==build['binary_sha256']==recovery['binary_sha256']
 ninja=ROOT/'integration_evidence/build-integrated-v2/build.ninja'
 assert sha(ninja)==build['build_ninja_sha256'] and '-DHAVE_MPI' in ninja.read_text()
 proof={}
 for name,anchors in PROOF.items():
  source=ROOT/name
  assert sha(source)==build['source_sha256'][name],name
  assert all(a in source.read_text() for a in anchors),name
  proof[name]=dict(sha256=sha(source),anchors=anchors)
 original=HERE/'transport_serial_v1'
 for name,pin in run['artifact_sha256'].items():assert sha(original/name)==pin,name
 log=original/'native_serial/native_serial.stdout'
 rows=records(log.read_text());assert len(rows)==40 and all(r['ranks']==1 for r in rows)
 wrappers=sum(r['tracked_wrapper_calls'] for r in rows)
 elections=sum(r['directory_elections'] for r in rows)
 output=Path(output).resolve();output.mkdir(exist_ok=False)
 result=dict(status='COVERAGE_LOWER_BOUND_PASS',source_proof=proof,rows=rows,
  wrapper_calls=wrappers,directory_elections=elections,minimum_mpi_collectives=wrappers+elections,
  minimum_untracked_fraction=elections/(wrappers+elections),
  build_validation_sha256=sha(build_path),original_run_sha256=sha(run_path),
  recovery_sha256=sha(recovery_path),log_sha256=sha(log),binary_sha256=build['binary_sha256'],
  analyzer_sha256=sha(Path(__file__)),
  scope='40 selected manufactured serialMPI1 engine records from one pinned binary. Minimum collective API calls = World wrapper calls + directory-election API calls. Each wrapper maps to at least one collective; each disjoint directory election performs at least one severity Allreduce. Hidden round collectives, World.Fail/other elections, direct/vendor/CFD communication remain omitted. Self MPI calls are included. Counts are work diagnostics, not wire traffic, pure MPI wait, repeated timing, scaling or a performance cause. Election elapsed includes its enclosing API work and is nested in engine phases; never sum it with exclusive parent time.')
 (output/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
 print(result['status'],'wrappers',wrappers,'directory elections',elections,'minimum collectives',wrappers+elections)

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
 main(parser.parse_args().output)
