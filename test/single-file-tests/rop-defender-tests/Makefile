# thank you SO http://stackoverflow.com/questions/14589614/make-recursive-all-c-files

CXX = riscv64-unknown-elf-g++
CXXLINUX = riscv64-unknown-linux-gnu-g++
NATIVECXX = g++

SOURCES=$(shell find . -type f -iname '*.cpp')
OBJECTS=$(foreach x, $(basename $(SOURCES)), $(x).o)
BINARIES=$(foreach x, $(basename $(SOURCES)), $(x))

#TARGET=bin

all: $(BINARIES)

#$(TARGET) : $(BINARIES)
#	$(CC) $^ -o $@

%: %.cpp
	mkdir -p ../bin/riscv/cpp
	mkdir -p ../bin/riscv-linux/cpp
	mkdir -p ../bin/native/cpp
	$(CXX) $(CXXFLAGS) $^ -o ../bin/riscv/cpp/$@
	$(CXXLINUX) $(CXXFLAGS) $^ -o ../bin/riscv-linux/cpp/$@
	$(NATIVECXX) $(CXXFLAGS) $^ -o ../bin/native/cpp/$@

clean:
	rm -rf ../bin/*/cpp
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
