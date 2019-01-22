
/* fool the compiler optimizer to make a jmp */
typedef void (*FUNCSHIM)();
void shim_prepare();

#ifdef _WIN64
#define MANGLE(x) x
#else
#define MANGLE(x) "_" x
#define BEGIN_NAKED_FUNCTION(name) __asm__(MANGLE(#name) ":");
#define END_NAKED_FUNCTION()
#endif

#if defined(_MSC_VER) && !defined(_WIN64)
    #define DECLARE_SHIM(name) \
    FUNCSHIM p##name; \
    __declspec(naked) void Shim##name() { \
    __asm call shim_prepare \
    __asm jmp [p##name] \
    }

#elif defined(__GNUC__)
    #ifdef _WIN64

        #define BEGIN_NAKED_FUNCTION(name) __declspec(naked) void name() {
        #define END_NAKED_FUNCTION() }

        BEGIN_NAKED_FUNCTION(shim_prepare2)
            __asm__ ("cmp $0, " MANGLE("shim_prepared") "(%rip)");
            __asm__ ("jz 114514f");
            __asm__ ("ret");
            __asm__ ("114514:");
            __asm__ ("push %r9");
            __asm__ ("push %r8");
            __asm__ ("push %rdx");
            __asm__ ("push %rcx");
            __asm__ ("call " MANGLE("shim_prepare"));
            __asm__ ("pop %rcx");
            __asm__ ("pop %rdx");
            __asm__ ("pop %r8");
            __asm__ ("pop %r9");
            __asm__ ("ret");
        END_NAKED_FUNCTION()

        #define DECLARE_SHIM(name) \
        FUNCSHIM p##name; \
        BEGIN_NAKED_FUNCTION( Shim##name ) \
            __asm__ ("call " MANGLE("shim_prepare2")); \
            __asm__ ("jmp *" MANGLE("p" #name) "(%rip)"); \
        END_NAKED_FUNCTION()
    #else
        #define BEGIN_NAKED_FUNCTION(name) __asm__(MANGLE(#name) ":");
        #define END_NAKED_FUNCTION()

        BEGIN_NAKED_FUNCTION(shim_prepare2)
            __asm__ ("cmp $0, " MANGLE("shim_prepared"));
            __asm__ ("jz 114514f");
            __asm__ ("ret");
            __asm__ ("114514:");
            __asm__ ("jmp " MANGLE("shim_prepare"));
        END_NAKED_FUNCTION()

        #define DECLARE_SHIM(name) \
        FUNCSHIM p##name; \
        BEGIN_NAKED_FUNCTION( Shim##name ) \
            __asm__ ("call " MANGLE("shim_prepare2")); \
            __asm__ ("jmp *" MANGLE("p" #name)); \
        END_NAKED_FUNCTION()

    #endif
#elif !defined(_WIN64)
    #if defined(__GNUC__) && !defined(__clang__)
        #define OPTIMIZE_ATTR __attribute__((optimize("O2")))
        #define HAS_OPTIMIZE_ATTR 1
    #elif defined(_MSC_VER)
        #define OPTIMIZE_ATTR __pragma(optimize("g2", on))
        #define HAS_OPTIMIZE_ATTR 1
    #else
        #define OPTIMIZE_ATTR
        #define HAS_OPTIMIZE_ATTR 0
    #endif

    #pragma message("depending on compiler optimization: check compiled code for tail call optimization!!")
    #if !defined(_MSC_VER) && !defined(__OPTIMIZE__) && !HAS_OPTIMIZE_ATTR
        #pragma message("not optimizing shim functions - shim is likely to fail")
    #endif
    /*
    we hope this will compile into:
        name:
        call shim_prepare
        jmp [pname]
    usually this can be achieved by reasonable optimization flags.
    */
    #define DECLARE_SHIM(name) \
    FUNCSHIM p##name; \
    OPTIMIZE_ATTR void Shim##name() { \
        shim_prepare(); \
        p##name(); \
    }
#else
    #define DECLARE_SHIM(name) \
    FUNCSHIM p##name; \
    extern void Shim##name();
#endif


DECLARE_SHIM(GetFileVersionInfoA)
DECLARE_SHIM(GetFileVersionInfoByHandle)
DECLARE_SHIM(GetFileVersionInfoExA)
DECLARE_SHIM(GetFileVersionInfoExW)
DECLARE_SHIM(GetFileVersionInfoSizeA)
DECLARE_SHIM(GetFileVersionInfoSizeExA)
DECLARE_SHIM(GetFileVersionInfoSizeExW)
DECLARE_SHIM(GetFileVersionInfoSizeW)
DECLARE_SHIM(GetFileVersionInfoW)
DECLARE_SHIM(VerFindFileA)
DECLARE_SHIM(VerFindFileW)
DECLARE_SHIM(VerInstallFileA)
DECLARE_SHIM(VerInstallFileW)
DECLARE_SHIM(VerLanguageNameA)
DECLARE_SHIM(VerLanguageNameW)
DECLARE_SHIM(VerQueryValueA)
DECLARE_SHIM(VerQueryValueW)

#include <windows.h>

#define FILL_SHIM(hmod, name) p##name = (FUNCSHIM)GetProcAddress(hmod, #name)

static HMODULE hDll;
volatile LONG shim_preparing = 0, shim_prepared = 0;

void shim_prepare() {
    if(shim_prepared) return;
    if(InterlockedCompareExchange(&shim_preparing, 1, 0) == 1) {
        // spin wait for other thread to prepare
        while(!shim_prepared);
        // prepared by other thread
        return;
    }
    wchar_t *dllPath = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, 65536);
	if (GetSystemDirectoryW(dllPath, 32768) != 0)	{
		lstrcatW(dllPath, L"\\version.dll");
		hDll = LoadLibraryW(dllPath);
        HeapFree(GetProcessHeap(), 0, dllPath);
        if(!hDll) {
            InterlockedCompareExchange(&shim_preparing, 0, 1);
            return;
        }
        FILL_SHIM(hDll, GetFileVersionInfoA);
        FILL_SHIM(hDll, GetFileVersionInfoByHandle);
        FILL_SHIM(hDll, GetFileVersionInfoExA);
        FILL_SHIM(hDll, GetFileVersionInfoExW);
        FILL_SHIM(hDll, GetFileVersionInfoSizeA);
        FILL_SHIM(hDll, GetFileVersionInfoSizeExA);
        FILL_SHIM(hDll, GetFileVersionInfoSizeExW);
        FILL_SHIM(hDll, GetFileVersionInfoSizeW);
        FILL_SHIM(hDll, GetFileVersionInfoW);
        FILL_SHIM(hDll, VerFindFileA);
        FILL_SHIM(hDll, VerFindFileW);
        FILL_SHIM(hDll, VerInstallFileA);
        FILL_SHIM(hDll, VerInstallFileW);
        FILL_SHIM(hDll, VerLanguageNameA);
        FILL_SHIM(hDll, VerLanguageNameW);
        FILL_SHIM(hDll, VerQueryValueA);
        FILL_SHIM(hDll, VerQueryValueW);
    }
    InterlockedCompareExchange(&shim_preparing, 0, 1);
    InterlockedCompareExchange(&shim_prepared, 1, 0);
}
