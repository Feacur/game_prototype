@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with Clang...

rem https://clang.llvm.org/docs/index.html
rem https://clang.llvm.org/docs/CommandGuide/clang.html
rem https://clang.llvm.org/docs/UsersManual.html
rem https://clang.llvm.org/docs/ClangCommandLineReference.html
rem https://lld.llvm.org/windows_support.html

rem |> OPTIONS
call build_options.bat %* || ( goto :eof )
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
