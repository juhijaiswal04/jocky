CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -Icompiler -Iruntime
LDFLAGS =

COMPILER_SRCS = src/arena.c src/lexer.c src/ast.c \
                src/parser.c src/symtable.c src/type.c \
                src/error.c src/sema.c src/codegen.c \
                src/stealth.c src/main.c

RUNTIME_SRCS  = runtime/jky_value.c runtime/jky_gc.c runtime/jky_host.c \
                runtime/jky_process.c runtime/jky_network.c runtime/jky_fs.c \
                runtime/jky_evidence.c runtime/jky_report.c \
                runtime/jky_crypto.c runtime/jky_log.c runtime/jky_print.c

all: jky runtime/libjocky.a

jky: $(COMPILER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

runtime/libjocky.a: $(RUNTIME_SRCS:.c=.o)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

debug: CFLAGS += -g -fsanitize=address,undefined
debug: all

clean:
	rm -f jky runtime/libjocky.a runtime/*.o compiler/*.o

test: all
	./jky compile examples/hello.jky -o examples/hello --debug
	./examples/hello

install: jky
	cp jky /usr/local/bin/jky

.PHONY: all debug clean test install
