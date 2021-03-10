@echo off
chcp 65001

set debug=dummy
rem set unity_build=dummy

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html
rem https://docs.microsoft.com/cpp/build/reference/linker-options

rem > PREPARE TOOLS
set "PATH=%PATH%;C:/Program Files/LLVM/bin"

if not defined unity_build (
	set VSLANG=1033
	pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
	call "vcvarsall.bat" x64
	popd
)

rem > OPTIONS
set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX
set libs=user32.lib gdi32.lib
set warnings=-Werror -Weverything -Wno-switch-enum -Wno-float-equal
set compiler=-fno-exceptions -fno-rtti
set linker=-nologo -WX -subsystem:console

if defined debug (
	set defines=%defines% -DGAME_TARGET_DEBUG
	set compiler=%compiler% -O0 -g
	set linker=%linker% -debug:full
) else (
	set compiler=%compiler% -O3
	set linker=%linker% -debug:none
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

if defined unity_build (
	set linker=-Wl,%linker: =,%
)

set warnings=%warnings% -Wno-reserved-id-macro -Wno-nonportable-system-include-path

rem > COMPILE AND LINK
set timeStart=%time%
set timeLink=%timeStart%
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

if defined unity_build (
	echo %compiler% %warnings% %linker%
	clang -std=c99 %compiler% %warnings% "../project/unity_build.c" -o"game.exe" %linker%
) else ( rem alternatively, compile a set of translation units
	if exist "./temp/unity_build*" del ".\temp\unity_build*"
	clang -std=c99 -c %compiler% %warnings% "../framework/*.c"
	clang -std=c99 -c %compiler% %warnings% "../framework/containers/*.c"
	clang -std=c99 -c %compiler% %warnings% "../framework/assets/*.c"
	clang -std=c99 -c %compiler% %warnings% "../framework/windows/*.c"
	clang -std=c99 -c %compiler% %warnings% "../framework/windows/opengl/*.c"
	clang -std=c99 -c %compiler% %warnings% "../framework/opengl/*.c"
	clang -std=c99 -c %compiler% %warnings% "../application/*.c"
	clang -std=c99 -c %compiler% %warnings% "../prototype/*.c"
	set timeLink=%time%
	rem alternatively, `cd temp`, build, link, `cd ..`
	rem but that seems awkward; `move` is quite fast anyways, unlike `lld-link` itself
	move ".\*.o" ".\temp"
	lld-link "./temp/*.o" libcmt.lib -out:"game.exe" %linker%
)

set timeStop=%time%

rem > REPORT
echo start: %timeStart%
echo link:  %timeLink%
echo stop:  %timeStop%
