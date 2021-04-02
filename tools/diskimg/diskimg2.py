import subprocess
import os.path
import json
import argparse
import pathlib
import logging
import errno
import getpass
import datetime

import time


class LoopDevice:
    __lo_name: str = ""
    __offset: int = 0
    __target: str = ""

    __has_setup: bool = False

    def __init__(self, target: str, offset: int):
        self.__target = target
        self.__offset = offset
        pass

    def __enter__(self):
        self.__lo_name = subprocess.getoutput("losetup -f")
        offset_option = ("-o {}".format(self.__offset) if self.__offset != 0 else "")
        try:
            cmd: str = "sudo losetup {} {} {}".format(offset_option, self.__lo_name, self.__target)
            subprocess.run(cmd, check=True, shell=True)
            self.__has_setup = True
            logging.info("Setting up loop device {} with {}".format(self.__lo_name, self.__target))
            return self
        except subprocess.CalledProcessError as err:
            logging.critical(
                "Can't setup loop device {} with file {} at offset {}.".format(self.__lo_name, self.__target,
                                                                               self.__offset))
            raise
        except Exception as err:
            logging.critical("Exception {} says {}.".format(type(err), err))
            raise

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.__has_setup:
            try:
                subprocess.run("sudo losetup -d {}".format(self.__lo_name), check=True, shell=True)
                self.__has_setup = False
                logging.info("Detach loop device {} with {}".format(self.__lo_name, self.__target))
            except subprocess.CalledProcessError as err:
                logging.critical(
                    "can't remove loop device {} with file {} at offset {}.".format(self.__lo_name, self.__target,
                                                                                    self.__offset))
                raise
            except Exception as err:
                logging.critical("exception {} says {}.".format(type(err), err))
                raise

    @property
    def is_ready(self) -> bool:
        return self.__has_setup

    @property
    def name(self) -> str:
        return self.__lo_name


class MountPoint:
    __path: str
    __dev: str = ""
    __has_mounted: bool = False

    def __command(self):
        cmd: str = "mount" if not self.__has_mounted else "umount"
        return "sudo {} {} {}".format(cmd, self.__dev if not self.__has_mounted else "", self.__path)

    def __init__(self, dev: str, path: str):
        self.__dev = dev
        self.__path = path
        pass

    def __enter__(self):
        try:
            subprocess.run(self.__command(), check=True, shell=True)
            self.__has_mounted = True
            logging.info("Mount {} on {}".format(self.__dev, self.__path))
            return self
        except subprocess.CalledProcessError as e:
            logging.critical("Can't mount {} on {}".format(self.__dev, self.__path))
            raise
        except Exception as err:
            logging.critical("Exception {} says {}.".format(type(err), err))
            raise

    def __exit__(self, exc_type, exc_val, exc_tb):
        try:
            subprocess.run(self.__command(), check=True, shell=True)
            self.__has_mounted = False
            logging.info("Umount {} from {}".format(self.__dev, self.__path))
        except subprocess.CalledProcessError as e:
            logging.critical("Can't umount {} on {}".format(self.__dev, self.__path))
            raise
        except Exception as err:
            logging.critical("Exception {} says {}.".format(type(err), err))
            raise

    @property
    def is_ready(self) -> bool:
        return self.__has_mounted

    @property
    def path(self) -> str:
        return self.__path


class MappingItem:
    __from: str
    __to: str

    def __command(self) -> str:
        return "sudo cp -f {} {}".format(self.__from, self.__to)

    def __init__(self, _fr: str, _to: str, parent: str, mount: str):
        fr: str = os.path.join(parent, _fr)
        to: str = os.path.join(mount, _to)

        if not os.path.exists(fr):
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), fr)

        self.__from = fr
        self.__to = to

        pass

    def apply(self):
        try:
            cmd: str = self.__command()
            subprocess.run(cmd, check=True, shell=True)
            logging.info("Mapping {} -> {} :\n Run \"{}\"".format(self.__from, self.__to, cmd))
        except subprocess.CalledProcessError as ce:
            logging.critical("Cannot copy from {} to {}".format(self.__from, self.__to))
            raise ce
        except Exception as err:
            logging.critical("Exception {} says {}.".format(type(err), err))
            raise err
        pass


LOOP1_OFFSET: int = 1048576
JSON_FILE_MAPPINGS_NAME: str = 'file_mappings'
JSON_DIRECTORY_NAME: str = 'image_directories'


def parse_config(args, mp: MountPoint):
    with open(str(args.config[0]), "r") as conf:
        conf_dict: dict = json.load(conf)
        fail: int = 0
        success: int = 0

        file_mappings_list: dict = conf_dict[JSON_FILE_MAPPINGS_NAME]
        for item in (MappingItem(mapping['from'], mapping['to'], str(args.directory[0]), mp.path)
                     for mapping in file_mappings_list):
            try:
                item.apply()
                success += 1
            except Exception as e:
                fail += 1
                raise e

        logging.info("Parse config: {} failed, {} succeeded.".format(fail, success))


def sync_grub_configuration(args, mp: MountPoint):
    if not os.path.exists(os.path.join(mp.path, "boot")):
        os.mkdir(os.path.join(mp.path, "boot"))
        logging.info("Create directory {}".format(os.path.join(mp.path, "boot")))

    if not os.path.exists(os.path.join(mp.path, "boot/grub")):
        os.mkdir(os.path.join(mp.path, "boot/grub"))
        logging.info("Create directory {}".format(os.path.join(mp.path, "boot/grub")))

    grub_mp_path: str = os.path.join(mp.path, "boot/grub/grub.cfg")
    subprocess.run("sudo cp -f {} {}".format(str(args.grub[0]), grub_mp_path), check=True, shell=True)
    logging.info("Sync grub configuration from {} to {}".format(str(args.grub[0]), grub_mp_path))
    pass


def workaround_permission(args, mp: MountPoint):
    file_path: str = str(args.file[0])

    try:
        cmd: str = "sudo chown {} {}".format(getpass.getuser(), file_path)
        subprocess.run(cmd, check=True, shell=True)
        logging.info("Change permissions :\n Run \"{}\"".format(cmd))
    except subprocess.CalledProcessError as ce:
        logging.critical("Cannot change permission: {}".format(str(ce)))
        raise ce
    except Exception as err:
        logging.critical("Exception {} says {}.".format(type(err), err))
        raise err
    pass


def update_image(parser: argparse, args):
    with LoopDevice(str(args.file[0]), 0) as loop0:
        with LoopDevice(loop0.name, LOOP1_OFFSET) as loop1:
            assert loop0.is_ready
            assert loop1.is_ready

            with MountPoint(loop1.name, str(args.mount[0])) as mp:
                sync_grub_configuration(args, mp)
                parse_config(args, mp)
                workaround_permission(args, mp)

                logging.info("Finished at {}".format(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")))


def resolve_args_dependency(parser: argparse, args):
    file_path: str = str(args.file[0])

    if args.action == 'update':
        if any([args.config is None, args.grub is None, args.directory is None, args.mount is None]):
            parser.error("arguments are not enough for update.")

    if not os.path.exists(file_path):
        if args.template is None:
            parser.error("{} is not exist, so template must be present.".format(file_path))

        template_path: str = str(args.template[0])
        if not os.path.exists(template_path):
            parser.error("{} is not exist,  template {} also not exist.".format(file_path, template_path))

        try:
            subprocess.run("sudo cp {} {}".format(template_path, file_path), check=True, shell=True)
            logging.info("Create target {} from template {}".format(file_path, template_path))
        except subprocess.CalledProcessError as cpe:
            parser.error("Error creating target {} from template {} :\n  {}".format(file_path, template_path, str(cpe)))

    pass


def main():
    def validate_disk_file(parser: argparse, arg: str) -> str:
        path: pathlib.Path = pathlib.Path(arg)
        abs_file_path: str = str(path.absolute().as_posix())
        return abs_file_path

    def validate_disk_template_file(parser: argparse, arg: str) -> str:
        path: pathlib.Path = pathlib.Path(arg)
        if path.exists():
            return str(path.absolute().as_posix())
        else:
            raise argparse.ArgumentError("FATAL: path {} not exists".format(arg))

    def validate_config_file(parser: argparse, arg: str) -> str:
        path: pathlib.Path = pathlib.Path(arg)
        if path.exists():
            return str(path.absolute().as_posix())
        else:
            raise argparse.ArgumentError("FATAL: path {} not exists".format(arg))

    def validate_build_directory(parser: argparse, arg: str) -> str:
        path: pathlib.Path = pathlib.Path(arg)
        if path.exists():
            return str(path.absolute().as_posix())
        else:
            raise argparse.ArgumentError("FATAL: path {} not exists".format(arg))

    def validate_mount_point(parser: argparse, arg: str) -> str:
        path: pathlib.Path = pathlib.Path(arg)
        abs_path_str: str = str(path.absolute().as_posix())

        if not path.exists():
            os.mkdir(abs_path_str)
            logging.info("Creating {}, which does not exists.".format(abs_path_str))

        return abs_path_str

    parser: argparse = argparse.ArgumentParser(description="disk img manager for project-dionysus",
                                               epilog="This script requires sudo")

    parser.add_argument('action', choices=['update', 'convert'])

    parser.add_argument('-f', '--file', type=lambda x: validate_disk_file(parser, x), nargs=1,
                        help="the disk image file",
                        required=True)
    parser.add_argument('-t', '--template', type=lambda x: validate_disk_template_file(parser, x), nargs=1,
                        help="the disk image template if disk image doesn't exist")

    parser.add_argument('-c', '--config', type=lambda x: validate_config_file(parser, x), nargs=1,
                        help="the configuration file")

    parser.add_argument('-g', '--grub', type=lambda x: validate_config_file(parser, x), nargs=1,
                        help="the grub configuration file")

    parser.add_argument('-d', '--directory', type=lambda x: validate_build_directory(parser, x), nargs=1,
                        default=os.path.join(os.getcwd(), "build"),
                        help="the build directory")

    parser.add_argument('-m', '--mount', type=lambda x: validate_mount_point(parser, x), nargs=1,
                        default=os.path.join(os.path.join(os.getcwd(), "build"), "mountpoint"),
                        help="the mount point directory")

    args = parser.parse_args()

    resolve_args_dependency(parser, args)

    if args.action == 'update':
        update_image(parser, args)
    else:
        parser.error("Action {} is not yet implemented.".format(args.action[0]))


if __name__ == "__main__":
    # Logging to screen
    formatter = logging.Formatter('%(message)s')
    logging.getLogger('').setLevel(logging.DEBUG)
    main()
