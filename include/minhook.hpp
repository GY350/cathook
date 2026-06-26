#pragma once
#include <MinHook.h>

#include <type_traits>

namespace mh {

template <typename Fn>
  requires std::is_function_v<Fn>
class Hook {
 public:
  using fn_ptr = std::add_pointer_t<Fn>;

  Hook(void* target, fn_ptr detour)
      : target_(target),
        status_(MH_CreateHook(target, reinterpret_cast<void*>(detour),
                              reinterpret_cast<void**>(&original_))) {}

  ~Hook() {
    if (status_ == MH_OK) MH_RemoveHook(target_);
  }

  Hook(const Hook&) = delete;
  Hook& operator=(const Hook&) = delete;

  [[nodiscard]] MH_STATUS enable() { return MH_EnableHook(target_); }
  [[nodiscard]] MH_STATUS disable() { return MH_DisableHook(target_); }
  [[nodiscard]] MH_STATUS status() const { return status_; }

  fn_ptr original() const { return original_; }

 private:
  void* target_ = nullptr;
  fn_ptr original_ = nullptr;
  MH_STATUS status_ = MH_UNKNOWN;
};

}  // namespace mh
