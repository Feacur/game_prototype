@echo off
chcp 65001 > nul
setlocal

rem |> PREPARE TOOLS
call :check_editor || (
	set "PATH=%PATH%;C:/Program Files/Sublime Text"
	call :check_editor || (
		echo.can't find editor
		goto :eof
	)
)

call "project/environment.bat" || ( goto :eof )

rem |> DO
start subl "__sublime_project/game_prototype.sublime-project"

rem |> FUNCTIONS
goto :eof

:check_editor
	where -q "subl.exe"
	rem return: `errorlevel`
goto :eof
