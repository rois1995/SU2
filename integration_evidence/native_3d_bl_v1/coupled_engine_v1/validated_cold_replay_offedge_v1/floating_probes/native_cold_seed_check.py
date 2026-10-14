exec(open('/tmp/native_cold_offedge_search.py').read().split('r=differential')[0])
reports=[]
for x,z in [(.34629839,.17411658),(.352388655,.16157705),(.524581,.08633989),(.266477,.1803863)]:
 for fraction,split in [( .5,.16873350382193855),(.55,.1916),(.565,.199)]:
  for e in [[.0933164,.0933164,.0933164],[.17565358,.17565358,.10542434],[.17565358,.17565358,.08042434],[.17565358,.17565358,.05542434],[.154416,.154416,.088677]]:
   q,j,det=assess([x,z,split,e[0],e[2]]);maximum=max(length(points[lookup[a]],points[lookup[b]]) for a,b in edges)
   if q>=.20 and j>=.05 and det>0:reports.append({'surface':[x,z],'fraction':fraction,'point':e,'q':q,'J':j,'max_length':maximum});print(reports[-1],flush=True)
Path('/tmp/native_cold_seed_check.json').write_text(json.dumps(reports,indent=2)+'\n')
