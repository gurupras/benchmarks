CC=arm-none-linux-gnueabi-gcc

all : arm

FILES = src/main.c src/cpu/cpu.c src/mem/mem.c src/include/common.c

NUMBERS := $(shell seq 1 32767)
arm :
	
	@echo "/* Auto generated file. DO NOT EDIT */"   > src/include/autoconf.h
	$(CC) $(FILES) -Isrc -lrt -static -o bench

host :
	@echo "/* Auto generated file. DO NOT EDIT */" >src/include/autoconf.h
	gcc $(FILES) -Isrc -lrt -static -g -o bench
	
clean :
	rm -rf *.o
	rm bench