import subprocess
import sys
import os.path
import fileinput
from enum import Enum


class MountAction(Enum):
    MOUNT = 1,
    UMOUNT = 2,


def getcmd_cp(target, dest, force: bool = False, sudo: bool = True, sanitycheck: bool = True):
    if sanitycheck:
        if os.path.exists(target) != True:
            print("Error in getcmd_cp: target isn't exist.")
            exit(-1)

    return ("sudo " if sudo else "") + "cp " + ("-f " if force else "") + target + " " + dest


def getcmd_losetup(loname, target, sudo: bool = True, offset: int = 0):
    return ("sudo " if sudo else "") + "losetup " + ("-o "+str(offset) if offset != 0 else "")+" " + loname + " " + target


def getcmd_delosetup(loname, sudo: bool = True):
    return ("sudo " if sudo else "") + "losetup -d " + loname


def getcmd_mount(act: MountAction, mountpoint, dev="", sudo: bool = True):
    if dev == "" and act == MountAction.MOUNT:
        print("Error in getcmd_mount: dev can't be empty if doing a mount")
        exit(-1)

    return ("sudo " if sudo else "")+("mount " if act == MountAction.MOUNT else "umount ")+dev+" "+mountpoint


def main(argv):
    if os.path.exists(argv[1]) != True:
        print("Work dir does not exist.")
        exit(-1)

    os.chdir(argv[1])

    if os.path.exists(argv[2]) != True:
        print("File list does not exist.")
        exit(-1)

    fname = argv[2]

    currentdir = os.getcwd()
    builddir = currentdir + "/build"
    diskimgtemplate = currentdir + "/disk.img"
    diskimgtarget = builddir + "/disk.img"
    qcow2target = builddir+"/disk.qcow2"
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
        subprocess.run(getcmd_cp(diskimgtarget, diskimgtemplate,
                                 False), check=True, shell=True)
        print("COMPLETE: "+getcmd_cp(diskimgtarget, diskimgtemplate,
                                     False))

    lo0name = subprocess.getoutput("losetup -f")
    setuplo0_cmd = getcmd_losetup(lo0name, diskimgtarget)
    subprocess.run(setuplo0_cmd, check=True, shell=True)
    print("COMPLETE: "+setuplo0_cmd)

    lo1name = subprocess.getoutput("losetup -f")
    setuplo1_cmd = getcmd_losetup(lo1name, lo0name, offset=1048576)
    subprocess.run(setuplo1_cmd, check=True, shell=True)
    print("COMPLETE: " + setuplo1_cmd)

    mount_cmd = getcmd_mount(MountAction.MOUNT, mountpoint, lo1name)

    subprocess.run(mount_cmd, check=True, shell=True)
    print("COMPLETE: " + mount_cmd)

    with open(fname) as f:
        for line in f:
            lhs, rhs = line.split(":")
            lhs = builddir + "/" + lhs
            rhs = mountpoint + "/" + rhs
            cpitem_cmd = getcmd_cp(lhs, rhs, True)
            subprocess.run(cpitem_cmd, check=True, shell=True)
            print("COMPLETE: " + cpitem_cmd)

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
    subprocess.run(delosetuplo0_cmd,check=True, shell=True)
    print("COMPLETE: " + delosetuplo0_cmd)

    print("All succeeded!")

    # print("Convert a qcow2 copy")
    # print("qemu-img convert -f raw -O qcow2 " +
    #       diskimgtarget + " " + qcow2target)
    # subprocess.run("qemu-img convert -f raw -O qcow2 " + diskimgtarget + " " + qcow2target,
    #                check=True, shell=True)

    pass


if __name__ == "__main__":
    main(sys.argv)
