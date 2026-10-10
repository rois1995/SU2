#include "Common/include/adaptation/CDistributedSearch.hpp"
#include <cassert>
#include <iostream>
int main() {
  std::vector<size_t> counts;
  const std::vector<uint64_t> empty;
  assert(CPassiveComm::Allgatherv(empty, &counts).empty());
  assert(counts == std::vector<size_t>{0});
  const std::vector<uint64_t> input{0, UINT64_MAX};
  assert(CPassiveComm::Allgatherv(input, &counts, SU2_MPI::GetComm()) == input);
  assert(counts == std::vector<size_t>{2});
  for (unsigned short dim : {2,3}) {
    CRankBoxTree tree;
    tree.Build(dim, {});
    assert(tree.GetnBox() == 0);
    std::vector<double> box(dim,0.); box.resize(2*dim,1.);
    tree.Build(dim, box, SU2_MPI::GetComm());
    assert(tree.GetnBox() == 1);
    const double point[3]{.5,.5,.5}; std::vector<int> ranks;
    tree.RanksContaining(point,ranks);
    assert(ranks == std::vector<int>{0});
  }
  std::cout << "PASS serial passive empty/default/explicit gather and 2D/3D rank-box controls\n";
}
