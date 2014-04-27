PROG_NAME=bench
MAKEFLAGS = --no-builtin-rules
# We don't use any suffix rules
.SUFFIXES :

LIBS     = -L. -lrt

MODULES=common cpu mem io microbench tuninglibrary
SOURCE_DIRS=$(addprefix src/, $(MODULES))
BUILD_DIR=build/
INCLUDE=src/include

vpath %.h $(INCLUDE)
vpath %.c $(SOURCE_DIRS)
VPATH=src:$(SOURCE_DIRS)

CFLAGS=-Isrc
CC_OPTS = $(CFLAGS) $($*_CC_OPTS) -Wall -g -static
CC =gcc

all : host

host    : CROSS=
arm lib : CROSS=arm-none-linux-gnueabi-
arm     : CFLAGS+= -D ARM
lib     : CFLAGS+= -fpic

host arm : tuninglibrary.o built-in.o main.o
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -g -o $(PROG_NAME) $^ $(LIBS)

	
built-in.o : common.o perf.o cpu.o mem.o io.o micro_benchmark.o
	$(addprefix $(CROSS), ld) -r $^ -o $@

tuninglibrary.o : tuning_library.o power_model.o
	$(addprefix $(CROSS), ld) -r $^ -o $@

power_model.o : power_model_cpu.o power_model_mem.o
	$(addprefix $(CROSS), ld) -r $^ -o $@
	

lib : libtuning.a
libtuning.a : power_model_cpu.o power_model_mem.o tuning_library.o
	$(addprefix $(CROSS), ar) rcs $@ $^
	

%.o : %.c
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -c $< -o $@


clean :
	rm -rf *.o *.so *.a $(PROG_NAME)
