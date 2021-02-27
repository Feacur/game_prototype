@echo off
chcp 65001

set debug=dummy
rem set unity_build=dummy
rem set dynamic_rt=dummy

rem https://docs.microsoft.com/cpp/build/reference/compiler-options
rem https://docs.microsoft.com/cpp/build/reference/linker-options

rem > PREPARE TOOLS
rem set "PATH=%PATH%;C:/Program Files/LLVM/bin"
rem possible `clang-cl` instead `cl -std:c11`
rem possible `lld-link` instead `link`

set VSLANG=1033
pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
call "vcvarsall.bat" x64
popd

rem > OPTIONS
set includes=-I".."
set defines=-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX
set libs=user32.lib gdi32.lib
set warnings=-WX -W4
set compiler=-nologo -diagnostics:caret -EHa- -GR- -Fo"./temp/"
set linker=-nologo -WX -subsystem:console

if defined dynamic_rt (
	set compiler=%compiler% -MD
) else (
	set compiler=%compiler% -MT
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
set timeStart=%time%
cd ..
if not exist bin mkdir bin
cd bin

if not exist temp mkdir temp

if defined unity_build (
	cl -std:c11 "../project/unity_build.c" -Fe"game.exe" %compiler% %warnings% %linker%
) else ( rem alternatively, compile a set of translation units
	if exist "./temp/unity_build*" del ".\temp\unity_build*"
	cl -std:c11 -c "../code/*.c" %compiler% %warnings%
	cl -std:c11 -c "../code/windows/*.c" %compiler% %warnings%
	link "./temp/*.obj" -out:"game.exe" %linker%
)

cd ../project
set timeStop=%time%

rem > REPORT
echo start: %timeStart%
echo stop:  %timeStop%
