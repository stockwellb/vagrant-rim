set_project("vagrant-rim")
set_version("0.1.0")

-- Global build settings
set_languages("c11")
set_warnings("all", "extra")
add_rules("mode.debug", "mode.release")

-- Dependencies (pulled from the xmake package repository)
add_requires("raylib")
add_requires("lua")

-- Main game executable
target("vagrant-rim")
    set_kind("binary")
    add_files("src/**.c")
    add_includedirs("src")
    add_packages("raylib", "lua")

    -- Ship data files (Lua config, assets) next to the executable.
    add_installfiles("assets/(**)", {prefixdir = "bin/assets"})
target_end()
