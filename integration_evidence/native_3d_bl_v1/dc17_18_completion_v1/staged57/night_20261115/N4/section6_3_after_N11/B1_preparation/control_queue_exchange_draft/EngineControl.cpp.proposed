// Applies the original control budget leaf; never changes a bound or owns a second state.
#include "EngineControl.hpp"
#include "EngineContext.hpp"

size_t EngineControl::SelectionBound(EngineContext& context, bool repairs) {
  auto& coarsening = *this->coarsening;
  auto& bidirectional = context.bidirectional;
  return transfer_memory::Add(sizeof(Selection::CellDebt), repairs ? 4096u : 0u,
                              (coarsening || bidirectional) ? (repairs ? 8192u : 5120u) : (repairs ? 6144u : 3072u));
}
