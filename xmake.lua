target("ircserv")
	set_kind("binary")
	add_files("src/**.cpp")
	add_includedirs("src")
	set_targetdir("$(projectdir)")

	set_languages("c++20")
	set_warnings("all", "extra", "error")
	set_runargs("6667", "secret")
	if is_mode("debug") then
		set_symbols("debug")
		set_optimize("none")
		add_defines("LOG_FORMAT=6")
		add_defines("LOG_LEVEL=2")
	end
	if is_mode("release") then
		set_symbols("hidden")
		set_strip("all")
		set_optimize("fastest")
		add_defines("LOG_FORMAT=6")
		add_defines("LOG_LEVEL=3")
	end
