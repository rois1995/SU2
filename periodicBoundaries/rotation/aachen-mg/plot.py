"""Plot the archived 10,000-iteration Aachen study without executing CFD."""
import csv,hashlib,io,json,os,tarfile
from pathlib import Path
os.environ['OMP_NUM_THREADS']='1'
os.environ['OPENBLAS_NUM_THREADS']='1'
os.environ.setdefault('MPLCONFIGDIR','/tmp/su2-aachen-mg-mpl')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
ROOT=Path(__file__).resolve().parent
CASES=['develop_mg1','pr_mg1','pr_mg2','pr_mg3']
LABELS=['develop MG1','PR MG1','PR MG2','PR MG3']
COLORS=['#555555','#2166ac','#d97706','#238b45']
STYLES=['-',(0,(6,2)),(0,(1,2)),(0,(6,2,1,2))]
MARKERS=['o','^','s','D']
FIELDS=['rms[Rho]','rms[RhoU]','rms[RhoV]','rms[RhoW]','rms[RhoE]','rms[nu]']
PERF=['TotTotEff[2]','PRTT[2]']+[f'MassFlow{s}_{z}[2]' for z in range(1,4) for s in ['In','Out']]
checks=json.loads((ROOT/'archived-files-sha256.json').read_text())
data={}
with tarfile.open(ROOT/'histories.tar.gz') as archive:
 def read(case,filename,fields):
  member=f'histories/{case}/{filename}';raw=archive.extractfile(member).read()
  assert hashlib.sha256(raw).hexdigest()==checks[member]
  rows=[{k.strip():v for k,v in row.items()} for row in csv.DictReader(io.StringIO(raw.decode()),skipinitialspace=True)]
  assert len(rows)==10000 and all(float(row['Outer_Iter'])==i for i,row in enumerate(rows))
  return {f:[float(row[f]) for row in rows] for f in fields}
 for case in CASES:
  data[case]={'h':[read(case,f'history_{z}.csv',FIELDS) for z in range(3)],'p':read(case,'aachen_3D_MP_restart.csv',PERF)}

def plot(ax,values,start=0):
 step=max(1,(10000-start)//(len(CASES)*12))
 for y,color,style in zip(values,COLORS,STYLES):
  ax.plot(range(start,10000),y[start:],color=color,ls=style,lw=1)
 for index,(y,color,marker) in enumerate(zip(values,COLORS,MARKERS)):
  positions=range(start+index*step+step//2,10000,step*len(CASES))
  ax.plot(list(positions),[y[i] for i in positions],ls='None',color=color,marker=marker,ms=4,markerfacecolor='white',zorder=4)
 ax.grid(alpha=.22);ax.set_xlabel('Outer iteration')

def save(fig,name,title):
 fig.suptitle(title,y=.995,fontsize=13)
 handles=[Line2D([],[],color=c,ls=s,marker=m,markerfacecolor='white',label=l) for c,s,m,l in zip(COLORS,STYLES,MARKERS,LABELS)]
 fig.legend(handles=handles,loc='upper center',bbox_to_anchor=(.5,.965),ncol=4)
 fig.tight_layout(rect=(0,0,1,.93))
 for ext in ('png','pdf'):fig.savefig(ROOT/f'{name}.{ext}',dpi=160)
 plt.close(fig)

zones=['Stator 1','Rotor','Stator 2']
fig,axes=plt.subplots(6,3,figsize=(16,18),sharex=True)
for row,field in enumerate(FIELDS):
 for zone in range(3):
  ax=axes[row,zone];plot(ax,[data[c]['h'][zone][field] for c in CASES]);ax.set_ylabel(field+'\nlog10 RMS')
  if row==0:ax.set_title(zones[zone])
save(fig,'all-residuals','Aachen turbine: develop and PR multigrid study\nAll runs completed 10,000 iterations; all requested levels retained')
fig,axes=plt.subplots(2,3,figsize=(16,10))
for row,start in enumerate((0,9000)):
 for zone in range(3):
  ax=axes[row,zone];plot(ax,[data[c]['h'][zone]['rms[RhoE]'] for c in CASES],start);ax.set_ylabel('Energy log10 RMS');ax.set_title(zones[zone]+(' — PR final 1000 iterations' if row else ''))
  if row:
   values=[v for c in CASES[1:] for v in data[c]['h'][zone]['rms[RhoE]'][start:]]
   low,high=min(values),max(values);margin=max(.01,.1*(high-low));ax.set_ylim(low-margin,high+margin)
save(fig,'energy-comparison','Aachen turbine energy residuals\nUpper: develop and PR; lower: PR MG1/2/3 over the final 1000 iterations')
fig,axes=plt.subplots(4,1,figsize=(13,13))
for row in range(4):
 values=[]
 for case in CASES:
  p=data[case]['p']
  if row<2:y=p[PERF[row]]
  elif row==2:y=[100*abs(a-b)/abs(a) for a,b in zip(p['MassFlowIn_1[2]'],p['MassFlowOut_3[2]'])]
  else:y=[max(100*abs(p[f'MassFlowOut_{z}[2]'][i]-p[f'MassFlowIn_{z+1}[2]'][i])/abs(p[f'MassFlowOut_{z}[2]'][i]) for z in (1,2)) for i in range(10000)]
  values.append(y)
 plot(axes[row],values);axes[row].set_ylabel(['Efficiency (%)','Total pressure ratio','Global mass mismatch (%)','Worst interface mismatch (%)'][row])
save(fig,'performance-comparison','Aachen turbine performance and reported mass-flow mismatch\nPerformance remains drifting at the iteration cap')
print('Archive hashes and 10,000 contiguous rows checked; direct-overlay plots generated.')
