"""Focused local producer qualification; MPI2 exists only to exercise an actual empty rank."""
from pathlib import Path
import hashlib,json,os,sys,tempfile
import run_cases
ROOT=Path(__file__).resolve().parent
assert len(sys.argv)==2 and sys.argv[1] in ("serial","mpi2")
mode=sys.argv[1];receipt=ROOT/("capture262_"+mode+"_wrapper.json");assert not receipt.exists()
assert json.loads((ROOT/"build262_terminal_handles.json").read_text())["all_owned_handles_terminal"]
assert json.loads((ROOT/"build262_status.json").read_text())["production_cfd_byte_identical"]
case="failure_capture" if mode=="serial" else "capture_empty_mpi2"
if mode=="mpi2":run_cases.CASES[case]=(2,"[NativeFailureCapture3D]")
for key in list(os.environ):
    if key.startswith("SU2_NATIVE_"):del os.environ[key]
scratch=ROOT.parents[3]/"integration_evidence/build-integrated-v2/compile-tmp"
os.environ.update(OMP_NUM_THREADS="1",OPENBLAS_NUM_THREADS="1",MKL_NUM_THREADS="1",TMPDIR=str(scratch),OMPI_MCA_osc="pt2pt")
os.environ["OMPI_MCA_orte_tmpdir_base"]=tempfile.mkdtemp(prefix="capture262-"+mode+"-",dir=scratch)
before=hashlib.file_digest(Path(__file__).open("rb"),"sha256").hexdigest()
previous=set((ROOT/"cases").glob(case+"_*"));sys.argv=[str(Path(run_cases.__file__)),"build262",case]
try:run_cases.main()
finally:
    assert before==hashlib.file_digest(Path(__file__).open("rb"),"sha256").hexdigest()
    created=set((ROOT/"cases").glob(case+"_*"))-previous;assert len(created)==1
    attempt=next(iter(created));receipt.write_text(json.dumps({"case":str(attempt),"mode":mode,"wrapper_sha256_before":before,"wrapper_sha256_after":before,"scope":"Focused capture-only check;MPI2givesactualemptyrank.No broadgeometry/MPIcampaign or M6acceptanceclaim."},indent=2)+"\n")
