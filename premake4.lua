solution "glbootstrap"
	targetdir "bin"
	configurations { "Debug", "Release" }
	project "glbootstrap"
		kind "WindowedApp"
		language "C"
		includedirs { "eglproxy/inc" }
		includedirs { "inc" }
		files { "inc/config.h", "inc/GL/glcorearb.h" }
		links { "eglproxy" }
		if os.is("windows") then
			flags { "WinMain" }
			files { "src/main_win32.c"}
		elseif os.is("linux") or os.is("bsd") then
			files { "src/main_x11.c"}
			links { "X11" }
		end

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols" }
		configuration "Release"
			defines { "NDEBUG"}
			flags { "Optimize" }

	include "eglproxy"
