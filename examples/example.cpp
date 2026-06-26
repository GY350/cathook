#include <windows.h>

#include <cstdio>

#include "cathook.hpp"

using MsgBoxFn = int WINAPI(HWND, LPCWSTR, LPCWSTR, UINT);

static int WINAPI on_MessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR caption,
                                 UINT type) {
  wprintf(L"intercepted: %s\n", text);
  return ch::original<MsgBoxFn>(hwnd, text, caption, type);
}

int main() {
  ch::Context ctx;

  auto hook = ch::hook_api(L"user32.dll", "MessageBoxW", &on_MessageBoxW);
  if (!hook.enable()) return 1;

  MessageBoxW(nullptr, L"Hello!", L"cathook.hpp", MB_OK);
}
