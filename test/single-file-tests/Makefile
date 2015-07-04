# thank you SO http://stackoverflow.com/questions/14589614/make-recursive-all-c-files


SHELL := /bin/bash
PATH := $(PWD)/env:$(PATH)

CCCLANG = riscv-clang
CC = export PATH=$(RISCV)/bin:$(PATH) && riscv64-unknown-linux-gnu-gcc
NATIVECC = gcc

SOURCES=$(shell find . -type f -iname '*.c')
OBJECTS=$(foreach x, $(basename $(SOURCES)), $(x).o)
BINARIES=$(foreach x, $(basename $(SOURCES)), $(x))
LINUX_BINARIES=$(foreach x, $(basename $(SOURCES)), $(x)-linux)

#TARGET=bin

all: $(BINARIES)

#$(TARGET) : $(BINARIES)
#	$(CC) $^ -o $@

linux: $(LINUX_BINARIES)

%-linux: %.c
	mkdir -p bin/riscv/`dirname $*`
	$(CC) $(CFLAGS) $^ -o bin/riscv/$*

%: %.c
	mkdir -p bin/riscv/`dirname $@`
	mkdir -p bin/riscv-clang/`dirname $@`
	mkdir -p bin/native/`dirname $@`
	$(CC) $(CFLAGS) $^ -o bin/riscv/$@
	$(CCCLANG) $(CFLAGS) $^ -o bin/riscv-clang/$@
	$(NATIVECC) $(CFLAGS) $^ -o bin/native/$@

clean:
	rm -rf bin/
	rm -f $(BINARIES)


# CFLAGS = -c -Wall -g -Os
# LD = $(CC)
# #LDFLAGS = -lfoo

# TARGET = bin

# SOURCES=$(shell find . -type f -iname '*.c')
# OBJECTS=$(foreach x, $(basename $(SOURCES)), $(x).o)

# all: $(TARGET)

# $(TARGET): $(OBJECTS)
# 	$(LD) -o $@ $^ $(LDFLAGS)

# # You don't even need to be explicit here,
# # compiling C files is handled automagically by Make.
# %.o: %.c
# 	$(CC) $(CFLAGS) $^ -o $@

# clean:
# 	rm $(TARGET) $(OBJECTS)
