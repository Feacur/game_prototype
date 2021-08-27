@echo off
setlocal enabledelayedexpansion
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem |> PREPARE PROJECT
set project_folder=%cd%
set project=game

rem |> DO
pushd ..
if exist "%project_folder%/%project%.rdbg" (
	call :check_debugger_online || (
		start remedybg "%project_folder%/%project%.rdbg"
		:wait_for_debugger
		call :check_debugger_online || goto wait_for_debugger
	)
	start remedybg start-debugging
) else (
	start remedybg "bin/%project%.exe"
	rem current working directory should be the root
	rem that's why I do not launch debugging right away
)
popd

rem |> FUNCTIONS
goto :eof

:check_debugger_online
tasklist -fi "IMAGENAME eq remedybg.exe" -nh | find /i /n "remedybg.exe" > nul
rem return is errorlevel == 1 means false; chain with `||`
rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof
