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

-- Core game logic as a static library so both the game binary and the test
-- suite link the exact same code. This is everything under src/ EXCEPT the entry
-- point (main.c) — a test binary brings its own main, and can't link a second.
target("vagrant-core")
    set_kind("static")
    add_files("src/**.c")
    remove_files("src/main.c")
    -- Marked public so dependents (the game binary, the tests) inherit the
    -- include path and package links without repeating them.
    add_includedirs("src", {public = true})
    add_packages("raylib", "raygui", "lua", {public = true})

    -- macOS controller input goes through Apple's GameController framework
    -- (gamepad_macos.m) because raylib/GLFW can't read Xbox pads there; every
    -- other platform uses the no-op gamepad_stub.c + raylib's own gamepad API.
    -- The frameworks are public so the final binary links them even though the
    -- .m compiles into this static lib.
    if is_plat("macosx") then
        add_files("src/ui/gamepad_macos.m")
        remove_files("src/ui/gamepad_stub.c")
        add_frameworks("GameController", "Foundation", {public = true})
        add_mxflags("-fobjc-arc") -- ARC manages the shim's NSMapTable/NSNumbers
    end
target_end()

-- Main game executable: just the entry point, over the core library.
target("vagrant-rim")
    set_kind("binary")
    add_files("src/main.c")
    add_deps("vagrant-core")

    -- Run from the project root during development so assets/ (config.lua,
    -- styles/) resolves relative to the working directory. Installed builds use
    -- the bin/assets layout below.
    set_rundir("$(projectdir)")

    -- Ship data files (Lua config, assets) next to the executable.
    -- Exclude macOS Finder metadata so it never lands in the install bundle.
    add_installfiles("assets/(**)|**.DS_Store", {prefixdir = "bin/assets"})
target_end()

-- ===========================================================================
-- Tests (headless). Run with:  xmake test
--
-- The vendored Unity framework is built once as a small static lib and shared.
-- Each test file is its own binary (Unity's model: every test brings its own
-- main/setUp/tearDown), links the same vagrant-core the game does, and passes
-- when it exits 0 (UNITY_END returns the failure count). None of the tests open
-- a window or audio device, so they run on CI without a display.
-- ===========================================================================
target("unity")
    set_kind("static")
    set_default(false)
    set_group("tests")
    set_warnings("none") -- vendored third-party code; don't hold it to -Wall -Wextra
    add_files("test/vendor/unity.c")
    add_includedirs("test/vendor", {public = true})
target_end()

-- One binary per test file. Add a new suite by dropping test/test_<name>.c and
-- appending "<name>" here.
for _, name in ipairs({"mathx", "save", "menunav", "config", "settings", "atomic_file",
                       "lua_util", "assets", "audio", "game_flow"}) do
    target("test_" .. name)
        set_kind("binary")
        set_default(false)
        set_group("tests")
        add_files("test/test_" .. name .. ".c")
        add_deps("vagrant-core", "unity")
        add_tests("run")
    target_end()
end
