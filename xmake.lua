add_rules("mode.debug","mode.release")	

target("kernel")	
    set_languages("c17", "cxx20")	
    set_kind("binary")	

    set_toolchain("cc","clang")	
    set_toolchain("as","clang")	
    set_toolchain("cxx","clang")	
    set_toolchain("ld","clang")	

    set_objectdir("$(buildir)/dionysus/kernel")	

    add_includedirs("include")	

    add_files("kern/**.cc")	
    add_files("drivers/**.cc","drivers/**.S")	
    add_files("kern/init/boot.S")	

    add_cxflags("--target=x86_64-pc-linux-elf")	
    add_cxflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")	
    add_cxflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")	
    add_cxflags("-std=c++2a")	

    add_asflags("--target=x86_64-pc-none-elf")	
    add_asflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")	
    add_asflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")	

    add_ldflags("-Wl,-T config/build/kernel.ld",{force = true})	
    add_ldflags("-z max-page-size=0x1000" ,"-no-pie" ,"-nostdlib" ,"-ffreestanding", "-nostartfiles" ,"-Wl,--build-id=none",{force = true})

target("ap_boot")
    set_languages("c17", "cxx20")	
    set_kind("binary")	

    set_toolchain("cc","clang")	
    set_toolchain("as","clang")	
    set_toolchain("cxx","clang")	
    set_toolchain("ld","clang")	

    set_objectdir("$(buildir)/dionysus/ap_boot")	

    add_includedirs("include")	

    add_files("kern/init/ap_boot.S")	

    add_cxflags("--target=x86_64-pc-linux-elf")	
    add_cxflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")	
    add_cxflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")	
    add_cxflags("-std=c++2a")	

    add_asflags("--target=x86_64-pc-none-elf")	
    add_asflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")	
    add_asflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")	

    add_ldflags("-Wl,-T config/build/ap_boot.ld",{force = true})	
    add_ldflags("-Wl,--omagic",{force=true})
    add_ldflags("-z max-page-size=0x1000" ,"-no-pie" ,"-nostdlib" ,"-ffreestanding", "-nostartfiles" ,"-Wl,--build-id=none",{force = true})