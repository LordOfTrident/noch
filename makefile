CFLAGS = -O2 -std=c99 -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations -g -I./

examples: utf8 json args colorer log common sv hashmap mathexpr

bin:
	mkdir -p bin

utf8: bin
	$(CC) examples/utf8/find.c $(CFLAGS) -o bin/utf8_find
	$(CC) examples/utf8/print.c $(CFLAGS) -o bin/utf8_print

json: bin
	$(CC) examples/json/read.c $(CFLAGS) -o bin/json_read
	$(CC) examples/json/write.c $(CFLAGS) -o bin/json_write

args: bin
	$(CC) examples/args/line.c $(CFLAGS) -o bin/args_line

colorer: bin
	$(CC) examples/colorer/colors.c $(CFLAGS) -o bin/colorer

log: bin
	$(CC) examples/log/log.c $(CFLAGS) -o bin/log

common: bin
	$(CC) examples/common/common.c $(CFLAGS) -o bin/common
	$(CC) examples/common/print_floats.c $(CFLAGS) -o bin/print_floats

sv: bin
	$(CC) examples/sv/find.c $(CFLAGS) -o bin/sv_find
	$(CC) examples/sv/trim.c $(CFLAGS) -o bin/sv_trim

hashmap: bin
	$(CC) examples/hashmap/hashmap.c $(CFLAGS) -o bin/hashmap

mathexpr: bin
	$(CC) examples/mathexpr/expr.c $(CFLAGS) -o bin/expr -lm

clean: bin
	rm bin/*

all:
	@echo examples, utf8, json, args, colorer, log, common, sv, hashmap, mathexpr, clean
