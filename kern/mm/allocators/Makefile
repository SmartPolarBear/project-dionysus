TOP_SRC = ../../..

SUBDIRS = buddy slab

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

clean: 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

.PHONY: all $(SUBDIRS) clean