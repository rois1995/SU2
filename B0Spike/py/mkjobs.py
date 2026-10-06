"""Job files of the B0 experiments (B_DESIGN_EXPLORATION.md 6.1). Usage: mkjobs.py <experiment> [args]"""
import os, sys

R = '/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/runs/b0'
BIN = R + '/bin/b0spike'
DIM = dict(box3s=3, box2d=2, box2x=2, box2i=2, naca0=2, naca1=2, cyl0=2, box3d=3, m60=3, m61=3)


def serial(case):
    return ','.join(f'{R}/n0/{case}/ser_s{i}.b0in' for i in range(6))


def allow(case):
    p = f'{R}/n0/allow{DIM[case]}d.txt'
    return open(p).read().strip() if os.path.exists(p) else ''


def run(case, wd, slots=1, extra='', ranks=1, timeout=3600):
    a = allow(case)
    cmd = f'timeout --kill-after=30s {timeout} '
    cmd += f'mpirun -n {ranks} ' if ranks > 1 else ''
    cmd += f'{BIN} run {R}/cases/{case}.b0in --serial {serial(case)} --out result.json'
    if a:
        cmd += f' --allow {a}'
    cmd += ' ' + extra
    return f'{max(slots, ranks)}|{wd}|{cmd}'


def n2():
    jobs = []
    for case in ['naca1', 'naca0', 'box2d', 'cyl0', 'box3d', 'm60', 'm61']:
        Ps = [2, 4, 8, 16] if DIM[case] == 2 else ([2, 8, 32] if case != 'm61' else [8, 32])
        for P in Ps:
            for pl in ['P0', 'Pb', 'P3']:
                for fl in [1, 0]:
                    wd = f'{R}/n2/{case}/{pl}_P{P}_f{fl}'
                    jobs.append(run(case, wd, extra=f'--placement {pl} --P {P} --floor {fl} --sched level0 --level0stats 1'))
    return jobs


def n2cal():
    jobs = []
    for case in ['naca1', 'box2d']:
        for (a, b) in [(50, 20), (10, 5), (200, 50), (50, 100), (0, 100), (200, 0)]:
            for P in [4, 16]:
                wd = f'{R}/n2cal/{case}/a{a}_b{b}_P{P}'
                jobs.append(run(case, wd, extra=f'--placement P3 --a {a} --b {b} --P {P} --floor 1 --sched level0 --level0stats 1'))
    return jobs



def n1inj(ranks='1'):
    """Failure injection at every stage, level 0 (step 0) and level 1 (step 1), first piece, S-base and S-batch."""
    ranks = int(ranks)
    jobs = []
    for case in ['box2d', 'naca1', 'box3d']:
        for sched in ['sbase', 'sbatch']:
            for stage in ['planning', 'allocation', 'migration', 'mmg', 'interpolation', 'validation', 'splice', 'commit']:
                for step in [0, 1]:
                    wd = f'{R}/n1/inj_r{ranks}/{case}/{sched}_{stage}_first_s{step}'
                    jobs.append(run(case, wd, ranks=ranks, timeout=1200,
                                    extra=f'--placement Pb --P 4 --sched {sched} --inject {stage}:first:{step}'))
            wd = f'{R}/n1/inj_r{ranks}/{case}/{sched}_none'
            jobs.append(run(case, wd, ranks=ranks, timeout=1200, extra=f'--placement Pb --P 4 --sched {sched}'))
    return jobs


def n1more():
    """2 real ranks, first-piece injection at step 0 and largest-piece injection at step 1,
    admission budget, MMG memory cap, repeats, several patches per rank (memory).
    Former all-virtual-rank cases are replaced by largest-piece controls: injection now selects one piece.
    """
    jobs = []
    for case in ['box2d', 'box3s']:
        for sched in ['sbase', 'sbatch']:
            for stage in ['planning', 'allocation', 'migration', 'mmg', 'interpolation', 'validation', 'splice', 'commit']:
                wd = f'{R}/n1/inj_r2/{case}/{sched}_{stage}_first_s0'
                jobs.append(run(case, wd, ranks=2, timeout=1200, extra=f'--placement Pb --P 4 --sched {sched} --inject {stage}:first:0'))
                wd = f'{R}/n1/inj_r2/{case}/{sched}_{stage}_largest_s1'
                jobs.append(run(case, wd, ranks=2, timeout=1200, extra=f'--placement Pb --P 4 --sched {sched} --inject {stage}:largest:1'))
            wd = f'{R}/n1/inj_r2/{case}/{sched}_none'
            jobs.append(run(case, wd, ranks=2, timeout=1200, extra=f'--placement Pb --P 4 --sched {sched}'))
    # repeats: same P, 1 rank twice (compare with the r1 runs and r2 runs)
    for case in ['box2d', 'naca1', 'box3d', 'm60']:
        for rep in [1, 2]:
            wd = f'{R}/n1/repeat/{case}/sbase_rep{rep}'
            jobs.append(run(case, wd, extra='--placement Pb --P 8 --sched sbase'))
        wd = f'{R}/n1/repeat/{case}/sbase_r2'
        jobs.append(run(case, wd, ranks=2, extra='--placement Pb --P 8 --sched sbase'))
    # admission budget (transaction peak): tight budgets -> unadmitted pieces, INCOMPLETE expected, no hang
    for case, budgets in [('box3s', [1200, 400, 150]), ('naca1', [20, 8])]:
        for b in budgets:
            wd = f'{R}/n1/budget/{case}/b{b}'
            jobs.append(run(case, wd, ranks=2, extra=f'--placement Pb --P 4 --sched sbase --budget {b}'))
    # several patches per rank: S-batch on 3D cases (memory measured per virtual rank vs predicted)
    for case in ['box3d', 'm60']:
        for P in [2, 4]:
            wd = f'{R}/n1/mem/{case}/sbatch_P{P}'
            jobs.append(run(case, wd, extra=f'--placement Pb --P {P} --sched sbatch'))
    # MMG at its memory cap (whole-mesh call, separate processes)
    for case, caps in [('box3d', [800, 300, 150, 60, 20]), ('m60', [1500, 400, 150, 50])]:
        for c in caps:
            wd = f'{R}/n1/mmgcap/{case}/cap{c}'
            jobs.append(f'1|{wd}|timeout --kill-after=30s 900 /usr/bin/time -v {BIN} mmgcap {R}/cases/{case}.b0in {c}')
    return jobs


CASES = ['naca1', 'box2d', 'cyl0', 'box3d', 'm60', 'm61']


def Ps(case):
    return [4, 16] if DIM[case] == 2 else [8, 32]


def n3(floor='1', a='50', b='100'):
    """Full runs, placement varied (Pb / P3), S-base, carried metric; L = 2 and 3 in 2D, L = 3 in 3D (diagnostic:
    L = 2 doubles the vertex count of thin 3D level-1 bands); plus the adversarial partitions."""
    jobs = []
    for case in CASES:
        Ls = [2, 3] if DIM[case] == 2 else [3]
        for P in (Ps(case) if DIM[case] == 2 else [8]):
            for pl in ['Pb', 'P3']:
                for L in Ls:
                    wd = f'{R}/n3/{case}/{pl}_P{P}_L{L}'
                    jobs.append(run(case, wd, timeout=9000, extra=f'--placement {pl} --P {P} --L {L} --floor {floor} --a {a} --b {b} --sched sbase'))
    adv = [('naca1', 'shockcut', 'line:0.62,0,1,0', 2), ('naca0', 'shockcut', 'line:0.62,0,1,0', 2),
           ('box2d', 'bandcut', 'line:0.5,0.5,0.8660254,0.5', 2), ('box2d', 'oblique', 'line:0.5,0.5,-0.5,0.8660254', 2),
           ('box2d', 'oblique45', 'line:0.5,0.5,0.9659258,-0.258819', 2),
           ('box3s', 'jumpcut', 'plane:0.5,0.5,0.5,1,0,0', 2), ('box3s', 'bandcut', 'plane:0.5,0.5,0.5,0.6666667,0.6666667,0.3333333', 2),
           ('box2d', 'random16', 'random:7', 16), ('naca1', 'random16', 'random:7', 16), ('box3s', 'random32', 'random:7', 32),
           ('box2d', 'emptymid3', 'emptymid', 3), ('box3s', 'emptymid5', 'emptymid', 5),
           ('box2x', 'disconnected4', 'Pb', 4), ('box2x', 'disconnectedP0_4', 'P0', 4),
           ('box2i', 'imbalP0_8', 'P0', 8), ('box2i', 'imbalPb_8', 'Pb', 8)]
    for case, tag, pl, P in adv:
        wd = f'{R}/n3adv/{case}/{tag}'
        L = 3 if DIM[case] == 3 else 2
        jobs.append(run(case, wd, extra=f'--placement {pl} --P {P} --L {L} --floor {floor} --sched sbase'))
    wd = f'{R}/n3adv/box2d/tinycap'
    jobs.append(run('box2d', wd, extra=f'--placement Pb --P 8 --floor {floor} --sched sbase --capmem 0.02'))
    return jobs


def n4(winner='P3', floor='1', a='50', b='100'):
    """Schedulers S-B4 / S-batch on Pb and the N3 candidate (S-base runs are the N3 runs). 3D: box3d and m60, P = 8."""
    jobs = []
    pls = ['Pb'] + ([winner] if winner != 'Pb' else [])
    for case in CASES:
        if case == 'm61':
            continue
        L = 2 if DIM[case] == 2 else 3
        for P in (Ps(case) if DIM[case] == 2 else [8]):
            for pl in pls:
                for sched in ['sb4', 'sbatch']:
                    wd = f'{R}/n4/{case}/{pl}_{sched}_P{P}'
                    jobs.append(run(case, wd, timeout=9000, extra=f'--placement {pl} --P {P} --L {L} --floor {floor} --a {a} --b {b} --sched {sched}'))
    return jobs


def n5(pl='Pb', floor='1', a='50', b='100'):
    """Carried metric vs immutable background metric. No N4 candidate passed Class II, so both the control (S-base)
    and the best measured scheduler (S-batch) are run with the background metric; the carried-metric counterparts are
    the N3 runs Pb_P{P}_L2 (2D) / Pb_P8_L3 (3D) and the N4 runs Pb_sbatch_P{P}. m61 left out (time)."""
    jobs = []
    for case in CASES:
        if case == 'm61':
            continue
        L = 2 if DIM[case] == 2 else 3
        for P in (Ps(case) if DIM[case] == 2 else [8]):
            for sched in ['sbase', 'sbatch']:
                wd = f'{R}/n5/{case}/bg_{pl}_{sched}_P{P}'
                jobs.append(run(case, wd, timeout=9000, extra=f'--placement {pl} --P {P} --L {L} --floor {floor} --a {a} --b {b} --sched {sched} --background 1'))
    return jobs


if __name__ == '__main__':
    exp = sys.argv[1]
    for j in globals()[exp](*sys.argv[2:]):
        print(j)
