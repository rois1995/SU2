#!/usr/bin/env python3
"""Restore only hash-pinned owned input duplicates; never overwrite changed data."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
for row in json.loads((HERE/'inputs.json').read_text())['pieces']:
 p=HERE/row['file']
 if p.exists():assert hashlib.sha256(p.read_bytes()).hexdigest()==row['sha256'];continue
 source=ROOT/row['compressed_source'];assert hashlib.sha256(source.read_bytes()).hexdigest()==row['compressed_sha256']
 h=hashlib.sha256()
 with gzip.open(source,'rb') as f,p.open('xb') as out:
  for b in iter(lambda:f.read(2**20),b''):h.update(b);out.write(b)
 assert h.hexdigest()==row['sha256']
print('Verified/restored only the four exact local owned input duplicates.')
