"""Losslessly archive historical sample text; verify full decompression before removing plain duplicate."""
from pathlib import Path
import gzip,hashlib,json,os,time
R=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57');D=R/'purpose_query_fix_20261114/cheap_group_20261115';receipt=D/'historical_profile_text_compression.json';assert not receipt.exists()
rows=[]
def digest(stream):
 h=hashlib.sha256();n=0
 while True:
  b=stream.read(1024*1024)
  if not b:break
  h.update(b);n+=len(b)
 return h.hexdigest(),n
for p in sorted((R/'measurement_20261114_v2/run_B/profile').glob('rank*.samples.txt')):
 assert p.is_file() and not p.is_symlink();st=p.stat();out=p.with_name(p.name+'.gz');assert not out.exists()
 with p.open('rb') as f:original,size=digest(f)
 with p.open('rb') as src,out.open('xb') as target:
  with gzip.GzipFile(filename='',mode='wb',fileobj=target,compresslevel=6,mtime=0) as packed:
   while True:
    b=src.read(1024*1024)
    if not b:break
    packed.write(b)
  target.flush();os.fsync(target.fileno())
 with gzip.open(out,'rb') as f:check,n=digest(f)
 assert check==original and n==size
 with out.open('rb') as f:compressed,_=digest(f)
 row={'original_path':str(p),'compressed_path':str(out),'original_bytes':size,'original_sha256':original,'compressed_bytes':out.stat().st_size,'compressed_sha256':compressed,'decompression_verified':True,'original_mode':st.st_mode,'original_mtime_ns':st.st_mtime_ns,'restoration':'gzip -dk <compressed_path>; check original SHA256 before use'}
 rows.append(row);receipt.write_text(json.dumps({'rows':rows,'all_content_preserved':True,'reason':'Respect unchanged global 2GB profile storage cap without changing sampling or deleting evidence','unix':time.time()},indent=2)+'\n');p.unlink()
print(json.dumps({'files':len(rows),'plain_bytes':sum(r['original_bytes'] for r in rows),'compressed_bytes':sum(r['compressed_bytes'] for r in rows),'all_decompression_hashes_verified':True}))
