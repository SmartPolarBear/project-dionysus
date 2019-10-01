ifndef TOP_SRC
TOP_SRC = .
endif

BUILD = $(TOP_SRC)/build
MOUNTPOINT=$(BUILD)/mount/
INCLUDE = $(TOP_SRC)/include

TOOLPREFIX = 

HOST_CXX = g++
HOST_CC = gcc

CC = $(TOOLPREFIX)clang
CXX = $(TOOLPREFIX)clang++
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

LDFLAGS=-z max-page-size=0x1000 -no-pie -nostdlib -Tkern/kernel.ld 

SHAREDFLAGS = -fno-builtin -O0 -nostdlib -ffreestanding -g -Wall -Wextra \
 -Werror -I$(TOP_SRC)/include -MMD -mno-red-zone -mcmodel=kernel -fno-pie

CFLAGS = -std=gnu17 $(SHAREDFLAGS)
ASFLAGS = $(SHAREDFLAGS)
CXXFLAGS = -std=gnu++17 $(SHAREDFLAGS)

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386
QEMU = qemu-system-x86_64.exe

# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

CPUS = 4

QEMUOPTS =  -drive file=$(BUILD)/disk.img,index=0,media=disk,format=raw -cpu max
QEMUOPTS += -smp $(CPUS) -m 4096 $(QEMUEXTRA)
#QEMUOPTS +=  -accel whpx

