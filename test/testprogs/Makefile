# thank you SO http://stackoverflow.com/questions/14589614/make-recursive-all-c-files

CC = riscv64-unknown-elf-gcc
CFLAGS = -c -Wall -g -Os
LD = $(CC)
#LDFLAGS = -lfoo

TARGET = tests

SOURCES=$(shell find . -type f -iname '*.c')
OBJECTS=$(foreach x, $(basename $(SOURCES)), $(x).o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

# You don't even need to be explicit here,
# compiling C files is handled automagically by Make.
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm $(TARGET) $(OBJECTS)


