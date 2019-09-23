ifndef TOP_SRC
TOP_SRC = .
endif

BUILD=$(TOP_SRC)/build
INCLUDE=$(TOP_SRC)/include

TOOLPREFIX=
OUTDIR=

CC=$(TOOLPREFIX)clang
CXX=$(TOOLPREFIX)clang++
LD = $(TOOLPREFIX)ld.lld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

AS=$(TOOLPREFIX)gcc

