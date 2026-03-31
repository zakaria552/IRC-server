set_languages("c++20")

target("ircserv")
    set_kind("binary")
    add_files("src/**.cpp")
    add_includedirs("src")
    set_targetdir("$(projectdir)")

    set_runargs("6667", "secret")
    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    end
    if is_mode("release") then
        set_symbols("hidden")
        set_strip ("all")
        set_optimize("fastest")
    end
