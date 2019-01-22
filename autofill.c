#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <wincred.h>
// #include <shlwapi.h>
#include <detours.h>

#define USERNAME_TIMER_ID 114514
#define PASSWORD_TIMER_ID 810893

HHOOK hHook = NULL;
LRESULT CALLBACK HookProc(int, WPARAM, LPARAM);
HMODULE hModule;

BOOL WINAPI MyShowWindow(HWND hwnd, int nCmdShow);
static BOOL(WINAPI *realShowWindow)(HWND hwnd, int nCmdShow) = ShowWindow;

UINT_PTR username_timer = 0, password_timer = 0;
HWND last_filled = NULL;

struct {
    wchar_t *username;
    wchar_t *password;
    wchar_t *target_name;
    int delay;
	BOOL commit;
} config = {0};

BOOL APIENTRY DllMain(HMODULE hMod, DWORD fdwReason, LPVOID lpReserved) {
    if (DetourIsHelperProcess()) return TRUE;
    if (!GetModuleHandleW(L"Impactor.dll")) return TRUE;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            hModule = hMod;
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach((void **)&realShowWindow, MyShowWindow);
            DetourTransactionCommit();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            if (config.username) free(config.username);
            if (config.password) free(config.password);
            if (hHook) UnhookWindowsHookEx(hHook);
            break;
    }
    return TRUE;
}

BOOL check_bool_str(wchar_t *s) {
    return lstrcmpiW(s, L"true") == 0 || lstrcmpiW(s, L"1") == 0 ? TRUE : FALSE;
}

void read_configuration(HWND hwnd) {
    wchar_t *path = (wchar_t*)malloc(32768 * sizeof(wchar_t));
    GetModuleFileNameW(hModule, path, 32768);
    wchar_t *lastslash = wcsrchr(path, L'\\');
    wcsncpy(lastslash+1, L"autofill.ini", 32767 - (lastslash - path));
    
    // PathRemoveFileSpecW(path);
    // PathAppendW(path, L"autofill.ini");
    fwprintf(stderr, L"configuration file: %ls\n", path);

	config.delay = GetPrivateProfileIntW(L"AutoFill", L"Delay", 100, path);
    config.target_name = (wchar_t*)malloc(2048);
    GetPrivateProfileStringW(L"AutoFill", L"CredTargetName", L"ImpactorAutoFill_AppleID", config.target_name, 1024, path);
    wchar_t bool_str[16];
	GetPrivateProfileStringW(L"AutoFill", L"Commit", L"false", bool_str, 16, path);
	config.commit = check_bool_str(bool_str);
    
    GetPrivateProfileStringW(L"AutoFill", L"ClearCred", L"false", bool_str, 16, path);
    if(check_bool_str(bool_str)) {
        fwprintf(stderr, L"removing saved credential\n");
        CredDeleteW(config.target_name, CRED_TYPE_GENERIC, 0);
        WritePrivateProfileStringW(L"AutoFill", L"ClearCred", L"false", path);
    }
    
    //GetPrivateProfileStringW(L"AutoFill", L"Username", L"username", config.username, 1024, path);
    //GetPrivateProfileStringW(L"AutoFill", L"Password", L"password", config.password, 1024, path);



    fwprintf(stderr, L"delay=%d, commit=%ls\n", config.delay, config.commit ? L"true" : L"false");
    free(path);
}

void load_credential(HWND hwnd) {
    config.username = (wchar_t*)malloc(2048);
    config.username[0] = 0;
    config.password = (wchar_t*)malloc(2048);
    config.password[0] = 0;
    CREDUI_INFOW uiinfo;
    uiinfo.cbSize = sizeof(uiinfo);
    uiinfo.hwndParent = hwnd;
    uiinfo.pszMessageText = L"Enter Apple ID and password.";
    uiinfo.pszCaptionText = L"ImpactorAutoFill";

    BOOL save = TRUE;
    
    CREDENTIALW *pcred;
    if(CredReadW(config.target_name, CRED_TYPE_GENERIC, 0, &pcred)) {
        fwprintf(stderr, L"found credential\n");
        wcsncpy(config.username, pcred->UserName, 1024);
        wcsncpy(config.password, (const wchar_t*)pcred->CredentialBlob, 1024);
        CredFree(pcred);
    } else {
        fwprintf(stderr, L"CredReadW = false, showing prompt\n");
        
        ULONG pkg;
        void* authbuf;
        ULONG authbuflen;

        DWORD result = CredUIPromptForWindowsCredentialsW(&uiinfo, 0, &pkg, NULL, 0, &authbuf, &authbuflen, &save, CREDUIWIN_GENERIC | CREDUIWIN_CHECKBOX);
        if(result != ERROR_SUCCESS) {
            fwprintf(stderr, L"CredUIPromptForWindowsCredentialsW failed\n");
            config.commit = FALSE;
        } else {
            DWORD userlen = 1024, domlen = 0, passlen = 1024;
            if(CredUnPackAuthenticationBufferW(0, authbuf, authbuflen, config.username, &userlen, NULL, &domlen, config.password, &passlen)) {
                CREDENTIALW cred = {0};
                cred.Type = CRED_TYPE_GENERIC;
                cred.TargetName = config.target_name;
                cred.CredentialBlobSize = passlen*sizeof(wchar_t);
                cred.CredentialBlob = (LPBYTE) config.password;
                cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
                cred.UserName = config.username;
                if(save && wcslen(config.username) > 0) {
                    BOOL br = CredWriteW(&cred, 0);
                    fwprintf(stderr, L"CredWrite = %d\n", br);
                }
            } else {
                fwprintf(stderr, L"CredUnPackAuthenticationBufferW = false\n");
                config.commit = FALSE;
            }
            SecureZeroMemory(authbuf, authbuflen);
            CoTaskMemFree(authbuf);
        }
    }
    fwprintf(stderr, L"#username=%d, #password=%d\n", wcslen(config.username), wcslen(config.password));

}

BOOL setup_hook(HWND hwnd) {
    read_configuration(hwnd);
    fprintf(stderr, "setting up hook\n");
    hHook = SetWindowsHookExW(WH_CBT, HookProc, NULL, GetCurrentThreadId());
    if (!hHook) fprintf(stderr, "SetWindowsHookExW failed\n");
    return !!hHook;
}

BOOL WINAPI MyShowWindow(HWND hwnd, int nCmdShow) {
    BOOL result = realShowWindow(hwnd, nCmdShow);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((void **)&realShowWindow, MyShowWindow);
    DetourTransactionCommit();
    setup_hook(hwnd);
    return result;
}

void commit_input(HWND hwnd) {
	if (!config.commit) return;
	fprintf(stderr, "committing input\n");
	PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(5100, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, 5100));
}

void CALLBACK fill_username(HWND hwnd, UINT msg, UINT_PTR wp, DWORD lp) {
    fprintf(stderr, "filling username\n");
    HWND editwnd = FindWindowExW(hwnd, NULL, L"Edit", NULL);
    if (editwnd) {
        KillTimer(hwnd, USERNAME_TIMER_ID);
        if(!config.username) load_credential(hwnd);
        if(!wcslen(config.username)) return;
        SetWindowTextW(editwnd, config.username);
        fprintf(stderr, "filled username\n");
		commit_input(hwnd);
    } else {
        fprintf(stderr, "unable to find edit control\n");
    }
}

void CALLBACK fill_password(HWND hwnd, UINT msg, UINT_PTR wp, DWORD lp) {
    fprintf(stderr, "filling password\n");
    // MessageBoxW(hwnd, L"filling password", L"fuck", 0);
    HWND editwnd = FindWindowExW(hwnd, NULL, L"Edit", NULL);
    if (editwnd) {
        KillTimer(hwnd, PASSWORD_TIMER_ID);
        if(!config.password) load_credential(hwnd);
        if(!wcslen(config.password)) return;
        SetWindowTextW(editwnd, config.password);
        fprintf(stderr, "filled password\n");
		commit_input(hwnd);
    } else {
        fprintf(stderr, "unable to find edit control\n");
    }
}

void fill_window(HWND hwnd) {
    wchar_t buf[256];
    if (last_filled == hwnd) return;
    GetWindowTextW(hwnd, buf, 256);
    if (wcscmp(buf, L"Apple ID Username") == 0) {
        username_timer = SetTimer(hwnd, USERNAME_TIMER_ID, config.delay, fill_username);
        last_filled = hwnd;
    } else if (wcscmp(buf, L"Apple ID Password") == 0) {
        password_timer = SetTimer(hwnd, PASSWORD_TIMER_ID, config.delay, fill_password);
        last_filled = hwnd;
    }
}

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) return CallNextHookEx(hHook, nCode, wParam, lParam);
    switch (nCode) {
        case HCBT_ACTIVATE:
            fill_window((HWND)wParam);
            break;
        default:
            break;
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

