@echo off
set timeHeader=%time%
chcp 65001 >nul

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
	call "vcvarsall.bat" x64 >nul
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

set warnings=%warnings% -Wno-reserved-id-macro -Wno-nonportable-system-include-path -Wno-assign-enum -Wno-bad-function-cast

rem > COMPILE AND LINK
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

set timeCompile=%time%
if defined unity_build ( rem > compile and auto-link unity build
	set timeLink=%time%
	clang -std=c99 %compiler% %warnings% "../project/unity_build.c" -o"game.exe" %linker%
) else ( rem > alternatively, compile a set of translation units
	for /f %%v in (../project/translation_units.txt) do ( rem > for each line %%v in the file
		clang -std=c99 -c %compiler% %warnings% "../%%v"
		if errorlevel 1 goto error
		move ".\*.o" ".\temp" >nul
	)
	set timeLink=%time%
	lld-link "./temp/*.o" libcmt.lib -out:"game.exe" %linker%
)

:error
set timeStop=%time%

rem > REPORT
echo header:  %timeHeader%
echo compile: %timeCompile%
echo link:    %timeLink%
echo stop:    %timeStop%
