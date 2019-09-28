TOP_SRC = ../..
KERN_SRCDIR= ..
include $(TOP_SRC)/Makefile.mk

OUTDIR=$(BUILD)/tools/diskimg
TARGETNAME=diskimg.py
TARGET=$(OUTDIR)/$(TARGETNAME)

OBJS = diskimg.py
_OBJS = $(addprefix $(OUTDIR)/,$(OBJS))

all: $(TARGET)

$(TARGET): $(_OBJS)

$(OUTDIR)/%.py: %.py | $(OUTDIR)
	@echo "[CP]" $< "->" $@
	@(cp -f $< $@)

clean: 
	@for o in $(_OBJS); do echo "REMOVE" $$o; rm -f $$o; done
	@for o in $(TARGET); do echo "REMOVE" $$o; rm -f $$o; done

$(OUTDIR):
	@echo "MKDIR" $@
	@mkdir -p $@

.PHONY: all clean

