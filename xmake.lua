add_rules("mode.debug","mode.release")

target("kernel")
    set_languages("c17", "cxx20")
    set_kind("binary")

    set_toolchain("cc","usr/bin/clang")
    set_toolchain("as","clang")
    set_toolchain("cxx","clang")
    set_toolchain("ld","clang")

    on_link(function (target) 
        if not os.exists("$(buildir)/bin") then
            os.mkdir("$(buildir)/bin")
        end

        local cpp_sourcebatch=target._SOURCEBATCHES["c++.build"]
        local asm_sourcebatch=target._SOURCEBATCHES["asm.build"]
        
        local link_args={"-z","max-page-size=0x1000" ,"-no-pie" ,"-nostdlib" ,"-ffreestanding", "-nostartfiles" ,"-Wl,--build-id=none","-Wl,-Tconfig/build/kernel.ld" ,"-o","build/bin/kernel"}
        table.insert(link_args,"build/dionysus/kernel/kernel/linux/x86_64/debug/kern/init/boot.S.o");
        for i, v in pairs(cpp_sourcebatch.objectfiles) do  
            table.insert(link_args,v) 
        end 
        
        for i, v in pairs(asm_sourcebatch.objectfiles) do  
            if(v~="build/dionysus/kernel/kernel/linux/x86_64/debug/kern/init/boot.S.o") then
                table.insert(link_args,v) 
            end 
        end
        -- print(link_args)
        os.execv("clang",link_args)
    end)

    -- on_build_file(function (target, sourcefile, opt)
    --     local build_args={"--target=x86_64-pc-none-elf","-fno-pie","-fno-exceptions" ,"-fno-rtti", "-ffreestanding", "-nostdlib" ,"-fno-builtin" ,"-gdwarf-2" ,"-Wall" ,"-Wextra","-march=x86-64", "-mtls-direct-seg-refs" ,"-mno-sse" ,"-mcmodel=large" ,"-mno-red-zone"}
    --     if not opt.configs then
    --         if not opt.configs.cxxflags then
    --             for i, v in pairs(opt.configs.cxxflags) do  
    --                 table.insert(build_args,v) 
    --             end 
    --         end
    --     end


    --     print(opt)
    -- end)
    
    set_objectdir("$(buildir)/dionysus/kernel")

    add_includedirs("include")

    add_files("kern/**.cc")
    add_files("drivers/**.cc","drivers/**.S")
    add_files("kern/init/boot.S")

    add_cxflags("--target=x86_64-pc-none-elf")
    add_cxflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
    add_cxflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")
    add_cxflags("-std=c++2a","-fmodules",{force = true})
    
    add_asflags("--target=x86_64-pc-none-elf")
    add_asflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
    add_asflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")

    add_ldflags("-Wl,-T config/build/kernel.ld",{force = true})
    add_ldflags("-z max-page-size=0x1000" ,"-no-pie" ,"-nostdlib" ,"-ffreestanding", "-nostartfiles" ,"-Wl,--build-id=none",{force = true})

-- target("ap_boot")
--     -- set_kind("binary")
--     set_languages("c17", "cxx20")

--     add_rules("kernel-debug","kernel-release")

--     set_objectdir("$(buildir)/dionysus/ap_boot")

--     add_includedirs("include")

--     add_files("kern/init/ap_boot.S")

--     set_toolchain("cc","clang")
--     set_toolchain("as","clang")
--     set_toolchain("cxx","clang++")
--     set_toolchain("ld","clang++")

--     add_asflags("--target=x86_64-pc-none-elf")
--     add_asflags("-fno-pie","-fno-exceptions","-fno-rtti","-ffreestanding","-nostdlib","-fno-builtin","-gdwarf-2","-Wall","-Wextra")
--     add_asflags("-march=x86-64","-mtls-direct-seg-refs","-mno-sse","-mcmodel=large","-mno-red-zone")

--     add_ldflags("-Wl,-T config/build/ap_boot.ld","-Wl,--omagic",{force = true})
--     add_ldflags("-z max-page-size=0x1000" ,"-no-pie" ,"-nostdlib" ,"-ffreestanding", "-nostartfiles" ,"-Wl,--build-id=none",{force = true})