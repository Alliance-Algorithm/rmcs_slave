function main(target)
    local file = io.open("bsp/HAL/Makefile", "r")
    if (file ~= nil) then
        local text = file:read("a"):gsub("\r", ""):gsub("\\ *\n", " ")
        text:match("C_SOURCES *=([^\r\n\t\v\f]+)\n"):gsub("[^ ]+", function(f)
            target:add("files", "bsp/HAL/" .. f)
        end)
        text:match("ASM_SOURCES *=([^\r\n\t\v\f]+)\n"):gsub("[^ ]+", function(f)
            target:add("files", "bsp/HAL/" .. f)
        end)
        text:match("C_INCLUDES *=([^\r\n\t\v\f]+)\n"):gsub("-I([^ ]+)", function(f)
            target:add("includedirs", "bsp/HAL/" .. f)
        end)
    end
end