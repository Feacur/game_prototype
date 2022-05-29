@echo off
chcp 65001 > nul
setlocal

rem |> PREPARE TOOLS
call :check_editor || (
	echo.can't find editor
	goto :eof
)

call "project/environment.bat" || ( goto :eof )

rem |> DO
call :check_terminal_emulator && (
	start wt nvim
	goto :eof
)
start nvim

rem |> FUNCTIONS
goto :eof

:check_terminal_emulator
	where -q "wt.exe"
	rem return: `errorlevel`
goto :eof

:check_editor
	where -q "nvim.exe"
	rem return: `errorlevel`
goto :eof
