#pragma once
#include <MinHook.h>

#include <expected>
#include <type_traits>
#include <vector>

namespace ch {

namespace detail {
template <typename Fn>
  requires std::is_function_v<Fn>
inline std::add_pointer_t<Fn> original = nullptr;
}  // namespace detail

class Context {
 public:
  Context() : status_(MH_Initialize()) {}

  ~Context() {
    if (status_ == MH_OK) MH_Uninitialize();
  }

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  [[nodiscard]] MH_STATUS status() const { return status_; }
  explicit operator bool() const { return status_ == MH_OK; }

 private:
  MH_STATUS status_ = MH_UNKNOWN;
};

template <typename Fn>
  requires std::is_function_v<Fn>
class Hook {
 public:
  using fn_ptr = std::add_pointer_t<Fn>;

  Hook(void* target, fn_ptr detour)
      : target_(target),
        status_(MH_CreateHook(target, reinterpret_cast<void*>(detour),
                              &original_)) {
    if (status_ == MH_OK)
      detail::original<Fn> = reinterpret_cast<fn_ptr>(original_);
  }

  Hook(fn_ptr target, fn_ptr detour)
      : Hook(reinterpret_cast<void*>(target), detour) {}

  Hook(Hook&& other) noexcept
      : target_(other.target_),
        original_(other.original_),
        status_(other.status_) {
    other.target_ = nullptr;
    other.status_ = MH_UNKNOWN;
  }

  Hook& operator=(Hook&& other) noexcept {
    if (this != &other) {
      if (status_ == MH_OK) MH_RemoveHook(target_);
      target_ = other.target_;
      original_ = other.original_;
      status_ = other.status_;
      other.target_ = nullptr;
      other.status_ = MH_UNKNOWN;
    }
    return *this;
  }

  ~Hook() {
    if (status_ == MH_OK) MH_RemoveHook(target_);
  }

  Hook(const Hook&) = delete;
  Hook& operator=(const Hook&) = delete;

  [[nodiscard]] std::expected<void, MH_STATUS> enable() {
    if (status_ != MH_OK) return std::unexpected(status_);
    if (auto s = MH_EnableHook(target_); s != MH_OK) return std::unexpected(s);
    return {};
  }

  [[nodiscard]] std::expected<void, MH_STATUS> disable() {
    if (status_ != MH_OK) return std::unexpected(status_);
    if (auto s = MH_DisableHook(target_); s != MH_OK) return std::unexpected(s);
    return {};
  }

  [[nodiscard]] MH_STATUS status() const { return status_; }
  explicit operator bool() const { return status_ == MH_OK; }

  fn_ptr original() const { return reinterpret_cast<fn_ptr>(original_); }

 private:
  Hook(MH_STATUS status) : target_(nullptr), status_(status) {}

  friend class HookSet;

  template <typename FnPtr>
    requires std::is_function_v<std::remove_pointer_t<FnPtr>>
  friend Hook<std::remove_pointer_t<FnPtr>> hook_api(const wchar_t*,
                                                     const char*, FnPtr);

  void* target_ = nullptr;
  void* original_ = nullptr;
  MH_STATUS status_ = MH_UNKNOWN;
};

class HookSet {
 public:
  template <typename Fn>
    requires std::is_function_v<Fn>
  void add(const Hook<Fn>& hook) {
    targets_.push_back(hook.target_);
  }

  [[nodiscard]] std::expected<void, MH_STATUS> enable() {
    for (auto* t : targets_)
      if (auto s = MH_QueueEnableHook(t); s != MH_OK) return std::unexpected(s);
    if (auto s = MH_ApplyQueued(); s != MH_OK) return std::unexpected(s);
    return {};
  }

  [[nodiscard]] std::expected<void, MH_STATUS> disable() {
    for (auto* t : targets_)
      if (auto s = MH_QueueDisableHook(t); s != MH_OK)
        return std::unexpected(s);
    if (auto s = MH_ApplyQueued(); s != MH_OK) return std::unexpected(s);
    return {};
  }

 private:
  std::vector<void*> targets_;
};

template <typename Fn>
  requires std::is_function_v<Fn>
inline std::add_pointer_t<Fn>& original = detail::original<Fn>;

template <typename FnPtr>
  requires std::is_function_v<std::remove_pointer_t<FnPtr>>
Hook<std::remove_pointer_t<FnPtr>> hook_api(const wchar_t* module,
                                            const char* proc, FnPtr detour) {
  if (auto hmod = GetModuleHandleW(module); !hmod)
    return {MH_ERROR_MODULE_NOT_FOUND};
  else if (auto addr = GetProcAddress(hmod, proc); !addr)
    return {MH_ERROR_FUNCTION_NOT_FOUND};
  else
    return {reinterpret_cast<void*>(addr), detour};
}

inline const char* status_string(MH_STATUS s) {
  return MH_StatusToString(s);
}

inline const char* status_string(const std::expected<void, MH_STATUS>& r) {
  return MH_StatusToString(r ? MH_OK : r.error());
}

}  // namespace ch
