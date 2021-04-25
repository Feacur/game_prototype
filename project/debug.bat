@echo off
chcp 65001 > nul

rem |> PREPARE PROJECT
set project_folder=%cd%
set project=game

rem |> PREPARE TOOLS
tasklist -fi "IMAGENAME eq remedybg.exe" -nh | find /i /n "remedybg.exe" > nul
if errorlevel == 1 set debugger_is_offline=dummy

rem |> DO
pushd ..
if exist "%project_folder%/%project%.rdbg" (
	if defined debugger_is_offline (
		start remedybg "%project_folder%/%project%.rdbg"
		timeout 1 -nobreak
	)
	start remedybg start-debugging
) else (
	start remedybg "bin/%project%.exe"
)
popd
