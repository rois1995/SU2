import gzip,hashlib,json,os,subprocess,time
from pathlib import Path
root=Path.cwd();out=root/'integration_evidence/disk_cleanup_20261019_hardlinks'
selection=json.loads((out/'selection.json').read_text());manifest=out/'archives.json'
assert not manifest.exists(), 'Review existing manifest before resuming'
tracked=set(subprocess.check_output(['git','ls-files','-z']).decode().split('\0'))
referenced=set()
for folder,dirs,names in os.walk(root,followlinks=False):
 dirs[:]=[d for d in dirs if d not in ('.git','externals','subprojects','ClusterResults')]
 for name in names:
  p=Path(folder)/name
  if p.is_symlink():referenced.add(p.resolve())
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
rows=[];started=time.time()
for names in selection:
 paths=[root/n for n in names];p=paths[0];st=p.stat()
 archives=[p.with_name(p.name+'.gz') for p in paths]
 temp=archives[0].with_name(archives[0].name+'.cleanup-partial')
 assert len(paths)==st.st_nlink and len(paths)>1
 assert all(not p.is_symlink() and p.resolve() not in referenced for p in paths)
 assert all(n not in tracked and n+'.gz' not in tracked for n in names)
 assert all(not a.exists() for a in archives) and not temp.exists()
 assert all((q.stat().st_dev,q.stat().st_ino)==(st.st_dev,st.st_ino) for q in paths)
 if in_use(st):print('SKIP in use',names,flush=True);continue
 digest=hashlib.sha256()
 with p.open('rb') as source,temp.open('xb') as raw:
  assert source.read(4)==b'\x7fELF';source.seek(0)
  with gzip.GzipFile(filename='',mode='wb',fileobj=raw,compresslevel=1,mtime=0) as dest:
   while chunk:=source.read(4*1024**2):digest.update(chunk);dest.write(chunk)
  raw.flush();os.fsync(raw.fileno())
 check=hashlib.sha256();count=0
 with gzip.open(temp,'rb') as stream:
  while chunk:=stream.read(4*1024**2):check.update(chunk);count+=len(chunk)
 assert check.digest()==digest.digest() and count==st.st_size
 assert not in_use(st)
 for q in paths:
  now=q.stat()
  assert (now.st_dev,now.st_ino,now.st_size,now.st_mtime_ns,now.st_nlink)==(st.st_dev,st.st_ino,st.st_size,st.st_mtime_ns,len(paths))
 archive_size=temp.stat().st_size
 assert archive_size<st.st_size
 os.replace(temp,archives[0])
 for a in archives[1:]:os.link(archives[0],a)
 row=dict(originals=names,archives=[str(a.relative_to(root)) for a in archives],sha256=digest.hexdigest(),original_bytes=st.st_size,archive_bytes=archive_size,mode=oct(st.st_mode&0o777),mtime_ns=st.st_mtime_ns,status='verified_archives_originals_retained')
 rows.append(row);manifest.write_text(json.dumps(rows,indent=2)+'\n')
 for q in paths:q.unlink()
 row['status']='compressed_originals_removed';manifest.write_text(json.dumps(rows,indent=2)+'\n')
 print('ARCHIVED',len(names),'names',names[0],'freed',st.st_size-archive_size,flush=True)
summary=dict(groups=len(rows),paths=sum(len(r['originals']) for r in rows),physical_bytes_recovered=sum(r['original_bytes']-r['archive_bytes'] for r in rows),elapsed_seconds=time.time()-started)
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary),flush=True)
