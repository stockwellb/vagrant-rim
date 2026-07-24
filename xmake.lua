set_project("vagrant-rim")
set_version("0.1.0")

-- Global build settings
set_languages("c11")
set_warnings("all", "extra")
add_rules("mode.debug", "mode.release")

-- Dependencies (pulled from the xmake package repository)
add_requires("raylib")
add_requires("raygui")
add_requires("lua")

-- Main game executable
target("vagrant-rim")
    set_kind("binary")
    add_files("src/**.c")
    add_includedirs("src")
    add_packages("raylib", "raygui", "lua")

    -- Run from the project root during development so assets/ (config.lua,
    -- styles/) resolves relative to the working directory. Installed builds use
    -- the bin/assets layout below.
    set_rundir("$(projectdir)")

    -- Ship data files (Lua config, assets) next to the executable.
    -- Exclude macOS Finder metadata so it never lands in the install bundle.
    add_installfiles("assets/(**)|**.DS_Store", {prefixdir = "bin/assets"})
target_end()
