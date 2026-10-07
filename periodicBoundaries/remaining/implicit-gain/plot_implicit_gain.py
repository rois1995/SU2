from pathlib import Path
import csv,json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
base=Path(__file__).resolve().parent
r=base/'implicit_gain' if (base/'implicit_gain').is_dir() else base
fig,axes=plt.subplots(1,3,figsize=(14.5,4.3),sharey=True)
for ax,name,title in zip(axes,('rotation45','rotation45-linear50','axis30'),('45° annulus, CFL 100, max 4 linear steps','45° annulus, CFL 100, max 50 linear steps','30° pipe including the axis, CFL 10')):
 for mode,label,color,lw,alpha,zorder in [('legacy','Previous coupling','0.5',4.8,.45,2),('fixed-initial','Coupled operator, raw preconditioner','#cc5555',1.5,1,3),('fixed','Coupled operator + projected preconditioner','#0072b2',1.6,1,4)]:
  with (r/name/mode/'history.csv').open() as f:
   rd=csv.reader(f);keys=[s.strip().strip('"') for s in next(rd)]
   rows=[dict(zip(keys,map(float,row))) for row in rd]
  ax.plot([v['Inner_Iter'] for v in rows],[v['rms[Rho]'] for v in rows],label=label,color=color,lw=lw,alpha=alpha,zorder=zorder)
 ax.axhline(-10,color='k',linestyle=':',lw=.8,label='Convergence target')
 ax.set_title(title);ax.set_xlabel('Outer iterations');ax.grid(alpha=.15);ax.set_ylim(-11,-1)
axes[0].set_ylabel('log10 RMS density residual')
axes[2].legend(fontsize=7,loc='upper right')
fig.tight_layout();fig.savefig(str(r/'implicit_convergence.png'),dpi=170)
