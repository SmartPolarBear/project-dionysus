ifndef TOP_SRC
TOP_SRC = .
endif

CONFIG=$(TOP_SRC)/config
BUILD_CONFIG=$(CONFIG)/build
CODEGEN_CONFIG=$(CONFIG)/codegen

BUILD = $(TOP_SRC)/build
MOUNTPOINT=$(BUILD)/mount/
INCLUDE = $(TOP_SRC)/include


QEMU = qemu-system-x86_64
QEMU_EXE = $(QEMU).exe


GDBPORT = 32678
QEMUGDB = -gdb tcp::$(GDBPORT)

CPUS = 4

QEMUOPTS = -no-reboot -vga std -drive file=$(BUILD)/disk.img,index=0,media=disk,format=raw,id=A1,if=none \
        -device nvme,drive=A1,serial=1234
QEMUOPTS += -d int
QEMUOPTS += -smp $(CPUS) -m 6G $(QEMUEXTRA)

VBOX_MACHINENAME = Test
VBOXMANAGE = VBoxManage.exe
VBOXMANAGE_FALGS = startvm --putenv VBOX_GUI_DBG_ENABLED=true 
