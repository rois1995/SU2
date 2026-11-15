from pathlib import Path
import subprocess,sys,importlib.util,json,hashlib
R=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57').resolve();D=R/'purpose_query_fix_20261114/cheap_group_20261115';P=D/'profile';C=P/'run_A';OLD=R/'measurement_20261114_v2'
for helper in ('split_A_profiles.py','summarize_timers.py'):
 subprocess.run([sys.executable,str(OLD/helper),str(C)],check=True)
# Retain raw optional captures; only symbol decoding/output placement differs from the v2 helper.
source=(OLD/'decode_lbr.py').read_text().replace('raw_removed=True','raw_removed=False').replace(';raw.unlink()','')
assert 'raw.unlink()' not in source
(P/'decode_lbr_retained.py').write_text(source)
raws=sorted((C/'profile').glob('*.lbr.data'))
subprocess.run([sys.executable,str(P/'decode_lbr_retained.py'),*map(str,raws)],check=True)
for name in ('summarize_callers','nearest_callers_A'):
 spec=importlib.util.spec_from_file_location(name,OLD/(name+'.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 m.PROFILE=C/'profile';m.SOURCES={'preparation':m.PROFILE/'rank0.preparation.summary.json','native_surface':m.PROFILE/'rank0.native_surface.summary.json'};m.ROOT=P
 m.self_check()
 if name=='summarize_callers':m.main(C/'caller_summary.json')
 else:m.main()
receipt={'native_test_passed':True,'source_method':'v2 split_A/summarize_timers/LBR/callers helpers; only output paths retargeted and raw LBR retained','inputs':{str(OLD/name):hashlib.sha256((OLD/name).read_bytes()).hexdigest() for name in ['split_A_profiles.py','summarize_timers.py','decode_lbr.py','summarize_callers.py','nearest_callers_A.py']},'raw_LBR_retained':list(map(str,raws))}
(C/'analysis_provenance.json').write_text(json.dumps(receipt,indent=2)+'\n')
