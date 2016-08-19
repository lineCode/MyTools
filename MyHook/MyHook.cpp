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
UINT	WM_MYHOOKMSG = RegisterWindowMessage(_T("MyHook's Message"));
const static TCHAR strMyTools[] = _T("MyTools");

const int MH_REMOVE = 0x0;	//for removed
const int MH_INSTALL = 0x1;	// for installed
const int SC_TOPMOST = 0x430;	// create on 2014-02-11
const int SC_DISABLE = 0x2130;	// create on 2014-02-13
#pragma data_seg()
#pragma comment(linker, "/section:SHARED,RWS")


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OutputDebugString(_T("DLL_PROCESS_ATTACH\n"));
		if (hInstance == NULL) {
			hInstance = hModule;
			InitializeCriticalSectionAndSpinCount(&csGlobal, 1000);
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

MYHOOK_API LRESULT CALLBACK CallWndProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		OutputDebugString(_T("HC_ACTION\n"));
		const LPCWPSTRUCT lpmsg = (LPCWPSTRUCT)lParam;
		if ((lpmsg->message == WM_MYHOOKMSG &&lpmsg->wParam == MH_INSTALL) || (lpmsg->message == WM_CREATE&&lpmsg->hwnd == GetAncestor(lpmsg->hwnd, GA_ROOT))) {
			OutputDebugString(_T("MH_INSTALLED\n"));
			SetProp(lpmsg->hwnd, strMyTools, (HANDLE)GetWindowLongPtr(lpmsg->hwnd, GWLP_WNDPROC));
			SetWindowLongPtr(lpmsg->hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
			HMENU hmenuSys = GetSystemMenu(lpmsg->hwnd, 0);
			if (hmenuSys != NULL) {
				LPTSTR	lpMenuTxt;
				AppendMenu(hmenuSys, MF_SEPARATOR, 0, NULL);
				LoadString(hInstance, IDS_SETTOPMOST, (LPTSTR)&lpMenuTxt, 0);
				AppendMenu(hmenuSys, MF_STRING, SC_TOPMOST, lpMenuTxt);
				LoadString(hInstance, IDS_DISABLE, (LPTSTR)&lpMenuTxt, 0);
				AppendMenu(hmenuSys, MF_STRING | MF_UNCHECKED, SC_DISABLE, lpMenuTxt);
			}
		}
		else if ((lpmsg->message == WM_MYHOOKMSG &&lpmsg->wParam == MH_REMOVE) {
			HMENU hmenuSys = GetSystemMenu(lpmsg->hwnd, 0);
			if (hmenuSys != NULL) {
				LPTSTR	lpMenuTxt;
				LoadString(hInstance, IDS_DISABLE, (LPTSTR)&lpMenuTxt, 0);
			}
	}
	return CallNextHookEx(hMyHook, nCode, wParam, lParam);
}

MYHOOK_API LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	WNDPROC proc = (WNDPROC)GetProp(hwnd, strMyTools);
	switch (uMsg) {
	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0) {
		case SC_TOPMOST:
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			break;
		case SC_DISABLE:

			//	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)proc);
			//	RemoveProp(hwnd, strMyTools);
			break;
		}
		break;
	case WM_NCDESTROY:
		RemoveProp(hwnd, strMyTools);
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);	// WM_NCDESTROY实际上是最后一个消息，这行代码只是稳妥起见
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
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + lstrlen((LPCTSTR)lpFormatBuf) + 1 /*strlen('12345678')-strlen('%s%lX%s')*/ + 1 /*for NUL*/) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		(LPTSTR)lpFormatBuf, lpszFunction, dwError, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

MYHOOK_API LRESULT CALLBACK InstallHook()
{
	OutputDebugString(_T("InstallHook\n"));
	__try {
		EnterCriticalSection(&csGlobal);
		if (!hMyHook) {
			hMyHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hInstance, 0);
			if (hMyHook != NULL)
				PostMessage(HWND_BROADCAST, WM_MYHOOKMSG, MH_INSTALL, 0);
			else
				ErrorNotify("InstallHook");
		}
	}
	__finally {
		LeaveCriticalSection(&csGlobal);
	}
	return (LONG_PTR)hMyHook;
}

MYHOOK_API BOOL CALLBACK RemoveHook()
{
	OutputDebugString(_T("RemoveHook\n"));
	BOOL bval = FALSE;
	__try {
		EnterCriticalSection(&csGlobal);

		if (hMyHook) {
			SendMessage(HWND_BROADCAST, WM_MYHOOKMSG, MH_REMOVE, 0 );
			bval = UnhookWindowsHookEx(hMyHook);
			if (bval)
				hMyHook = NULL;
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
