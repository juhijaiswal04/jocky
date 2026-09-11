/* JOCKY Runtime — Process / Part of libjocky. / Process Information Implementation */
#include "jky_process.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
static void _jky_enable_debug_privilege() {
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LUID luid;
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
            TOKEN_PRIVILEGES tkp;
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Luid = luid;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL);
        }
        CloseHandle(hToken);
    }
}
#elif __linux__
#include <dirent.h>
#include <ctype.h>
#endif

// --- Internal JSON String Builder (Self-contained, no external deps) ---
typedef struct {
    char* data;
    size_t len;
    size_t cap;
} JsonBuilder;

static void jb_init(JsonBuilder* jb) {
    jb->cap = 4096;
    jb->data = (char*)malloc(jb->cap);
    jb->len = 0;
    jb->data[0] = '\0';
}

static void jb_append(JsonBuilder* jb, const char* str) {
    size_t slen = strlen(str);
    while (jb->len + slen + 1 > jb->cap) {
        jb->cap *= 2;
        jb->data = (char*)realloc(jb->data, jb->cap);
    }
    memcpy(jb->data + jb->len, str, slen);
    jb->len += slen;
    jb->data[jb->len] = '\0';
}

static void jb_append_int(JsonBuilder* jb, int64_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)val);
    jb_append(jb, buf);
}

static void jb_append_escaped(JsonBuilder* jb, const char* str) {
    jb_append(jb, "\"");
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            char escape[3] = { '\\', str[i], '\0' };
            jb_append(jb, escape);
        } else {
            char c[2] = { str[i], '\0' };
            jb_append(jb, c);
        }
    }
    jb_append(jb, "\"");
}

static char* jb_finish(JsonBuilder* jb) {
    return jb->data; // Caller must free()
}


// --- Feature #1: Process Enumeration Implementation ---
char* jky_process_enumerate(void) {
    JsonBuilder jb;
    jb_init(&jb);
    jb_append(&jb, "[\n");

#ifdef _WIN32
    _jky_enable_debug_privilege();
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        int first = 1;

        if (Process32First(hSnap, &pe)) {
            do {
                char fullPath[MAX_PATH] = {0};
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProc) {
                    DWORD size = MAX_PATH;
                    QueryFullProcessImageNameA(hProc, 0, fullPath, &size);
                    CloseHandle(hProc);
                }

                if (!first) jb_append(&jb, ",\n");
                first = 0;

                jb_append(&jb, "{\"pid\":");
                jb_append_int(&jb, pe.th32ProcessID);
                jb_append(&jb, ",\"name\":");
                jb_append_escaped(&jb, pe.szExeFile);
                jb_append(&jb, ",\"ppid\":");
                jb_append_int(&jb, pe.th32ParentProcessID);
                jb_append(&jb, ",\"path\":");
                jb_append_escaped(&jb, fullPath[0] ? fullPath : "ACCESS_DENIED");
                jb_append(&jb, "}");
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
#elif __linux__
    DIR* dir = opendir("/proc");
    if (dir) {
        struct dirent* entry;
        int first = 1;

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                int is_pid = 1;
                for (int i = 1; entry->d_name[i] != '\0'; i++) {
                    if (!isdigit(entry->d_name[i])) { is_pid = 0; break; }
                }
                if (!is_pid) continue;

                char path[256];
                snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
                int64_t pid = atoll(entry->d_name); // Safer: use directory name
                
                FILE* f = fopen(path, "r");
                if (f) {
                    char line[1024];
                    if (fgets(line, sizeof(line), f)) {
                        int64_t ppid = 0;
                        char comm[256] = {0};
                        
                        char* start_paren = strchr(line, '(');
                        char* end_paren = strrchr(line, ')');
                        
                        if (start_paren && end_paren && end_paren > start_paren) {
                            size_t name_len = end_paren - start_paren - 1;
                            if (name_len < sizeof(comm)) {
                                strncpy(comm, start_paren + 1, name_len);
                                comm[name_len] = '\0';
                            }
                            
                            // state is 1 char after ')', then space, then ppid
                            sscanf(end_paren + 1, " %*c %lld", (long long*)&ppid);

                            if (!first) jb_append(&jb, ",\n");
                            first = 0;

                            jb_append(&jb, "{\"pid\":");
                            jb_append_int(&jb, pid);
                            jb_append(&jb, ",\"name\":");
                            jb_append_escaped(&jb, comm);
                            jb_append(&jb, ",\"ppid\":");
                            jb_append_int(&jb, ppid);
                            jb_append(&jb, "}");
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(dir);
    }
#endif

    jb_append(&jb, "\n]");
    return jb_finish(&jb);
}