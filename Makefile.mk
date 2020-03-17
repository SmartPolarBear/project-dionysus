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

CPUS = 2

QEMUOPTS = -no-reboot -drive file=$(BUILD)/disk.img,index=0,media=disk,format=raw -cpu max -d int,cpu_reset
#QEMUOPTS +=  -accel whpx
QEMUOPTS += -smp $(CPUS) -m 8G $(QEMUEXTRA)

VBOX_MACHINENAME = Test
VBOXMANAGE = VBoxManage.exe
VBOXMANAGE_FALGS = startvm --putenv VBOX_GUI_DBG_ENABLED=true 
