/* Shared rank-local election and all-rank packing for native local surface operations. */
#pragma once

#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../include/adaptation/CTransferMemory.hpp"
#include <stdexcept>

namespace SU2NativeLocal3D {
template <class Action>
void LocalStep(const World& world, Action action) {
  if (world.size == 1) {
    action();
    return;
  }
  CLocalFailure local;
  try { action(); }
  catch (const std::exception& e) { local.Set(2, 0, e.what()); }
  const auto failure = world.elect(local);
  if (failure.any) throw failure;
}

template <class Row>
std::vector<Row> SendEverywhere(World& world, const std::vector<Row>& local, MemoryBudget& budget, size_t caller) {
  using Buckets = std::vector<std::vector<Row>>;
  budget.Require(world, transfer_memory::Add(caller, transfer_memory::Mul(world.size, transfer_memory::Add(sizeof(std::vector<Row>), transfer_memory::Bytes(local)))),
                 "Native local surface send lists exceed working-storage admission.");
  Buckets outgoing;
  LocalStep(world, [&] { outgoing.assign(world.size, local); });
  std::vector<Row> received;
  LocalStep(world, [&] {
    bool admitted = false;
    received = world.exchange(outgoing, &admitted, budget.ceiling, caller);
    budget.maximum_bytes = std::max(budget.maximum_bytes, world.max_exchange_work_bytes);
    if (!admitted) throw std::length_error("Native local surface exchange exceeds working-storage admission.");
  });
  return received;
}

}  // namespace SU2NativeLocal3D
