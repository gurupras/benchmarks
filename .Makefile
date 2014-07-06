# DO NOT USE THIS AS YOUR PRIMARY MAKEFILE
# This file is used to generate your primary Makefile

PROG_NAME=bench
MAKEFLAGS = --no-builtin-rules
# We don't use any suffix rules
.SUFFIXES :

LIBS     = -lpatuning -lrt -lpthread

MODULES=common cpu mem io microbench annotation_test
SOURCE_DIRS=$(addprefix src/, $(MODULES))
BUILD_DIR=build/
INCLUDE=src/include

vpath %.h $(INCLUDE)
vpath %.c $(SOURCE_DIRS)
VPATH=src:$(SOURCE_DIRS)

TUNING_LIB_PATH=
CFLAGS=-Isrc -I$(TUNING_LIB_PATH)
LDFLAGS=-L$(TUNING_LIB_PATH) 
CC_OPTS = $(CFLAGS) $($*_CC_OPTS) $(LDFLAGS) -Wall -g -static
CC =gcc
LD=ld

all : host

host    : CROSS=
arm lib : CROSS=arm-none-linux-gnueabi-
arm     : CFLAGS+= -D ARM

host arm : common.o perf.o cpu.o mem.o io.o micro_benchmark.o annotation_test.o main.o
	cd $(TUNING_LIB_PATH) && CROSS=$(CROSS) make && cd -
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -g -o $(PROG_NAME) $^ $(LIBS)

%.o : %.c
	$(addprefix $(CROSS), $(CC)) $(CC_OPTS) -c $< -o $@


clean :
	rm -rf *.o *.so *.a $(PROG_NAME)
