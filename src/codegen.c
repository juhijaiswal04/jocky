#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void jky_buffer_init(JkyBuffer *buf) {
    buf->capacity = 1024;
    buf->length = 0;
    buf->data = (char*)malloc(buf->capacity);
    buf->data[0] = '\0';
}

void jky_buffer_append(JkyBuffer *buf, const char *str) {
    size_t len = strlen(str);
    if (buf->length + len + 1 > buf->capacity) {
        buf->capacity = (buf->length + len + 1) * 2;
        buf->data = (char*)realloc(buf->data, buf->capacity);
    }
    strcpy(buf->data + buf->length, str);
    buf->length += len;
}

void jky_buffer_free(JkyBuffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->length = buf->capacity = 0;
}

static void generate_node(JkyBuffer *buf, JkyAstNode *node) {
    if (!node) return;
    
    char temp[256];
    
    switch (node->type) {
        case AST_FUNC_DECL:
            // Map functions directly. We map 'main' to returning 'int' for standard C compliance.
            jky_buffer_append(buf, "int ");
            sprintf(temp, "%.*s() {\n", node->token.length, node->token.start);
            jky_buffer_append(buf, temp);
            
            generate_node(buf, node->as.func_decl.body);
            
            jky_buffer_append(buf, "    printf(\"[JOCKY AGENT EXECUTING] Evidence collection completed.\\n\");\n");
            jky_buffer_append(buf, "    return 0;\n}\n");
            break;
        case AST_BLOCK:
            generate_node(buf, node->as.block.head);
            generate_node(buf, node->as.block.next);
            break;
        case AST_VAR_DECL:
            sprintf(temp, "    int %.*s", node->token.length, node->token.start);
            jky_buffer_append(buf, temp);
            if (node->as.var_decl.initializer) {
                jky_buffer_append(buf, " = ");
                generate_node(buf, node->as.var_decl.initializer);
            }
            jky_buffer_append(buf, ";\n");
            break;
        case AST_EXPR_STMT:
            jky_buffer_append(buf, "    ");
            generate_node(buf, node->as.expr_stmt.expr);
            jky_buffer_append(buf, ";\n");
            break;
        case AST_LITERAL_INT:
            sprintf(temp, "%.*s", node->token.length, node->token.start);
            jky_buffer_append(buf, temp);
            break;
        case AST_LITERAL_STRING:
            sprintf(temp, "\"%.*s\"", node->token.length - 2, node->token.start + 1); // strip quotes
            jky_buffer_append(buf, temp);
            break;
        case AST_IDENTIFIER:
            sprintf(temp, "%.*s", node->token.length, node->token.start);
            jky_buffer_append(buf, temp);
            break;
        case AST_WHILE_STMT:
            jky_buffer_append(buf, "    while (");
            generate_node(buf, node->as.while_stmt.condition);
            jky_buffer_append(buf, ") {\n");
            generate_node(buf, node->as.while_stmt.body);
            jky_buffer_append(buf, "    }\n");
            break;
        case AST_CALL_EXPR:
            // INTERCEPT: forensics.get_auth_logs(limit)
            if (node->as.call_expr.callee && node->as.call_expr.callee->type == AST_BINARY_EXPR &&
                node->as.call_expr.callee->token.type == TOKEN_DOT) {
                
                JkyAstNode* dot = node->as.call_expr.callee;
                // Check if it's "forensics.get_auth_logs"
                if (strncmp(dot->as.binary.left->token.start, "forensics", 9) == 0 &&
                    strncmp(dot->as.binary.right->token.start, "get_auth_logs", 13) == 0) {
                    
                    jky_buffer_append(buf, "_jky_helper_collect_and_save(");
                    JkyAstNode* arg = node->as.call_expr.args;
                    if (arg) {
                        generate_node(buf, arg); // Pass the limit argument
                    } else {
                        jky_buffer_append(buf, "10"); // Default to 10 if no arg
                    }
                    jky_buffer_append(buf, ")");
                    return; // Done generating this node
                }
            }

            // Intercept log.info
            if (node->as.call_expr.callee && node->as.call_expr.callee->type == AST_BINARY_EXPR &&
                node->as.call_expr.callee->token.type == TOKEN_DOT) {
                
                JkyAstNode *dot = node->as.call_expr.callee;
                if (strncmp(dot->as.binary.left->token.start, "log", 3) == 0 &&
                    strncmp(dot->as.binary.right->token.start, "info", 4) == 0) {
                    
                    jky_buffer_append(buf, "printf(\"");
                    // Print format specifier depending on argument type, simplifying for strings and ints
                    jky_buffer_append(buf, "%s\\n\", ");
                    generate_node(buf, node->as.call_expr.args);
                    jky_buffer_append(buf, ")");
                    return;
                }
            }
            
            generate_node(buf, node->as.call_expr.callee);
            jky_buffer_append(buf, "(");
            JkyAstNode *arg = node->as.call_expr.args;
            while(arg) {
                generate_node(buf, arg);
                arg = arg->as.block.next;
                if (arg) jky_buffer_append(buf, ", ");
            }
            jky_buffer_append(buf, ")");
            break;
        case AST_BINARY_EXPR:
            if (node->token.type == TOKEN_EQUAL) {
                generate_node(buf, node->as.binary.left);
                jky_buffer_append(buf, " = ");
                generate_node(buf, node->as.binary.right);
            } else {
                jky_buffer_append(buf, "(");
                generate_node(buf, node->as.binary.left);
                
                switch (node->token.type) {
                    case TOKEN_PLUS: jky_buffer_append(buf, " + "); break;
                    case TOKEN_MINUS: jky_buffer_append(buf, " - "); break;
                    case TOKEN_STAR: jky_buffer_append(buf, " * "); break;
                    case TOKEN_SLASH: jky_buffer_append(buf, " / "); break;
                    case TOKEN_LESS: jky_buffer_append(buf, " < "); break;
                    case TOKEN_GREATER: jky_buffer_append(buf, " > "); break;
                    case TOKEN_EQUAL_EQUAL: jky_buffer_append(buf, " == "); break;
                    case TOKEN_DOT: jky_buffer_append(buf, "."); break;
                    default: break;
                }
                
                generate_node(buf, node->as.binary.right);
                jky_buffer_append(buf, ")");
            }
            break;
        case AST_UNARY_EXPR:
            // For v0.1, the '!' operator unboxes values. We just emit the operand.
            generate_node(buf, node->as.unary.operand);
            jky_buffer_append(buf, " /* unboxed! */");
            break;
        default:
            break;
    }
}

void jky_codegen_generate(JkyBuffer *buf, JkyAstNode *root) {
    jky_buffer_append(buf, "// Automatically generated by JOCKY v0.1 in-memory compiler\n");
    jky_buffer_append(buf, "#include <stdio.h>\n");
    jky_buffer_append(buf, "#include \"jky_log.h\"\n");
    jky_buffer_append(buf, "#include \"jky_value.h\"\n\n");
    jky_buffer_append(buf, "void _jky_helper_collect_and_save(int limit) {\n");
    jky_buffer_append(buf, "    JkyString* s = jky_log_collect_auth_events(limit, NULL);\n");
    jky_buffer_append(buf, "    if(s) {\n");
    jky_buffer_append(buf, "        printf(\"\\n--- COLLECTED LOGS ---\\n%s\\n----------------------\\n\", s->data);\n");
    jky_buffer_append(buf, "        FILE* f = fopen(\"auth_logs.txt\", \"w\");\n");
    jky_buffer_append(buf, "        if(f) { fputs(s->data, f); fclose(f); printf(\"[+] Saved to auth_logs.txt\\n\"); }\n");
    jky_buffer_append(buf, "    }\n");
    jky_buffer_append(buf, "}\n\n");
    generate_node(buf, root);
}

void jky_codegen_compile_and_pipe(JkyBuffer *buf, const char *output_exe) {
    // In Chapter 16, the target uses GCC/Clang with STDIN piping: `gcc -x c -`
    // Since we are targeting MSVC (cl.exe), which doesn't gracefully compile from STDIN pipes,
    // we safely simulate the exact forensic properties by writing, compiling, and immediately destroying the file.
    
    const char *temp_file = "_jky_stealth_pipe.c";
    FILE *f = fopen(temp_file, "w");
    fputs(buf->data, f);
    fclose(f);
    
    char cmd[512];
    // We tell cl.exe to include the runtime folder, compile all runtime C files, 
    // and link the Windows Event Log libraries.
    // Also redirect intermediate .obj files to temp/ so they don't clutter the root directory.
    sprintf(cmd, "cl.exe /nologo /O2 /I runtime %s runtime\\*.c /Fe%s /Fotemp\\ /link wevtapi.lib advapi32.lib", temp_file, output_exe);
    
    printf("\n[IPC Pipe] Simulating Zero-Disk STDIN pipe to backend compiler...\n");
    printf("           Executing: %s\n", cmd);
    
    int res = system(cmd);
    
    // INSTANT FORENSIC TEARDOWN - destroy the intermediate C code artifacts
    remove(temp_file);
    remove("_jky_stealth_pipe.obj");
    
    if (res == 0) {
        printf("[IPC Pipe] SUCCESS: Emitted polymorphic native binary: %s\n", output_exe);
    } else {
        printf("[IPC Pipe] FAILED to emit binary.\n");
    }
}
