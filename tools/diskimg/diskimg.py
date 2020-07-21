import subprocess
import sys
import os.path
import json
import fileinput
from enum import Enum


class MountAction(Enum):
    MOUNT = 1,
    UMOUNT = 2,


def getcmd_cp(src: str, dest: str, force: bool = False, sudo: bool = True, sanitycheck: bool = True):
    if sanitycheck:
        if os.path.exists(src) != True:
            print("Error in getcmd_cp: " + src + " isn't exist.")
            exit(-1)

    return ("sudo " if sudo else "") + "cp " + ("-f " if force else "") + src + " " + dest


def getcmd_losetup(loname, target, sudo: bool = True, offset: int = 0):
    return ("sudo " if sudo else "") + "losetup " + (
        "-o " + str(offset) if offset != 0 else "") + " " + loname + " " + target


def getcmd_delosetup(loname, sudo: bool = True):
    return ("sudo " if sudo else "") + "losetup -d " + loname


def getcmd_mount(act: MountAction, mountpoint, dev="", sudo: bool = True):
    if dev == "" and act == MountAction.MOUNT:
        print("Error in getcmd_mount: dev can't be empty if doing a mount")
        exit(-1)

    return ("sudo " if sudo else "") + ("mount " if act == MountAction.MOUNT else "umount ") + dev + " " + mountpoint


def get_argidx(base, offset):
    return base + offset


argidx_st: int = 2


def func_process_file_mappings(conf_name: str, builddir: str, mountpoint: str):
    with open(conf_name) as conf_f:
        json_obj: list = json.load(conf_f)
        mappings: list = json_obj['file_mappings']
        for mapping in mappings:
            mapping_from: str = builddir + "/" + str(mapping['from'])
            mapping_to: str = mountpoint + "/" + str(mapping['to'])
            print("File mapping: " + "[build_dir]/" + str(mapping['from']) + " -> " + "[target_dir]/" + str(
                mapping['to']) + ":")
            shell_cmd = getcmd_cp(mapping_from, mapping_to, True)
            subprocess.run(shell_cmd, check=True, shell=True)
            print("     DONE: " + shell_cmd)
    return


# fname: filelist name
def func_update_diskimage(main_argv):
    if os.path.exists(main_argv[get_argidx(argidx_st, 1)]) != True:
        print("File list does not exist.")
        exit(-1)

    currentdir = os.getcwd()
    builddir = currentdir + "/build"
    diskimgtemplate = currentdir + "/disk.img"
    diskimgtarget = builddir + "/disk.img"
    # qcow2target = builddir+"/disk.qcow2"
    mountpoint = builddir + "/mountpoint"

    if os.path.exists(mountpoint) != True:
        subprocess.run(["mkdir", mountpoint])

    if os.path.exists(builddir) != True:
        print("Error: No build dir.")
        exit(-1)

    if os.path.exists(diskimgtemplate) != True:
        print("Error: No disk.img.")
        exit(-1)

    if os.path.exists(diskimgtarget) != True:
        subprocess.run(getcmd_cp(src=diskimgtemplate,
                                 dest=diskimgtarget, force=False), check=True, shell=True)
        print("COMPLETE: " + getcmd_cp(src=diskimgtemplate,
                                       dest=diskimgtarget, force=False))

    lo0name = subprocess.getoutput("losetup -f")
    setuplo0_cmd = getcmd_losetup(lo0name, diskimgtarget)
    subprocess.run(setuplo0_cmd, check=True, shell=True)
    print("COMPLETE: " + setuplo0_cmd)

    lo1name = subprocess.getoutput("losetup -f")
    setuplo1_cmd = getcmd_losetup(lo1name, lo0name, offset=1048576)
    subprocess.run(setuplo1_cmd, check=True, shell=True)
    print("COMPLETE: " + setuplo1_cmd)

    mount_cmd = getcmd_mount(MountAction.MOUNT, mountpoint, lo1name)

    subprocess.run(mount_cmd, check=True, shell=True)
    print("COMPLETE: " + mount_cmd)

    config_file_name: str = main_argv[get_argidx(argidx_st, 1)]
    func_process_file_mappings(config_file_name, builddir, mountpoint)

    cpcfg_cmd = getcmd_cp("grub.cfg", mountpoint +
                          "/boot/grub/grub.cfg", force=True)
    subprocess.run(cpcfg_cmd, check=True, shell=True)
    print("COMPLETE: " + cpcfg_cmd)

    umount_cmd = getcmd_mount(MountAction.UMOUNT, mountpoint=mountpoint)
    subprocess.run(umount_cmd, check=True, shell=True)
    print("COMPLETE: " + umount_cmd)

    delosetuplo1_cmd = getcmd_delosetup(lo1name)
    subprocess.run(delosetuplo1_cmd, check=True, shell=True)
    print("COMPLETE: " + delosetuplo1_cmd)

    delosetuplo0_cmd = getcmd_delosetup(lo0name)
    subprocess.run(delosetuplo0_cmd, check=True, shell=True)
    print("COMPLETE: " + delosetuplo0_cmd)

    print("Update all succeeded!")
    return


def func_convert_diskimage(main_argv):
    target_type: str = main_argv[get_argidx(argidx_st, 1)]
    src: str = main_argv[get_argidx(argidx_st, 2)]
    dest: str = main_argv[get_argidx(argidx_st, 3)]

    if any({target_type == "qcow2", target_type == "vmdk", target_type == "vhd"}):
        print("Convert from raw to " + target_type)
    else:
        print("Target type should only be qcow2 and vmdk, but " +
              target_type + "is given.")
        exit(-1)

    qemuimg_cmd = "qemu-img convert -f raw -O " + target_type + " " + src + " " + dest
    subprocess.run(qemuimg_cmd, check=True, shell=True)
    print("COMPLETE: " + qemuimg_cmd)

    print("Convert all succeeded!")
    return


def func_test_prog(main_argv):
    # config_file_name: str = main_argv[get_argidx(argidx_st, 1)]
    # func_process_file_mappings(config_file_name)
    return


def main(argv):
    funcname: str = argv[get_argidx(argidx_st, -1)]

    if (funcname != "update" and funcname != "convert" and funcname != "test"):
        print("Only update and convert and test is available, but " + funcname + " is given.")
        exit(-1)

    if os.path.exists(argv[get_argidx(argidx_st, 0)]) != True:
        print("Work dir does not exist.")
        exit(-1)

    print("Working dir:" + argv[get_argidx(argidx_st, 0)])

    os.chdir(argv[get_argidx(argidx_st, 0)])

    if funcname == "update":
        func_update_diskimage(argv)
        exit(0)

    if funcname == "convert":
        func_convert_diskimage(argv)
        exit(0)

    if funcname == "test":
        func_test_prog(argv)
        exit(0)

    pass


if __name__ == "__main__":
    main(sys.argv)
