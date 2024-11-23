-- requires xmake v2.8.5+
---@diagnostic disable: undefined-global
add_rules("plugin.compile_commands.autoupdate", { lsp = "clangd", outputdir = "." })
set_policy("build.warning", true)
set_policy("check.auto_ignore_flags", false)
set_languages("c11", "c++20")

set_config("plat", "cross")           -- 交叉编译
set_config("cross", "arm-none-eabi-") -- 设置交叉编译平台
set_toolchains("gnu-rm")              -- 使用gnu-arm工具链

target("application", function(t)
    local version = "2.1.0"
    set_version(version)
    add_defines("APP_VERSION=\"" .. version .. "\"")

    set_kind("binary")
    add_rules("c++", "asm")
    set_extension(".elf")

    if is_mode("debug") then       -- 调试模式，开启O3优化
        set_optimize("fastest")
    elseif is_mode("release") then -- 发布模式，开启O3优化和NDEBUG标志
        set_optimize("fastest")
        add_defines("NDEBUG")
    end

    -- 从CubeMX生成的Makefile中读取hal的源文件和头文件
    on_load("script.read_hal_makefile")

    -- 添加源文件和头文件
    add_files("bsp/SEGGER/**.c", "bsp/SEGGER/**.S")
    add_files("app/**.cpp", "utility/**.cpp")
    add_includedirs(".")

    -- 在任何模式下都生成调试信息
    add_cxflags("-g", "-gdwarf-2")

    -- 为gcc设置编译平台(STM32F407)
    add_cxflags("-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard")
    add_asflags("-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard")
    add_ldflags("-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard")

    -- 启用all和extra级别的警告，启用变量遮蔽(shadow)的警告，并将所有警告视为错误
    add_cxflags("-Wall", "-Wextra", "-Wshadow", "-Werror")
    -- (白名单)未使用的变量视为警告，未使用的函数参数不警告
    add_cxflags("-Wno-error=unused", "-Wno-error=unused-variable")
    add_cxflags("-Wno-error=unused-but-set-variable", "-Wno-error=unused-function", "-Wno-unused-parameter")
    add_cxxflags("-Wno-error=unused-local-typedefs")
    -- 对于gcc编译器，需要加一句-pedantic-errors禁用所有GNU扩展
    add_cxflags("-pedantic-errors")

    -- 定义HAL库相关的宏
    add_defines("USE_HAL_DRIVER", "STM32F407xx")

    -- 将全局变量和函数放置在目标文件中的单独部分中，允许链接器在链接过程中删除未使用的变量和函数，减少最终文件的大小
    add_cxflags("-fdata-sections", "-ffunction-sections")

    -- 禁用C++的异常和RTTI特性(标准库未支持)
    add_cxxflags("-fno-exceptions", "-fno-rtti")
    -- 禁止static变量初始化时自动加锁
    add_cxxflags("-fno-threadsafe-statics")

    -- 指定链接脚本
    add_ldflags("-T bsp/HAL/STM32F407IGHx_FLASH.ld")
    -- 链接标准c库，数学库和标准c++库
    add_ldflags("-lc", "-lm", "-lstdc++")
    -- 在链接时打印内存占用
    add_ldflags("-Wl,--print-memory-usage")

    -- 启用垃圾收集：在链接过程中删除未使用的变量和函数
    add_ldflags("-Wl,--gc-sections")
end)
