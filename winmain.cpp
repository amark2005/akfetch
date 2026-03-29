#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <direct.h>
#include <lmcons.h>

static void get_user_host(char *user, size_t ul, char *host, size_t hl) {
    DWORD u_len = (DWORD)ul;
    DWORD h_len = (DWORD)hl;
    if (!GetUserNameA(user, &u_len)) strncpy(user, "unknown", ul);
    if (!GetComputerNameA(host, &h_len)) strncpy(host, "unknown", hl);
}

static void get_os(char *buf, size_t len) {
    const char* os_name = getenv("OS");
    snprintf(buf, len, "%s", os_name ? os_name : "Windows");
}

static void get_device(char *buf, size_t len) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SystemInformation", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD s = (DWORD)len;
        RegQueryValueExA(hKey, "SystemProductName", NULL, NULL, (LPBYTE)buf, &s);
        RegCloseKey(hKey);
    } else {
        strncpy(buf, "Windows Device", len - 1);
    }
}

static void get_shell(char *buf, size_t len) {
    const char *s = getenv("ComSpec");
    if (!s) { strncpy(buf, "unknown", len - 1); return; }
    const char *base = strrchr(s, '\\');
    strncpy(buf, base ? base + 1 : s, len - 1);
}

static void get_cpu(char *buf, size_t len) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD s = (DWORD)len;
        RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)buf, &s);
        RegCloseKey(hKey);
    } else {
        strncpy(buf, "Unknown CPU", len - 1);
    }
}

static void get_ram(char *buf, size_t len) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    unsigned long total = (unsigned long)(statex.ullTotalPhys / (1024 * 1024));
    unsigned long used = (unsigned long)((statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024));
    snprintf(buf, len, "%lu MiB / %lu MiB", used, total);
}

int main(void) {
    char user[64], host[64], os[128], device[128], shell[64], cpu[128], ram[64];

    get_user_host(user, sizeof user, host, sizeof host);
    get_os(os, sizeof os);
    get_device(device, sizeof device);
    get_shell(shell, sizeof shell);
    get_cpu(cpu, sizeof cpu);
    get_ram(ram, sizeof ram);

    printf("\t%s@%s\n", user, host);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("OS:       %s\n", os);
    printf("Device:   %s\n", device);
    printf("Shell:    %s\n", shell);
    printf("CPU:      %s\n", cpu);
    printf("RAM:      %s\n", ram);

    return 0;
}
