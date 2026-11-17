from pathlib import Path
import importlib.util,subprocess,os,json,sys
v=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation').resolve();script=v/'run_restart552.py';spec=importlib.util.spec_from_file_location('restart552_actual_setup_stub',script);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);core=m.runner_context().load_base();core.apply_owned_cpu_budget();source=core.retargeted_source();case=v/'R3_restart_setup_attempt1/prelaunch_stub_case';assert not case.exists();os.environ.update(SU2_SECTION61_SOURCE_CASE=str(v/'full552'),SU2_SECTION61_RESTART_CASE=str(case));original=subprocess.Popen;seen=[]
class SetupReached(Exception):pass
def stub(*args,**kwargs):
 argv=args[0] if args else kwargs['args'];values=[str(x) for x in argv]
 if 'mpirun' in values and any(x.endswith('/SU2_CFD') for x in values):
  seen.append(dict(argv=values,cwd=str(kwargs.get('cwd'))));raise SetupReached()
 return original(*args,**kwargs)
subprocess.Popen=stub
try:
 try:exec(compile(source,str(script),'exec'),dict(__name__='__main__',__file__=str(script)))
 except SetupReached:pass
finally:subprocess.Popen=original
assert len(seen)==1 and (case/'resources.jsonl').stat().st_size==0 and (case/'run.log').stat().st_size==0;(v/'R3_restart_setup_attempt1/actual_prelaunch_stub.json').write_text(json.dumps(dict(passed=True,stopped_before_CFD_Popen=True,numerical_solver_launched=False,commands=seen),indent=2)+'\n');print('Actual restart setup reached CFD Popen stub; no numerical launch.')
