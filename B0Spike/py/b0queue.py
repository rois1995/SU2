"""Job queue for the B0 runs: at most SLOTS processes of this agent, load-aware.

Job file: one job per line, "slots|workdir|command". Lines starting with # are skipped. A job whose workdir already
holds a file named DONE is skipped (restartable). Each job writes log.txt and DONE (with the exit code).
"""
import os, subprocess, sys, time

SLOTS = int(os.environ.get('B0_SLOTS', '1'))
MAXLOAD = float(os.environ.get('B0_MAXLOAD', '7'))


def load1():
    with open('/proc/loadavg') as f:
        return float(f.read().split()[0])


def main(jobfile):
    jobs = []
    for line in open(jobfile):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        s, wd, cmd = line.split('|', 2)
        if int(s) > SLOTS:
            raise ValueError(f'job needs {s} slots, limit is {SLOTS}: {wd}')
        jobs.append((int(s), wd, cmd))
    running = []
    i = 0
    while i < len(jobs) or running:
        running = [(p, s, wd) for (p, s, wd) in running if p.poll() is None or finish(p, wd)]
        used = sum(s for (_, s, _) in running)
        if i < len(jobs):
            s, wd, cmd = jobs[i]
            if os.path.exists(os.path.join(wd, 'DONE')):
                i += 1
                continue
            if used + s <= SLOTS and load1() <= MAXLOAD:
                os.makedirs(wd, exist_ok=True)
                print(f'start {i + 1}/{len(jobs)} {wd} (load {load1():.2f})', flush=True)
                log = open(os.path.join(wd, 'log.txt'), 'w')
                p = subprocess.Popen(cmd, shell=True, cwd=wd, stdout=log, stderr=subprocess.STDOUT)
                log.close()
                running.append((p, s, wd))
                i += 1
                continue
        time.sleep(2)
    print('all done', flush=True)


def finish(p, wd):
    with open(os.path.join(wd, 'DONE'), 'w') as f:
        f.write(str(p.returncode) + '\n')
    print(f'finish rc={p.returncode} {wd}', flush=True)
    return False


if __name__ == '__main__':
    main(sys.argv[1])
