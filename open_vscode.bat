@echo off
chcp 65001 > nul
setlocal

rem @note: "code.cmd" doesn't close its terminal window, but
rem there's a line, which you can prepend with `start "" `
rem "%~dp0..\Code.exe" "%~dp0..\resources\app\out\cli.js" %*
rem doing so will fix the issue

rem |> PREPARE TOOLS
call :check_editor || (
	set "PATH=%PATH%;C:/Program Files/Microsoft VS Code/bin/"
	call :check_editor || (
		echo.can't find editor
		goto :eof
	)
)

call "project/environment.bat" || ( goto :eof )

rem |> DO
start code "__vscode_workspace/game_prototype.code-workspace"

rem |> FUNCTIONS
goto :eof

:check_editor
	where -q "code.cmd"
	rem return: `errorlevel`
goto :eof
