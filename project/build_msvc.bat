@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with MSVC...

set func=%cd%/functions.bat
set build=%cd%/build_actions.bat

call %func% get_millis time_zero

rem [any]
set project=%~1
if [%project%] == [] ( set "project=game" )

rem tiny|fast|tiny_dev|fast_dev|debug
set configuration=%~2
if [%configuration%] == [] ( set "configuration=fast" )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%~3
if [%runtime_mode%] == [] ( set "runtime_mode=static" )

rem shared|console|windows
set arch_mode=%~4
if [%arch_mode%] == [] ( set "arch_mode=console" )

rem normal|unity|unity_link
set build_mode=%~5
if [%build_mode%] == [] ( set "build_mode=unity" )

rem https://learn.microsoft.com/cpp/build/reference/compiler-options

rem |> PREPARE PROJECT
if not %arch_mode% == shared (
	call %func% check_exe_online %project% && (
		rem @todo: mind potential live reloading; hot reloading is safe, though
		echo.executable "%project%" is running
		goto :eof
	)
)

rem |> PREPARE TOOLS
call "environment.bat" || ( goto :eof )

rem |> OPTIONS
call build_options.bat || ( goto :eof )
set compiler=%compiler% -nologo -diagnostics:caret
set compiler=%compiler% -std:c11 -options:strict
set compiler=%compiler% -EHa- -GR-
set compiler=%compiler% -WX -W4

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

if %build_mode% == unity_link (
	rem compile and link
	set compiler=cl %compiler%
	set linker=-link %linker%
	set output=-Fe:"%output%"
) else (
	rem compile then link
	set compiler=cl -c %compiler%
	set linker=link %linker%
	set output=-out:"%output%"
)

rem |> BUILD
call %build% prep
call %build% comp -Fo .obj
call %build% link
call %build% post
call %build% stat

endlocal
