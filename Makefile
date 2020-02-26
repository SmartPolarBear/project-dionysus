TOP_SRC=.
include $(TOP_SRC)/Makefile.mk

qemu: #all 
	$(QEMU_EXE) -serial mon:stdio $(QEMUOPTS)

qemu-whpx: all 
	$(QEMU_EXE) -serial mon:stdio -accel whpx $(QEMUOPTS)

vbox: all $(BUILD)/disk.qcow2
	$(VBOXMANAGE) $(VBOXMANAGE_FALGS) $(VBOX_MACHINENAME)

debug: all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &
	@sleep 2
	@$(GDB) -q -x ./gdbinit

debug4vsc: #all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &

.PHONY: qemu qemu-whpx debug debug4vsc