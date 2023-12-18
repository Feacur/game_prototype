#include "framework/formatter.h"
#include "framework/systems/arena_system.h"
#include "framework/systems/memory_system.h"

#include "framework/graphics/opengl/functions.h"
#include "framework/graphics/opengl/internal/functions_to_gpu_library.h"
#include "framework/graphics/opengl/internal/graphics_to_gpu_library.h"

#include "internal/system.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>

#include "framework/__warnings_push.h"
	#include <GL/wgl.h>
#include "framework/__warnings_pop.h"


static struct Gpu_Library {
	HMODULE handle;

	struct {
		PFNWGLGETPROCADDRESSPROC    GetProcAddress;
		PFNWGLCREATECONTEXTPROC     CreateContext;
		PFNWGLDELETECONTEXTPROC     DeleteContext;
		PFNWGLMAKECURRENTPROC       MakeCurrent;
		PFNWGLGETCURRENTCONTEXTPROC GetCurrentContext;
		PFNWGLGETCURRENTDCPROC      GetCurrentDC;
		PFNWGLSHARELISTSPROC        ShareLists;
	} dll;

	struct {
		PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribiv;
		PFNWGLCREATECONTEXTATTRIBSARBPROC   CreateContextAttribs;
		bool multisample;
		bool context_flush_control;
		bool create_context_no_error;
		bool create_context_robustness;
		bool create_context_profile;
	} arb;

	struct {
		PFNWGLGETSWAPINTERVALEXTPROC     GetSwapInterval;
		PFNWGLSWAPINTERVALEXTPROC        SwapInterval;
		bool swap_control;
		bool framebuffer_sRGB;
	} ext;
} gs_gpu_library;

//
#include "internal/gpu_library_to_system.h"

static void * gpu_library_get_function(struct CString name);
static bool gpu_library_wgl_init(void) {
	HDC const device = gs_gpu_library.dll.GetCurrentDC();

	// ARB
	gs_gpu_library.arb.GetPixelFormatAttribiv = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(void *)gs_gpu_library.dll.GetProcAddress("wglGetPixelFormatAttribivARB");
	gs_gpu_library.arb.CreateContextAttribs   = (PFNWGLCREATECONTEXTATTRIBSARBPROC)  (void *)gs_gpu_library.dll.GetProcAddress("wglCreateContextAttribsARB");

	PFNWGLGETEXTENSIONSSTRINGARBPROC arbGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGARBPROC)   (void *)gs_gpu_library.dll.GetProcAddress("wglGetExtensionsStringARB");
	char const * arbString = arbGetExtensionsString(device);
#define HAS_ARB(name) contains_full_word(arbString, S_("WGL_ARB_" #name))
	gs_gpu_library.arb.multisample               = HAS_ARB(multisample);
	gs_gpu_library.arb.context_flush_control     = HAS_ARB(context_flush_control);
	gs_gpu_library.arb.create_context_no_error   = HAS_ARB(create_context_no_error);
	gs_gpu_library.arb.create_context_robustness = HAS_ARB(create_context_robustness);
	gs_gpu_library.arb.create_context_profile    = HAS_ARB(create_context_profile);
#undef HAS_ARB

	// EXT
	gs_gpu_library.ext.GetSwapInterval = (PFNWGLGETSWAPINTERVALEXTPROC)    (void *)gs_gpu_library.dll.GetProcAddress("wglGetSwapIntervalEXT");
	gs_gpu_library.ext.SwapInterval    = (PFNWGLSWAPINTERVALEXTPROC)       (void *)gs_gpu_library.dll.GetProcAddress("wglSwapIntervalEXT");

	PFNWGLGETEXTENSIONSSTRINGEXTPROC extGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void *)gs_gpu_library.dll.GetProcAddress("wglGetExtensionsStringEXT");
	char const * extString = extGetExtensionsString();
#define HAS_EXT(name) contains_full_word(extString, S_("WGL_EXT_" #name))
	gs_gpu_library.ext.framebuffer_sRGB = HAS_EXT(framebuffer_sRGB);
	gs_gpu_library.ext.swap_control     = HAS_EXT(swap_control);
#undef HAS_EXT

	// OGL
	functions_to_gpu_library_init(gpu_library_get_function);
	return true;
}

static bool gpu_library_do_using_temporary_context(bool (* action)(void));
bool gpu_library_to_system_init(void) {
	gs_gpu_library.handle = LoadLibrary(TEXT("opengl32.dll"));
	if (gs_gpu_library.handle == NULL) { return false; }

	// WGL
	gs_gpu_library.dll.GetProcAddress    = (PFNWGLGETPROCADDRESSPROC)   (void *)GetProcAddress(gs_gpu_library.handle, "wglGetProcAddress");
	gs_gpu_library.dll.CreateContext     = (PFNWGLCREATECONTEXTPROC)    (void *)GetProcAddress(gs_gpu_library.handle, "wglCreateContext");
	gs_gpu_library.dll.DeleteContext     = (PFNWGLDELETECONTEXTPROC)    (void *)GetProcAddress(gs_gpu_library.handle, "wglDeleteContext");
	gs_gpu_library.dll.MakeCurrent       = (PFNWGLMAKECURRENTPROC)      (void *)GetProcAddress(gs_gpu_library.handle, "wglMakeCurrent");
	gs_gpu_library.dll.GetCurrentContext = (PFNWGLGETCURRENTCONTEXTPROC)(void *)GetProcAddress(gs_gpu_library.handle, "wglGetCurrentContext");
	gs_gpu_library.dll.GetCurrentDC      = (PFNWGLGETCURRENTDCPROC)     (void *)GetProcAddress(gs_gpu_library.handle, "wglGetCurrentDC");
	gs_gpu_library.dll.ShareLists        = (PFNWGLSHARELISTSPROC)       (void *)GetProcAddress(gs_gpu_library.handle, "wglShareLists");

	return gpu_library_do_using_temporary_context(gpu_library_wgl_init);

	// https://learn.microsoft.com/windows/win32/api/wingdi/
	// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
}

void gpu_library_to_system_free(void) {
	functions_to_gpu_library_free();
	FreeLibrary(gs_gpu_library.handle);
	common_memset(&gs_gpu_library, 0, sizeof(gs_gpu_library));
}

//
#include "framework/platform/gpu_context.h"

struct Pixel_Format {
	int id;
	int color, alpha, depth, stencil;
	int srgb, samples, swap;
};

struct Gpu_Context {
	HGLRC handle;
	struct Pixel_Format pixel_format;
};

static struct Gpu_Context gpu_context_internal_create(HDC device, HGLRC shared);

struct Gpu_Context * gpu_context_init(void * device) {
	struct Gpu_Context * gpu_context = ALLOCATE(struct Gpu_Context);
	*gpu_context = gpu_context_internal_create(device, NULL);

	if (gpu_context->handle != NULL) {
		gs_gpu_library.dll.MakeCurrent(device, gpu_context->handle);
		graphics_to_gpu_library_init();
		gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	}

	return gpu_context;
}

void gpu_context_free(struct Gpu_Context * gpu_context) {
	graphics_to_gpu_library_free();

	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	gs_gpu_library.dll.DeleteContext(gpu_context->handle);

	common_memset(gpu_context, 0, sizeof(*gpu_context));
	FREE(gpu_context);
}

void gpu_context_start_frame(struct Gpu_Context const * gpu_context, void * device) {
	if (gs_gpu_library.dll.GetCurrentContext() != NULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (gs_gpu_library.dll.GetCurrentDC() != NULL)      { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	if (!device) { DEBUG_BREAK(); return; }

	gs_gpu_library.dll.MakeCurrent(device, gpu_context->handle);
}

void gpu_context_draw_frame(struct Gpu_Context const * gpu_context) {
	(void)gpu_context;
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	HDC const device = gs_gpu_library.dll.GetCurrentDC();
	if (device == NULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	if (SwapBuffers(device)) { return; }
	gl.Flush();
}

void gpu_context_end_frame(struct Gpu_Context const * gpu_context) {
	(void)gpu_context;
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
}

int32_t gpu_context_get_vsync(struct Gpu_Context const * gpu_context) {
	(void)gpu_context;
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return 0; }
	if (gs_gpu_library.dll.GetCurrentDC() == NULL)      { REPORT_CALLSTACK(); DEBUG_BREAK(); return 0; }

	if (!gs_gpu_library.ext.swap_control) { return 1; }
	return gs_gpu_library.ext.GetSwapInterval();

	// https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
}

void gpu_context_set_vsync(struct Gpu_Context * gpu_context, int32_t value) {
	(void)gpu_context;
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (gs_gpu_library.dll.GetCurrentDC() == NULL)      { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	if (!gs_gpu_library.ext.swap_control) { return; }
	gs_gpu_library.ext.SwapInterval(value);
}

//

inline static int pixel_format_translate_swap_method(int value) {
	switch (value) {
		case WGL_SWAP_UNDEFINED_ARB: return 1;
		case WGL_SWAP_COPY_ARB:      return 2;
		case WGL_SWAP_EXCHANGE_ARB:  return 3;
	}
	return 0;
}

inline static bool pixel_format_is_preferable(struct Pixel_Format const * v1, struct Pixel_Format const * v2) {
	// adequate storage
	if (v2->color   < 32) { return false; }
	if (v2->alpha   <  8) { return false; }
	if (v2->depth   < 24) { return false; }
	if (v2->stencil <  8) { return false; }

	// prefer...
	if (v1->srgb    < v2->srgb)    { return true; } // enabled sRGB color space
	if (v1->samples > v2->samples) { return true; } // disabled multisampling
	if (v1->swap    < v2->swap)    { return true; } // buffer pointers exchange

	return false;
}

static struct Pixel_Format get_pixel_format(HDC device) {
	enum Keys {
		KEY_ACCELERATION,
		KEY_DRAW_TO_WINDOW,
		KEY_SUPPORT_OPENGL,
		KEY_DOUBLE_BUFFER,
		KEY_PIXEL_TYPE,
		//
		KEY_COLOR,
		KEY_ALPHA,
		KEY_DEPTH,
		KEY_STENCIL,
		//
		KEY_SRGB,
		KEY_SAMPLES,
		KEY_SWAP,
		//
		KEYS_COUNT,
	};

	int const keys[] = {
		[KEY_ACCELERATION]   = WGL_ACCELERATION_ARB,
		[KEY_DRAW_TO_WINDOW] = WGL_DRAW_TO_WINDOW_ARB,
		[KEY_SUPPORT_OPENGL] = WGL_SUPPORT_OPENGL_ARB,
		[KEY_DOUBLE_BUFFER]  = WGL_DOUBLE_BUFFER_ARB,
		[KEY_PIXEL_TYPE]     = WGL_PIXEL_TYPE_ARB,
		//
		[KEY_COLOR]   = WGL_COLOR_BITS_ARB,
		[KEY_ALPHA]   = WGL_ALPHA_BITS_ARB,
		[KEY_DEPTH]   = WGL_DEPTH_BITS_ARB,
		[KEY_STENCIL] = WGL_STENCIL_BITS_ARB,
		//
		[KEY_SRGB]    = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,
		[KEY_SAMPLES] = WGL_SAMPLES_ARB,
		[KEY_SWAP]    = WGL_SWAP_METHOD_ARB,
	};
	int vals[KEYS_COUNT];

	struct Pixel_Format result = {0};

	int const formats_request = WGL_NUMBER_PIXEL_FORMATS_ARB; int capacity = 0;
	gs_gpu_library.arb.GetPixelFormatAttribiv(device, 0, 0, 1, &formats_request, &capacity);
	for (int i = 0; i < capacity; i++) {
		int pfd_id = i + 1;
		if (!gs_gpu_library.arb.GetPixelFormatAttribiv(device, pfd_id, 0, KEYS_COUNT, keys, vals)) {
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		// discard non-supported
		if (!gs_gpu_library.ext.framebuffer_sRGB && vals[KEY_SRGB]) { continue; }
		if (!gs_gpu_library.arb.multisample && vals[KEY_SAMPLES] > 0) { continue; }

		// hardware-accelerated
		if (vals[KEY_DRAW_TO_WINDOW] == false) { continue; }
		if (vals[KEY_SUPPORT_OPENGL] == false) { continue; }
		if (vals[KEY_DOUBLE_BUFFER]  == false) { continue; }
		if (vals[KEY_PIXEL_TYPE]   != WGL_TYPE_RGBA_ARB) { continue; }
		if (vals[KEY_ACCELERATION] != WGL_FULL_ACCELERATION_ARB) { continue; }

		struct Pixel_Format const format = {
			.id = pfd_id,
			.color   = vals[KEY_COLOR],
			.alpha   = vals[KEY_ALPHA],
			.depth   = vals[KEY_DEPTH],
			.stencil = vals[KEY_STENCIL],
			.srgb    = vals[KEY_SRGB],
			.samples = vals[KEY_SAMPLES],
			.swap    = pixel_format_translate_swap_method(vals[KEY_SWAP]),
		};
		if (!pixel_format_is_preferable(&result, &format)) { continue; }
		result = format;
	}

	return result;

	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
}

struct Context_Format {
	int version, profile, deprecate;
	int flush, no_error, debug, robust;
};

static struct Gpu_Context gpu_context_internal_create(HDC device, HGLRC shared) {
	struct Pixel_Format const pixel_format = get_pixel_format(device);
	if (pixel_format.id == 0) { return (struct Gpu_Context){0}; }

	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(device, pixel_format.id, sizeof(pfd), &pfd);
	SetPixelFormat(device, pixel_format.id, &pfd);

	HGLRC handle = gs_gpu_library.arb.CreateContextAttribs(device, shared, (int[]){
		// use the newest API
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		// without legacy
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		// and adequate debug tools
	#if !defined (GAME_TARGET_RELEASE)
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
	#endif
		// that's it
		0,
	});

	return (struct Gpu_Context){
		.handle = handle,
		.pixel_format = pixel_format,
	};

	// https://www.khronos.org/opengl/wiki/OpenGL_Context
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
	// https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_context_flush_control.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_create_context_no_error.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context_robustness.txt
}

//

static void * gpu_library_get_function(struct CString name) {
	if (name.data == NULL) { return NULL; }

	PROC ogl_address = gs_gpu_library.dll.GetProcAddress(name.data);
	if (ogl_address != NULL) { return (void *)ogl_address; }

	FARPROC dll_address = GetProcAddress(gs_gpu_library.handle, name.data);
	if (dll_address != NULL) { return (void *)dll_address; }

	return NULL;
}

static bool gpu_library_do_using_temporary_context(bool (* action)(void)) {
#define OPENGL_CLASS_NAME "opengl_class"
	if (action == NULL) { return false; }

	DWORD const flags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER
		| PFD_SUPPORT_COMPOSITION | PFD_SWAP_EXCHANGE
		;

	PIXELFORMATDESCRIPTOR const pfd_hint = {
		.nSize        = sizeof(pfd_hint),
		.nVersion     = 1,
		.dwFlags      = flags,
		.iPixelType   = PFD_TYPE_RGBA,
		.iLayerType   = PFD_MAIN_PLANE,
		.cColorBits   = 32, .cAlphaBits = 8,
		.cDepthBits   = 24, .cStencilBits = 8,
	};

	// ... a temporary window
	HINSTANCE const module = system_to_internal_get_module();
	RegisterClassEx(&(WNDCLASSEX){
		.cbSize = sizeof(WNDCLASSEX),
		.lpszClassName = TEXT(OPENGL_CLASS_NAME),
		.hInstance = module,
		.lpfnWndProc = DefWindowProc,
	});

	HWND const hwnd = CreateWindowEx(
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
		TEXT(OPENGL_CLASS_NAME), TEXT(""),
		WS_CLIPSIBLINGS,
		0, 0, 1, 1,
		HWND_DESKTOP, NULL, module, NULL
	);
	HDC const device = GetDC(hwnd);

	// ... with a temporary pixel format
	PIXELFORMATDESCRIPTOR pfd;
	int const pfd_id = ChoosePixelFormat(device, &pfd_hint);
	DescribePixelFormat(device, pfd_id, sizeof(pfd), &pfd);
	SetPixelFormat(device, pfd_id, &pfd);

	// ... or a temporary rendering context
	HGLRC const context = gs_gpu_library.dll.CreateContext(device);
	gs_gpu_library.dll.MakeCurrent(device, context);

	// ... for a persistent WGL
	bool success = action();

	// ... and done
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	gs_gpu_library.dll.DeleteContext(context);

	ReleaseDC(hwnd, device);
	DestroyWindow(hwnd);
	UnregisterClass(TEXT(OPENGL_CLASS_NAME), module);

	return success;

#undef OPENGL_CLASS_NAME
}
