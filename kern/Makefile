TOP_SRC=..
include $(TOP_SRC)/Makefile.mk

SUBDIRS = init lib mm

all: $(SUBDIRS)

clean: 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

.PHONY: all clean $(SUBDIRS)