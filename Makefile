TOP_SRC = .
include $(TOP_SRC)/Makefile.mk

SUBDIRS = tools kern drivers

BUILD=./build
BASELIST = $(shell cat $(BUILD_CONFIG)/base.list)
BASEOBJS = $(addprefix $(BUILD)/,$(BASELIST))

BINLIST = $(shell cat $(BUILD_CONFIG)/bin.list)
BINOBJS =  $(addprefix $(BUILD)/bin/,$(BINLIST))

all: $(BUILD) $(SUBDIRS) $(BUILD)/ap_boot $(BUILD)/kernel $(BUILD)/disk.img

BUILDDISTDIRS = $(shell ls -d $(BUILD)/*/)
clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	for dir in $(BUILDDISTDIRS); do rm -rf $$dir; done
	@rm -f $(BUILD)/kernel*
	@rm -f $(BUILD)/ap_boot*
	#@rm -rf $(BUILD)

qemu: all 
	$(QEMU_EXE) -serial mon:stdio $(QEMUOPTS)

qemu-whpx: all 
	$(QEMU_EXE) -serial mon:stdio -accel whpx $(QEMUOPTS)

vbox: all $(BUILD)/disk.qcow2
	$(VBOXMANAGE) $(VBOXMANAGE_FALGS) $(VBOX_MACHINENAME)

debug: all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &
	@sleep 2
	@$(GDB) -q -x ./gdbinit

debug4vsc: all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &


$(BUILD)/ap_boot: $(BUILD)/kern/init/ap_boot.o
	$(LD) $(LDFLAGS) --omagic -e start -Ttext 0x7000 -o $(BUILD)/ap_boot.o $^
	$(OBJCOPY) -S -O binary -j .text $(BUILD)/ap_boot.o $(BUILD)/ap_boot
	$(OBJDUMP) -S $(BUILD)/ap_boot.o > $(BUILD)/ap_boot.asm
	
$(BUILD)/kernel: $(BASEOBJS)
	$(LD) $(LDFLAGS) -Tkern/kernel.ld  -o $@ $^ -b binary $(BUILD)/ap_boot
	$(OBJDUMP) -S $@ > $(BUILD)/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernel.sym

$(BUILD)/disk.img: $(TOP_SRC)/disk.img $(BUILD)/kernel $(BUILD)/tools/diskimg/diskimg.py
	$(PYTHON) $(DISKIMG_PY) update $(TOP_SRC) $(BUILD_CONFIG)/hdimage.list

$(BUILD)/disk.qcow2: $(BUILD)/disk.img $(BUILD)/tools/diskimg/diskimg.py
	$(PYTHON) $(DISKIMG_PY) convert $(TOP_SRC) qcow2 $< $@

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

$(BUILD): 
	@echo "[MKDIR] $(BUILD)" 
	@mkdir -p $@

$(MOUNTPOINT): 
	@echo "[MKDIR] $(MOUNTPOINT)" 
	@mkdir -p $@

.PHONY: all clean qemu qemu-whpx debug debug4vsc $(SUBDIRS) $(BUILD) $(MOUNTPOINT)