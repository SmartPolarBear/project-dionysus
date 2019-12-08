TOP_SRC = ../..
KERN_SRCDIR= ..
include $(TOP_SRC)/Makefile.mk

OUTDIR=$(BUILD)/tools/vectors
TARGETNAME=gvectors.py
TARGET=$(OUTDIR)/$(TARGETNAME)

OBJS = gvectors.py
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

