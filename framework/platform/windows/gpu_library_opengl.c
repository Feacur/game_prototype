#include "framework/formatter.h"
#include "framework/platform/system.h"
#include "framework/systems/memory.h"

#include "framework/graphics/opengl/functions.h"
#include "framework/graphics/opengl/internal/functions_to_gpu_library.h"
#include "framework/graphics/opengl/internal/graphics_to_gpu_library.h"

#include "__platform.h"

#include "framework/__warnings_push.h"
	#include <GL/wgl.h>
#include "framework/__warnings_pop.h"


static struct GPU_Library {
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
		bool samples;
	} arb;

	struct {
		PFNWGLGETSWAPINTERVALEXTPROC     GetSwapInterval;
		PFNWGLSWAPINTERVALEXTPROC        SwapInterval;
		bool srgb;
		bool swap;
	} ext;
} gs_gpu_library;

//
#include "framework/platform/gpu_context.h"

struct Pixel_Format {
	int id;
	int color, alpha, depth, stencil;
	int srgb, samples, swap;
};

struct GPU_Context {
	HGLRC handle;
	struct Pixel_Format pixel_format;
};

inline static int pixel_format_translate_swap_method(int value) {
	switch (value) {
		case WGL_SWAP_UNDEFINED_ARB: return 1;
		case WGL_SWAP_COPY_ARB:      return 2;
		case WGL_SWAP_EXCHANGE_ARB:  return 3;
	}
	return 0;
}

static struct Pixel_Format choose_pixel_format(HDC device, Comparator compare) {
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
		if (!gs_gpu_library.ext.srgb && vals[KEY_SRGB])       { continue; }
		if (!gs_gpu_library.arb.samples && vals[KEY_SAMPLES]) { continue; }

		// hardware-accelerated
		if (!vals[KEY_DRAW_TO_WINDOW]) { continue; }
		if (!vals[KEY_SUPPORT_OPENGL]) { continue; }
		if (!vals[KEY_DOUBLE_BUFFER])  { continue; }
		if (vals[KEY_PIXEL_TYPE]   != WGL_TYPE_RGBA_ARB)         { continue; }
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
		if (compare(&result, &format) >= 0) { continue; }
		result = format;
	}

	return result;

	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
}

static COMPARATOR(compare_pixel_format) {
	struct Pixel_Format const * pf1 = v1;
	struct Pixel_Format const * pf2 = v2;

	// adequate storage
	if (pf2->color   < 32) { return 1; }
	if (pf2->alpha   <  8) { return 1; }
	if (pf2->depth   < 24) { return 1; }
	if (pf2->stencil <  8) { return 1; }

	// prefer...
	if (pf1->srgb    < pf2->srgb)    { return -1; } // enabled sRGB color space
	if (pf1->samples > pf2->samples) { return -1; } // disabled multisampling
	if (pf1->swap    < pf2->swap)    { return -1; } // buffer pointers exchange

	return 0;
}

static struct GPU_Context gpu_context_internal_create(HDC surface, HGLRC shared) {
	struct Pixel_Format const pixel_format = choose_pixel_format(surface, compare_pixel_format);
	if (pixel_format.id == 0) { return (struct GPU_Context){0}; }

	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(surface, pixel_format.id, sizeof(pfd), &pfd);
	SetPixelFormat(surface, pixel_format.id, &pfd);

	HGLRC handle = gs_gpu_library.arb.CreateContextAttribs(surface, shared, (int[]){
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

	return (struct GPU_Context){
		.handle = handle,
		.pixel_format = pixel_format,
	};

	// https://www.khronos.org/opengl/wiki/OpenGL_Context
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
}

struct GPU_Context * gpu_context_init(void * surface) {
	struct GPU_Context * gpu_context = ALLOCATE(struct GPU_Context);
	*gpu_context = gpu_context_internal_create(surface, NULL);

	if (gpu_context->handle != NULL) {
		gs_gpu_library.dll.MakeCurrent(surface, gpu_context->handle);
		graphics_to_gpu_library_init();
		gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	}

	return gpu_context;
}

void gpu_context_free(struct GPU_Context * gpu_context) {
	graphics_to_gpu_library_free();

	if (gs_gpu_library.dll.GetCurrentContext() == gpu_context->handle) {
		gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	}
	gs_gpu_library.dll.DeleteContext(gpu_context->handle);

	zero_out(CBMP_(gpu_context));
	FREE(gpu_context);
}

bool gpu_context_exists(struct GPU_Context const * gpu_context) {
	return gpu_context->handle != NULL;
}

void gpu_context_start_frame(struct GPU_Context const * gpu_context, void * surface) {
	gs_gpu_library.dll.MakeCurrent(surface, gpu_context->handle);
}

void gpu_context_end_frame(struct GPU_Context const * gpu_context) {
	if (gs_gpu_library.dll.GetCurrentContext() != gpu_context->handle) { return; }
	SwapBuffers(gs_gpu_library.dll.GetCurrentDC());
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
}

int32_t gpu_context_get_vsync(struct GPU_Context const * gpu_context) {
	if (gs_gpu_library.dll.GetCurrentContext() != gpu_context->handle) { return 1; }
	if (!gs_gpu_library.ext.swap) { return 1; }
	return gs_gpu_library.ext.GetSwapInterval();

	// https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
}

void gpu_context_set_vsync(struct GPU_Context * gpu_context, int32_t value) {
	if (gs_gpu_library.dll.GetCurrentContext() != gpu_context->handle) { return; }
	if (!gs_gpu_library.ext.swap) { return; }
	gs_gpu_library.ext.SwapInterval(value);
}

//
#include "internal/gpu_library_to_system.h"

static PROC_GETTER(gpu_library_get_proc_fallback) {
	void * address = (void *)GetProcAddress(gs_gpu_library.handle, name.data);
	if (address != NULL) {
		TRC("loaded %.*s", name.length, name.data);
		return address;
	}
	TRC("failed to load %.*s", name.length, name.data);
	return NULL;
}

static PROC_GETTER(gpu_library_get_proc_address) {
	void * address = (void *)gs_gpu_library.dll.GetProcAddress(name.data);
	if (address != NULL) {
		TRC("loaded %.*s", name.length, name.data);
		return address;
	}
	return gpu_library_get_proc_fallback(name);
}

static bool gpu_library_init(void) {
	HDC const surface = gs_gpu_library.dll.GetCurrentDC();

	// ARB
	TRC("loading ARB");
	gs_gpu_library.arb.GetPixelFormatAttribiv = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(void *)gpu_library_get_proc_address(S_("wglGetPixelFormatAttribivARB"));
	gs_gpu_library.arb.CreateContextAttribs   = (PFNWGLCREATECONTEXTATTRIBSARBPROC)  (void *)gpu_library_get_proc_address(S_("wglCreateContextAttribsARB"));

	PFNWGLGETEXTENSIONSSTRINGARBPROC arbGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGARBPROC)   (void *)gpu_library_get_proc_address(S_("wglGetExtensionsStringARB"));
	char const * arbString = arbGetExtensionsString(surface);
	LOG("ARB extensions: %s\n", arbString);
#define HAS_ARB(name) contains_full_word(arbString, S_("WGL_ARB_" #name))
	gs_gpu_library.arb.samples = HAS_ARB(multisample);
#undef HAS_ARB

	// EXT
	TRC("loading EXT");
	gs_gpu_library.ext.GetSwapInterval = (PFNWGLGETSWAPINTERVALEXTPROC)(void *)gpu_library_get_proc_address(S_("wglGetSwapIntervalEXT"));
	gs_gpu_library.ext.SwapInterval    = (PFNWGLSWAPINTERVALEXTPROC)   (void *)gpu_library_get_proc_address(S_("wglSwapIntervalEXT"));

	PFNWGLGETEXTENSIONSSTRINGEXTPROC extGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void *)gpu_library_get_proc_address(S_("wglGetExtensionsStringEXT"));
	char const * extString = extGetExtensionsString();
	LOG("EXT extensions: %s\n", extString);
#define HAS_EXT(name) contains_full_word(extString, S_("WGL_EXT_" #name))
	gs_gpu_library.ext.srgb = HAS_EXT(framebuffer_sRGB);
	gs_gpu_library.ext.swap = HAS_EXT(swap_control);
#undef HAS_EXT

	// OGL
	functions_to_gpu_library_init(gpu_library_get_proc_address);
	return true;
}

static bool gpu_library_bootstrap(bool (* init)(void)) {
#define OPENGL_CLASS_NAME "opengl_class"
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
	TRC("creating temporary bootstrap for WGL");
	HINSTANCE const module = (HINSTANCE)platform_system_get_module();
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
	HDC const surface = GetDC(hwnd);

	// ... with a temporary pixel format
	PIXELFORMATDESCRIPTOR pfd;
	int const pfd_id = ChoosePixelFormat(surface, &pfd_hint);
	DescribePixelFormat(surface, pfd_id, sizeof(pfd), &pfd);
	SetPixelFormat(surface, pfd_id, &pfd);

	// ... and a temporary rendering context
	HGLRC const context = gs_gpu_library.dll.CreateContext(surface);
	gs_gpu_library.dll.MakeCurrent(surface, context);

	// ... for a persistent WGL
	bool success = init();

	// ... ta-da
	TRC("destroying bootstrap context for WGL");
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	gs_gpu_library.dll.DeleteContext(context);

	ReleaseDC(hwnd, surface);
	DestroyWindow(hwnd);
	UnregisterClass(TEXT(OPENGL_CLASS_NAME), module);

	return success;

#undef OPENGL_CLASS_NAME
}

bool gpu_library_to_system_init(void) {
	gs_gpu_library.handle = LoadLibrary(TEXT("opengl32.dll"));
	if (gs_gpu_library.handle == NULL) { return false; }

	TRC("loading WGL");
	gs_gpu_library.dll.GetProcAddress    = (PFNWGLGETPROCADDRESSPROC)   gpu_library_get_proc_fallback(S_("wglGetProcAddress"));
	gs_gpu_library.dll.CreateContext     = (PFNWGLCREATECONTEXTPROC)    gpu_library_get_proc_fallback(S_("wglCreateContext"));
	gs_gpu_library.dll.DeleteContext     = (PFNWGLDELETECONTEXTPROC)    gpu_library_get_proc_fallback(S_("wglDeleteContext"));
	gs_gpu_library.dll.MakeCurrent       = (PFNWGLMAKECURRENTPROC)      gpu_library_get_proc_fallback(S_("wglMakeCurrent"));
	gs_gpu_library.dll.GetCurrentContext = (PFNWGLGETCURRENTCONTEXTPROC)gpu_library_get_proc_fallback(S_("wglGetCurrentContext"));
	gs_gpu_library.dll.GetCurrentDC      = (PFNWGLGETCURRENTDCPROC)     gpu_library_get_proc_fallback(S_("wglGetCurrentDC"));
	gs_gpu_library.dll.ShareLists        = (PFNWGLSHARELISTSPROC)       gpu_library_get_proc_fallback(S_("wglShareLists"));

	return gpu_library_bootstrap(gpu_library_init);

	// https://learn.microsoft.com/windows/win32/api/wingdi/
	// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
}

void gpu_library_to_system_free(void) {
	functions_to_gpu_library_free();
	FreeLibrary(gs_gpu_library.handle);
	zero_out(CBM_(gs_gpu_library));
	TRC("unloaded WGL, ARB, EXT");
}
