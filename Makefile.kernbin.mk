
$(BUILD)/ap_boot: $(BUILD)/kern/init/ap_boot.o
	$(LD) $(LDFLAGS) -Wl,--omagic -Wl,-Tconfig/build/ap_boot.ld -o $(BUILD)/ap_boot.o $^
	#ld -z max-page-size=0x1000 -no-pie -nostdlib --omagic -e start -Ttext 0x7000 -o $(BUILD)/ap_boot2.o $^
	$(OBJCOPY) -S -O binary -j .text $(BUILD)/ap_boot.o $(BUILD)/ap_boot
	$(OBJDUMP) -S $(BUILD)/ap_boot.o > $(BUILD)/ap_boot.asm