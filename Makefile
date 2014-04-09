
MAKEFLAGS = --no-builtin-rules
# We don't use any suffix rules
.SUFFIXES :

LIBS     = -L. -lrt -lpthread

CC_OPTS = $(CFLAGS) $($*_CC_OPTS) -g 
CC =gcc

all : host

host : CROSS=
arm  : CROSS=arm-none-linux-gnueabi-
host arm : built-in.o cpu.o mem.o common.o bench.o
	$(addprefix $(CROSS), $(CC)) $(CFLAGS) -g -static -o bench built-in.o $(LIBS)
	@rm -rf *.o *.a

built-in.o : common.o cpu.o mem.o io.o bench.o
	$(addprefix $(CROSS), ld) -r $^ -o $@
	
bench.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/main.c -o $@
	
common.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -c src/include/common.c -o $@

cpu.o : 
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/cpu/cpu.c -o $@

#Memory benchmark files
mem.o : 
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/mem/mem.c -o $@

#IO benchmark files
io.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/io/io.c -o $@

clean :
	rm -rf *.o *.a bench
