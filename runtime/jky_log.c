/* JOCKY Runtime — Log / Part of libjocky. / Log Collection Implementation */
#include "jky_log.h"
#include "jky_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")

JkyString* jky_log_collect_auth_events(int hours, JkyError** err) {
    char cmd[1024];
    sprintf(cmd, "powershell -NoProfile -Command \"try { $events = Get-WinEvent -FilterHashtable @{LogName='Security'; Id=4624,4625; StartTime=(Get-Date).AddHours(-%d)} -ErrorAction Stop; if ($events) { $events | Select-Object TimeCreated, Id, @{Name='User';Expression={$_.Properties[5].Value}}, @{Name='IP';Expression={$_.Properties[18].Value}} | ConvertTo-Json -Compress } else { '[]' } } catch { '[{\\\"error\\\": \\\"Failed to query Security log. Run as Administrator.\\\"}]' }\"", hours);
    
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) {
        return jky_string_from_cstr("Failed to execute PowerShell.");
    }
    
    char buffer[16384] = {0};
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, pipe);
    buffer[read_bytes] = '\0';
    _pclose(pipe);
    
    // Add newlines between JSON objects in the array
    char out_buffer[32768] = {0};
    int j = 0;
    for (int i = 0; buffer[i] != '\0'; i++) {
        out_buffer[j++] = buffer[i];
        if (buffer[i] == '}' && buffer[i+1] == ',' && buffer[i+2] == '{') {
            out_buffer[j++] = ',';
            out_buffer[j++] = '\n';
            i++; // skip the ',' in original buffer
        }
    }
    
    return jky_string_from_cstr(out_buffer);
}

#elif __linux__
#include <sys/stat.h>

JkyString* jky_log_collect_auth_events(int limit, JkyError** err) {
    FILE *fp = fopen("/var/log/auth.log", "r");
    if (!fp) fp = fopen("/var/log/secure", "r");
    
    if (!fp) {
        return jky_string_from_cstr("[{\"error\": \"Could not open auth logs. Run with sudo.\"}]");
    }

    char json_buffer[8192];
    int offset = 0;
    offset += sprintf(json_buffer + offset, "[");
    
    char line[1024];
    int count = 0;
    
    while (count < limit && fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Failed password") || strstr(line, "Accepted password")) {
            if (count > 0) offset += sprintf(json_buffer + offset, ",");
            offset += sprintf(json_buffer + offset, "{\"log_line\": \"");
            
            // Basic JSON escaping
            for (int i = 0; line[i] != '\0'; i++) {
                if (line[i] == '"') offset += sprintf(json_buffer + offset, "\\\"");
                else if (line[i] == '\n' || line[i] == '\r') continue;
                else {
                    char c[2] = {line[i], '\0'};
                    offset += sprintf(json_buffer + offset, "%s", c);
                }
            }
            offset += sprintf(json_buffer + offset, "\"}");
            count++;
        }
    }
    fclose(fp);
    offset += sprintf(json_buffer + offset, "]");
    
    return jky_string_from_cstr(json_buffer);
}
#else
JkyString* jky_log_collect_auth_events(int limit, JkyError** err) {
    return jky_string_from_cstr("[{\"error\": \"Unsupported OS\"}]");
}
#endif