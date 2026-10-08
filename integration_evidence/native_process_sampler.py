"""Low-cost host process and rank-memory sampling for native benchmark runners."""
import subprocess
from pathlib import Path

def status_memory(status):
    return {key: int(value.split()[0]) for key, _, value in
            (line.partition(':') for line in status.splitlines()) if key in ('VmRSS', 'VmHWM')}

assert status_memory('Name: test\nVmRSS: 12 kB\nVmHWM: 34 kB\n') == {'VmRSS': 12, 'VmHWM': 34}

def compute_processes(wd):
    result = []
    for line in subprocess.check_output(['ps', '-eo', 'pid,stat,comm'], text=True).splitlines()[1:]:
        pid, flags, name = line.split(maxsplit=2)
        if 'Z' in flags or not (name in ('ninja', 'cc1plus', 'test_driver', 'test_driver_AD', 'test_memory') or name.startswith('SU2_CFD')):
            continue
        proc = Path('/proc') / pid
        try:
            cwd = str((proc / 'cwd').resolve(strict=True))
            status = (proc / 'status').read_text()
        except OSError:
            if not proc.exists():
                continue
            # Keep visible heavy processes in the launch gate even if proc status is restricted.
            cwd, status = '<unavailable>', ''
        labels=dict(line.partition(':')[::2] for line in status.splitlines() if ':' in line)
        result.append(dict(cpu_allowed_list=labels.get('Cpus_allowed_list','').strip(),threads=int(labels.get('Threads','0')),pid=int(pid), name=name, working_directory=cwd,
                           owned_solver=(name.startswith('SU2_CFD') or name.startswith('test_driver')) and cwd == str(wd),
                           memory_kib=status_memory(status)))
    return result
