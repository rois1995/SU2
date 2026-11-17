# M25 saved492 split measurements

Rows: 2408; attempted: 2408; complete: 2354; interrupted at time limit: 0; not attempted: 0.

Improvement means complete and `after < before * (1 - 1e-12)`; the share denominator is complete rows. Bound ratios/child shares use every measured qualifying parent face, including supplementary bounds when the prediction returned before scoring. The 2x2 table treats `n11_first_verdict=accepted` as positive and `guidance` as negative; `none` rows have no N11 trial and are excluded from the 2x2 table.

| limiting_failing_faces | n11_first_verdict | rows | complete | improving / complete | median smaller-child / parent | median larger-child / parent | children <= tau | children > parent + tau | qualifying parent faces |
|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 2 | accepted | 48 | 44 | 44/44 | 0.303562 | 0.955667 | 73/192 | 0/192 | 96 |
| 2 | guidance | 204 | 201 | 26/201 | 0.339136 | 1.10774 | 377/816 | 0/816 | 408 |
| 2 | none | 814 | 807 | 533/807 | 0.327785 | 0.917651 | 1770/3256 | 0/3256 | 1628 |
| 1 | accepted | 44 | 44 | 44/44 | 0.237944 | 0.353239 | 81/106 | 0/106 | 53 |
| 1 | guidance | 43 | 35 | 5/35 | 0.420248 | 1.00000 | 55/88 | 0/88 | 44 |
| 1 | none | 769 | 757 | 461/757 | 0.336113 | 0.604086 | 1116/1590 | 0/1590 | 795 |
| 0 | accepted | 70 | 60 | 13/60 | 0.692309 | 1.13260 | 208/208 | 0/208 | 104 |
| 0 | guidance | 416 | 406 | 0/406 | 0.431332 | 0.936852 | 70/70 | 0/70 | 35 |
| 0 | none | 0 | 0 | n/a | n/a | n/a | n/a | n/a | 0 |

## Prediction versus N11 first verdict

| prediction improves | accepted | not accepted |
|---|---:|---:|
| yes | 101 | 31 |
| no/incomplete | 61 | 632 |
