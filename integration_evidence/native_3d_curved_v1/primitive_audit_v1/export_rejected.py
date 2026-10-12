from pathlib import Path
import hashlib,json,sys
HERE=Path(__file__).resolve().parent;case=HERE.parent/sys.argv[1]
receipt=json.loads((case/'validation.json').read_text());report={}
for path in sorted(case.glob('mpi*/*_rejected.json')):
 key=str(path.relative_to(case));digest=hashlib.sha256(path.read_bytes()).hexdigest()
 assert digest==receipt['artifact_sha256'][key]
 row=json.loads(path.read_text());ids={n[0]:i for i,n in enumerate(row['nodes'])}
 facets={f['id']:f for f in row['facets']};markers={}
 for f in row['surface']:
  name=row['marker_names'][facets[f['facet']]['marker']];markers.setdefault(name,[]).append(f['nodes'])
 dest=HERE/sys.argv[1]/path.parent.name;dest.mkdir(parents=True,exist_ok=True)
 target=dest/(path.stem+'.su2')
 with target.open('w') as out:
  out.write('NDIME= 3\nNELEM= '+str(len(row['cells']))+'\n')
  for i,c in enumerate(row['cells']):out.write('10 '+' '.join(str(ids[n]) for n in c['nodes'])+' '+str(i)+'\n')
  out.write('NPOIN= '+str(len(ids))+'\n')
  for i,n in enumerate(row['nodes']):out.write(' '.join(format(v,'.17g') for v in n[1:])+' '+str(i)+'\n')
  out.write('NMARK= '+str(len(markers))+'\n')
  for name,faces in markers.items():
   out.write('MARKER_TAG= '+name+'\nMARKER_ELEMS= '+str(len(faces))+'\n')
   for f in faces:out.write('5 '+' '.join(str(ids[n]) for n in f)+'\n')
 report[key]={'input_sha256':digest,'export':str(target.relative_to(HERE)),'output_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'scope':'Rejected private replay; never published as accepted CFD mesh. Serial replay uses same rank/worker count; multi-rank replay may differ from backend worker count.'}
(HERE/(sys.argv[1]+'_rejected_exports.json')).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
