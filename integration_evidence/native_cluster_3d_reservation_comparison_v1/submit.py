"""Submit four serialized SGE jobs only when explicitly invoked: correctness, then N1/2/4 comparisons."""
from pathlib import Path
import argparse,json,os,subprocess,time
from common import sha,verify
ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('package',type=Path)
    p.add_argument('--size',type=int,choices=(4,8),default=8);p.add_argument('--mpi-profile',action='store_true',help='Append three paired MPI API profiling jobs; both packages need mpiP')
    p.add_argument('--hold-jid',help='Existing job to finish before the first comparison job')
    a=p.parse_args();bundle=a.package.resolve();bundle.relative_to(ROOT);manifest=verify(bundle)
    if manifest.get('comparison_kind')not in ('rotating_reservation','candidate_batch'):raise ValueError('Wrong scheduler comparison bundle')
    if a.mpi_profile and (not (bundle/'baseline/bin/libmpiP.so').is_file() or not (bundle/'candidate/bin/libmpiP.so').is_file()):
        raise ValueError('Prepare both packages with mpiP before submission')
    if Path.cwd().resolve()!=ROOT:raise ValueError('Submit from repository root')
    if a.hold_jid and not a.hold_jid.isdigit():raise ValueError('hold-jid must be a numeric job ID')
    out=ROOT/'ClusterResults'/('native3d_reservation_submission_'+str(time.time_ns())+'_'+str(os.getpid())+'.json')
    out.parent.mkdir(exist_ok=True);jobs=[];record=dict(status='SUBMITTING',manifest_sha256=sha(bundle/'comparison_manifest.json'),jobs=jobs,pending=None)
    def save():out.write_text(json.dumps(record,indent=2)+'\n')
    previous=a.hold_jid;gate=None
    jobspec=[('correctness',4,False)]+[('timing',n,False) for n in (1,2,4)]+([('timing',n,True) for n in (1,2,4)] if a.mpi_profile else [])
    for mode,ranks,profiling in jobspec:
        cmd=['qsub','-terse','-N','Native3DReservation_'+mode+'_'+str(ranks),'-pe','mpi',str(ranks)]
        inherited=['PATH']+(['LD_LIBRARY_PATH'] if 'LD_LIBRARY_PATH' in os.environ else [])
        cmd+=['-v',','.join(inherited)]
        if previous:cmd+=['-hold_jid',previous]
        cmd+=[str(bundle/'suite/RunReservationComparisonSGE.sh'),str(bundle),mode,'--size',str(a.size)]
        if profiling:cmd+=['--mpi-profile']
        if gate:cmd+=['--gate',str(gate)]
        record['pending']=dict(command=cmd);save()
        try:
            reply=subprocess.check_output(cmd,text=True).strip();record['pending']['reply']=reply;save();job=reply.split('.')[0]
            if not job.isdigit():raise RuntimeError('Unrecognized qsub reply; inspect queue before resubmitting')
        except Exception as e:record.update(status='FAIL',failure=str(e));save();raise
        results=ROOT/'ClusterResults'/('native3d_reservation_'+job+'_'+mode+'_n'+str(ranks))
        if mode=='correctness':gate=results
        jobs.append(dict(job=job,mode=mode,ranks=ranks,mpi_profile=profiling,results=str(results),command=cmd));record['pending']=None;save();previous=job
        print('Submitted',job,mode,ranks,'retained:',results)
    record['status']='SUBMITTED';save();print('Submission receipt:',out)
if __name__=='__main__':main()
