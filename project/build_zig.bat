@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with Zig...

rem |> OPTIONS
call build_options.bat %* || ( goto :eof )
set compiler=%compiler% -std=c99
set compiler=%compiler% -fno-exceptions -fno-rtti
set compiler=%compiler% -Werror -Weverything

set compiler=%compiler% -target x86_64-windows
set compiler=%compiler% -fno-sanitize=undefined
set compiler=%compiler% -DWINVER=0x0A00

set linker=-Wl,-implib=

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
	set compiler=zig cc %compiler%
	set linker=%libs% %linker%
	set output=-o "%output%"
) else (
	rem compile then link
	set compiler=zig cc -c %compiler%
	set linker=zig cc %libs% %linker%
	set output=-o "%output%"
)

rem |> BUILD
call %build% prep
call %build% comp -o .o
call %build% link
call %build% post
call %build% stat

endlocal
