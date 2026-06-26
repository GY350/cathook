#pragma once
#include <MinHook.h>

#include <expected>
#include <type_traits>

namespace mh {

class Context {
 public:
  Context() : status_(MH_Initialize()) {}

  ~Context() {
    if (status_ == MH_OK) MH_Uninitialize();
  }

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  [[nodiscard]] MH_STATUS status() const { return status_; }

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
                              &original_)) {}

  Hook(fn_ptr target, fn_ptr detour)
      : Hook(reinterpret_cast<void*>(target), detour) {}

  ~Hook() {
    if (status_ == MH_OK) MH_RemoveHook(target_);
  }

  Hook(const Hook&) = delete;
  Hook& operator=(const Hook&) = delete;

  [[nodiscard]] std::expected<void, MH_STATUS> enable() {
    if (auto s = MH_EnableHook(target_); s != MH_OK) return std::unexpected(s);
    return {};
  }

  [[nodiscard]] std::expected<void, MH_STATUS> disable() {
    if (auto s = MH_DisableHook(target_); s != MH_OK) return std::unexpected(s);
    return {};
  }

  [[nodiscard]] MH_STATUS status() const { return status_; }

  fn_ptr original() const { return reinterpret_cast<fn_ptr>(original_); }

 private:
  Hook(MH_STATUS status) : target_(nullptr), status_(status) {}

  template <typename F>
  friend Hook<F> hook_api(const wchar_t*, const char*,
                          typename Hook<F>::fn_ptr);

  void* target_ = nullptr;
  void* original_ = nullptr;
  MH_STATUS status_ = MH_UNKNOWN;
};

template <typename Fn>
  requires std::is_function_v<Fn>
Hook<Fn> hook_api(const wchar_t* module, const char* proc,
                  typename Hook<Fn>::fn_ptr detour) {
  if (auto hmod = GetModuleHandleW(module); !hmod)
    return {MH_ERROR_MODULE_NOT_FOUND};
  else if (auto addr = GetProcAddress(hmod, proc); !addr)
    return {MH_ERROR_FUNCTION_NOT_FOUND};
  else
    return {reinterpret_cast<void*>(addr), detour};
}

}  // namespace mh
