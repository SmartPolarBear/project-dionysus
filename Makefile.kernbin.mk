
$(BUILD)/ap_boot: $(BUILD)/kern/init/ap_boot.o
	$(LD) $(LDFLAGS) --omagic -e start -Ttext 0x7000 -o $(BUILD)/ap_boot.o $^
	$(OBJCOPY) -S -O binary -j .text $(BUILD)/ap_boot.o $(BUILD)/ap_boot
	$(OBJDUMP) -S $(BUILD)/ap_boot.o > $(BUILD)/ap_boot.asm