@echo off
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem @note: can be used to prepare environment before launching an editor
rem        so that build system doesn't waste time on resetting tools paths
rem        especially this is crucial for "vcvarsall.bat", which takes 2-3 seconds

if not [%environment_is_ready%] == [] (
	goto :eof
)

rem |> MSVC
set VSLANG=1033
call :check_msvc || (
	call :check_vcvarsall && ( call "vcvarsall.bat" x64 > nul ) || (
		pushd "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
		call :check_vcvarsall && ( call "vcvarsall.bat" x64 > nul ) || (
			echo.can't find MSVC's vcvarsall
			goto :eof
		)
		popd
	)
)

rem |> Clang
call :check_clang || (
	set "PATH=%PATH%;C:/Program Files/LLVM/bin"
	call :check_clang || (
		echo.can't find Clang's compiler/linker
		goto :eof
	)
)

set environment_is_ready=true

rem |> FUNCTIONS
goto :eof

:check_vcvarsall
	where -q "vcvarsall.bat"
	rem return is errorlevel == 1 means false; chain with `||`
	rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof

:check_msvc
	where -q "cl.exe" || where -q "link.exe"
	rem return is errorlevel == 1 means false; chain with `||`
	rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof

:check_clang
	where -q "clang.exe" || where -q "lld-link.exe"
	rem return is errorlevel == 1 means false; chain with `||`
	rem return is errorlevel != 1 means true;  chain with `&&`
goto :eof
