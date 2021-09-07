@echo off
chcp 65001 > nul
setlocal

rem |> PREPARE TOOLS
call :check_editor || (
	set "PATH=%PATH%;C:/Program Files/Microsoft VS Code/bin/"
	call :check_editor || (
		echo.can't find editor
		goto :eof
	)
)

rem |> DO
call "project/environment.bat" || ( goto :eof )
call code "__vscode_workspace/game_prototype.code-workspace"

rem |> FUNCTIONS
goto :eof

:check_editor
	where -q "code.cmd"
	rem return is errorlevel == 1 means false; chain with `||`
	rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof
