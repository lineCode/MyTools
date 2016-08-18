// MyHook.cpp : Defines the exported functions for the DLL application.
//

#include <stdio.h>
#include <tchar.h>
//#include <map>
#include <Strsafe.h>
#include "MyHook.h"
#include "resource.h"
using namespace std;

#pragma data_seg("SHARED")
static HHOOK hMyHook = NULL;
static HMODULE	hInstance = NULL;
CRITICAL_SECTION csGlobal;
const static TCHAR strMyTools[] = _T("MyTools");
UINT	WM_MYHOOKMSG;
#pragma data_seg()
#pragma comment(linker, "/section:SHARED,RWS")

const int MH_REMOVED = 0x0;	//for removed
const int MH_INSTALLED = 0x1;	// for installed
const int SC_TOPMOST = 0x430;	// create on 2014-02-11
const int SC_REMOVEHOOK = 0x2130;	// create on 2014-02-13

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (hInstance == NULL) {
			hInstance = hModule;
			WM_MYHOOKMSG = RegisterWindowMessage(_T("MyHook's Message"));
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

BOOL CALLBACK AttachMenu(_In_ HWND hwnd) {
	HMENU hmenuSys = GetSystemMenu(hwnd, 0);
	if (hmenuSys == NULL)
		return FALSE;
	AppendMenu(hmenuSys, MF_SEPARATOR, 0, NULL);
	AppendMenu(hmenuSys, MF_STRING, SC_TOPMOST, TEXT("Set TopMost"));
	return TRUE;
}

//LRESULT CALLBACK RegisterWnd(_In_ HWND hwnd)
//{
//	SetProp(hwnd, strMyTools, (HANDLE)GetWindowLongPtr(hwnd, GWLP_WNDPROC));
//	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
//	AttachMenu(hwnd);
//	return 0;
//}

MYHOOK_API LRESULT CALLBACK CallWndProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		LPCWPSTRUCT lpmsg = (LPCWPSTRUCT)lParam;
		if (lpmsg->message == WM_SHOWWINDOW && lpmsg->lParam == 0) {
			if (GetParent(lpmsg->hwnd) == NULL && GetWindow(lpmsg->hwnd, GW_OWNER) == NULL && lpmsg->wParam != 0
				&& ((GetWindowLong(lpmsg->hwnd, GWL_EXSTYLE)&(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW)) == 0)) {
#ifdef _DEBUG
				TCHAR	tMessage[256];
				TCHAR	tClassName[128];
				GetClassName(lpmsg->hwnd, tClassName, 128);
				swprintf_s(tMessage, 256, _T("Register: %s.\r\n"), tClassName);
				OutputDebugString(tMessage);
#endif
				//				RegisterWnd(lpmsg->hwnd);
			}
		}
	}
	return CallNextHookEx(hMyHook, nCode, wParam, lParam);
}

MYHOOK_API LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	WNDPROC proc = (WNDPROC)GetProp(hwnd, strMyTools);
	switch (uMsg) {
	case WM_CREATE:
		break;
	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0) {
		case SC_TOPMOST:
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			break;
		case SC_REMOVEHOOK:
#ifdef _DEBUG
			TCHAR	tMessage[256];
			swprintf_s(tMessage, 256, _T("Remove subclass proc address: %d.\r\n"), proc);
			OutputDebugString(tMessage);
#endif
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)proc);
			RemoveProp(hwnd, strMyTools);
			break;
		}
		break;
	case WM_NCDESTROY:
		RemoveProp(hwnd, strMyTools);
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)proc);	// WM_NCDESTROY实际上是最后一个消息，这行代码只是稳妥起见
		break;
	}

	return CallWindowProc(proc, hwnd, uMsg, wParam, lParam);
}

// display information ref: https://msdn.microsoft.com/en-us/library/ms680582(v=vs.85).aspx
void ErrorNotify(LPCSTR lpszFunction) {
	LPVOID	lpMsgBuf, lpDisplayBuf, lpFormatBuf;
	DWORD dwError = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	LoadString(hInstance, IDS_FAILED_WITH_ERROR, (LPTSTR)&lpFormatBuf, 0);
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + lstrlen((LPCTSTR)lpFormatBuf) + (10 - 6) /*strlen('0x12345678')-strlen('%d%s%s')*/ + 1 /*for NUL*/) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		(LPTSTR)lpFormatBuf, lpszFunction, dwError, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

// 参数用来指定调用者进程的窗口，用来接受通知信息，存储被钩窗口的信息
MYHOOK_API LRESULT CALLBACK InstallHook()
{
	__try {
		EnterCriticalSection(&csGlobal);
		if (!hMyHook) {
			hMyHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hInstance, 0);
			if (hMyHook == NULL) {
				ErrorNotify("InstallHook");
			}
		}
	}
	__finally {
		LeaveCriticalSection(&csGlobal);
	}
	return (LONG)hMyHook;
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
	if (GetProp(hwnd, strMyTools) != NULL) {
		SendMessage(hwnd, WM_SYSCOMMAND, SC_REMOVEHOOK, 0);
	}
	return TRUE;
}

MYHOOK_API BOOL CALLBACK RemoveHook()
{
	BOOL bval = FALSE;
	__try {
		EnterCriticalSection(&csGlobal);

		if (hMyHook) {
			PostMessage(HWND_BROADCAST, WM_REMOVEHOOK, 0, 0L);
			EnumWindows(EnumWindowsProc, 0);
			bval = UnhookWindowsHookEx(hMyHook);
			if (bval)
				hMyHook = FALSE;
			else {
				ErrorNotify("RemoveHook");
			}
		}
	}
	__finally {
		LeaveCriticalSection(&csGlobal);
	}
	return bval;
}
