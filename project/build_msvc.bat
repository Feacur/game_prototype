@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.building with MSVC...

rem https://learn.microsoft.com/cpp/build/reference/compiler-options

rem |> OPTIONS
call build_options.bat %* || ( goto :eof )
set compiler=%compiler% -nologo -diagnostics:caret
set compiler=%compiler% -std:c11 -options:strict
set compiler=%compiler% -EHa- -GR-
set compiler=%compiler% -WX -W4

set linker=%linker% -noimplib -noexp

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
	set linker=-link %linker% %libs%
	set output=-Fe:"%output%"
) else (
	rem compile then link
	set compiler=cl -c %compiler%
	set linker=link %linker% %libs%
	set output=-out:"%output%"
)

rem |> BUILD
call %build% prep
call %build% comp -Fo .obj
call %build% link
call %build% post
call %build% stat

endlocal
