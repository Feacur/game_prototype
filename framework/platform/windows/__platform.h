#if !defined(FRAMEWORK_PLATFORM)
#define FRAMEWORK_PLATFORM

/*
Libs:
- always:      kernel32.lib, user32.lib, gdi32.lib
- non-release: dbghelp.lib
*/

#if !defined(STRICT)
	#define STRICT
#endif
#if !defined(VC_EXTRALEAN)
	#define VC_EXTRALEAN
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
	#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
	#define NOMINMAX
#endif

// @note: explicitly disregard -W API and rely on UTF-8 via -A API and `activeCodePage` manifest;
//        also CP_UTF8 for console is being set. it's trivial to restore the compatability code
//        via the `MultiByteToWideChar` and following defines; but for now I can stop caring.
#undef DBGHELP_TRANSLATE_TCHAR
#undef UNICODE

#include "framework/__warnings_push.h"
	#include <initguid.h> // `DEFINE_GUID`
	#include <Windows.h>

	#include <hidusage.h>
	#include <signal.h>

	#if !defined (GAME_TARGET_RELEASE)
		#include <DbgHelp.h>
	#endif
#include "framework/__warnings_pop.h"

#endif
