cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# FIXME: these modules is no longer used, or no longer built successfully
add_subdirectory(conf)
add_subdirectory(fs)
add_subdirectory(media)
add_subdirectory(posix)
add_subdirectory(userinit)
add_subdirectory(userinput)

# TODO: IRC listening method
# Severs: userinput(keyboard, mouse, etc), media (video,audio, etc), conf (system configuration), fs (file system and device manager), posix
# Load fs and config first, and then the others.
# Each sever write or load its own driver