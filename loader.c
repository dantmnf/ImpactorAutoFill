#include <windows.h>
#include <detours.h>

const wchar_t* w32strerror(DWORD err) {
    __declspec(thread) static wchar_t buf[4096];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, sizeof(buf), NULL);
    return buf;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE _, LPWSTR lpCmdLine, int nCmdShow) {
    STARTUPINFOW si = {.cb = sizeof(si), 0};
    PROCESS_INFORMATION pi;
    const char* dllname = "autofill.dll";
	wchar_t *path = malloc(32768 * sizeof(wchar_t));
	GetModuleFileNameW(hInstance, path, 32768);
	wchar_t *lastslash = wcsrchr(path, L'\\');
	wcsncpy(lastslash + 1, L"Impactor.exe", 32767 - (lastslash - path));
    if (!DetourCreateProcessWithDllsW(path, lpCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi, 1, &dllname, NULL)) {
        DWORD err = GetLastError();
        MessageBoxW(NULL, w32strerror(err), L"error", 0);
    }
	free(path);
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD ex = 255;
    GetExitCodeProcess(pi.hProcess, &ex);
    return ex;
}
