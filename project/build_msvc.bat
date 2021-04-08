@echo off
set timeHeader=%time%
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem set debug=dummy
set unity_build=dummy
rem set dynamic_rt=dummy

rem https://docs.microsoft.com/cpp/build/reference/compiler-options
rem https://docs.microsoft.com/cpp/build/reference/linker-options

rem > PREPARE TOOLS
set VSLANG=1033
pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
call "vcvarsall.bat" x64 > nul
popd

rem > OPTIONS
set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE 
set libs=user32.lib gdi32.lib
set warnings=-WX -W4
set compiler=-nologo -diagnostics:caret -EHa- -GR- -Fo"./temp/"
set linker=-nologo -WX -subsystem:console

set linker=%linker% -nodefaultlib:libcmt.lib
if defined dynamic_rt (
	set libs=%libs% msvcrt.lib
	set defines=%defines% -D_MT -D_DLL
) else (
	set libs=%libs% libcmt.lib
)

if defined debug (
	set defines=%defines% -DGAME_TARGET_DEBUG
	set compiler=%compiler% -Od -Zi
	set linker=%linker% -debug:full
) else (
	set compiler=%compiler% -O2
	set linker=%linker% -debug:none
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

if defined unity_build (
	set linker=-link %linker%
)

set warnings=%warnings% -wd5105

rem > COMPILE AND LINK
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

set timeCompile=%time%
if defined unity_build ( rem > compile and auto-link unity build
	set timeLink=%time%
	cl -std:c11 %compiler% %warnings% "../project/unity_build.c" -Fe"game.exe" %linker%
	if exist "./temp/unity_build*" del ".\temp\unity_build*"
) else ( rem > alternatively, compile a set of translation units
	for /f %%v in (../project/translation_units.txt) do ( rem > for each line %%v in the file
		cl -std:c11 -c %compiler% %warnings% "../%%v"
		if errorlevel 1 goto error
	)
	set timeLink=%time%
	link "./temp/*.obj" -out:"game.exe" %linker%
)

:error
set timeStop=%time%

rem > REPORT
echo header:  %timeHeader%
echo compile: %timeCompile%
echo link:    %timeLink%
echo stop:    %timeStop%
