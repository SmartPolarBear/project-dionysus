TOP_SRC=.
include $(TOP_SRC)/Makefile.mk

qemu: #all 
	$(QEMU_EXE) -serial mon:stdio -cpu Icelake-Client-v2 $(QEMUOPTS)

qemu-kvm: #all
	$(QEMU) -serial mon:stdio --enable-kvm -cpu host $(QEMUOPTS)

qemu-whpx: #all
	$(QEMU_EXE) -serial mon:stdio -accel whpx -cpu Icelake-Client-v2 $(QEMUOPTS)

idedebug: #all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -cpu Icelake-Client-v2 -S $(QEMUGDB) &

idedebug-kvm: #all
	@$(QEMU) -serial mon:stdio --enable-kvm -cpu host $(QEMUOPTS) -S $(QEMUGDB) &

vbox: #all $(BUILD)/disk.qcow2
	$(VBOXMANAGE) $(VBOXMANAGE_FALGS) $(VBOX_MACHINENAME)


.PHONY: qemu qemu-kvm qemu-whpx idedebug idedebug-kvm