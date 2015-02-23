# thank you SO http://stackoverflow.com/questions/14589614/make-recursive-all-c-files

CC = riscv64-unknown-elf-gcc
NATIVECC = gcc

SOURCES=$(shell find . -type f -iname '*.c')
OBJECTS=$(foreach x, $(basename $(SOURCES)), $(x).o)
BINARIES=$(foreach x, $(basename $(SOURCES)), $(x))

#TARGET=bin

all: $(BINARIES)

#$(TARGET) : $(BINARIES)
#	$(CC) $^ -o $@

%: %.c
	mkdir -p bin/riscv/`dirname $@`
	mkdir -p bin/native/`dirname $@`
	$(CC) $(CFLAGS) $^ -o bin/riscv/$@
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
