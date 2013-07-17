
MAKEFLAGS = --no-builtin-rules
# We don't use any suffix rules
.SUFFIXES :

LIBS     = -L. -lrt

CC_OPTS = $(CFLAGS) $($*_CC_OPTS)
CC =gcc

all : host

host : CROSS=
arm  : CROSS=arm-none-linux-gnueabi-
host arm : built-in.o cpu.o mem.o common.o bench.o
	$(addprefix $(CROSS), $(CC)) $(CFLAGS) -static -o bench built-in.o $(LIBS)
	@rm -rf *.o *.a

built-in.o : cpu.o mem.o common.o bench.o
	$(addprefix $(CROSS), ld) -r $^ -o $@
	
bench.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/main.c -o $@

cpu.o : 
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/cpu/cpu.c -o $@

common.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -c src/include/common.c -o $@

mem.o : misc_lib.o mtest.o
	$(addprefix $(CROSS), ld) -r $^ -o $@

mtest.o :
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -Isrc -c src/mem/mtest.c -o $@

misc_lib.o : 
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -c src/mem/misc_lib.c -o $@

clean :
	rm -rf *.o *.a bench
