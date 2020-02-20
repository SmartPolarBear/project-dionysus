ifndef TOP_SRC
TOP_SRC = .
endif

CONFIG=$(TOP_SRC)/config
BUILD_CONFIG=$(CONFIG)/build
CODEGEN_CONFIG=$(CONFIG)/codegen

BUILD = $(TOP_SRC)/build
MOUNTPOINT=$(BUILD)/mount/
INCLUDE = $(TOP_SRC)/include

TOOLPREFIX = 

HOST_CXX = clang++
HOST_CC = clang

PYTHON=python3
DISKIMG_PY=$(BUILD)/tools/diskimg/diskimg.py
GVECTORS_PY=$(BUILD)/tools/vectors/gvectors.py

CC = $(TOOLPREFIX)clang
CXX = $(TOOLPREFIX)clang++
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
GDB=gdb

LDFLAGS=-z max-page-size=0x1000 -no-pie -nostdlib

SHAREDFLAGS += --target=x86_64-pc-none-elf 
SHAREDFLAGS = -fno-pie -fno-exceptions -fno-rtti -ffreestanding -nostdlib -fno-builtin -gdwarf-2 -Wall -Wextra
SHAREDFLAGS += -march=x86-64 -mtls-direct-seg-refs -mno-sse -mcmodel=large -mno-red-zone -fmodules
SHAREDFLAGS += -I$(TOP_SRC)/include

CFLAGS = -std=c17 $(SHAREDFLAGS)
ASFLAGS = $(SHAREDFLAGS)
CXXFLAGS = -std=c++2a $(SHAREDFLAGS)

QEMU = qemu-system-x86_64
QEMU_EXE = $(QEMU).exe


GDBPORT = 32678
QEMUGDB = -gdb tcp::$(GDBPORT)

CPUS = 4

QEMUOPTS =  -drive file=$(BUILD)/disk.img,index=0,media=disk,format=raw -cpu max
#QEMUOPTS +=  -accel whpx
QEMUOPTS += -smp $(CPUS) -m 8G $(QEMUEXTRA)

VBOX_MACHINENAME = Test
VBOXMANAGE = VBoxManage.exe
VBOXMANAGE_FALGS = startvm --putenv VBOX_GUI_DBG_ENABLED=true 
