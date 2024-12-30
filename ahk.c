#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
HHOOK g_hHook = NULL;
DWORD g_dwProcessId = 0;

DWORD GetProcessIdByName(const char *processName) {
  DWORD processId = 0;
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
      do {
	// printf("== %s\n", pe32.szExeFile);
        if (strcmp(pe32.szExeFile, processName) == 0) {
          processId = pe32.th32ProcessID;
          break;
        }
      } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
  }
  return processId;
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    PKBDLLHOOKSTRUCT pKeyBoard = (PKBDLLHOOKSTRUCT)lParam;

    // 判断是否按下 Ctrl + P
    if ((pKeyBoard->vkCode == 'P' || pKeyBoard->vkCode == 0x50) &&
        wParam == WM_KEYDOWN &&               // 'P' 键的虚拟键码
        (GetKeyState(VK_CONTROL) & 0x8000)) { // 检查 Ctrl 键是否按下

      // 获取当前激活窗口的进程 ID
      HWND foregroundWindow = GetForegroundWindow();
      DWORD currentProcessId = 0;
      GetWindowThreadProcessId(foregroundWindow, &currentProcessId);
      // 判断是否是 windowsterminal.exe 进程
      char title[255] = {0};
      GetWindowTextA(foregroundWindow, title, sizeof(title));
      printf("title: %s\n", title);

      if ((currentProcessId == g_dwProcessId) && (strstr(title, "Windows PowerShell") != NULL)) {
	printf("<C-p> mapping to <UP>\n");

        // 释放 Ctrl 键，避免干扰
        INPUT ctrlUp = {0};
        ctrlUp.type = INPUT_KEYBOARD;
        ctrlUp.ki.wVk = VK_CONTROL;
        ctrlUp.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &ctrlUp, sizeof(INPUT));

        // 模拟按下 Up 键
        INPUT input[2];
        ZeroMemory(input, sizeof(input));

        // Key down event for Up arrow
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = VK_UP;
        input[0].ki.dwFlags = 0;
        SendInput(1, &input[0], sizeof(INPUT));

        // Key up event for Up arrow
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = VK_UP;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input[1], sizeof(INPUT));
        // 重新按下 Ctrl 键，保持原状态
        INPUT ctrlDown = {0};
        ctrlDown.type = INPUT_KEYBOARD;
        ctrlDown.ki.wVk = VK_CONTROL;
        SendInput(1, &ctrlDown, sizeof(INPUT));

        return 1;
      }
    }
  }
  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

int main() {
  // 获取 windowsterminal.exe 的进程 ID
  g_dwProcessId = GetProcessIdByName("WindowsTerminal.exe");
  // g_dwProcessId = GetProcessIdByName("CalculatorApp.exe");
  printf("pid: %d\n", g_dwProcessId);
  if (g_dwProcessId == 0) {
    MessageBoxW(NULL, L"找不到WindowsTerminal.exe进程！", L"错误",
                MB_ICONERROR);
    return 1;
  }

  g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                             GetModuleHandle(NULL), 0);
  if (g_hHook == NULL) {
    MessageBoxW(NULL, L"无法安装键盘钩子！", L"错误", MB_ICONERROR);
    return 1;
  }

  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnhookWindowsHookEx(g_hHook);

  return 0;
}
