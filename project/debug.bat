@echo off
chcp 65001 > nul

tasklist -fi "IMAGENAME eq remedybg.exe" -nh | find /i /n "remedybg.exe" > nul
if %ERRORLEVEL% == 1 (
	set debugger_is_offline=dummy
)

pushd ..
if exist "project/game.rdbg" (
	if defined debugger_is_offline (
		start remedybg "project/game.rdbg"
		timeout 0 -nobreak
	)
	start remedybg start-debugging
) else (
	start remedybg "bin/game.exe"
)
popd
