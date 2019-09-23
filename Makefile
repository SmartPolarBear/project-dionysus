TOP_SRC = .
include $(TOP_SRC)/Makefile.mk

SETSDIR=$(TOP_SRC)/distrib/sets
SUBDIRS = tools kern

BASEBINLIST = $(shell cat $(SETSDIR)/base-bin.list)
BASEBINOBJS = $(addprefix $(BUILDDIR)/,$(BASEBINLIST))

BINLIST = $(shell cat $(SETSDIR)/bin.list)
BINOBJS =  $(addprefix $(BUILDDIR)/bin/,$(BINLIST))

all: $(BUILD) $(SUBDIRS)

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	@echo "[RM] $(BUILD)" 
	@rm -rf $(BUILD)

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

$(BUILD): 
	@echo "[MKDIR] $(BUILD)" 
	@mkdir -p $@

.PHONY: all clean $(SUBDIRS) $(BUILD)