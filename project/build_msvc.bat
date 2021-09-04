@echo off
setlocal enabledelayedexpansion
set timeHeader=!time!
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem [any]
set project=%1
if [%project%] == [] ( set project=game )

rem optimized|development|debug
set configuration=%2
if [%configuration%] == [] ( set configuration=optimized )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%3
if [%runtime_mode%] == [] ( set runtime_mode=static )

rem normal|unity|unity_link
set build_mode=%4
if [%build_mode%] == [] ( set build_mode=unity_link )

rem https://docs.microsoft.com/cpp/build/reference/compiler-options
rem https://docs.microsoft.com/cpp/build/reference/linker-options
rem https://docs.microsoft.com/cpp/c-runtime-library
rem https://docs.microsoft.com/windows-server/administration/windows-commands/for
rem https://docs.microsoft.com/windows-server/administration/windows-commands/setlocal

rem |> PREPARE PROJECT
set project_folder=%cd%
set binary_folder=bin

rem |> PREPARE TOOLS
set VSLANG=1033
call :check_msvc_exists && ( call "vcvarsall.bat" x64 > nul ) || (
	rem @idea: list potential paths in a text file
	pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
	call :check_msvc_exists && ( call "vcvarsall.bat" x64 > nul ) || (
		echo.can't find msvc in the path
		goto :eof
	)
	popd
)

call :check_compiler_exists || (
	echo.can't find compiler in the path
	goto :eof
)

call :check_linker_exists || (
	echo.can't find linker in the path
	goto :eof
)

rem |> OPTIONS
set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DSTRICT -DVC_EXTRALEAN -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE 
set libs=kernel32.lib user32.lib gdi32.lib
set warnings=-WX -W4
set compiler=-nologo -diagnostics:caret -EHa- -GR-
set linker=-nologo -WX -subsystem:console -incremental:no

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
	set compiler=%compiler% -O2
	set linker=%linker% -debug:none
	set defines=%defines% -DGAME_TARGET_OPTIMIZED
	rem [linker] -opt:ref -opt:icf -opt:lbr
) else if %configuration% == development (
	set compiler=%compiler% -O2 -Zi
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEVELOPMENT
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
) else if %configuration% == debug (
	set compiler=%compiler% -Od -Zi
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEBUG
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

set warnings=%warnings% -wd5105

rem |> COMPILE AND LINK
pushd ..
if not exist %binary_folder% mkdir %binary_folder%
pushd %binary_folder%

set timeCompile=!time!
if %build_mode% == normal ( rem compile a set of translation units, then link them
	rem @note: the folder may contain outdated objects
	if not exist temp mkdir temp
	for /f %%v in (%project_folder%/%project%_translation_units.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		cl -std:c11 -c -Fo"./temp/!object_file_name!.obj" %compiler% %warnings% "../%%v"
		if errorlevel == 1 goto error
	)
	set timeLink=!time!
	link "./temp/*.obj" -out:"%project%.exe" %linker%
) else if %build_mode% == unity ( rem compile as a unity build, then link separately
	cl -std:c11 -c -Fo"./%project%_unity_build.obj" %compiler% %warnings% "%project_folder%/%project%_unity_build.c"
	if errorlevel == 1 goto error
	set timeLink=!time!
	link "./%project%_unity_build.obj" -out:"%project%.exe" %linker%
) else if %build_mode% == unity_link ( rem compile and link as a unity build
	set timeLink=unknown
	cl -std:c11 %compiler% %warnings% "%project_folder%/%project%_unity_build.c" -Fe"./%project%.exe" -link %linker%
)

:error
set timeStop=!time!

popd
popd

rem |> REPORT
echo.header:  %timeHeader%
echo.compile: %timeCompile%
echo.link:    %timeLink%
echo.stop:    %timeStop%

rem |> FUNCTIONS
goto :eof

:check_msvc_exists
where -q "vcvarsall.bat"
rem return is errorlevel == 1 means false; chain with `||`
rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof

:check_compiler_exists
where -q "cl.exe"
rem return is errorlevel == 1 means false; chain with `||`
rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof

:check_linker_exists
where -q "link.exe"
rem return is errorlevel == 1 means false; chain with `||`
rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof
