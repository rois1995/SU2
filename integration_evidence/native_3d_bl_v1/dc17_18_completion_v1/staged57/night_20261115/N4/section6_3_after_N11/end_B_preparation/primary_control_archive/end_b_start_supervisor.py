from pathlib import Path
import subprocess,json,time,os
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/end_B_preparation/R3_GQ_controller_recovery').resolve();p.mkdir(exist_ok=True)
with (p/'supervisor.log').open('w') as log:
 process=subprocess.Popen(['/usr/bin/python3','/tmp/end_b_detached_supervisor.py'],start_new_session=True,stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,close_fds=True)
 f=(Path('/proc')/str(process.pid)/'stat').read_text().rsplit(')',1)[1].split();r=dict(actual_host_proc=True,unix=time.time(),supervisor_pid=process.pid,start_ticks=int(f[19]),numerical_job_not_restarted=True);(p/'detached_launch.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
