add_rules("mode.debug","mode.release")	

set_config("cc", "clang")
set_config("as", "clang")
set_config("cxx", "clang++")
set_config("ld", "clang++")

target("disk.img")
    add_deps("kernel","ap_boot.elf")
    set_objectdir("$(buildir)")

    set_default(true)

    on_run(function (target) 
        os.execv("qemu-system-x86_64",
            {"-serial","mon:stdio",
            "-drive","file="..val("buildir").."/disk.img"..",index=0,media=disk,format=raw",
            "-cpu","max",
            "-smp","6",
            "-m","8G"})
    end)

    on_build(function (target) 
        os.execv("python3",{val("projectdir").."/tools/diskimg/diskimg.py","update",val("projectdir"),val("projectdir").."/config/build/hdimage.list"})
    end)



-- this target builds the kernel
target("kernel")
    on_run(function (target) 
    end)

    set_languages("c17", "cxx20")	
    set_kind("binary")	

    before_build(function (target)
        os.execv("python3",{val("projectdir").."/tools/vectors/gvectors.py",val("projectdir").."/config/codegen/gvectors/gvectors.json",val("projectdir").."/drivers/apic/vectors.S"})
    end)

    after_link(function (target) 
        os.execv("cp",{target:targetfile(),"build/kernel"})
    end)

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

-- this finally output ap_boot
target("ap_boot.elf")
    on_run(function (target) 
    end)

    set_languages("c17", "cxx20")	
    set_kind("binary")	

    after_link(function (target) 
        os.execv("objcopy",{"-S","-O","binary","-j",".text",target:targetfile(),"build/ap_boot"})
    end)

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

