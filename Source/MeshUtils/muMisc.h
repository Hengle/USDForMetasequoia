#pragma once

#if defined(_WIN32)
    #define muDLLPrefix 
    #define muDLLSuffix     ".dll"
    #define muEXESuffix     ".exe"
    #define muPathSep       "\\"
    #define muPathListSep   ";"
#else
    #define muDLLPrefix     "lib"
    #if defined(__APPLE__)
        #define muDLLSuffix ".dylib"
    #else
        #define muDLLSuffix ".so"
    #endif
    #define muEXESuffix 
    #define muPathSep       "/"
    #define muPathListSep   ":"
#endif

#include <functional>

namespace mu {

template < typename T, size_t N >
size_t countof(T(&arr)[N]) { return std::extent< T[N] >::value; }

using nanosec = uint64_t;
nanosec Now();
inline float NS2MS(nanosec ns) { return (float)((double)ns / 1000000.0); }
inline float NS2S(nanosec ns) { return (float)((double)ns / 1000000000.0); }
inline double NS2Sd(nanosec ns) { return ((double)ns / 1000000000.0); }
inline nanosec MS2NS(float ms) { return (nanosec)((double)ms * 1000000.0); }
inline nanosec S2NS(float s) { return (nanosec)((double)s * 1000000000.0); }
inline nanosec S2NS(double s) { return (nanosec)(s * 1000000000.0); }

using PrintHandler = std::function<void(const char*)>;
void SetPrintHandler(const PrintHandler& v);
void Print(const char *fmt, ...);
void Print(const wchar_t *fmt, ...);
const char* Format(const char *fmt, ...);

std::string ToUTF8(const char* src);
std::string ToUTF8(const wchar_t* src);
std::string ToUTF8(const std::string& src);
std::string ToUTF8(const std::wstring& src);
std::string ToANSI(const char* src);
std::string ToANSI(const wchar_t* src);
std::string ToANSI(const std::string& src);
std::string ToANSI(const std::wstring& src);

std::string ToMBS(const wchar_t* src);
std::string ToMBS(const std::wstring& src);
std::wstring ToWCS(const char* src);
std::wstring ToWCS(const std::string& src);

std::string SanitizeFileName(const std::string& src);
std::string GetDirectory(const char* src);
std::string GetDirectory(const wchar_t* src);
std::string GetParentDirectory(const char* src);
std::string GetFilename(const char* src);
std::string GetFilename(const wchar_t* src);
std::string GetFilename_NoExtension(const char* src);
std::string GetFilename_NoExtension(const wchar_t* src);

std::string GetCurrentModuleDirectory();

void SetEnv(const char* name, const char* value);
void* LoadModule(const char *path);
void* GetMainModule();
void* GetModule(const char *module_name);
std::string GetModuleName(void *mod);
void* GetSymbol(void *module, const char *name);

void InitializeSymbols(const char *path = nullptr);
void* FindSymbolByName(const char *name);
void* FindSymbolByName(const char *name, const char *module_name);
int CaptureCallstack(void** dst, size_t dst_len);
void AddressToSymbolName(char* dst, size_t dst_len, void* address);
void DbgBreak();



enum class MemoryFlags
{
    ExecuteRead,
    ReadWrite,
    ExecuteReadWrite,
};
void SetMemoryProtection(void *addr, size_t size, MemoryFlags flags);

template<class T>
inline void ForceWrite(void *dst, const T &src)
{
    SetMemoryProtection(dst, sizeof(T), MemoryFlags::ExecuteReadWrite);
    memcpy(dst, &src, sizeof(T));
    SetMemoryProtection(dst, sizeof(T), MemoryFlags::ExecuteRead);
}


#ifdef _WIN32
// F: [](const char *dllname, const char *funcname, DWORD ordinal, void *import_address) -> void
template<class F>
inline void EnumerateDLLImports(HMODULE module, const F &f)
{
    if (module == NULL) { return; }

    size_t ImageBase = (size_t)module;
    auto pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) { return; }

    auto pNTHeader = (PIMAGE_NT_HEADERS)(ImageBase + pDosHeader->e_lfanew);
    DWORD RVAImports = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (RVAImports == 0) { return; }

    auto *pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)(ImageBase + RVAImports);
    while (pImportDesc->Name != 0) {
        const char *pDLLName = (const char*)(ImageBase + pImportDesc->Name);
        auto* pThunkOrig = (IMAGE_THUNK_DATA*)(ImageBase + pImportDesc->OriginalFirstThunk);
        auto* pThunk = (IMAGE_THUNK_DATA*)(ImageBase + pImportDesc->FirstThunk);
        while (pThunkOrig->u1.AddressOfData != 0) {
            const char *name = nullptr;
            DWORD ordinal = 0;

#ifdef _WIN64
            if (pThunkOrig->u1.Ordinal & IMAGE_ORDINAL_FLAG64) {
                ordinal = IMAGE_ORDINAL64(pThunkOrig->u1.Ordinal);
            }
#else
            if (pThunkOrig->u1.Ordinal & IMAGE_ORDINAL_FLAG32) {
                ordinal = IMAGE_ORDINAL32(pThunkOrig->u1.Ordinal);
            }
#endif
            else {
                auto* pIBN = (IMAGE_IMPORT_BY_NAME*)(ImageBase + pThunkOrig->u1.AddressOfData);
                name = pIBN->Name;
            }
            f(pDLLName, name, ordinal, *(void**)pThunk);
            ++pThunkOrig;
            ++pThunk;
        }
        ++pImportDesc;
    }
    return;
}
#endif //_WIN32
} // namespace mu
