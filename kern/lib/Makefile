TOP_SRCDIR = ../..

SUBDIRS = libc libcxxrt

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

clean: 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

.PHONY: all $(SUBDIRS) clean