ifndef TOP_SRC
TOP_SRC = .
endif

BUILD = $(TOP_SRC)/build
MOUNTPOINT=$(BUILD)/mount/
INCLUDE = $(TOP_SRC)/include

TOOLPREFIX = 

HOST_CXX = clang++
HOST_CC = clang

CC = $(TOOLPREFIX)clang
CXX = $(TOOLPREFIX)clang++
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

LDFLAGS=-z max-page-size=0x1000 -no-pie -nostdlib -Tkern/kernel.ld 

SHAREDFLAGS = -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra \
 -Werror -I$(TOP_SRC)/include -MMD -mno-red-zone -mcmodel=kernel -fno-pie

CFLAGS = -std=gnu17 $(SHAREDFLAGS)
ASFLAGS = $(SHAREDFLAGS)
CXXFLAGS = -std=gnu++17 $(SHAREDFLAGS)
