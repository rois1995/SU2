import gzip,hashlib,json,os,subprocess,time
from pathlib import Path
root=Path.cwd(); out=root/'integration_evidence/disk_cleanup_20261018'
manifest=out/'archives.json'; rows=json.loads(manifest.read_text()) if manifest.exists() else []
tracked=set(subprocess.check_output(['git','ls-files','-z']).decode().split('\0'))
files=[]
for d in (root/'integration_evidence').iterdir():
 if not (d.is_dir() and (d.name=='ad_repair_archive_v14' or d.name.startswith('native_') and '_build_v' in d.name)):continue
 for p in d.iterdir():
  if p.name not in ('SU2_CFD','test_driver','SU2_CFD_AD','test_driver_AD') or p.is_symlink() or not p.is_file():continue
  s=p.stat()
  if s.st_size>100*1024**2 and s.st_nlink==1: files.append(p)
def in_use(st):
 for proc in Path('/proc').iterdir():
  if not proc.name.isdigit() or int(proc.name)==os.getpid():continue
  candidates=[proc/'exe']
  try:candidates.extend((proc/'fd').iterdir())
  except OSError:pass
  for candidate in candidates:
   try:q=candidate.stat()
   except OSError:continue
   if (q.st_dev,q.st_ino)==(st.st_dev,st.st_ino):return True
 return False
for p in sorted(files):
 rel=str(p.relative_to(root)); st=p.stat(); archive=p.with_name(p.name+'.gz'); temp=p.with_name(p.name+'.cleanup-partial')
 assert rel not in tracked and str(archive.relative_to(root)) not in tracked
 if archive.exists() or temp.exists() or in_use(st):
  print('SKIP occupied/archive exists',rel,flush=True); continue
 digest=hashlib.sha256()
 with p.open('rb') as source, temp.open('xb') as raw:
  assert source.read(4)==b'\x7fELF',rel
  source.seek(0)
  with gzip.GzipFile(filename='',mode='wb',fileobj=raw,compresslevel=1,mtime=0) as dest:
   while chunk:=source.read(4*1024**2):digest.update(chunk);dest.write(chunk)
  raw.flush();os.fsync(raw.fileno())
 check=hashlib.sha256(); count=0
 with gzip.open(temp,'rb') as f:
  while chunk:=f.read(4*1024**2):check.update(chunk);count+=len(chunk)
 assert check.digest()==digest.digest() and count==st.st_size
 now=p.stat()
 assert (now.st_ino,now.st_size,now.st_mtime_ns,now.st_nlink)==(st.st_ino,st.st_size,st.st_mtime_ns,1)
 assert not in_use(st)
 archive_size=temp.stat().st_size
 if archive_size>=st.st_size:temp.unlink();continue
 os.replace(temp,archive)
 row=dict(original=rel,archive=str(archive.relative_to(root)),sha256=digest.hexdigest(),original_bytes=st.st_size,archive_bytes=archive_size,mode=oct(st.st_mode&0o777),mtime_ns=st.st_mtime_ns,status='verified_archive_original_retained')
 rows.append(row);manifest.write_text(json.dumps(rows,indent=2)+'\n')
 p.unlink();row['status']='compressed_original_removed';manifest.write_text(json.dumps(rows,indent=2)+'\n')
 print('ARCHIVED',rel,'freed',st.st_size-archive_size,'total',sum(r['original_bytes']-r['archive_bytes'] for r in rows if r['status']=='compressed_original_removed'),flush=True)
print('DONE',len(rows),'archives',flush=True)
