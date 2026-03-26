#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>


static bool read_file(const char *path, char *buf, size_t len)
{
    FILE *f = fopen(path, "r");
    if (!f) return false;
    bool ok = fgets(buf, (int)len, f) != nullptr;
    fclose(f);
    if (ok) {
        size_t n = strlen(buf);
        if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';
    }
    return ok;
}

static bool run_cmd(const char *cmd, char *buf, size_t len)
{
    FILE *p = popen(cmd, "r");
    if (!p) return false;
    bool ok = fgets(buf, (int)len, p) != nullptr;
    pclose(p);
    if (ok) {
        size_t n = strlen(buf);
        if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';
    }
    return ok;
}

static void get_user_host(char *user, size_t ul, char *host, size_t hl)
{
    struct passwd *pw = getpwuid(getuid());
    strncpy(user, pw ? pw->pw_name : "unknown", ul - 1);
    gethostname(host, (int)hl);
}

static void get_os(char *buf, size_t len)
{
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        struct utsname u;
        uname(&u);
        snprintf(buf, len, "%s %s", u.sysname, u.release);
        return;
    }
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "PRETTY_NAME=", 12) != 0) continue;
        char *val = line + 12;
        size_t n = strlen(val);
        if (n && val[n - 1] == '\n') val[--n] = '\0';
        if (val[0] == '"') { val++; n--; }
        if (n && val[n - 1] == '"') val[n - 1] = '\0';
        strncpy(buf, val, len - 1);
        fclose(f);
        return;
    }
    fclose(f);
    strncpy(buf, "Unknown OS", len - 1);
}

static void get_device(char *buf, size_t len)
{
    char name[128] = {}, ver[128] = {};
    bool has_name = read_file("/sys/class/dmi/id/product_name", name, sizeof name);
    bool has_ver  = read_file("/sys/class/dmi/id/product_version", ver, sizeof ver);

    if (has_name && has_ver && strcmp(ver, "None") != 0 && ver[0] != '\0')
        snprintf(buf, len, "%s %s", name, ver);
    else if (has_name)
        strncpy(buf, name, len - 1);
    else
        strncpy(buf, "Unknown Device", len - 1);
}

static void get_shell(char *buf, size_t len)
{
    const char *s = getenv("SHELL");
    if (!s) { strncpy(buf, "unknown", len - 1); return; }
    const char *base = strrchr(s, '/');
    strncpy(buf, base ? base + 1 : s, len - 1);
}

static void get_cpu(char *buf, size_t len)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) { strncpy(buf, "Unknown CPU", len - 1); return; }
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "model name", 10) != 0) continue;
        char *colon = strchr(line, ':');
        if (!colon) continue;
        colon += 2;
        size_t n = strlen(colon);
        if (n && colon[n - 1] == '\n') colon[n - 1] = '\0';
        strncpy(buf, colon, len - 1);
        fclose(f);
        return;
    }
    fclose(f);
    strncpy(buf, "Unknown CPU", len - 1);
}

static void get_ram(char *buf, size_t len)
{
    struct sysinfo si;
    if (sysinfo(&si) != 0) { strncpy(buf, "Unknown", len - 1); return; }
    unsigned long used  = (si.totalram - si.freeram) * si.mem_unit / (1024 * 1024);
    unsigned long total = si.totalram * si.mem_unit / (1024 * 1024);
    snprintf(buf, len, "%lu MiB / %lu MiB", used, total);
}

static void get_gpu(char *buf, size_t len)
{
    if (!run_cmd("lspci 2>/dev/null | grep -m1 -iE 'VGA|3D|Display' | sed 's/.*: //'", buf, len))
        strncpy(buf, "Unknown GPU", len - 1);
}

static void get_battery(char *buf, size_t len)
{
    char cap_path[128], stat_path[128];
    for (int i = 0; i <= 2; i++) {
        snprintf(cap_path,  sizeof cap_path,  "/sys/class/power_supply/BAT%d/capacity", i);
        snprintf(stat_path, sizeof stat_path, "/sys/class/power_supply/BAT%d/status", i);
        char cap[16] = {}, status[32] = {};
        if (!read_file(cap_path, cap, sizeof cap)) continue;
        read_file(stat_path, status, sizeof status);
        snprintf(buf, len, "%s%% [%s]", cap, status[0] ? status : "Unknown");
        return;
    }
    strncpy(buf, "No battery / N/A", len - 1);
}

static void print_sep(const char *user, const char *host)
{
    int len = (int)(strlen(user) + 1 + strlen(host));
    for (int i = 0; i < len; i++) putchar('~');
    putchar('\n');
}

int main(void)
{
    char user[64], host[64];
    char os[128], device[128], shell[64];
    char cpu[128], ram[64], gpu[128], battery[64];

    get_user_host(user, sizeof user, host, sizeof host);
    get_os(os, sizeof os);
    get_device(device, sizeof device);
    get_shell(shell, sizeof shell);
    get_cpu(cpu, sizeof cpu);
    get_ram(ram, sizeof ram);
    get_gpu(gpu, sizeof gpu);
    get_battery(battery, sizeof battery);

    printf("\t" "%s" "@" "%s" "\n", user, host);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    //print_sep(user, host);

    printf( "OS"      ":       %s\n", os);
    printf( "Device"  ":   %s\n", device);
    printf( "Shell"   ":    %s\n", shell);
    printf( "CPU"     ":      %s\n", cpu);
    printf( "RAM"     ":      %s\n", ram);
    printf( "GPU"     ":      %s\n", gpu);
    printf( "Battery" ":  %s\n", battery);

    return 0;
}