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
GDB=gdb

LDFLAGS=-z max-page-size=0x1000 -no-pie -nostdlib -Tkern/kernel.ld 

SHAREDFLAGS = -mno-sse -fno-exceptions -fno-rtti -ffreestanding -nostdlib -fno-builtin -Wall -Wextra -mcmodel=large -mno-red-zone
SHAREDFLAGS += -I$(TOP_SRC)/include
SHAREDFLAGS += --target=x86_64-none-elf -gdwarf-2  -fno-pie

CFLAGS = -std=c17 $(SHAREDFLAGS)
ASFLAGS = $(SHAREDFLAGS)
CXXFLAGS = -std=c++2a $(SHAREDFLAGS)

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386
QEMU = qemu-system-x86_64
#QEMU = qemu-system-x86_64.exe


GDBPORT = 32678
QEMUGDB = -gdb tcp::$(GDBPORT)

CPUS = 4

QEMUOPTS =  -drive file=$(BUILD)/disk.img,index=0,media=disk,format=raw -cpu max
#QEMUOPTS +=  -accel whpx
QEMUOPTS += -smp $(CPUS) -m 4G $(QEMUEXTRA)


VBOX_MACHINENAME = Test
VBOXMANAGE = VBoxManage.exe
VBOXMANAGE_FALGS = startvm --putenv VBOX_GUI_DBG_ENABLED=true 
