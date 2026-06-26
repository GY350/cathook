#include <windows.h>

#include <cstdio>

#include "minhook.hpp"

using MsgBoxFn = int WINAPI(HWND, LPCWSTR, LPCWSTR, UINT);

static int WINAPI on_MessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR caption,
                                 UINT type) {
  wprintf(L"intercepted: %s\n", text);
  return mh::original<MsgBoxFn>(hwnd, text, caption, type);
}

int main() {
  mh::Context ctx;

  auto hook = mh::hook_api(L"user32.dll", "MessageBoxW", &on_MessageBoxW);
  if (!hook.enable()) return 1;

  MessageBoxW(nullptr, L"Hello!", L"minhook.hpp", MB_OK);
}
