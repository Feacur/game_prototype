@echo off
chcp 65001 > nul

rem enable ANSI escape codes for CMD: set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
rem enable UTF-8 by default for CMD: set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`

rem @note: `if errorlevel n` means `if %errorlevel% geq n`
rem        you can chain those with `&&` when true and `||` when false

rem @note: can be used to prepare environment before launching an editor
rem        so that build system doesn't waste time on resetting tools paths
rem        especially this is crucial for "vcvarsall.bat", which takes 2-3 seconds

if not [%environment_is_ready%] == [] (
	goto :eof
)

rem |> Environment
set VSLANG=1033
call :check_environment && ( call "vcvarsall.bat" x64 > nul & goto msvc_found )

rem C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/
pushd "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/" && (
	call :check_environment && ( call "vcvarsall.bat" x64 > nul & popd & goto msvc_found ) || popd
)

echo.can't find MSVC's environment
goto :eof

:msvc_found

rem |> MSVC
call :check_msvc || (
	echo.can't find MSVC's compiler/linker
	goto :eof
)

rem |> Clang
call :check_clang || (
	rem https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.6 - with MSVC 2019
	rem https://github.com/llvm/llvm-project/releases/tag/llvmorg-15.0.7 - with MSVC 2022
	set "PATH=%PATH%;C:/Program Files/LLVM/bin/"
	call :check_clang || (
		echo.can't find Clang's compiler/linker
		goto :eof
	)
)

set environment_is_ready=true

rem |> FUNCTIONS
goto :eof

:check_environment
	where -q "vcvarsall.bat"
	rem return: `errorlevel`
goto :eof

:check_msvc
	where -q "cl.exe" || where -q "link.exe"
	rem return: `errorlevel`
goto :eof

:check_clang
	where -q "clang.exe" || where -q "lld-link.exe"
	rem return: `errorlevel`
goto :eof
