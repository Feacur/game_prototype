@echo off
chcp 65001 > nul
echo.debugging with RemedyBG...

setlocal enabledelayedexpansion

set func=%cd%/functions.bat

rem [any]
set project=%1
if [%project%] == [] ( set project=game )

rem https://learn.microsoft.com/windows-server/administration/windows-commands/tasklist
rem https://learn.microsoft.com/windows-server/administration/windows-commands/find

rem |> PREPARE PROJECT
set project_folder=%cd%
set binary_folder=bin

rem |> PREPARE TOOLS
call :check_debugger_exists || (
	echo.can't find debugger in the path
	goto :eof
)

if not exist "../%binary_folder%/%project%.exe" (
	echo.can't find target executable
	goto :eof
)

rem |> DO
pushd ..
start remedybg -g -q "%binary_folder%/%project%.exe"
popd

rem |> FUNCTIONS
goto :eof

:check_debugger_exists
	where -q "remedybg.exe"
	rem return: `errorlevel`
goto :eof

endlocal
