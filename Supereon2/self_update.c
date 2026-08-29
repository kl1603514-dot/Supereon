#include <windows.h>
#include <urlmon.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>
#include "self_update.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "bcrypt.lib")

/* ---- EDIT THESE TWO ------------------------------------------------ */
#define CURRENT_VERSION "1.0.0"
#define VERSION_URL "https://raw.githubusercontent.com/kl1603514-dot/Supereon/main/version.txt"
/* ------------------------------------------------------------------- */

static void logline(const char *m) {
    OutputDebugStringA("[Supereon update] "); OutputDebugStringA(m); OutputDebugStringA("\n");
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

    if (URLDownloadToFileA(NULL, full, part, 0, NULL) != S_OK) {
        logline("download failed"); DeleteFileA(part); return 0;
    }
    HANDLE h = CreateFileA(part, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { DeleteFileA(part); return 0; }
    LARGE_INTEGER sz={0}; GetFileSizeEx(h, &sz);
    unsigned char mz[2]={0}; DWORD got=0; ReadFile(h, mz, 2, &got, NULL);
    CloseHandle(h);
    if (sz.QuadPart < 64) { logline("file too small"); DeleteFileA(part); return 0; }
    if (expect_exe && !(mz[0]=='M' && mz[1]=='Z')) {
        logline("not an exe"); DeleteFileA(part); return 0;
    }
    DeleteFileA(dest);
    if (!MoveFileA(part, dest)) { DeleteFileA(part); return 0; }
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
    char selfdir[MAX_PATH], self[MAX_PATH];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    strcpy(selfdir, self);
    char *bs = strrchr(selfdir, '\\'); if (bs) *bs = 0;

    char leftover[MAX_PATH];
    snprintf(leftover, sizeof leftover, "%s\\Supereon_update.exe", selfdir);

    for (int i = 1; i < argc - 1; ++i)
        if (!strcmp(argv[i], "--updated-to") && is_newer(argv[i+1], CURRENT_VERSION)) {
            logline("update didn't take; running current build");
            return 0;
        }

    char mpath[MAX_PATH];
    GetTempPathA(MAX_PATH, mpath);
    strncat(mpath, "supereon_manifest.txt", MAX_PATH - strlen(mpath) - 1);
    if (!download(VERSION_URL, mpath, 0, 1)) { logline("offline; skip"); return 0; }

    FILE *fp = fopen(mpath, "rb");
    if (!fp) return 0;
    char l1[128]={0}, l2[160]={0}, l3[512]={0};
    fgets(l1,sizeof l1,fp); fgets(l2,sizeof l2,fp); fgets(l3,sizeof l3,fp);
    fclose(fp); DeleteFileA(mpath);

    char *remote = clean(l1), *want_sha = clean(l2), *exe_url = clean(l3);

    if (!remote[0] || !is_newer(remote, CURRENT_VERSION)) { DeleteFileA(leftover); return 0; }
    if (!exe_url[0]) { logline("manifest missing url"); return 0; }

    if (!download(exe_url, leftover, 1, 0)) { logline("fetch new build failed"); return 0; }

    if (want_sha[0]) {
        char got[65];
        if (!sha256_file(leftover, got) || _stricmp(got, want_sha) != 0) {
            logline("SHA-256 mismatch; discarding"); DeleteFileA(leftover); return 0;
        }
    }

    char bat[MAX_PATH];
    GetTempPathA(MAX_PATH, bat);
    strncat(bat, "supereon_apply.bat", MAX_PATH - strlen(bat) - 1);
    FILE *b = fopen(bat, "wb");
    if (!b) { DeleteFileA(leftover); return 0; }
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
        logline("helper launch failed"); DeleteFileA(leftover); return 0;
    }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 1;
}