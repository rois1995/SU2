from pathlib import Path
import json,math,hashlib,sys
HERE=Path(__file__).resolve().parent;case=HERE.parent/sys.argv[1];results={};inputs={}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for work in sorted(case.glob('mpi*')):
 for physics_file in sorted(work.glob('resolved_vortex_m*_physics.json')):
  name=physics_file.stem[:-len('_physics')];p=json.loads(physics_file.read_text());seed_path=work/(name+'_seed_states.json');evolved_path=work/(name+'_s0_evolved_states.json')
  if not evolved_path.exists():continue
  seed=json.loads(seed_path.read_text())['accepted'];evolved=json.loads(evolved_path.read_text())['accepted'];assert len(seed)==676
  original={tuple(r['point']):r['states'] for r in seed};error=0.;profile=set();minimum=1.;maximum_change=0.
  for r in seed:
   x,y,z=r['point'];a=(x+.25)/p['radius'];b=y/p['radius'];shape=math.exp(1-a*a-b*b);g=p['gamma'];theta=1-(g-1)*p['beta']**2*shape/(8*g*math.pi**2)
   rho=p['rho0']*theta**(1/(g-1));pressure=p['p0']*theta**(g/(g-1));swirl=p['beta']/(2*math.pi)*math.sqrt(p['p0']/p['rho0']*shape);u=p['u0']-swirl*b;v=p['v0']+swirl*a
   expected=[rho,rho*u,rho*v,0,pressure/(g-1)+rho*(u*u+v*v)/2]
   error=max(error,max(abs(a-b)/max(1,abs(a)) for a,b in zip(expected,r['states'][:5])))
   assert r['states'][:5]==r['states'][5:10]==r['states'][10:15]
   if abs(y)<1e-12 and .2<z<.5 and abs(x+.25)<=2*p['radius']:profile.add(x)
  assert error<1e-12 and len(profile)>=8 and len(profile)==p['initial_profile_samples']
  for r in evolved:
   for off in (0,5,10):
    rho,mx,my,mz,E=r['states'][off:off+5];pressure=(p['gamma']-1)*(E-(mx*mx+my*my+mz*mz)/(2*rho));assert rho>0 and pressure>0
    if not off:minimum=min(minimum,pressure/p['p0'])
   maximum_change=max(maximum_change,max(abs(a-b)/max(1,abs(a),abs(b)) for a,b in zip(original[tuple(r['point'])],r['states'])))
  assert .5<minimum<.9 and maximum_change>1e-8
  key=str(physics_file.relative_to(case));results[key]={'analytic_seed_relative_error':error,'central_profile_samples':len(profile),'minimum_evolved_pressure_ratio':minimum,'maximum_state_change':maximum_change,'all_current_BDF_states_positive':True}
  for path in (physics_file,seed_path,evolved_path):inputs[str(path.relative_to(case))]=sha(path)
assert results,'No actual evolved vortex to audit'
out={'status':'PASS_INITIAL_AND_EVOLVED_PHYSICS_ONLY','results':results,'inputs_sha256':inputs,'script_sha256':sha(Path(__file__).resolve()),'scope':'Exact analytic compressible-vortex initialization and measured spatial sampling, genuinely changed actual Euler states and current/BDF positivity. Curved finite-domain evolution is not an exact unbounded-vortex solution; no remesh/accepted-mesh/restart/convergence/full Goal2 claim.'}
(HERE/'physics_audit.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(results,indent=2))
