@echo off
chcp 65001 > nul
echo.building with MSVC...

setlocal enabledelayedexpansion
rem |> execute relative to this script
cd /D "%~dp0"

rem https://learn.microsoft.com/cpp/build/reference/compiler-options

rem |> OPTIONS
call build_options.bat %* || ( goto :end )
set compiler=%compiler% -nologo -diagnostics:caret
set compiler=%compiler% -std:c11 -options:strict
set compiler=%compiler% -EHa- -GR-
set compiler=%compiler% -WX -W4
set compiler=%compiler% -Fd"./temp/%project%"

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

if [%separate_linking%] == [] (
	set compiler=cl %compiler%
	set linker=-link %linker% %libs%
	set output=-Fe:"%output%" -Fo:"./temp/"
) else (
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

:end
endlocal
