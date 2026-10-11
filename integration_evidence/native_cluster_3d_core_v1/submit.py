"""Submit serialized SGE jobs with result gates; run only when explicitly requested by the user."""
from pathlib import Path
import argparse,json,os,subprocess,time
from run import verify_package
ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('package',type=Path)
    parser.add_argument('--limits',action='store_true',help='Append one four-slot n8 practical-limit campaign; stops on its first failed case')
    parser.add_argument('--mpi-profile',action='store_true',help='Append paired whole-process mpiP campaigns; requires a frozen --mpip-library bundle')
    args=parser.parse_args()
    root=ROOT;bundle=args.package.resolve();bundle.relative_to(root);manifest=verify_package(bundle)
    if args.mpi_profile and 'bin/libmpiP.so' not in manifest['files_sha256']:raise ValueError('Prepare --mpip-library before submitting mpi-profile')
    if Path.cwd().resolve()!=root:raise ValueError('Submit from repository root')
    previous=None;correctness=None;lifecycles={};jobs=[]
    destination=root/'ClusterResults'/('native3d_submission_'+str(time.time_ns())+'_'+str(os.getpid())+'.json')
    destination.parent.mkdir(exist_ok=True)
    receipt=dict(status='SUBMITTING',jobs=jobs,pending=None)
    def save():destination.write_text(json.dumps(receipt,indent=2)+'\n')
    script=bundle/'integration_evidence/native_cluster_3d_core_v1/RunNative3DSGE.sh'
    for mode,ranks in [('correctness',4)]+[(kind,n) for kind in ('lifecycle','timing') for n in (1,2,4)]+([('limits',4)] if args.limits else [])+([('mpi-profile',n) for n in (1,2,4)] if args.mpi_profile else []):
        cmd=['qsub','-terse','-N','Native3D_'+mode+'_'+str(ranks),'-pe','mpi',str(ranks)]
        # SGE does not inherit the login compiler/MPI/library search paths by default.
        inherited=['PATH']+(['LD_LIBRARY_PATH'] if 'LD_LIBRARY_PATH' in os.environ else [])
        cmd+=['-v',','.join(inherited)]
        if previous:cmd+=['-hold_jid',previous]
        cmd += [str(script),str(bundle),mode]
        if correctness:cmd+=['--gate',str(correctness)]
        if mode in ('timing','limits','mpi-profile'):cmd+=['--gate',str(lifecycles[ranks])]
        receipt['pending']=dict(command=cmd);save()
        try:
            reply=subprocess.check_output(cmd,text=True).strip();receipt['pending']['reply']=reply;save();job=reply.split('.')[0]
            if not job.isdigit():raise RuntimeError('Unrecognized qsub reply; inspect queue before resubmitting: '+reply)
        except Exception as error:
            receipt.update(status='FAIL',failure=str(error));save();raise
        output=root/'ClusterResults'/('native3d_'+job+'_'+mode+'_n'+str(ranks))
        if mode=='correctness':correctness=output
        elif mode=='lifecycle':lifecycles[ranks]=output
        previous=job;jobs.append(dict(job=job,mode=mode,ranks=ranks,command=cmd,results=str(output)))
        receipt['pending']=None;save()
        print('Submitted',job,mode,ranks,'retained:',output,flush=True)
    receipt['status']='SUBMITTED';save();print('Submission receipt:',destination)
if __name__=='__main__':main()
