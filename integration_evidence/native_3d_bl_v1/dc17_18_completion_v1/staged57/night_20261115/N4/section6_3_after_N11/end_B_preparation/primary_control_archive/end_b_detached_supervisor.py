from pathlib import Path
import os,subprocess,json,time,signal
signal.signal(signal.SIGHUP,signal.SIG_IGN)
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/end_B_preparation/R3_GQ_controller_recovery').resolve();p.mkdir(exist_ok=True)
def identity(pid):
 f=(Path('/proc')/str(pid)/'stat').read_text().rsplit(')',1)[1].split();return dict(pid=pid,start_ticks=int(f[19]))
with (p/'adopted_monitor.log').open('w') as log:
 process=subprocess.Popen(['/usr/bin/python3','/tmp/end_b_adopt_gq.py'],stdout=log,stderr=subprocess.STDOUT,stdin=subprocess.DEVNULL)
 (p/'supervisor_launch.json').write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),supervisor=identity(os.getpid()),worker=identity(process.pid)),indent=2)+'\n')
 code=process.wait()
(p/'supervisor_completion.json').write_text(json.dumps(dict(actual_host_proc=True,unix=time.time(),worker_actual_exit_code=code,worker_pid=process.pid),indent=2)+'\n')
raise SystemExit(code)
