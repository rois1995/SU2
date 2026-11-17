# `ContextValue` lifetime fixture plan

Target helper: `Common/src/adaptation/native_engine3d/ContextValue.hpp`. The adjacent `lifetime_fixture.cpp` is a standalone, no-dependency assertion fixture. It includes the header after `<optional>`, `<type_traits>`, and `<utility>` as required by the template.

Coverage:

- Static assertions reject copy/move construction and assignment of `ContextValue`, and verify a nonmovable, nothrow-destructible payload is accepted.
- Interleaved `ContextValue` owners and ordinary guards must destruct in lexical reverse order (`4, 3, 2, 1`), and each optional payload must be disengaged exactly once.
- An ordinary early return must reset its payload before control returns.
- A throwing `optional::emplace` initializer first destroys a prior payload, records the attempted initializer, leaves the optional disengaged, and does not destroy an incompletely constructed replacement.
- The canonical payload address is the address stored in the optional. A bound `const int` remains const and refers to the same contained object.
- A budget-like payload temporarily changes external state and restores the original value on `ContextValue` scope exit.

Suggested manual command from the repository root (root controls whether/when to execute):

```sh
g++ -std=c++17 -Wall -Wextra -Werror -UNDEBUG \
  -I Common/src/adaptation/native_engine3d \
  integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/A3_Control_preparation/lifetime_fixture.cpp \
  -o /tmp/context_value_lifetime_fixture && /tmp/context_value_lifetime_fixture
```

The fixture only models lifetime mechanics; it does not instantiate engine types or claim to test MPI, resource admission, or production behavior. It has not been compiled or run in this preparation task.
