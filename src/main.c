#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "codegen.h"

// Reads an entire file into memory
static char* read_file(const char* path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file \"%s\".\n", path);
        exit(1);
    }
    
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Not enough memory to read \"%s\".\n", path);
        exit(1);
    }
    
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        printf("JOCKY v0.1 Compiler\n");
        printf("Usage: jky run <filename.jky>\n");
        return 1;
    }
    
    const char *file_path = argv[2];
    
    clock_t compile_start = clock();
    
    char *source = read_file(file_path);
    
    printf("[JOCKY] Compiling %s...\n", file_path);

    JkyLexer lexer;
    jky_lexer_init(&lexer, source);
    
    JkyArena arena;
    jky_arena_init(&arena, 4 * 1024 * 1024); 

    JkyParser parser;
    jky_parser_init(&parser, &lexer, &arena);
    JkyAstNode *ast = jky_parse(&parser);
    
    if (parser.had_error) {
        jky_arena_free(&arena);
        free(source);
        return 1;
    }

    JkySema sema;
    jky_sema_init(&sema, &arena);
    
    // Suppress semantic debug output for clean CLI experience
    // visit_node_pass1 and pass2 will still print some debug in our v0.1 hack, 
    // but we can live with that for now.
    jky_sema_analyze(&sema, ast);
    
    if (sema.had_error) {
        jky_arena_free(&arena);
        free(source);
        return 1;
    }
    
    JkyBuffer buf;
    jky_buffer_init(&buf);
    jky_codegen_generate(&buf, ast);
    
    const char *out_exe = "agent_v01.exe";
    jky_codegen_compile_and_pipe(&buf, out_exe);
    
    jky_buffer_free(&buf);
    jky_arena_free(&arena);
    free(source);
    
    clock_t compile_end = clock();
    double compile_time = (double)(compile_end - compile_start) / CLOCKS_PER_SEC;
    
    printf("\n[JOCKY] Executing %s...\n", out_exe);
    printf("----------------------------------------\n");
    fflush(stdout); // Flush buffers before child process takes over stdout
    
    // Run the generated binary
    system(out_exe);
    
    clock_t run_end = clock();
    double run_time = (double)(run_end - compile_end) / CLOCKS_PER_SEC;
    
    printf("----------------------------------------\n");
    printf("[JOCKY] Compile time: %.3f seconds\n", compile_time);
    printf("[JOCKY] Run time:     %.3f seconds\n", run_time);
    return 0;
}
