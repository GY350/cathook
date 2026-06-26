#include <windows.h>

#include <cstdio>
#include <print>

#include "cathook.hpp"

using MsgBoxFn = int WINAPI(HWND, LPCWSTR, LPCWSTR, UINT);

static int WINAPI on_MessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR caption,
                                 UINT type) {
  wprintf(L"intercepted: %s\n", text);
  return ch::original<MsgBoxFn>(hwnd, text, caption, type);
}

int main() {
  ch::Context ctx;
  std::println("MH_Initialize: {}", ch::status_string(ctx.status()));
  if (!ctx) return 1;

  auto hook = ch::hook_api(L"user32.dll", "MessageBoxW", &on_MessageBoxW);
  auto hook_enable = hook.enable();
  std::println("enable: {}", ch::status_string(hook_enable));
  if (!hook_enable) return 1;

  MessageBoxW(nullptr, L"Hello!", L"cathook.hpp", MB_OK);
}
