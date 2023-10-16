@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with Clang...

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

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html

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
set compiler=%compiler% -std=c99
set compiler=%compiler% -fno-exceptions -fno-rtti
set compiler=%compiler% -Werror -Weverything

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

if %build_mode% == unity_link (
	rem compile and link
	set compiler=clang %compiler%
	set linker=-Wl,%linker: =,%
	set output=-o:"%output%"
) else (
	rem compile then link
	set compiler=clang -c %compiler%
	set linker=lld-link %linker%
	set output=-out:"%output%"
)

rem |> BUILD
call %build% prep
call %build% comp -o .o
call %build% link
call %build% post
call %build% stat

endlocal
