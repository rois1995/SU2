// Owns only empty optional data slots; Adapt scopes control active payload lifetimes.
#include "EngineSlot.hpp"

SlotState::SlotState() = default;
SlotState::~SlotState() = default;
