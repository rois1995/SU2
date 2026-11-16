# M16 D9 raw counter extraction

No actual numbers are embedded in this helper; this report is generated only from the supplied workflow logs.
Counts remain separated by phase and site. Site `d` is inclusive and may nest `b`/`c`; site counts are not summed as an exclusive partition.
Query `seconds` is the instrumented function scope, not pure backend time. `MetricAt` argument lookup is outside kernel QJ timing; the candidate residual is reported explicitly.
Work part `other` has inclusive time 0 by design while its QJ time is real; no QJ value is subtracted from that zero inclusive value.
Star counts include early-return searches. Negative residuals indicate overlap or nonseparable inclusive scopes.

Workflow: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow`

## Event/rank accounting

| Event | Rank | Stars | Candidates | Candidate s | Export s | Export/candidate s | Shared QJ s | Shared QJ/candidate s | Candidate residual s | Noncandidate star residual s | Overlap warning | Source line | Source SHA-256 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---:|---:|---|
| 1 | 0 | 39491 | 48053 | 183.9020973009995 | 2.7340182989999935 | 5.6895892015066565e-05 | 0.0 | 0.0 | 7.514083683999992 | 0.633938590001426 | False | 1047 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | 26792 | 26548 | 85.01655132400013 | 1.2907957120000029 | 4.8621203555823524e-05 | 0.0 | 0.0 | 4.201895427999856 | 0.37936586600025834 | False | 1774 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 1 | 48590 | 72170 | 206.63952313099657 | 3.7679262210000015 | 5.220903728696136e-05 | 0.0 | 0.0 | 8.48064177299689 | 0.8545549370039769 | False | 771 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | 34102 | 30510 | 62.9891901120002 | 1.742884119000002 | 5.712501209439534e-05 | 0.0 | 0.0 | 4.554207306999971 | 0.4595956819989704 | False | 1235 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 2 | 31338 | 43140 | 211.54626929299815 | 2.0286578829999984 | 4.702498569772829e-05 | 0.0 | 0.0 | 10.89955158399816 | 0.6696121840013474 | False | 772 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | 36522 | 32196 | 135.66671713999983 | 1.5427184230000002 | 4.7916462386631886e-05 | 0.0 | 0.0 | 4.352534560999664 | 0.4557891130011704 | False | 1237 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 3 | 45301 | 60526 | 190.2940629489992 | 3.3452388409999965 | 5.5269451822357274e-05 | 0.0 | 0.0 | 8.037918424999441 | 0.7635058390029319 | False | 771 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | 8614 | 7968 | 55.884571311000045 | 0.38659076200000003 | 4.8517916917670686e-05 | 0.0 | 0.0 | 1.6019484790001384 | 0.13420659099978577 | False | 1235 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 4 | 50956 | 67979 | 98.31292455499886 | 4.334810586000003 | 6.376690722134782e-05 | 0.0 | 0.0 | 7.8466704839990955 | 0.8566295010020326 | False | 774 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | 6932 | 9370 | 54.739310877000044 | 0.4337040609999995 | 4.62864526147278e-05 | 0.0 | 0.0 | 1.2212053700000816 | 0.13150085399989642 | False | 1239 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 5 | 26474 | 33456 | 215.33041656900144 | 1.623000805999998 | 4.851150185318024e-05 | 0.0 | 0.0 | 6.27311202100075 | 0.46925099600101783 | False | 771 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | 18347 | 16219 | 138.94780658700049 | 0.678278501 | 4.181999512916949e-05 | 0.0 | 0.0 | 3.067266922000589 | 0.2576548730003706 | False | 1235 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |

## Per-site query counters

| Event | Rank | Phase | Site | Direct | Upper | Classification | Magnitude | Limit | Memo direct | Memo union | Upper work | Source line | Source SHA-256 |
|---:|---:|---|:---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | 0 | quality | a | 18881 | 46019 | 44629 | 844 | 546 | 11639 | 30725 | 63992452 | 1035 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | quality | b | 8726 | 35291 | 34288 | 242 | 761 | 2968 | 12060 | 86431297 | 1036 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | quality | c | 3 | 34 | 34 | 0 | 0 | 0 | 0 | 43897 | 1037 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1038 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | quality | e | 7758 | 22555 | 22139 | 416 | 0 | 5873 | 18613 | 18761875 | 1039 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1040 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1041 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1042 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1043 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1044 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1045 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 0 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 43 | 3702 | 107628422 | 1046 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | a | 5873 | 32059 | 32059 | 0 | 0 | 3491 | 21077 | 37144687 | 1762 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | b | 2094 | 20243 | 20243 | 0 | 0 | 550 | 5833 | 38853488 | 1763 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | c | 4 | 52 | 52 | 0 | 0 | 0 | 0 | 382468 | 1764 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1765 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | e | 2530 | 16082 | 16082 | 0 | 0 | 1881 | 13938 | 5734478 | 1766 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1767 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1768 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1769 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1770 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1771 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1772 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 2 | 0 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 51 | 4546 | 130628381 | 1773 | 2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3 |
| 1 | 1 | quality | a | 23230 | 61439 | 61040 | 229 | 170 | 14812 | 40203 | 59400496 | 759 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | quality | b | 10218 | 46877 | 46621 | 55 | 201 | 2799 | 13792 | 88782636 | 760 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | quality | c | 15 | 24 | 22 | 1 | 1 | 1 | 5 | 101437 | 761 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | quality | e | 9909 | 31729 | 31649 | 80 | 0 | 8298 | 26102 | 13624039 | 763 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 1 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 201 | 3023 | 114747744 | 770 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | a | 6257 | 28998 | 28998 | 0 | 0 | 3758 | 20250 | 14392449 | 1223 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | b | 2158 | 22258 | 22258 | 0 | 0 | 593 | 6720 | 25186426 | 1224 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | c | 0 | 10 | 10 | 0 | 0 | 0 | 0 | 3162 | 1225 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | e | 2634 | 14933 | 14933 | 0 | 0 | 2238 | 13121 | 3221073 | 1227 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 2 | 1 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 51 | 3830 | 142555788 | 1234 | bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1 |
| 1 | 2 | quality | a | 18328 | 47476 | 43388 | 2321 | 1767 | 12466 | 34334 | 59195866 | 760 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | quality | b | 11807 | 34742 | 31964 | 623 | 2155 | 5115 | 13596 | 94998052 | 761 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | quality | c | 9 | 39 | 39 | 0 | 0 | 0 | 0 | 53487 | 762 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 763 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | quality | e | 6626 | 18380 | 17298 | 1082 | 0 | 5409 | 15635 | 12038363 | 764 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 770 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 2 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 1514 | 2769 | 113663614 | 771 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | a | 6272 | 41203 | 41203 | 0 | 0 | 3584 | 27303 | 63648022 | 1225 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | b | 2399 | 24908 | 24908 | 0 | 0 | 537 | 7114 | 60551851 | 1226 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | c | 2 | 28 | 28 | 0 | 0 | 0 | 0 | 71330 | 1227 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | e | 2791 | 20883 | 20883 | 0 | 0 | 2102 | 17699 | 19010806 | 1229 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1234 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1235 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 2 | 2 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 95 | 4029 | 132354538 | 1236 | d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6 |
| 1 | 3 | quality | a | 21071 | 55961 | 52453 | 1969 | 1539 | 12154 | 38377 | 47259104 | 759 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | quality | b | 9642 | 47266 | 44599 | 622 | 2045 | 2591 | 15389 | 86989048 | 760 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | quality | c | 6 | 20 | 20 | 0 | 0 | 0 | 0 | 46769 | 761 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | quality | e | 8411 | 25382 | 24415 | 967 | 0 | 5743 | 22174 | 10569752 | 763 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 3 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 225 | 2081 | 118073402 | 770 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | a | 4070 | 11737 | 11737 | 0 | 0 | 2790 | 8588 | 14471523 | 1223 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | b | 2004 | 6884 | 6884 | 0 | 0 | 629 | 2120 | 22771972 | 1224 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | c | 9 | 25 | 25 | 0 | 0 | 0 | 0 | 69715 | 1225 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | e | 1792 | 5618 | 5618 | 0 | 0 | 1386 | 4932 | 2956210 | 1227 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 2 | 3 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 2188 | 5177 | 133103907 | 1234 | 888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002 |
| 1 | 4 | quality | a | 21075 | 58564 | 58372 | 107 | 85 | 13056 | 39436 | 28807532 | 762 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | quality | b | 8281 | 42561 | 42419 | 21 | 121 | 2471 | 13270 | 36283335 | 763 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | quality | c | 4 | 20 | 20 | 0 | 0 | 0 | 0 | 26909 | 764 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | quality | e | 8578 | 30347 | 30304 | 43 | 0 | 7077 | 26557 | 4905955 | 766 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 770 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 771 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 772 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 4 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 110 | 3046 | 118407364 | 773 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | a | 2664 | 10168 | 10168 | 0 | 0 | 1521 | 6779 | 14111623 | 1227 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | b | 1474 | 7727 | 7727 | 0 | 0 | 355 | 2174 | 23292779 | 1228 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | c | 9 | 18 | 18 | 0 | 0 | 0 | 0 | 77211 | 1229 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | e | 1156 | 4840 | 4840 | 0 | 0 | 871 | 4117 | 2845606 | 1231 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1234 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1235 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1236 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1237 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 2 | 4 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 1034 | 3267 | 138880625 | 1238 | 1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687 |
| 1 | 5 | quality | a | 14243 | 36749 | 35620 | 633 | 496 | 8851 | 24991 | 51704360 | 759 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | quality | b | 7307 | 28351 | 26976 | 307 | 1068 | 1793 | 7612 | 99744352 | 760 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | quality | c | 11 | 31 | 31 | 0 | 0 | 0 | 0 | 74846 | 761 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 762 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | quality | e | 5580 | 17400 | 17062 | 338 | 0 | 4665 | 14807 | 10303121 | 763 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 764 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 765 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 766 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 767 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 768 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 769 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 1 | 5 | final_proof | g | 37393 | 31622 | 31622 | 0 | 0 | 1116 | 3466 | 104164704 | 770 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | a | 6336 | 22412 | 22412 | 0 | 0 | 3650 | 14881 | 48565672 | 1223 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | b | 3030 | 13349 | 13349 | 0 | 0 | 748 | 4052 | 64663122 | 1224 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | c | 11 | 51 | 51 | 0 | 0 | 0 | 0 | 240481 | 1225 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1226 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | e | 3013 | 11027 | 11027 | 0 | 0 | 2050 | 9343 | 10058600 | 1227 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | quality | g | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1228 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | a | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1229 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | b | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1230 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | c | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1231 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | d | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1232 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | e | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1233 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |
| 2 | 5 | final_proof | g | 29007 | 35291 | 35291 | 0 | 0 | 563 | 4953 | 122695485 | 1234 | 020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821 |

## Per-rank log provenance

- Rank 0: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.0/stdout` SHA-256 `2a81cbaeba2fffe27bcd01491db5b8c1b389345e8a23e37cf944d45567eedfc3`
- Rank 1: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.1/stdout` SHA-256 `bdaec5dfe0ef37c41b60a1b8e48e0fe061a591a2a0a943ffb3da9355af35e0f1`
- Rank 2: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.2/stdout` SHA-256 `d1dc668e59408749cd532cd64d6ad238b03be201a14feb466fd8244f7765cfc6`
- Rank 3: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.3/stdout` SHA-256 `888dcf0a014b2a29fddc742e5dc17533c7b16afd3deb4377610fcf8d7a609002`
- Rank 4: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.4/stdout` SHA-256 `1680ace1f44d447746b568db889c0ad3ef64694127b5abf53d4e57776f01c687`
- Rank 5: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/full_workflow/rank_output/1/rank.5/stdout` SHA-256 `020ee90455869ff5431fb0c8372f0a00a58308619b0f8f8bdc4cd0194ceec821`

Detailed per-site counts, source line numbers, and source hashes are in the CSV and JSON outputs.
