import subprocess
import sys
import os.path
import fileinput


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
        subprocess.run("cp " + diskimgtemplate+" " +
                       diskimgtarget, check=True, shell=True)

    lo0 = subprocess.getoutput("losetup -f")
    subprocess.run("sudo losetup "+lo0+" " +
                   diskimgtarget, check=True, shell=True)
    print("sudo losetup "+lo0+" " +
          diskimgtarget)
    lo1 = subprocess.getoutput("losetup -f")
    subprocess.run("sudo losetup -o 1048576 " +
                   lo1 + " " + lo0, check=True, shell=True)
    print("sudo losetup -o 1048576 " +
          lo1+" "+lo0)
    subprocess.run("sudo mount " + lo1+" " +
                   mountpoint, check=True, shell=True)
    print("sudo mount " + lo1+" " +
          mountpoint)

    with open(fname) as f:
        for line in f:
            lhs, rhs = line.split(":")
            lhs = builddir + "/" + lhs
            rhs = mountpoint+"/"+rhs
            subprocess.run(
                "sudo cp -f "+lhs+" "+rhs, check=True, shell=True)
            print("sudo cp "+builddir+"/"+lhs+" "+mountpoint+"/"+rhs)

    subprocess.run("sudo umount"+" "+mountpoint, check=True, shell=True)
    print("sudo umount"+" "+mountpoint)
    subprocess.run("sudo losetup"+" " + "-d "+lo1,
                   check=True, shell=True)
    print("sudo losetup"+" " + "-d "+lo1)
    subprocess.run("sudo losetup"+" " + "-d "+lo0,
                   check=True, shell=True)
    print("sudo losetup"+" " + "-d "+lo0)
    print("Succeeded!")

    pass


if __name__ == "__main__":
    main(sys.argv)
