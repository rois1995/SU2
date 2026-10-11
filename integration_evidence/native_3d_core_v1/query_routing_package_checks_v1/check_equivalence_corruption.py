"""Reject altered transferred fields and reversed actual tetrahedra before reusing a certificate."""
from pathlib import Path
import contextlib,hashlib,io,json,shutil,sys,tempfile
folder=Path(__file__).resolve().parent;where=folder.parent
sys.path.insert(0,str(where))
from audit_query_routing_equivalence import main
case=where/'query_routing_n8_n1_m1_rounds1024_v1';reference=where/'full_star_facet_n8_n1_m1_rounds1024_v1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checks=[]
for kind in ('state','actual_orientation'):
 with tempfile.TemporaryDirectory(dir=folder) as temp:
  target=Path(temp)
  for p in case.iterdir():
   if p.is_file():shutil.copyfile(p,target/p.name)
  if kind=='state':
   path=target/'envelope_s0_states.json';data=json.loads(path.read_text());data['accepted'][0]['states'][10]+=.1
   path.write_text(json.dumps(data))
  else:
   path=target/'envelope_mesh_00001.su2';lines=path.read_text().splitlines()
   first=next(i for i,s in enumerate(lines) if s.startswith('NELEM='))+1
   row=lines[first].split();row[1],row[2]=row[2],row[1];lines[first]=' '.join(row);path.write_text('\n'.join(lines)+'\n')
  try:
   with contextlib.redirect_stdout(io.StringIO()):main(target,reference)
  except AssertionError as e:
   reason=str(e)
   assert ('Current numerical artifact differs' in reason if kind=='state' else 'Nonpositive actual SU2 tetrahedron' in reason),reason
   checks.append(dict(case=kind,rejected=True,reason=reason))
  else:raise AssertionError('Corruption inherited a certificate: '+kind)
result=dict(status='PASS',checks=checks,script_sha256=sha(Path(__file__).resolve()),
 auditor_sha256=sha(where/'audit_query_routing_equivalence.py'),
 current_certificate_sha256=sha(case/'independent_audit.json'),reference_certificate_sha256=sha(reference/'independent_audit.json'),
 scope='Fresh negative controls against the actual equivalence auditor; regular disposable copies only, no original artifacts modified. Changed current/BDF data and reversed actual SU2 cell orientation must reject. No repeated expensive geometry audit or performance claim.')
(folder/'equivalence_corruption_checks.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS two fresh certificate-corruption controls')
