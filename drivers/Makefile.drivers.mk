ifndef TOP_SRC
    TOP_SRC = ..
endif
include $(TOP_SRC)/Makefile.mk

_OBJS = $(addprefix $(OUTDIR)/,$(OBJS))

all:$(_OBJS)

clean:
	@for o in $(_OBJS); do echo "REMOVE" $$o; rm -f $$o; done
	@rm -f $(OUTDIR)/*.d
	@rm -f $(OUTDIR)/*.asm
	@rm -f $(OUTDIR)/*.out

.PHONY: all clean

include $(TOP_SRC)/Makefile.common.mk