TOP_SRC = .
include $(TOP_SRC)/Makefile.mk

SETS=$(TOP_SRC)/distrib/sets
SUBDIRS = tools kern

BUILD=./build
BASELIST = $(shell cat $(SETS)/base.list)
BASEOBJS = $(addprefix $(BUILD)/,$(BASELIST))

BINLIST = $(shell cat $(SETS)/bin.list)
BINOBJS =  $(addprefix $(BUILD)/bin/,$(BINLIST))

all: $(BUILD) $(SUBDIRS) $(BUILD)/kernel $(BUILD)/disk.img

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	@echo "[RM] $(BUILD)" 
	@rm -rf $(BUILD)

$(BUILD)/kernel: $(BASEOBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	$(OBJDUMP) -S $@ > $(BUILD)/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernel.sym


$(BUILD)/disk.img: $(TOP_SRC)/disk.img $(BUILD)/kernel $(BUILD)/tools/diskimg/diskimg.py
	python3 $(BUILD)/tools/diskimg/diskimg.py $(TOP_SRC) $(SETS)/hdimage.list

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

$(BUILD): 
	@echo "[MKDIR] $(BUILD)" 
	@mkdir -p $@

$(MOUNTPOINT): 
	@echo "[MKDIR] $(MOUNTPOINT)" 
	@mkdir -p $@

.PHONY: all clean $(SUBDIRS) $(BUILD) $(MOUNTPOINT)