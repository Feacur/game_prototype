@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with MSVC...

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

rem https://docs.microsoft.com/cpp/build/reference/compiler-options
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
set compiler=%compiler% -nologo -diagnostics:caret
set compiler=%compiler% -EHa- -GR-

if %configuration% == tiny (
	set compiler=%compiler% -O1
) else if %configuration% == fast (
	set compiler=%compiler% -O2
) else if %configuration% == tiny_dev (
	set compiler=%compiler% -O1 -Zi
) else if %configuration% == fast_dev (
	set compiler=%compiler% -O2 -Zi
) else if %configuration% == debug (
	set compiler=%compiler% -Od -Zi
)

set compiler=%compiler% -WX -W4
set compiler=%compiler% -wd5105
rem [5105] - macro expansion producing 'defined' has undefined behavior

rem |> BUILD
pushd ..
if not exist %binary_folder% mkdir %binary_folder%
pushd %binary_folder%

if not exist temp mkdir temp

call %func% get_millis time_comp
if %build_mode% == normal ( rem compile a set of translation units, then link them
	rem @note: the folder may contain outdated objects
	for /f %%v in (%project_folder%/translation_units_%project%.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		cl -std:c11 -c %compiler% "../%%v" -Fo"./temp/!object_file_name!.obj" || ( goto error )
	)
	call %func% get_millis time_link
	set objects=
	for /f %%v in (%project_folder%/translation_units_%project%.txt) do ( rem for each line %%v in the file
		set object_file_name=%%~v
		set object_file_name=!object_file_name:/=_!
		set object_file_name=!object_file_name:.c=!
		set objects=!objects! "./temp/!object_file_name!.o"
	)
	link !objects! %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity ( rem compile as a unity build, then link separately
	cl -std:c11 -c %compiler% "%project_folder%/translation_units_%project%.c" -Fo"./temp/unity_build_%project%.obj" || ( goto error )
	call %func% get_millis time_link
	link "./temp/unity_build_%project%.obj" %linker% -out:"%project%.exe" || ( goto error )
) else if %build_mode% == unity_link ( rem compile and link as a unity build
	rem @note: this option is less preferable as it ignores already setup environment
	call %func% get_millis time_link
	cl -std:c11 %compiler% "%project_folder%/translation_units_%project%.c" -Fe"./%project%.exe" -link %linker% || ( goto error )
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
