#!/usr/bin/env python3
"""Native composed-cost breakdown on preserved complete-support clouds; no CFD or relaxation."""
import csv,gzip,json,math,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
import run
ROOT=run.ROOT
EXTRA=[ROOT/p for p in ('UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp',
 'UnitTests/SU2_CFD/adaptation/NativeBLCostDiagnostic3D.hpp','SU2_CFD/src/adaptation/CNativeComplexity3D.cpp',
 'Common/src/adaptation/CNativeDonor3D.cpp')]+[Path(__file__).resolve()]
BASE_PINS=run.pins
run.pins=lambda:dict(BASE_PINS(),**{str(p.relative_to(ROOT)):run.sha(p) for p in EXTRA})

def statistics(values,count):
 y=np.asarray(values,dtype=np.longdouble)
 mean=y.sum()/count
 se=np.sqrt(max(np.longdouble(0),(np.sum(y*y)-count*mean*mean)/(count*(count-1))))
 return {'value':float(mean),'heuristic_3SE':float(3*se),'nonzero':int(np.count_nonzero(y))}

def main():
 label=sys.argv[1];ranks=int(sys.argv[2]);assert ranks in (2,4)
 original=HERE.parent/label;prep=json.loads((original/'preparation.json').read_text());count=prep['samples'];assert count==8192
 domain=ROOT/prep['domain'];queries=original/'queries.bin';fixture=run.OLD/'cell329/replay.dat'
 assert run.sha(domain)==prep['domain_sha256'] and run.sha(queries)==prep['queries_sha256']
 assert run.sha(original/'proposal_samples.npz')==prep['proposal_sha256']
 for name,digest in prep['sha256'].items():assert run.sha(ROOT/name)==digest,name
 folder=HERE/label/('np%d'%ranks);folder.mkdir(parents=True,exist_ok=True)
 sources=EXTRA+[ROOT/name for name in BASE_PINS() if (ROOT/name).suffix in ('.cpp','.hpp','.py')]
 capsule=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
 archive=folder/'executed_source.json.gz'
 if archive.exists():assert gzip.decompress(archive.read_bytes())==gzip.decompress(capsule)
 else:archive.write_bytes(capsule)
 report=run.invoke(folder,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(domain),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(queries),
  'SU2_NATIVE_BL_COST_HMAX':'100'},fixture,ranks)
 rows=[]
 for p in sorted(folder.glob('bl_cost_rank*.csv')):rows.extend(csv.DictReader(p.open()))
 rows.sort(key=lambda row:(int(row['row']),int(row['wall'])))
 assert len({(r['row'],r['wall']) for r in rows})==len(rows)
 baseline=[]
 for p in (original/'np4').glob('correction_rank*.csv'):baseline.extend(csv.DictReader(p.open()))
 baseline.sort(key=lambda row:int(row['row']));assert len(baseline)==count
 by_row={}
 for r in rows:
  assert 0<=int(r['row'])<count
  for key in ('distance','hn','weight','ht_min','ht_max','original_weighted_marginal','coarsest_weighted_marginal'):
   assert math.isfinite(float(r[key]))
  assert float(r['hn'])>0 and 0<float(r['weight'])<=1 and 0<float(r['ht_min'])<=float(r['ht_max'])
  by_row[int(r['row'])]=by_row.get(int(r['row']),0.)+float(r['original_weighted_marginal'])
 old=np.array([float(r['weighted_correction']) for r in baseline]);now=np.array([by_row.get(i,0.) for i in range(count)])
 drift=float(np.max(abs(old-now)/np.maximum(1,abs(old))))
 assert drift<1e-8,(drift,'Changed frozen composed cost')
 lines=fixture.read_text().splitlines();n,_=map(int,lines[1].split());h0,growth,thickness=map(float,lines[2].split())
 xyz=np.array([line.split() for line in lines[3:3+n]],float);axis=int(np.argmax(np.ptp(xyz,axis=0)));lo,hi=xyz[:,axis].min(),xyz[:,axis].max()
 levels=sorted(set([0.,thickness]+[d for d in (h0,2*h0,5*h0,10*h0,50*h0,100*h0,.5*thickness,.9*thickness) if 0<d<thickness]))
 def bucket(selected):
  result={key:statistics([float(r[key+'_weighted_marginal']) for r in selected],count) for key in ('original','coarsest')}
  mass=sum(max(0,float(r['coarsest_weighted_marginal'])) for r in selected)
  result['points']=len(selected)
  if mass:
   for key in ('hn','ht_min','ht_max'):
    result['coarsest_positive_mass_weighted_'+key]=sum(max(0,float(r['coarsest_weighted_marginal']))*float(r[key]) for r in selected)/mass
  return result
 bins=[]
 for a,b in zip(levels,levels[1:]):
  selected=[r for r in rows if a<=float(r['distance'])<b]
  bins.append(dict(lower=a,upper=b,**bucket(selected)))
 regions=[]
 for j in range(4):
  a=lo+(hi-lo)*j/4;b=lo+(hi-lo)*(j+1)/4
  selected=[r for r in rows if min(3,max(0,int(4*(float(r[('foot_x','foot_y','foot_z')[axis]])-lo)/(hi-lo))))==j]
  regions.append(dict(lower=a,upper=b,axis=axis,**bucket(selected)))
 protected=[]
 for b in levels[1:]:protected.append(dict(distance=b,**bucket([r for r in rows if float(r['distance'])<b])))
 result={'scope':'Original RAW9 sensor and explicit constant HMAX100 coarsest endpoint, complete-support proposal. No profile relaxation, production normalization or construction.',
  'all':bucket(rows),'distance_bins':bins,'surface_coordinate_regions':regions,'protected_band_cumulative':protected,
  'samples':count,'ranks':ranks,'heuristic_uncertainty':True,'proven_bound':False,'wall_marginal_order':'existing sorted marker order; not independent wall mass',
  'sensor_background_original':prep['sensor'],'normal_growth':growth,'first_height':h0,'unchanged_thickness':thickness,
  'maximum_saved_original_row_relative_drift':drift,'native_seconds':report['elapsed_seconds'],
  'provenance_sha256':run.sha(folder/'provenance.json'),'input_preparation_sha256':run.sha(original/'preparation.json'),
  'rows_sha256':{p.name:run.sha(p) for p in folder.glob('bl_cost_rank*.csv')},'helper_sha256':run.sha(Path(__file__))}
 if ranks==4 and (HERE/label/'np2/diagnosis.json').exists():
  other=[]
  for p in (HERE/label/'np2').glob('bl_cost_rank*.csv'):other.extend(csv.DictReader(p.open()))
  other.sort(key=lambda row:(int(row['row']),int(row['wall'])))
  assert rows==other,'MPI changes a diagnostic sample'
  result['MPI2_4_rows_identical']=True
 (folder/'diagnosis.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
 print('DIAGNOSED',label,ranks,json.dumps(result['all']),flush=True)
if __name__=='__main__':main()
