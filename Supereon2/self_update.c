#include <windows.h>
#include <objbase.h>
#include <urlmon.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "self_update.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ole32.lib")

/* ---- EDIT THESE TWO ------------------------------------------------ */
#define CURRENT_VERSION "1.0.0"
#define VERSION_URL "https://raw.githubusercontent.com/kl1603514-dot/Supereon/main/Supereon2/version.txt"
/* ------------------------------------------------------------------- */

static void logmsg(const char *fmt, ...) {
    char line[1024];
    va_list ap; va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    OutputDebugStringA("[Supereon] "); OutputDebugStringA(line); OutputDebugStringA("\n");
    char p[MAX_PATH];
    GetTempPathA(MAX_PATH, p);
    strncat(p, "supereon_update.log", MAX_PATH - strlen(p) - 1);
    FILE *f = fopen(p, "a");
    if (f) { fprintf(f, "%s\n", line); fclose(f); }
}

static char *clean(char *s) {
    if ((unsigned char)s[0]==0xEF && (unsigned char)s[1]==0xBB && (unsigned char)s[2]==0xBF) s += 3;
    s += strspn(s, " \t");
    s[strcspn(s, "\r\n")] = 0;
    size_t n = strlen(s);
    while (n && (s[n-1]==' ' || s[n-1]=='\t')) s[--n] = 0;
    return s;
}

static int is_newer(const char *remote, const char *local) {
    int r[3]={0,0,0}, l[3]={0,0,0};
    if (sscanf(remote, "%d.%d.%d", &r[0], &r[1], &r[2]) < 1) return 0;
    sscanf(local, "%d.%d.%d", &l[0], &l[1], &l[2]);
    for (int i=0;i<3;++i) if (r[i]!=l[i]) return r[i] > l[i];
    return 0;
}

static int download(const char *url, const char *dest, int expect_exe, int bust) {
    char full[1200];
    if (bust) snprintf(full, sizeof full, "%s%cnc=%lu",
                       url, strchr(url,'?')?'&':'?', (unsigned long)GetTickCount());
    else      snprintf(full, sizeof full, "%s", url);

    char part[MAX_PATH];
    snprintf(part, sizeof part, "%s.part", dest);
    DeleteFileA(part);

    HRESULT hr = URLDownloadToFileA(NULL, full, part, 0, NULL);
    logmsg("download %s -> hr=0x%08lX", full, (unsigned long)hr);
    if (hr != S_OK) { DeleteFileA(part); return 0; }

    HANDLE h = CreateFileA(part, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { logmsg("open .part failed"); DeleteFileA(part); return 0; }
    LARGE_INTEGER sz={0}; GetFileSizeEx(h, &sz);
    unsigned char mz[2]={0}; DWORD got=0; ReadFile(h, mz, 2, &got, NULL);
    CloseHandle(h);
    logmsg("downloaded %lld bytes, first2=%02X %02X", (long long)sz.QuadPart, mz[0], mz[1]);
    if (sz.QuadPart < 64) { DeleteFileA(part); return 0; }
    if (expect_exe && !(mz[0]=='M' && mz[1]=='Z')) { logmsg("not an exe"); DeleteFileA(part); return 0; }

    DeleteFileA(dest);
    if (!MoveFileA(part, dest)) { logmsg("move .part failed err=%lu", GetLastError()); DeleteFileA(part); return 0; }
    return 1;
}

static int sha256_file(const char *path, char out[65]) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE hh=NULL;
    unsigned char dig[32], buf[65536]; DWORD n; int ok=0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0)) goto done;
    if (BCryptCreateHash(alg, &hh, NULL, 0, NULL, 0, 0)) goto done;
    while (ReadFile(f, buf, sizeof buf, &n, NULL) && n)
        if (BCryptHashData(hh, buf, n, 0)) goto done;
    if (BCryptFinishHash(hh, dig, sizeof dig, 0)) goto done;
    for (int i=0;i<32;++i) sprintf(out + i*2, "%02x", dig[i]);
    out[64]=0; ok=1;
done:
    if (hh) BCryptDestroyHash(hh);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(f);
    return ok;
}

int self_update_check(int argc, char **argv) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    logmsg("=== check start, CURRENT_VERSION=%s ===", CURRENT_VERSION);

    char selfdir[MAX_PATH], self[MAX_PATH];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    strcpy(selfdir, self);
    char *bs = strrchr(selfdir, '\\'); if (bs) *bs = 0;
    logmsg("self=%s", self);

    char leftover[MAX_PATH];
    snprintf(leftover, sizeof leftover, "%s\\Supereon_update.exe", selfdir);

    for (int i = 1; i < argc - 1; ++i)
        if (!strcmp(argv[i], "--updated-to") && is_newer(argv[i+1], CURRENT_VERSION)) {
            logmsg("relaunched but still old; giving up");
            return 0;
        }

    char mpath[MAX_PATH];
    GetTempPathA(MAX_PATH, mpath);
    strncat(mpath, "supereon_manifest.txt", MAX_PATH - strlen(mpath) - 1);
    if (!download(VERSION_URL, mpath, 0, 1)) { logmsg("manifest download failed"); return 0; }

    FILE *fp = fopen(mpath, "rb");
    if (!fp) { logmsg("cannot open manifest"); return 0; }
    char l1[128]={0}, l2[160]={0}, l3[512]={0};
    fgets(l1,sizeof l1,fp); fgets(l2,sizeof l2,fp); fgets(l3,sizeof l3,fp);
    fclose(fp); DeleteFileA(mpath);

    char *remote = clean(l1), *want_sha = clean(l2), *exe_url = clean(l3);
    logmsg("remote=%s  sha=%s  url=%s", remote, want_sha, exe_url);

    if (!remote[0] || !is_newer(remote, CURRENT_VERSION)) { logmsg("no update needed"); DeleteFileA(leftover); return 0; }
    if (!exe_url[0]) { logmsg("manifest missing url"); return 0; }

    if (!download(exe_url, leftover, 1, 0)) { logmsg("exe download failed"); return 0; }

    if (want_sha[0]) {
        char got[65];
        int okh = sha256_file(leftover, got);
        logmsg("sha check: got=%s want=%s", okh?got:"(fail)", want_sha);
        if (!okh || _stricmp(got, want_sha) != 0) { logmsg("SHA mismatch; discard"); DeleteFileA(leftover); return 0; }
    }

    char bat[MAX_PATH];
    GetTempPathA(MAX_PATH, bat);
    strncat(bat, "supereon_apply.bat", MAX_PATH - strlen(bat) - 1);
    FILE *b = fopen(bat, "wb");
    if (!b) { logmsg("cannot write bat"); DeleteFileA(leftover); return 0; }
    fprintf(b,
        "@echo off\r\n"
        "cd /d \"%s\"\r\n"
        "set /a n=0\r\n"
        ":retry\r\n"
        "move /y \"%s\" \"%s\" >nul 2>&1\r\n"
        "if not exist \"%s\" goto ok\r\n"
        "set /a n+=1\r\n"
        "if %%n%% geq 150 goto ok\r\n"
        ">nul ping -n 2 127.0.0.1\r\n"
        "goto retry\r\n"
        ":ok\r\n"
        "start \"\" \"%s\" --updated-to %s\r\n"
        "del \"%%~f0\" >nul 2>&1\r\n",
        selfdir, leftover, self, leftover, self, remote);
    fclose(b);

    char cmd[MAX_PATH*2];
    snprintf(cmd, sizeof cmd, "cmd.exe /c \"%s\"", bat);
    STARTUPINFOA si = { sizeof si };
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        logmsg("CreateProcess failed err=%lu", GetLastError());
        DeleteFileA(leftover); return 0;
    }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    logmsg("helper launched, exiting for swap");
    return 1;
}
