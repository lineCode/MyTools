#include <Windows.h>

#ifdef MYHOOK_EXPORTS
#define MYHOOK_API __declspec(dllexport)
#else
#define MYHOOK_API __declspec(dllimport)
#endif

#ifdef _cplusplus
extern "C" {
#endif

MYHOOK_API LRESULT CALLBACK CallWndProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
MYHOOK_API LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
MYHOOK_API LRESULT CALLBACK InstallHook();
MYHOOK_API BOOL CALLBACK RemoveHook();

#ifdef _cplusplus
}
#endif