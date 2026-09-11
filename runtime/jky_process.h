/* JOCKY Runtime — Process / Part of libjocky. / Process Information */
#ifndef JKY_PROCESS_H
#define JKY_PROCESS_H

#include "jky_value.h"
#include <stdint.h>

typedef struct JkyModuleEntry {
    JkyString *name;
    JkyString *path;
    int64_t    base_addr;
    int64_t    size;
} JkyModuleEntry;

typedef struct JkyProcessEntry {
    int64_t    pid;
    int64_t    ppid;
    JkyString *name;
    JkyString *exe_path;
    JkyString *cmdline;
    JkyString *username;
    int64_t    create_time;
    int64_t    mem_rss;
    int64_t    mem_virt;
    int        thread_count;
    JkyList   *modules;
} JkyProcessEntry;

JkyList         *jky_process_list(JkyError **err);
JkyProcessEntry *jky_process_info(int64_t pid, JkyError **err);
JkyList         *jky_process_modules(int64_t pid, JkyError **err);
JkyProcessEntry *jky_process_find(JkyString *name, int *found, JkyError **err);
JkyString       *jky_process_entry_json(JkyProcessEntry *p);
JkyString       *jky_process_list_json(JkyList *procs);

char* jky_process_enumerate(void);

#endif // JKY_PROCESS_H
