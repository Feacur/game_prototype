@echo off
setlocal enabledelayedexpansion
set timeHeader=!time!
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem optimized|development|debug
set configuration=%1
if [%configuration%] == [] ( set configuration=optimized )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%2
if [%runtime_mode%] == [] ( set runtime_mode=static )

rem normal|unity|unity_link
set build_mode=%3
if [%build_mode%] == [] ( set build_mode=unity_link )

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html
rem https://docs.microsoft.com/cpp/build/reference/linker-options
rem https://docs.microsoft.com/cpp/c-runtime-library

rem |> PREPARE PROJECT
set project_folder=%cd%
set project=game

rem |> PREPARE TOOLS
set "PATH=%PATH%;C:/Program Files/LLVM/bin"

if not %build_mode% == unity_link (
	set VSLANG=1033
	pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
	call "vcvarsall.bat" x64 > nul
	popd
)

rem |> OPTIONS
set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE
set libs=kernel32.lib user32.lib gdi32.lib
set warnings=-Werror -Weverything
set compiler=-fno-exceptions -fno-rtti
set linker=-nologo -WX -subsystem:console

set linker=%linker% -nodefaultlib
if %runtime_mode% == static (
	set libs=%libs% libucrt.lib libvcruntime.lib libcmt.lib
) else if %runtime_mode% == dynamic (
	set libs=%libs% ucrt.lib vcruntime.lib msvcrt.lib
	set defines=%defines% -D_MT -D_DLL
) else if %runtime_mode% == static_debug (
	set libs=%libs% libucrtd.lib libvcruntimed.lib libcmtd.lib
	set defines=%defines% -D_DEBUG
) else if %runtime_mode% == dynamic_debug (
	set libs=%libs% ucrtd.lib vcruntimed.lib msvcrtd.lib
	set defines=%defines% -D_MT -D_DLL -D_DEBUG
)

if %configuration% == optimized (
	set compiler=%compiler% -O3
	set linker=%linker% -debug:none
	set defines=%defines% -DGAME_TARGET_OPTIMIZED
) else if %configuration% == development (
	set compiler=%compiler% -O3 -g
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEVELOPMENT
) else if %configuration% == debug (
	set compiler=%compiler% -O0 -g
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEBUG
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

set warnings=%warnings% -Wno-switch-enum
set warnings=%warnings% -Wno-float-equal
set warnings=%warnings% -Wno-reserved-id-macro
set warnings=%warnings% -Wno-nonportable-system-include-path
set warnings=%warnings% -Wno-assign-enum
set warnings=%warnings% -Wno-bad-function-cast
set warnings=%warnings% -Wno-documentation-unknown-command

rem |> COMPILE AND LINK
pushd ..
if not exist bin mkdir bin
pushd bin

set timeCompile=!time!
if %build_mode% == normal ( rem |> compile a set of translation units, then link them
	if not exist temp mkdir temp
	for /f %%v in (%project_folder%/%project%_translation_units.txt) do ( rem |> for each line %%v in the file
		clang -std=c99 -c -o"./temp/%%~nv.o" %compiler% %warnings% "../%%v"
		if errorlevel == 1 goto error
	)
	set timeLink=!time!
	lld-link "./temp/*.o" -out:"%project%.exe" %linker%
) else if %build_mode% == unity ( rem |> compile as a unity build, then link separately
	clang -std=c99 -c -o"./%project%_unity_build.o" %compiler% %warnings% "%project_folder%/%project%_unity_build.c"
	set timeLink=!time!
	lld-link "./%project%_unity_build.o" -out:"%project%.exe" %linker%
) else if %build_mode% == unity_link ( rem |> compile and link as a unity build
	set timeLink=!time!
	clang -std=c99 %compiler% %warnings% "%project_folder%/%project%_unity_build.c" -o"./%project%.exe" -Wl,%linker: =,%
)

:error
set timeStop=!time!

popd
popd

rem |> REPORT
echo header:  %timeHeader%
echo compile: %timeCompile%
echo link:    %timeLink%
echo stop:    %timeStop%
