from pathlib import Path
import json,hashlib,statistics,sys
root=Path(__file__).resolve().parents[2];base=Path(__file__).resolve().parent
new=base/'reader_runtime_v2';old=base/'profile_runtime_v2';skew=base/'reader_skew_v1'
full=json.loads((new/'validation.json').read_text());follow=json.loads((skew/'validation.json').read_text())
assert full['status']==follow['status']=='PASS'
assert full['library_sha256']==follow['library_sha256'] and full['thin_archive_member_sha256']==follow['thin_archive_member_sha256']
assert [p for p,h in full['source_sha256'].items() if follow['source_sha256'].get(p)!=h]==['UnitTests/SU2_CFD/adaptation/AdaptationMPI_tests.cpp']
rows=[]
for n in (1,2,4):
 for line in (skew/f'budget_mpi{n}.stdout').read_text().splitlines():
  if not line.startswith('READERBUDGET '):continue
  row={k:int(v) for k,v in (word.split('=') for word in line.split()[1:])}
  assert row['ranks']==n and 0<=row['rank']<n
  assert row['maximum']==max(row[f'phase{p}'] for p in range(6))
  if row['rank']==0:assert row['phase3']>max(row[f'phase{p}'] for p in range(3))
  rows.append(row)
assert len(rows)==7
(skew/'phase_bounds.json').write_text(json.dumps(dict(status='PASS',rank_rows=rows,scope='Modeled requested-storage bounds for disjoint tetrahedra with all point keys routed to rank0. C++ checks legacy identity, late linear-point refusal, output preservation and retry. No allocator-inclusive certificate.'),indent=2)+'\n')
comparisons=[]
for n in (1,2,4):
 groups=[]
 for folder in (old,new):
  d=json.loads((folder/f'lifecycle_mpi{n}/lifecycle_audit.json').read_text());assert d['status']=='PASS'
  by_rank={rank:[r for r in d['outer_case_rows'] if r['rank']==rank and r['detailed']==1] for rank in range(n)}
  returned={rank:[r for r in d['nested_rank_details'] if r['rank']==rank and r['stage']=='return'] for rank in range(n)}
  assert all(len(by_rank[rank])==len(returned[rank]) for rank in range(n))
  group={}
  for i,c in enumerate(by_rank[0]):
   key=(int(c['workers']),int(c['repeat']))
   assert all((by_rank[rank][i]['workers'],by_rank[rank][i]['repeat'])==key for rank in range(n))
   group[key]=dict(reader=max(returned[rank][i]['reader'] for rank in range(n)),whole_return=max(returned[rank][i]['total'] for rank in range(n)),outer=max(by_rank[rank][i]['wall'] for rank in range(n)))
  groups.append(group)
 assert groups[0].keys()==groups[1].keys()
 for m in sorted({key[0] for key in groups[0]}):
  comparisons.append(dict(ranks=n,workers=m,old_samples=[groups[0][m,p] for p in range(3)],new_samples=[groups[1][m,p] for p in range(3)]))
(new/'reader_phase_comparison.json').write_text(json.dumps(dict(status='PASS',comparisons=comparisons,scope='Three matched fresh-input detailed-ON samples per N/M, same accepted mesh/target. Per-phase rank maxima are diagnostic and not additive. Old and new checkpoints ran at different times with foreign FSI contention; no isolated admission overhead or speedup/scaling conclusion.'),indent=2)+'\n')
for c in comparisons:
 print(c['ranks'],c['workers'],'reader median ms old/new',*[round(1000*statistics.median(row['reader'] for row in c[k]),4) for k in ('old_samples','new_samples')])
for p,h in follow['source_sha256'].items():
 if p in ('Common/include/adaptation/CReaderSlices.hpp','Common/src/adaptation/CReaderSlices.cpp','Common/src/adaptation/CNativeImport3D.cpp','UnitTests/SU2_CFD/adaptation/AdaptationMPI_tests.cpp'):
  assert hashlib.sha256((root/p).read_bytes()).hexdigest()==h, p
checks=['reader_runtime_v1/validation.json','reader_runtime_v2/validation.json','reader_skew_v1/validation.json','reader_runtime_v2/predecessor_comparison.json','reader_runtime_v2/lifecycle_phase_summary.json','reader_runtime_v2/lifecycle_rank_phases.csv','reader_runtime_v2/reader_phase_comparison.json','reader_skew_v1/phase_bounds.json']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=dict(status='PASS',analysis_sha256=sha(Path(__file__)),branch='codex/native-3d-core',parent_commit='4c9f56b8c861433158e58459ee0d5c157742bb9b',source_sha256={p:sha(root/p) for p in ('Common/include/adaptation/CReaderSlices.hpp','Common/src/adaptation/CReaderSlices.cpp','Common/src/adaptation/CNativeImport3D.cpp','UnitTests/SU2_CFD/adaptation/AdaptationMPI_tests.cpp')},evidence_sha256={p:sha(base/p) for p in checks},scope='Native3D recoverable reader/assembly requested-storage admission; full42-stage actual-archive MPI1/2/4 qualification and11-stage reader-only follow-up with unchanged production archives. All126 mesh frames/84 transported-state frames/36 computed-metric lifecycle frames equal predecessor. Seven skew rank bounds. Full Goal1 remains active; allocator/MPI/ParMETIS internals, complete wait/traffic and practical envelope/cluster gates remain open.')
(base/'READER_CHECKPOINT_VALIDATION.json').write_text(json.dumps(receipt,indent=2)+'\n')
