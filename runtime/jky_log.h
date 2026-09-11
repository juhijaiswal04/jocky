/* JOCKY Runtime — Log / Part of libjocky. / Log Collection Interface */
#ifndef JKY_LOG_H
#define JKY_LOG_H

#include "jky_value.h"
#include <stdio.h>

typedef enum { JKY_LOG_INFO, JKY_LOG_WARN, JKY_LOG_ERROR } JkyLogLevel;

void jky_log_init(JkyLogLevel min_level);
void jky_log_info(JkyString *msg);
void jky_log_warn(JkyString *msg);
void jky_log_error(JkyString *msg);
void jky_log_set_output(FILE *f);

// DFIR Function
JkyString* jky_log_collect_auth_events(int limit, JkyError** err);

#endif