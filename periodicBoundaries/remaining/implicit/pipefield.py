#!/usr/bin/env python3
"""pipefield.py <mesh.su2> <N> <out solution.csv>
Writes an ASCII restart with a smooth compressible field that is periodic with the rotation of 360/N degrees about z
(not axisymmetric inside a sector when the sector has more than one cell), zero velocity at the wall r = 1 and no
velocity normal to the axis on the axis."""
import sys, math
mesh, N, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
L = open(mesh).read().split("\n"); i = [k for k, l in enumerate(L) if l.startswith("NPOIN")][0]
n = int(L[i].split("=")[1].split()[0]); g = 1.4
o = ['"PointID","x","y","z","Density","Momentum_x","Momentum_y","Momentum_z","Energy"']
for p in range(n):
    x, y, z = map(float, L[i+1+p].split()[:3]); r = math.hypot(x, y); th = math.atan2(y, x) if r > 1e-12 else 0.0
    c = math.cos(N*th); w = 1.0 - r*r
    rho = 1.15*(1.0 + 0.02*math.cos(0.5*math.pi*z)*(1.0 - 0.5*r*r) + 0.01*r*r*c)
    ur = 5.0*r*w*(1.0 + 0.3*c)*math.sin(0.5*math.pi*z); ut = 8.0*r*w*(1.0 + 0.2*z); uz = 30.0*w*(1.0 + 0.1*r*r*c) + 2.0*w*z
    pr = 1.0e5*(1.0 + 0.01*(1.0 - 0.5*z) + 0.002*r*r*(1.0 + 0.5*c))
    ux = ur*math.cos(th) - ut*math.sin(th); uy = ur*math.sin(th) + ut*math.cos(th)
    E = pr/(g-1.0) + 0.5*rho*(ux*ux + uy*uy + uz*uz)
    o.append("%d, %.17e, %.17e, %.17e, %.17e, %.17e, %.17e, %.17e, %.17e" % (p, x, y, z, rho, rho*ux, rho*uy, rho*uz, E))
open(out, "w").write("\n".join(o) + "\n")
