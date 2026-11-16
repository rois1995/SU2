# M16 D9 raw counter extraction

No actual numbers are embedded in this helper; this report is generated only from the supplied workflow logs.
Counts remain separated by phase and site. Site `d` is inclusive and may nest `b`/`c`; site counts are not summed as an exclusive partition.
Query `seconds` is the instrumented function scope, not pure backend time. `MetricAt` argument lookup is outside kernel QJ timing; the candidate residual is reported explicitly.
Work part `other` has inclusive time 0 by design while its QJ time is real; no QJ value is subtracted from that zero inclusive value.
Star counts include early-return searches. Negative residuals indicate overlap or nonseparable inclusive scopes.

Workflow: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat`

## Event/rank accounting

| Event | Rank | Stars | Candidates | Candidate s | Export s | Export/candidate s | Shared QJ s | Shared QJ/candidate s | Candidate residual s | Noncandidate star residual s | Overlap warning | Source line | Source SHA-256 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---:|---:|---|
| 1 | 0 | 39491 | 48053 | 165.02585057099836 | 2.330381358 | 4.8496063887790564e-05 | 0.0 | 0.0 | 6.704400729998415 | 0.5748356350016306 | False | 1047 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | 26792 | 26548 | 75.89077704100019 | 1.1429250850000008 | 4.305126883381048e-05 | 0.0 | 0.0 | 3.793583052000177 | 0.38215948099986097 | False | 1774 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 1 | 48590 | 72170 | 184.2257588170008 | 3.384760575000004 | 4.6899827837051465e-05 | 0.0 | 0.0 | 7.5032760440007245 | 0.7779339440021404 | False | 771 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | 34102 | 30510 | 56.688574713000186 | 1.5752554549999993 | 5.163079170763682e-05 | 0.0 | 0.0 | 3.992440149000112 | 0.42060028899934343 | False | 1235 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 2 | 31338 | 43140 | 185.5210005659989 | 1.7986532469999976 | 4.169339932777e-05 | 0.0 | 0.0 | 9.693017232998622 | 0.5954639230018652 | False | 772 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | 36522 | 32196 | 120.23432330199921 | 1.3888474729999998 | 4.313726776618213e-05 | 0.0 | 0.0 | 3.9385807609992654 | 0.4222270720006507 | False | 1237 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 3 | 45301 | 60526 | 170.65210005099976 | 3.0407932890000056 | 5.0239455589333604e-05 | 0.0 | 0.0 | 7.121841669999042 | 0.6923692540028283 | False | 771 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | 8614 | 7968 | 49.99640765800008 | 0.3567139669999998 | 4.4768319151606404e-05 | 0.0 | 0.0 | 1.375332035000076 | 0.11916703299992315 | False | 1235 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 4 | 50956 | 67979 | 87.49588079100019 | 3.7061697599999923 | 5.451933332352627e-05 | 0.0 | 0.0 | 6.976443254000085 | 0.7691173529993045 | False | 774 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | 6932 | 9370 | 48.50683660400015 | 0.3910395779999996 | 4.1733145997865485e-05 | 0.0 | 0.0 | 1.0855453020001853 | 0.11610509299982169 | False | 1239 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 5 | 26474 | 33456 | 192.26969330600053 | 1.4478065210000026 | 4.327494383668109e-05 | 0.0 | 0.0 | 5.53355448200051 | 0.4331897230021453 | False | 771 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | 18347 | 16219 | 123.92780401599971 | 0.5926080190000003 | 3.6537888834083496e-05 | 0.0 | 0.0 | 2.6383701409995646 | 0.2213363930006229 | False | 1235 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |

## Per-site query counters

| Event | Rank | Phase | Site | Direct | Upper | Classification | Magnitude | Limit | Memo direct | Memo union | Upper work | Source line | Source SHA-256 |
|---:|---:|---|:---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | 0 | quality | a | 18881 | 46019 | 44629 | 844 | 546 | 11639 | 30725 | 63992452 | 1035 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | quality | b | 8726 | 35291 | 34288 | 242 | 761 | 2968 | 12060 | 86431297 | 1036 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | quality | c | 3 | 34 | 34 | 0 | 0 | 0 | 0 | 43897 | 1037 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1038 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | quality | e | 7758 | 22555 | 22139 | 416 | 0 | 5873 | 18613 | 18761875 | 1039 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1040 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1041 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1042 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1043 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1044 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1045 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 0 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 43 | 3702 | 107628422 | 1046 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | a | 5873 | 32059 | 32059 | 0 | 0 | 3491 | 21077 | 37144687 | 1762 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | b | 2094 | 20243 | 20243 | 0 | 0 | 550 | 5833 | 38853488 | 1763 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | c | 4 | 52 | 52 | 0 | 0 | 0 | 0 | 382468 | 1764 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1765 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | e | 2530 | 16082 | 16082 | 0 | 0 | 1881 | 13938 | 5734478 | 1766 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1767 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1768 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1769 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1770 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1771 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1772 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 2 | 0 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 51 | 4546 | 130628381 | 1773 | a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609 |
| 1 | 1 | quality | a | 23230 | 61439 | 61040 | 229 | 170 | 14812 | 40203 | 59400496 | 759 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | quality | b | 10218 | 46877 | 46621 | 55 | 201 | 2799 | 13792 | 88782636 | 760 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | quality | c | 15 | 24 | 22 | 1 | 1 | 1 | 5 | 101437 | 761 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | quality | e | 9909 | 31729 | 31649 | 80 | 0 | 8298 | 26102 | 13624039 | 763 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 1 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 201 | 3023 | 114747744 | 770 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | a | 6257 | 28998 | 28998 | 0 | 0 | 3758 | 20250 | 14392449 | 1223 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | b | 2158 | 22258 | 22258 | 0 | 0 | 593 | 6720 | 25186426 | 1224 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | c | 0 | 10 | 10 | 0 | 0 | 0 | 0 | 3162 | 1225 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | e | 2634 | 14933 | 14933 | 0 | 0 | 2238 | 13121 | 3221073 | 1227 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 2 | 1 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 51 | 3830 | 142555788 | 1234 | 7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b |
| 1 | 2 | quality | a | 18328 | 47476 | 43388 | 2321 | 1767 | 12466 | 34334 | 59195866 | 760 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | quality | b | 11807 | 34742 | 31964 | 623 | 2155 | 5115 | 13596 | 94998052 | 761 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | quality | c | 9 | 39 | 39 | 0 | 0 | 0 | 0 | 53487 | 762 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 763 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | quality | e | 6626 | 18380 | 17298 | 1082 | 0 | 5409 | 15635 | 12038363 | 764 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 770 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 2 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 1514 | 2769 | 113663614 | 771 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | a | 6272 | 41203 | 41203 | 0 | 0 | 3584 | 27303 | 63648022 | 1225 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | b | 2399 | 24908 | 24908 | 0 | 0 | 537 | 7114 | 60551851 | 1226 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | c | 2 | 28 | 28 | 0 | 0 | 0 | 0 | 71330 | 1227 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | e | 2791 | 20883 | 20883 | 0 | 0 | 2102 | 17699 | 19010806 | 1229 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1234 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1235 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 2 | 2 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 95 | 4029 | 132354538 | 1236 | a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049 |
| 1 | 3 | quality | a | 21071 | 55961 | 52453 | 1969 | 1539 | 12154 | 38377 | 47259104 | 759 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | quality | b | 9642 | 47266 | 44599 | 622 | 2045 | 2591 | 15389 | 86989048 | 760 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | quality | c | 6 | 20 | 20 | 0 | 0 | 0 | 0 | 46769 | 761 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | quality | e | 8411 | 25382 | 24415 | 967 | 0 | 5743 | 22174 | 10569752 | 763 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 3 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 225 | 2081 | 118073402 | 770 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | a | 4070 | 11737 | 11737 | 0 | 0 | 2790 | 8588 | 14471523 | 1223 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | b | 2004 | 6884 | 6884 | 0 | 0 | 629 | 2120 | 22771972 | 1224 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | c | 9 | 25 | 25 | 0 | 0 | 0 | 0 | 69715 | 1225 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | e | 1792 | 5618 | 5618 | 0 | 0 | 1386 | 4932 | 2956210 | 1227 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 2 | 3 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 2188 | 5177 | 133103907 | 1234 | 11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e |
| 1 | 4 | quality | a | 21075 | 58564 | 58372 | 107 | 85 | 13056 | 39436 | 28807532 | 762 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | quality | b | 8281 | 42561 | 42419 | 21 | 121 | 2471 | 13270 | 36283335 | 763 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | quality | c | 4 | 20 | 20 | 0 | 0 | 0 | 0 | 26909 | 764 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | quality | e | 8578 | 30347 | 30304 | 43 | 0 | 7077 | 26557 | 4905955 | 766 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 770 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 771 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 772 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 4 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 110 | 3046 | 118407364 | 773 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | a | 2664 | 10168 | 10168 | 0 | 0 | 1521 | 6779 | 14111623 | 1227 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | b | 1474 | 7727 | 7727 | 0 | 0 | 355 | 2174 | 23292779 | 1228 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | c | 9 | 18 | 18 | 0 | 0 | 0 | 0 | 77211 | 1229 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | e | 1156 | 4840 | 4840 | 0 | 0 | 871 | 4117 | 2845606 | 1231 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1234 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1235 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1236 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1237 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 2 | 4 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 1034 | 3267 | 138880625 | 1238 | e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b |
| 1 | 5 | quality | a | 14243 | 36749 | 35620 | 633 | 496 | 8851 | 24991 | 51704360 | 759 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | quality | b | 7307 | 28351 | 26976 | 307 | 1068 | 1793 | 7612 | 99744352 | 760 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | quality | c | 11 | 31 | 31 | 0 | 0 | 0 | 0 | 74846 | 761 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | quality | e | 5580 | 17400 | 17062 | 338 | 0 | 4665 | 14807 | 10303121 | 763 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 1 | 5 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 1116 | 3466 | 104164704 | 770 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | a | 6336 | 22412 | 22412 | 0 | 0 | 3650 | 14881 | 48565672 | 1223 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | b | 3030 | 13349 | 13349 | 0 | 0 | 748 | 4052 | 64663122 | 1224 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | c | 11 | 51 | 51 | 0 | 0 | 0 | 0 | 240481 | 1225 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | e | 3013 | 11027 | 11027 | 0 | 0 | 2050 | 9343 | 10058600 | 1227 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |
| 2 | 5 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 563 | 4953 | 122695485 | 1234 | 0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283 |

## Event-weighted candidate and site summary

Candidate totals are event-wide sums across six ranks. Per-candidate figures divide inclusive site function seconds by event candidates; they are descriptive weighted averages, not exclusive attribution.

| Event | Candidates | Stars | Candidate-scope seconds | Candidate ms/candidate | Candidate residual s | Star residual s |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 325,324 | 242,150 | 985.190284 | 3.028336 | 43.532533 | 3.842910 |
| 2 | 122,811 | 131,309 | 475.244723 | 3.869724 | 16.823851 | 1.681595 |

| Event | Site | D/U/Classify/Magnitude/Limit counts | Function seconds | ms/candidate | Upper-work/candidate |
|---:|:---:|---:|---:|---:|---:|
| 1 | a | 116,828 / 306,208 / 295,502 / 6,103 / 4,603 | 546.460778 | 1.679743 | 954.002 |
| 1 | b | 55,981 / 235,088 / 226,867 / 1,870 / 6,351 | 1,004.667505 | 3.088206 | 1,516.115 |
| 1 | c | 48 / 168 / 166 / 1 / 1 | 1.012861 | 0.003113 | 1.068 |
| 1 | d | 0 / 0 / 0 / 0 / 0 | 0.007186 | 0.000022 | 0 |
| 1 | e | 46,862 / 145,793 / 142,867 / 2,926 / 0 | 140.240436 | 0.431079 | 215.794 |
| 1 | g final proof | 224,358 / 189,732 / 189,732 / 0 / 0 | 3,024.807885 | 9.297832 | 2,080.035 |
| 2 | a | 31,472 / 146,577 / 146,577 / 0 / 0 | 349.010723 | 2.841852 | 1,566.097 |
| 2 | b | 13,159 / 95,369 / 95,369 / 0 / 0 | 482.332597 | 3.927438 | 1,916.112 |
| 2 | c | 35 / 184 / 184 / 0 / 0 | 1.896005 | 0.015438 | 6.875 |
| 2 | d | 0 / 0 / 0 / 0 / 0 | 0.005315 | 0.000043 | 0 |
| 2 | e | 13,916 / 73,383 / 73,383 / 0 / 0 | 88.736227 | 0.722543 | 356.864 |
| 2 | g final proof | 174,042 / 211,746 / 211,746 / 0 / 0 | 3,750.915777 | 30.542181 | 6,515.855 |

The D/U/classification/magnitude/limit site counters can overlap and must not be summed as unique calls. Site `d` is an inclusive/nested scope: its function time is nonzero even though its reported counters are zero. Function-scope seconds include caller work and are not pure geometry backend time. `MetricAt` argument lookup is excluded from kernel QJ timing. Candidate and star residuals expose time not accounted for by the named summaries; overlap and nesting prevent treating the rows as a disjoint compute-time partition.

## Per-rank log provenance

- Rank 0: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.0/stdout` SHA-256 `a24a3654a94b0d34083558b297e4eb73ddf617c782a6bf832bc88bda25aa3609`
- Rank 1: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.1/stdout` SHA-256 `7f02e735ce9c10118ec2964659d6e63ec4f988dfcbf6b03f143b83990d46755b`
- Rank 2: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.2/stdout` SHA-256 `a2ea2e04c0321ef3470c6b980c8df9a04f374812da398cf1d65edd5174848049`
- Rank 3: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.3/stdout` SHA-256 `11079a147e857accac58d5a31ce38aeaaef2ee832ac3b136afa53a28a3937d6e`
- Rank 4: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.4/stdout` SHA-256 `e9b431a37ef6c51aa4e2b4d159c81f3f44d3bb661d36d862de67ad04d9bd651b`
- Rank 5: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow_R1_repeat/rank_output/1/rank.5/stdout` SHA-256 `0dc09b3494ce5d488f4acd406f0f3b60840e8fe6e08a660f2ce40c858660b283`

Detailed per-site counts, source line numbers, and source hashes are in the CSV and JSON outputs.
