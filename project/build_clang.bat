@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with Clang...

set func=%cd%/functions.bat

call %func% get_millis time_zero

rem [any]
set project=%1
if [%project%] == [] ( set "project=game" )

rem tiny|fast|tiny_dev|fast_dev|debug
set configuration=%2
if [%configuration%] == [] ( set "configuration=fast" )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%3
if [%runtime_mode%] == [] ( set "runtime_mode=static" )

rem shared|console|windows
set arch_mode=%4
if [%arch_mode%] == [] ( set "arch_mode=console" )

rem normal|unity|unity_link
set build_mode=%5
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

call %func% check_exe_online && (
	rem @todo: mind potential live reloading; hot reloading is safe, though
	echo.executable "%project%.exe" is running
	goto :eof
)

rem |> PREPARE TOOLS
call "environment.bat" || ( goto :eof )

rem |> OPTIONS
call build_options.bat
set compiler=%compiler% -fno-exceptions -fno-rtti

if %configuration% == tiny (
	set compiler=%compiler% -Oz
) else if %configuration% == fast (
	set compiler=%compiler% -O3
) else if %configuration% == tiny_dev (
	set compiler=%compiler% -Oz -g
) else if %configuration% == fast_dev (
	set compiler=%compiler% -O3 -g
) else if %configuration% == debug (
	set compiler=%compiler% -O0 -g
)

set compiler=%compiler% -Werror -Weverything
set compiler=%compiler% -Wno-switch-enum
set compiler=%compiler% -Wno-float-equal
set compiler=%compiler% -Wno-reserved-id-macro
set compiler=%compiler% -Wno-reserved-identifier
set compiler=%compiler% -Wno-nonportable-system-include-path
set compiler=%compiler% -Wno-assign-enum
set compiler=%compiler% -Wno-bad-function-cast
set compiler=%compiler% -Wno-documentation-unknown-command
set compiler=%compiler% -Wno-declaration-after-statement
set compiler=%compiler% -Wno-unsafe-buffer-usage
rem [editor-only] -Wno-unused-macros
rem [editor-only] -Wno-unused-function

rem |> BUILD
pushd ..
if not exist %binary_folder% mkdir %binary_folder%
pushd %binary_folder%

if not exist temp mkdir temp
if exist "temp/*.tmp" del "temp\\*.tmp" /q

call %func% get_millis time_comp
if %build_mode% == normal ( rem compile a set of translation units, then link them
	rem @note: the folder may contain outdated objects
	for /f %%v in (%project_folder%/translation_units_%project%.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		clang -std=c99 -c %compiler% "../%%v" -o"./temp/!object_file_name!.o" || ( goto error )
	)
	call %func% get_millis time_link
	set objects=
	for /f %%v in (%project_folder%/translation_units_%project%.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		set objects=!objects! "./temp/!object_file_name!.o"
	)
	lld-link !objects! %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity ( rem compile as a unity build, then link separately
	clang -std=c99 -c %compiler% "%project_folder%/translation_units_%project%.c" -o"./temp/unity_build_%project%.o" || ( goto error )
	call %func% get_millis time_link
	lld-link "./temp/unity_build_%project%.o" %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity_link ( rem compile and link as a unity build
	rem @note: this option is less preferable as it ignores already setup environment
	call %func% get_millis time_link
	clang -std=c99 %compiler% "%project_folder%/translation_units_%project%.c" -o"./%project%.exe" -Wl,%linker: =,% || ( goto error )
)

if not exist ".\%project%.exe.manifest" (
	copy "..\project\windows_dpi_awareness.manifest" ".\%project%.exe.manifest"
)

:error
popd
popd

rem |> REPORT
call %func% get_millis time_done
if [%time_link%] == [] ( set time_link=%time_done% )
call %func% report_millis_delta "prep .." time_zero time_comp
call %func% report_millis_delta "comp .." time_comp time_link
call %func% report_millis_delta "link .." time_link time_done

rem |> FUNCTIONS
goto :eof

endlocal
