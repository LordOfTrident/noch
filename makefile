CFLAGS = -O2 -std=c99 -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations -g -I./

examples: utf8 json args colorer log common

bin:
	mkdir -p bin

utf8: bin
	cc examples/utf8/find.c $(CFLAGS) -o bin/utf8_find
	cc examples/utf8/print.c $(CFLAGS) -o bin/utf8_print

json: bin
	cc examples/json/read.c $(CFLAGS) -o bin/json_read
	cc examples/json/write.c $(CFLAGS) -o bin/json_write

args: bin
	cc examples/args/line.c $(CFLAGS) -o bin/args_line

colorer: bin
	cc examples/colorer/colors.c $(CFLAGS) -o bin/colorer

log: bin
	cc examples/log/log.c $(CFLAGS) -o bin/log

common: bin
	cc examples/common/common.c $(CFLAGS) -o bin/common

clean: bin
	rm bin/*

all:
	@echo examples, utf8, json, args, colorer, log, common, clean
