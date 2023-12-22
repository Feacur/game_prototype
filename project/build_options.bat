@echo off
chcp 65001 > nul

set func=%cd%/functions.bat
set build=%cd%/build_actions.bat
call %func% get_millis time_zero

rem [any]
set project=%~1
if [%project%] == [] ( set "project=game" )

rem tiny|fast|tiny_dev|fast_dev|debug
set configuration=%~2
if [%configuration%] == [] ( set "configuration=fast" )

rem static|dynamic|static_debug|dynamic_debug
set runtime_mode=%~3
if [%runtime_mode%] == [] ( set "runtime_mode=static" )

rem shared|console|windows
set arch_mode=%~4
if [%arch_mode%] == [] ( set "arch_mode=console" )

rem normal|unity|unity_link|auto
set build_mode=%~5
if [%build_mode%] == [] ( set "build_mode=unity" )

rem |> PREPARE PROJECT
if not %arch_mode% == shared (
	call %func% check_exe_online %project% && (
		rem @todo: mind potential live reloading; hot reloading is safe, though
		echo.executable "%project%" is running
		exit /b 1
	)
)

rem |> PREPARE TOOLS
call "environment.bat" || ( goto :eof )

rem @note: it is admittedly overengineered! but I wanted to support
rem        - two compilers (clang, msvc)
rem        - three build modes (multiple TUs, single TU, autolinking)
rem        - universally select optimization level
rem        - universally select CRT
rem it would be saner to make a single dedicated script instead
rem but this setup helps me to verify
rem        - headers correctness
rem        - absence of name collisions
rem        - absence of API leaks
rem also it is fun

rem @note: set any variable only once per if statement
rem        if immediate expansion is employed; otherwise
rem        only last one will take the effect

rem @note: relies on global variables

rem linker options for MSVC `link` or clang `lld-link`

rem https://learn.microsoft.com/cpp/c-runtime-library
rem https://learn.microsoft.com/cpp/build/reference/entry-entry-point-symbol
rem https://learn.microsoft.com/cpp/build/reference/subsystem-specify-subsystem
rem https://learn.microsoft.com/cpp/build/reference/dot-res-files-as-linker-input
rem https://learn.microsoft.com/cpp/build/reference/linker-options

rem https://learn.microsoft.com/windows-server/administration/windows-commands/for
rem https://learn.microsoft.com/windows-server/administration/windows-commands/setlocal

set compiler=%compiler% -I".." -I"../third_party"

set linker=%linker% -nologo -WX -incremental:no
set libs=%libs% kernel32.lib user32.lib gdi32.lib

set project_folder=%cd%
set source=%cd%/translation_units_%project%
set unity_object=unity_build_%project%

set linker=%linker% -nodefaultlib
if %runtime_mode% == static (
	set linker=%linker% libucrt.lib libvcruntime.lib libcmt.lib
) else if %runtime_mode% == dynamic (
	set linker=%linker% ucrt.lib vcruntime.lib msvcrt.lib
) else if %runtime_mode% == static_debug (
	set linker=%linker% libucrtd.lib libvcruntimed.lib libcmtd.lib
) else if %runtime_mode% == dynamic_debug (
	set linker=%linker% ucrtd.lib vcruntimed.lib msvcrtd.lib
) else (
	echo.unknown runtime_mode "%runtime_mode%"
	exit /b 1
)

if %arch_mode% == shared (
	set compiler=%compiler% -DGAME_ARCH_SHARED
	set linker=%linker% -machine:x64 -DLL
	set output=%project%.dll
	rem [linker] -entry:_DllMainCRTStartup
) else if %arch_mode% == console (
	set compiler=%compiler% -DGAME_ARCH_CONSOLE
	set linker=%linker% -machine:x64 -subsystem:console
	set libs=%libs% "temp/%project%.res"
	set output=%project%.exe
	rem [linker] -entry:mainCRTStartup
) else if %arch_mode% == windows (
	set compiler=%compiler% -DGAME_ARCH_WINDOWS
	set linker=%linker% -machine:x64 -subsystem:windows
	set libs=%libs% "temp/%project%.res"
	set output=%project%.exe
	rem [linker] -entry:WinMainCRTStartup
) else (
	echo.unknown arch_mode "%arch_mode%"
	exit /b 1
)

if %configuration% == tiny (
	set compiler=%compiler% -DGAME_TARGET_RELEASE
	set linker=%linker% -debug:none
) else if %configuration% == fast (
	set compiler=%compiler% -DGAME_TARGET_RELEASE
	set linker=%linker% -debug:none
) else if %configuration% == tiny_dev (
	set compiler=%compiler% -DGAME_TARGET_DEVELOPMENT
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
) else if %configuration% == fast_dev (
	set compiler=%compiler% -DGAME_TARGET_DEVELOPMENT
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
) else if %configuration% == debug (
	set compiler=%compiler% -DGAME_TARGET_DEBUG
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
) else (
	echo.unknown configuration "%configuration%"
	exit /b 1
)

if %build_mode% == normal (
	rem compile units, then link
	set separate_linking=true
) else if %build_mode% == unity (
	rem compile single unit, then link
	set separate_linking=true
) else if %build_mode% == unity_link (
	rem compile single unit and link
) else if %build_mode% == auto (
	rem compile units and link
) else (
	echo.unknown build_mode "%build_mode%"
	exit /b 1
)

set resource_compiler=rc -nologo -fo "temp/%project%.res" "%project_folder%/windows_resources.rc"
