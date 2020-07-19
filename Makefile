TOP_SRC=.
include $(TOP_SRC)/Makefile.mk

qemu: #all 
	$(QEMU_EXE) -serial mon:stdio $(QEMUOPTS)

qemu-kvm: #all
	$(QEMU) -serial mon:stdio --enable-kvm $(QEMUOPTS)

qemu-whpx: #all
	$(QEMU_EXE) -serial mon:stdio -accel whpx $(QEMUOPTS)

idedebug: #all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &

idedebug-kvm: #all
	@$(QEMU) -serial mon:stdio --enable-kvm $(QEMUOPTS) -S $(QEMUGDB) &

vbox: #all $(BUILD)/disk.qcow2
	$(VBOXMANAGE) $(VBOXMANAGE_FALGS) $(VBOX_MACHINENAME)


.PHONY: qemu qemu-kvm qemu-whpx idedebug idedebug-kvm