@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion
echo.debugging with RemedyBG...

set func=%cd%/functions.bat

rem [any]
set project=%1
if [%project%] == [] ( set project=game )

rem https://docs.microsoft.com/windows-server/administration/windows-commands/tasklist
rem https://docs.microsoft.com/windows-server/administration/windows-commands/find

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
if exist "%project_folder%/debug_%project%.rdbg" (
	call %func% check_exe_online remedybg || (
		start remedybg -q "%project_folder%/debug_%project%.rdbg"
		:wait_for_debugger
		call %func% check_exe_online remedybg || goto wait_for_debugger
	)
	rem instead of the `-g` flag, command it to start: existing instances should be drived too
	start remedybg.exe start-debugging
) else (
	start remedybg "%binary_folder%/%project%.exe"
	rem current working directory should be the root
	rem that's why I do not launch debugging right away
)
popd

rem |> FUNCTIONS
goto :eof

:check_debugger_exists
	where -q "remedybg.exe"
	rem return: `errorlevel`
goto :eof

endlocal
