// Storage belongs to the context; the old local scope still controls the value lifetime.
#pragma once

template <class T>
class ContextValue {
  std::optional<T>& storage;

 public:
  template <class... Args>
  explicit ContextValue(std::optional<T>& slot, Args&&... args) : storage(slot) {
    static_assert(std::is_nothrow_destructible_v<T>);
    storage.emplace(std::forward<Args>(args)...);
  }
  ~ContextValue() { storage.reset(); }
  T& Get() const { return *storage; }
  ContextValue(const ContextValue&) = delete;
  ContextValue& operator=(const ContextValue&) = delete;
  ContextValue(ContextValue&&) = delete;
  ContextValue& operator=(ContextValue&&) = delete;
};

template <class T, class... Args>
ContextValue(std::optional<T>&, Args&&...) -> ContextValue<T>;
