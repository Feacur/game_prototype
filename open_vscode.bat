@echo off
chcp 65001 > nul
setlocal

rem |> PREPARE TOOLS
call :check_editor || (
	set "PATH=%PATH%;C:/Program Files/Microsoft VS Code"
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
	where -q "code.exe"
	rem return: `errorlevel`
goto :eof
