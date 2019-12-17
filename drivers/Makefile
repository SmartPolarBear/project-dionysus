TOP_SRC=..
include $(TOP_SRC)/Makefile.mk

SUBDIRS = console debug acpi apic lock apic_timer

all: prepare $(SUBDIRS)

prepare:
	$(PYTHON) $(GVECTORS_PY) $(CODEGEN_CONFIG)/gvectors/gvectors.json apic/vectors.S

clean: 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	rm -f apic/vectors.S

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

.PHONY: all clean prepare $(SUBDIRS)