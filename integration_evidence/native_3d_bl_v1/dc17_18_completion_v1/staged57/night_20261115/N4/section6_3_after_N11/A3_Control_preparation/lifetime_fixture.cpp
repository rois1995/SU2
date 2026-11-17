#include <array>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "ContextValue.hpp"

struct Events {
  std::array<int, 16> values{};
  std::size_t size = 0;
  void Add(int value) noexcept {
    assert(size < values.size());
    values[size++] = value;
  }
  bool Is(const std::array<int, 8>& expected, std::size_t count) const noexcept {
    if (size != count) return false;
    for (std::size_t i = 0; i < count; ++i) {
      if (values[i] != expected[i]) return false;
    }
    return true;
  }
};

struct Tracked {
  Events* events;
  int id;
  Tracked(Events& log, int value) noexcept : events(&log), id(value) {}
  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;
  Tracked(Tracked&&) = delete;
  Tracked& operator=(Tracked&&) = delete;
  ~Tracked() noexcept { events->Add(id); }
};

struct Guard {
  Events* events;
  int id;
  ~Guard() noexcept { events->Add(id); }
};

struct MaybeThrows {
  Events* events;
  int id;
  MaybeThrows(Events& log, int value, bool fail) : events(&log), id(value) {
    events->Add(100 + id);
    if (fail) throw std::runtime_error("initializer fixture");
  }
  ~MaybeThrows() noexcept { events->Add(id); }
};

struct BudgetLease {
  int* budget;
  const int* canonical;
  int saved;
  Events* events;
  BudgetLease(int& value, const int& authority, Events& log) noexcept
      : budget(&value), canonical(&authority), saved(value), events(&log) {
    value += 5;
  }
  BudgetLease(const BudgetLease&) = delete;
  BudgetLease& operator=(const BudgetLease&) = delete;
  BudgetLease(BudgetLease&&) = delete;
  BudgetLease& operator=(BudgetLease&&) = delete;
  ~BudgetLease() noexcept {
    *budget = saved;
    events->Add(9);
  }
};

static_assert(!std::is_copy_constructible_v<ContextValue<Tracked>>);
static_assert(!std::is_copy_assignable_v<ContextValue<Tracked>>);
static_assert(!std::is_move_constructible_v<ContextValue<Tracked>>);
static_assert(!std::is_move_assignable_v<ContextValue<Tracked>>);
static_assert(!std::is_copy_constructible_v<Tracked>);
static_assert(!std::is_move_constructible_v<Tracked>);
static_assert(std::is_nothrow_destructible_v<Tracked>);
static_assert(std::is_same_v<decltype(std::declval<const ContextValue<const int>&>().Get()),
                             const int&>);

int ReturnEarly(std::optional<Tracked>& slot, Events& log) {
  ContextValue value(slot, log, 5);
  assert(&value.Get() == &slot.value());
  return 17;
}

int main() {
  Events log;
  std::optional<Tracked> first, second;
  {
    ContextValue a(first, log, 1);
    Guard between{&log, 2};
    ContextValue b(second, log, 3);
    Guard inner{&log, 4};
    (void)between;
    (void)inner;
    assert(&a.Get() == &first.value());
    assert(&b.Get() == &second.value());
  }
  assert(log.Is({4, 3, 2, 1}, 4));
  assert(!first && !second);
  first.reset();
  assert(log.size == 4);

  assert(ReturnEarly(first, log) == 17);
  assert(!first && log.Is({4, 3, 2, 1, 5}, 5));

  Events throws;
  std::optional<MaybeThrows> throwing_slot;
  throwing_slot.emplace(throws, 7, false);
  bool caught = false;
  try {
    ContextValue failed(throwing_slot, throws, 8, true);
  } catch (const std::runtime_error&) {
    caught = true;
  }
  assert(caught && !throwing_slot);
  assert(throws.Is({107, 7, 108}, 3));
  throwing_slot.reset();
  assert(throws.size == 3);

  const int canonical = 42;
  int budget = 11;
  Events budget_events;
  std::optional<BudgetLease> lease_slot;
  {
    ContextValue lease(lease_slot, budget, canonical, budget_events);
    assert(&lease.Get() == &lease_slot.value());
    assert(lease.Get().budget == &budget);
    assert(lease.Get().canonical == &canonical);
    assert(budget == 16);
  }
  assert(budget == 11 && !lease_slot && budget_events.Is({9}, 1));

  std::optional<const int> const_slot;
  {
    ContextValue<const int> bound(const_slot, canonical);
    const int& reference = bound.Get();
    assert(&reference == &const_slot.value());
    assert(reference == 42);
  }
  assert(!const_slot);
  return 0;
}
