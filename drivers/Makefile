TOP_SRC=..
include $(TOP_SRC)/Makefile.mk

SUBDIRS = console debug acpi apic lock

all: prepare $(SUBDIRS)

prepare:
	python3 $(BUILD)/tools/vectors/gvectors.py apic/vectors.S

clean: 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	rm -f apic/vectors.S

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

.PHONY: all clean prepare $(SUBDIRS)