CC=arm-none-linux-gnueabi-gcc

all : arm

FILES = src/main.c src/cpu/cpu.c src/mem/mem.c src/include/common.c

NUMBERS := $(shell seq 1 32767)
arm :
	
	@echo "/* Auto generated file. DO NOT EDIT */"   > src/include/autoconf.h
	@echo "#define ICACHE_SIZE 32 * 1024ULL" 		>> src/include/autoconf.h
	@echo "#define DCACHE_SIZE 32 * 1024ULL" 		>> src/include/autoconf.h
	
	@echo "#define ENSURE_ICACHE_MISS \\"				>> src/include/autoconf.h
	@echo "    { \\"										>> src/include/autoconf.h
	@echo "        int val; \\"								>> src/include/autoconf.h
	@echo "        asm(\"mrc p15, 0, %0, c1, c0, 0\" : \"=r\"(val)); \\"	>> src/include/autoconf.h
	@echo "        val &= 0xFFFFEFFB; \\"		>> src/include/autoconf.h
	
	@echo "    }"										>> src/include/autoconf.h
    
	$(CC) $(FILES) -Isrc -lrt -static -o bench

host :
	@echo "/* Auto generated file. DO NOT EDIT */" >src/include/autoconf.h
	@echo "#define ICACHE_SIZE 32 * 1024ULL" 		>> src/include/autoconf.h
	@echo "#define DCACHE_SIZE 8 * 1024 * 1024ULL" 	>> src/include/autoconf.h

	@echo '#define ENSURE_ICACHE_MISS \\'		>> src/include/autoconf.h
	
	@for i in ${NUMBERS} ; do echo "asm(\"nop\"); \\"			>> src/include/autoconf.h ; done ;
	@echo "asm(\"nop\"); "			>> src/include/autoconf.h
    
	gcc $(FILES) -Isrc -lrt -static -g -o bench
	
clean :
	rm -rf *.o
	rm bench