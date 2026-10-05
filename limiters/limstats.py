#!/usr/bin/env python3
"""limstats.py restart.csv : min/max/mean and count of zero / negative values of every Limiter_* column."""
import sys, csv
rows = list(csv.reader(open(sys.argv[1])))
head = [h.strip().strip('"') for h in rows[0]]
data = [[float(x) for x in r] for r in rows[1:] if r]
print(f"{sys.argv[1]}: {len(data)} points")
for j, h in enumerate(head):
    if not h.startswith("Limiter"): continue
    col = [r[j] for r in data]
    neg = sum(v < 0 for v in col); zero = sum(v == 0 for v in col); gt1 = sum(v > 1 for v in col)
    print(f"  {h:24s} min {min(col):+.4e} max {max(col):.4e} mean {sum(col)/len(col):.4f}  <0: {neg}  ==0: {zero}  >1: {gt1}")
