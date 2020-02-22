add_rules("mode.release", "mode.debug") 

target("kernel")
    on_link(function (target) 
        if target._SOURCEBATCHES["c++.build"].objectfiles==nil then
            print("Error:No c++ objects found.")
        elseif target._SOURCEBATCHES["asm.build"].objectfiles ==nil then 
            print("Error:No asm objects found.")
        else
            import("core.tool.linker")
            local cmdstr = linker.linkcmd("binary", "ld", {"a.o", "b.o", "c.o"}, target:targetfile(), {target = target})
            print(cmdstr)
        end

    end)

    set_kind("binary")
    set_languages("c17", "cxx2a")

    set_objectdir("$(buildir)/kernel")
    
    set_toolchain("cc","clang")
    set_toolchain("as","clang")
    set_toolchain("cxx","clang++")
    set_toolchain("ld","ld")

    add_includedirs("include")
    add_includedirs("/usr/include")
    add_files("kern/**.cc","kern/**.S")
    add_files("drivers/**.cc","drivers/**.S")

    add_cxflags("--target=x86_64-pc-none-elf")
    add_cxflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
    add_cxflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")
    add_cxflags("-std=c++2a","-fmodules",{force = true})

    add_cflags("--target=x86_64-pc-none-elf")
    add_cflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
    add_cflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")
    add_cflags("-std=c17",{force = true})

    add_cflags("--target=x86_64-pc-none-elf")
    add_cflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
    add_cflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")

    add_ldflags("-z max-page-size=0x1000","-no-pie","-nostdlib")