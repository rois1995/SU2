#!/usr/bin/env python3
"""Run a command (mpirun ... SU2_CFD ...), sample the RAM of all its SU2_CFD descendants.

Usage: rsswatch.py OUTPREFIX DT -- cmd args...
Writes OUTPREFIX.timeline.csv (t, nranks, sum rss/anon/file/shmem [KiB], max rank rss),
       OUTPREFIX.log (SU2 stdout+stderr, each line prefixed with its time stamp),
       OUTPREFIX.summary (key=value lines). Exit code = the command's exit code.
Anon RSS comes from /proc/pid/status (RssAnon) or, on old kernels, statm (resident - shared).
"""
import os, sys, time, subprocess, threading

PAGE_KIB = os.sysconf("SC_PAGE_SIZE") // 1024


def children_map():
    kids = {}
    for d in os.listdir("/proc"):
        if not d.isdigit():
            continue
        try:
            with open(f"/proc/{d}/stat") as f:
                s = f.read()
            ppid = int(s[s.rfind(")") + 2:].split()[1])
            kids.setdefault(ppid, []).append(int(d))
        except (OSError, ValueError, IndexError):
            pass
    return kids


def descendants(root):
    kids, out, stack = children_map(), [], [root]
    while stack:
        p = stack.pop()
        for c in kids.get(p, []):
            out.append(c); stack.append(c)
    return out


def mem(pid):
    try:
        with open(f"/proc/{pid}/status") as f:
            st = f.read()
    except OSError:
        return None
    if "\nName:\t" + os.environ.get("WATCH_NAME", "SU2_CFD") + "\n" not in "\n" + st:
        return None
    v = {}
    for line in st.splitlines():
        k, _, rest = line.partition(":")
        if k in ("VmRSS", "VmHWM", "RssAnon", "RssFile", "RssShmem"):
            v[k] = int(rest.split()[0])
    if "RssAnon" not in v:
        try:
            with open(f"/proc/{pid}/statm") as f:
                _, res, shared = (int(x) for x in f.read().split()[:3])
            v["RssAnon"] = (res - shared) * PAGE_KIB
            v["RssFile"] = shared * PAGE_KIB
            v["RssShmem"] = -1
        except (OSError, ValueError):
            return None
    # A process that is exiting (zombie) has no memory lines: skip it.
    if not all(k in v for k in ("VmRSS", "VmHWM", "RssAnon", "RssFile", "RssShmem")):
        return None
    return v


def main():
    sep = sys.argv.index("--")
    prefix, dt, cmd = sys.argv[1], float(sys.argv[2]), sys.argv[sep + 1:]
    t0 = time.time()
    log = open(prefix + ".log", "w")
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, universal_newlines=True)

    def pump():
        for line in proc.stdout:
            log.write(f"{time.time() - t0:10.3f} {line}")
        log.flush()
    th = threading.Thread(target=pump, daemon=True); th.start()

    hwm = {}
    peak = {"rss": 0, "anon": 0, "t_rss": 0.0, "t_anon": 0.0, "nranks": 0}
    with open(prefix + ".timeline.csv", "w") as tl:
        tl.write("t,nranks,rss_kib,anon_kib,file_kib,shmem_kib,max_rank_rss_kib\n")
        while True:
            done = proc.poll() is not None
            vals = {}
            for p in descendants(proc.pid):
                m = mem(p)
                if m:
                    vals[p] = m
            if vals:
                t = time.time() - t0
                s = {k: sum(m[k] for m in vals.values()) for k in ("VmRSS", "RssAnon", "RssFile", "RssShmem")}
                mx = max(m["VmRSS"] for m in vals.values())
                tl.write(f"{t:.3f},{len(vals)},{s['VmRSS']},{s['RssAnon']},{s['RssFile']},{s['RssShmem']},{mx}\n")
                for p, m in vals.items():
                    hwm[p] = max(hwm.get(p, 0), m["VmHWM"])
                peak["nranks"] = max(peak["nranks"], len(vals))
                if s["VmRSS"] > peak["rss"]:
                    peak["rss"], peak["t_rss"] = s["VmRSS"], t
                if s["RssAnon"] > peak["anon"]:
                    peak["anon"], peak["t_anon"] = s["RssAnon"], t
            if done:
                break
            time.sleep(dt)
    th.join(timeout=30)
    rc = proc.wait()
    log.close()
    with open(prefix + ".summary", "w") as f:
        f.write(f"rc={rc}\nwall_s={time.time() - t0:.2f}\nnranks={peak['nranks']}\n")
        f.write(f"peak_sum_rss_kib={peak['rss']}\nt_peak_rss={peak['t_rss']:.3f}\n")
        f.write(f"peak_sum_anon_kib={peak['anon']}\nt_peak_anon={peak['t_anon']:.3f}\n")
        f.write(f"sum_rank_hwm_kib={sum(hwm.values())}\nmax_rank_hwm_kib={max(hwm.values()) if hwm else 0}\n")
    sys.exit(rc)


if __name__ == "__main__":
    main()
