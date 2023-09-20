@echo off
chcp 65001 > nul

rem linker options for MSVC `link` or clang `lld-link`

rem https://learn.microsoft.com/cpp/c-runtime-library/crt-library-features
rem https://learn.microsoft.com/cpp/build/reference/entry-entry-point-symbol
rem https://learn.microsoft.com/cpp/build/reference/subsystem-specify-subsystem

set includes=-I".." -I"../third_party"
set defines=-D_CRT_SECURE_NO_WARNINGS -DSTRICT -DVC_EXTRALEAN -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE
set libs=kernel32.lib user32.lib gdi32.lib
set linker=-nologo -WX -incremental:no

set linker=%linker% -nodefaultlib
if %runtime_mode% == static (
	set libs=%libs% libucrt.lib libvcruntime.lib libcmt.lib
) else if %runtime_mode% == dynamic (
	set libs=%libs% ucrt.lib vcruntime.lib msvcrt.lib
	set defines=%defines% -D_MT -D_DLL
) else if %runtime_mode% == static_debug (
	set libs=%libs% libucrtd.lib libvcruntimed.lib libcmtd.lib
	set defines=%defines% -D_DEBUG
) else if %runtime_mode% == dynamic_debug (
	set libs=%libs% ucrtd.lib vcruntimed.lib msvcrtd.lib
	set defines=%defines% -D_MT -D_DLL -D_DEBUG
)

if %arch_mode% == shared (
	set defines=%defines% -DGAME_ARCH_SHARED
	rem set linker=%linker% -entry:_DllMainCRTStartup
) else if %arch_mode% == console (
	set defines=%defines% -DGAME_ARCH_CONSOLE
	set linker=%linker% -subsystem:console,5.02
	rem set linker=%linker% -entry:mainCRTStartup
) else if %arch_mode% == windows (
	set defines=%defines% -DGAME_ARCH_WINDOWS
	set linker=%linker% -subsystem:windows,5.02
	rem set linker=%linker% -entry:WinMainCRTStartup
)

if %configuration% == tiny (
	set linker=%linker% -debug:none
	set defines=%defines% -DGAME_TARGET_RELEASE
	rem [linker] -opt:ref -opt:icf -opt:lbr
) else if %configuration% == fast (
	set linker=%linker% -debug:none
	set defines=%defines% -DGAME_TARGET_RELEASE
	rem [linker] -opt:ref -opt:icf -opt:lbr
) else if %configuration% == tiny_dev (
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
	set defines=%defines% -DGAME_TARGET_DEVELOPMENT
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
) else if %configuration% == fast_dev (
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
	set defines=%defines% -DGAME_TARGET_DEVELOPMENT
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
) else if %configuration% == debug (
	set linker=%linker% -debug:full
	set libs=%libs% dbghelp.lib
	set defines=%defines% -DGAME_TARGET_DEBUG
	rem [linker] -opt:noref -opt:noicf -opt:nolbr
)

set compiler=%includes% %defines%
set linker=%linker% %libs%
