TOP_SRC = .
include $(TOP_SRC)/Makefile.mk

SETS=$(TOP_SRC)/distrib/sets
SUBDIRS = tools kern

BASELIST = $(shell cat $(SETS)/base.list)
BASEOBJS = $(addprefix $(BUILD)/,$(BASELIST))

BINLIST = $(shell cat $(SETS)/bin.list)
BINOBJS =  $(addprefix $(BUILD)/bin/,$(BINLIST))

all: $(BUILD) $(SUBDIRS) $(BUILD)/kernel

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	@echo "[RM] $(BUILD)" 
	@rm -rf $(BUILD)

$(BUILD)/kernel: $(BASEOBJS)
	$(LD) -z max-page-size=0x1000 -no-pie -Tkern/kernel.ld -o $@ $^

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

$(BUILD): 
	@echo "[MKDIR] $(BUILD)" 
	@mkdir -p $@

.PHONY: all clean $(SUBDIRS) $(BUILD)