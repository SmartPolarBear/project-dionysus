TOP_SRC = ../..
KERN_SRCDIR= ..
include $(TOP_SRC)/Makefile.mk

OUTDIR=$(BUILD)/tools/diskimg
TARGETNAME=diskimg
TARGET=$(OUTDIR)/$(TARGETNAME)

OBJS = diskimg.o
_OBJS = $(addprefix $(OUTDIR)/,$(OBJS))

all: $(TARGET)

$(TARGET): $(_OBJS)
	@echo "[LINKER]" $^ "->" $@
	@$(HOST_CXX) $(_OBJS) -o $@

$(OUTDIR)/%.o: %.c | $(OUTDIR)
	@echo "[HOST CC]" $< "->" $@
	@($(HOST_CC) $< $(HOST_CFLAGS) -c -o $@)

$(OUTDIR)/%.o: %.cc | $(OUTDIR)
	@echo "[HOST CXX]" $< "->" $@
	@($(HOST_CXX) $< $(HOST_CXXFLAGS) -c -o $@)

clean: 
	@for o in $(_OBJS); do echo "REMOVE" $$o; rm -f $$o; done
	@for o in $(TARGET); do echo "REMOVE" $$o; rm -f $$o; done

$(OUTDIR):
	@echo "MKDIR" $@
	@mkdir -p $@

.PHONY: all clean

