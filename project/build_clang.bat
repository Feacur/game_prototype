@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with Clang...

call :get_millis time_zero

rem [any]
set project=%1
if [%project%] == [] ( set "project=game" )

rem optimized|development|debug
set configuration=%2
if [%configuration%] == [] ( set "configuration=optimized" )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%3
if [%runtime_mode%] == [] ( set "runtime_mode=static" )

rem normal|unity|unity_link
set build_mode=%4
if [%build_mode%] == [] ( set "build_mode=unity" )

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html
rem https://docs.microsoft.com/cpp/build/reference/linker-options
rem https://docs.microsoft.com/cpp/c-runtime-library
rem https://docs.microsoft.com/windows-server/administration/windows-commands/for
rem https://docs.microsoft.com/windows-server/administration/windows-commands/setlocal

rem |> PREPARE PROJECT
set project_folder=%cd%
set binary_folder=bin

call :check_executable_online && (
	rem @todo: mind potential live reloading; hot reloading is safe, though
	echo.executable "%project%.exe" is running
	goto :eof
)

rem |> PREPARE TOOLS
call "environment.bat" || ( goto :eof )

rem |> OPTIONS
set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DSTRICT -DVC_EXTRALEAN -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE
set libs=kernel32.lib user32.lib gdi32.lib
set warnings=-Werror -Weverything
set compiler=-fno-exceptions -fno-rtti
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
	set compiler=%compiler% -O3
	set linker=%linker% -debug:none
	set defines=%defines% -DGAME_TARGET_OPTIMIZED
	rem [linker] -opt:ref -opt:icf -opt:lbr
) else if %configuration% == development (
	set compiler=%compiler% -O3 -g
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEVELOPMENT
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
) else if %configuration% == debug (
	set compiler=%compiler% -O0 -g
	set linker=%linker% -debug:full
	set defines=%defines% -DGAME_TARGET_DEBUG
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
)

set compiler=%compiler% %includes% %defines%
set linker=%linker% %libs%

set warnings=%warnings% -Wno-switch-enum
set warnings=%warnings% -Wno-float-equal
set warnings=%warnings% -Wno-reserved-id-macro
set warnings=%warnings% -Wno-reserved-identifier
set warnings=%warnings% -Wno-nonportable-system-include-path
set warnings=%warnings% -Wno-assign-enum
set warnings=%warnings% -Wno-bad-function-cast
set warnings=%warnings% -Wno-documentation-unknown-command
rem [editor-only] -Wno-unused-macros
rem [editor-only] -Wno-unused-function

rem |> BUILD
pushd ..
if not exist %binary_folder% mkdir %binary_folder%
pushd %binary_folder%

call :get_millis time_comp
if %build_mode% == normal ( rem compile a set of translation units, then link them
	rem @note: the folder may contain outdated objects
	if not exist temp mkdir temp
	if exist "temp/*.tmp" del "temp\\*.tmp" /q
	for /f %%v in (%project_folder%/%project%_translation_units.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		clang -std=c99 -c %compiler% %warnings% "../%%v" -o"./temp/!object_file_name!.o" || ( goto error )
	)
	call :get_millis time_link
	lld-link "./temp/*.o" %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity ( rem compile as a unity build, then link separately
	if exist "./*.tmp" del ".\\*.tmp" /q
	clang -std=c99 -c %compiler% %warnings% "%project_folder%/%project%_unity_build.c" -o"./%project%_unity_build.o" || ( goto error )
	call :get_millis time_link
	lld-link "./%project%_unity_build.o" %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity_link ( rem compile and link as a unity build
	rem @note: this option is less preferable as it ignores already setup environment
	call :get_millis time_link
	clang -std=c99 %compiler% %warnings% "%project_folder%/%project%_unity_build.c" -o"./%project%.exe" -Wl,%linker: =,% || ( goto error )
)

:error
popd
popd

rem |> REPORT
call :get_millis time_done
if [%time_link%] == [] ( set time_link=%time_done% )
call :report_millis_delta "prep .." time_zero time_comp "ms"
call :report_millis_delta "comp .." time_comp time_link "ms"
call :report_millis_delta "link .." time_link time_done "ms"

rem |> FUNCTIONS
goto :eof

:check_executable_online
	tasklist -fi "IMAGENAME eq %project%.exe" -nh | find /i /n "%project%.exe" > nul
	rem return: `errorlevel`
goto :eof

:get_millis
	rem @note: `time` is locale-dependent and can have leading space
	set local_time=%time: =%

	rem @note: the format could have been customized
	for /F "tokens=1-3 delims=0123456789" %%a in ("%local_time%") do ( set "delims=%%a%%b%%c" )

	rem @note: hours, minutes, seconds, centiseconds
	for /F "tokens=1-4 delims=%delims%" %%a in ("%local_time%") do (
		rem @note: force a leading zero, which anyway occur some of the times;
		rem        later, they'll be erased by prepending with 1 and subtracting 100
		if 1%%a lss 20 ( set "hh=0%%a" ) else ( set "hh=%%a" )
		if 1%%b lss 20 ( set "mm=0%%b" ) else ( set "mm=%%b" )
		if 1%%c lss 20 ( set "ss=0%%c" ) else ( set "ss=%%c" )
		if 1%%d lss 20 ( set "cc=0%%d" ) else ( set "cc=%%d" )
	)

	set /a millis = 0
	set /a millis = millis *  24 + "(1%hh% - 100)"
	set /a millis = millis *  60 + "(1%mm% - 100)"
	set /a millis = millis *  60 + "(1%ss% - 100)"
	set /a millis = millis * 100 + "(1%cc% - 100)"
	set /a millis = millis * 10

	rem return: argument `%~1`
	set "%~1=%millis%"
goto :eof

:report_millis_delta
	set /a delta = (86400000 + %~3 - %~2) %% 86400000
	echo.%~1 %delta% %~4
goto :eof
