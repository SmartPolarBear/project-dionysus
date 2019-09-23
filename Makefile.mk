ifndef TOP_SRC
TOP_SRC = .
endif

BUILD = $(TOP_SRC)/build
INCLUDE = $(TOP_SRC)/include

TOOLPREFIX =
OUTDIR =

CC = $(TOOLPREFIX)clang
CXX = $(TOOLPREFIX)clang++
LD = $(TOOLPREFIX)ld.lld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

SHAREDFLAGS = -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra \
 -Werror -I. -MMD -mno-red-zone -mcmodel=kernel -fno-pie

CFLAGS = -std=gnu17 $(SHAREDFLAGS)
ASFLAGS = $(SHAREDFLAGS) -Wa,--divide
CXXFLAGS = -std=gnu++17 $(SHAREDFLAGS)