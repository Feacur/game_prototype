#include "framework/memory.h"
#include "framework/logger.h"

#include "framework/gpu_opengl/functions.h"
#include "framework/gpu_opengl/graphics_to_gpu_library.h"

#include "system_to_internal.h"

#include <Windows.h>
#include <GL/wgl.h>

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
		PFNWGLGETEXTENSIONSSTRINGARBPROC    GetExtensionsString;
		PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribiv;
		PFNWGLCREATECONTEXTATTRIBSARBPROC   CreateContextAttribs;
		char const * extensions;
	} arb;

	struct {
		PFNWGLGETEXTENSIONSSTRINGEXTPROC GetExtensionsString;
		PFNWGLGETSWAPINTERVALEXTPROC     GetSwapInterval;
		PFNWGLSWAPINTERVALEXTPROC        SwapInterval;
		char const * extensions;
		bool has_extension_swap_control;
	} ext;
} gs_gpu_library;

//
#include "gpu_library_to_system.h"

static bool contains_full_word(char const * container, struct CString value);
static void * gpu_library_get_function(struct CString name);
static bool gpu_library_wgl_init(void * device) {
	gs_gpu_library.arb.GetExtensionsString    = (PFNWGLGETEXTENSIONSSTRINGARBPROC)   (void *)gs_gpu_library.dll.GetProcAddress("wglGetExtensionsStringARB");
	gs_gpu_library.arb.GetPixelFormatAttribiv = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)(void *)gs_gpu_library.dll.GetProcAddress("wglGetPixelFormatAttribivARB");
	gs_gpu_library.arb.CreateContextAttribs   = (PFNWGLCREATECONTEXTATTRIBSARBPROC)  (void *)gs_gpu_library.dll.GetProcAddress("wglCreateContextAttribsARB");

	gs_gpu_library.ext.GetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)(void *)gs_gpu_library.dll.GetProcAddress("wglGetExtensionsStringEXT");
	gs_gpu_library.ext.GetSwapInterval     = (PFNWGLGETSWAPINTERVALEXTPROC)    (void *)gs_gpu_library.dll.GetProcAddress("wglGetSwapIntervalEXT");
	gs_gpu_library.ext.SwapInterval        = (PFNWGLSWAPINTERVALEXTPROC)       (void *)gs_gpu_library.dll.GetProcAddress("wglSwapIntervalEXT");

	gs_gpu_library.arb.extensions = gs_gpu_library.arb.GetExtensionsString(device);
	gs_gpu_library.ext.extensions = gs_gpu_library.ext.GetExtensionsString();

#define HAS_EXT(name) contains_full_word(gs_gpu_library.ext.extensions, S_("WGL_EXT_" # name))
	gs_gpu_library.ext.has_extension_swap_control = HAS_EXT(swap_control);
#undef HAS_EXT

	gpu_library_functions_init(gpu_library_get_function);

	return true;
}

static void gpu_library_do_using_temporary_context(bool (* action)(void * device));
void gpu_library_to_system_init(void) {
	gs_gpu_library.handle = LoadLibrary(TEXT("opengl32.dll"));
	if (gs_gpu_library.handle == NULL) {
		logger_to_console("'LoadLibrary(\"opengl32.dll\")' failed\n"); DEBUG_BREAK();
		common_exit_failure();
	}

	gs_gpu_library.dll.GetProcAddress    = (PFNWGLGETPROCADDRESSPROC)   (void *)GetProcAddress(gs_gpu_library.handle, "wglGetProcAddress");
	gs_gpu_library.dll.CreateContext     = (PFNWGLCREATECONTEXTPROC)    (void *)GetProcAddress(gs_gpu_library.handle, "wglCreateContext");
	gs_gpu_library.dll.DeleteContext     = (PFNWGLDELETECONTEXTPROC)    (void *)GetProcAddress(gs_gpu_library.handle, "wglDeleteContext");
	gs_gpu_library.dll.MakeCurrent       = (PFNWGLMAKECURRENTPROC)      (void *)GetProcAddress(gs_gpu_library.handle, "wglMakeCurrent");
	gs_gpu_library.dll.GetCurrentContext = (PFNWGLGETCURRENTCONTEXTPROC)(void *)GetProcAddress(gs_gpu_library.handle, "wglGetCurrentContext");
	gs_gpu_library.dll.GetCurrentDC      = (PFNWGLGETCURRENTDCPROC)     (void *)GetProcAddress(gs_gpu_library.handle, "wglGetCurrentDC");
	gs_gpu_library.dll.ShareLists        = (PFNWGLSHARELISTSPROC)       (void *)GetProcAddress(gs_gpu_library.handle, "wglShareLists");

	gpu_library_do_using_temporary_context(gpu_library_wgl_init);

	// https://docs.microsoft.com/windows/win32/api/wingdi/
	// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
}

void gpu_library_to_system_free(void) {
	gpu_library_functions_free();
	FreeLibrary(gs_gpu_library.handle);
	common_memset(&gs_gpu_library, 0, sizeof(gs_gpu_library));
}

//
#include "framework/gpu_context.h"

struct Pixel_Format {
	int id;
	int double_buffering, swap_method;
	int r, g, b, a;
	int depth, stencil;
	int samples, sample_buffers;
};

struct Gpu_Context {
	HGLRC handle;
	struct Pixel_Format pixel_format;
};

static HGLRC create_context_auto(HDC device, HGLRC shared, struct Pixel_Format * selected_pixel_format);
struct Gpu_Context * gpu_context_init(void * device) {
	struct Pixel_Format pixel_format;
	HGLRC const handle = create_context_auto(device, NULL, &pixel_format);

	gs_gpu_library.dll.MakeCurrent(device , handle);
	graphics_to_gpu_library_init();
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);

	struct Gpu_Context * gpu_context = MEMORY_ALLOCATE(&gs_gpu_library, struct Gpu_Context);
	*gpu_context = (struct Gpu_Context){
		.handle = handle,
		.pixel_format = pixel_format,
	};

	return gpu_context;
}

void gpu_context_free(struct Gpu_Context * gpu_context) {
	graphics_to_gpu_library_free();

	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
	gs_gpu_library.dll.DeleteContext(gpu_context->handle);

	common_memset(gpu_context, 0, sizeof(*gpu_context));
	MEMORY_FREE(&gs_gpu_library, gpu_context);
}

void gpu_context_start_frame(struct Gpu_Context const * gpu_context, void * device) {
	if (gs_gpu_library.dll.GetCurrentContext() != NULL) { DEBUG_BREAK(); return; }
	if (gs_gpu_library.dll.GetCurrentDC() != NULL) { DEBUG_BREAK(); return; }

	if (!device) { DEBUG_BREAK(); return; }

	gs_gpu_library.dll.MakeCurrent(device, gpu_context->handle);
}

void gpu_context_draw_frame(struct Gpu_Context const * gpu_context) {
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { DEBUG_BREAK(); return; }

	HDC const device = gs_gpu_library.dll.GetCurrentDC();
	if (device == NULL) { DEBUG_BREAK(); return; }

	if (gpu_context->pixel_format.double_buffering) {
		if (SwapBuffers(device)) { return; }
	}
	glFinish();
}

void gpu_context_end_frame(void) {
	gs_gpu_library.dll.MakeCurrent(NULL, NULL);
}

int32_t gpu_context_get_vsync(struct Gpu_Context const * gpu_context) {
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { DEBUG_BREAK(); return 0; }
	if (gs_gpu_library.dll.GetCurrentDC() == NULL) { DEBUG_BREAK(); return 0; }

	if (!gpu_context->pixel_format.double_buffering) { return 0; }
	if (!gs_gpu_library.ext.has_extension_swap_control) { /*default is 1*/ return 1; }
	return gs_gpu_library.ext.GetSwapInterval();

	// https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
}

void gpu_context_set_vsync(struct Gpu_Context * gpu_context, int32_t value) {
	if (gs_gpu_library.dll.GetCurrentContext() == NULL) { DEBUG_BREAK(); return; }
	if (gs_gpu_library.dll.GetCurrentDC() == NULL) { DEBUG_BREAK(); return; }

	if (!gpu_context->pixel_format.double_buffering) { return; }
	if (!gs_gpu_library.ext.has_extension_swap_control) { return; }
	gs_gpu_library.ext.SwapInterval(value);
}

//

static bool contains_full_word(char const * container, struct CString value) {
	if (container == NULL)  { return false; }
	if (value.data == NULL) { return false; }

	for (char const * start = container, * end = container; *start; start = end) {
		while (*start == ' ') { start++; }

		end = strchr(start, ' ');
		if (end == NULL) { end = container + strlen(container); }

		if ((size_t)(end - start) != value.length) { continue; }
		if (common_memcmp(start, value.data, value.length) == 0) { return true; }
	}

	return false;
}

static int dictionary_int_int_get_value(int const * keys, int const * vals, int key) {
	for (int i = 0; keys[i]; i++) {
		if (keys[i] == key) { return vals[i]; }
	}
	return 0;
}

static struct Pixel_Format * allocate_pixel_formats_arb(HDC device) {
#define HAS_ARB(name) contains_full_word(gs_gpu_library.arb.extensions, S_("WGL_ARB_" # name))
#define HAS_EXT(name) contains_full_word(gs_gpu_library.ext.extensions, S_("WGL_EXT_" # name))
#define KEYS_COUNT (sizeof(request_keys) / sizeof(*request_keys))
#define GET_VALUE(key) dictionary_int_int_get_value(request_keys, request_vals, key)

	if (!HAS_ARB(pixel_format)) { return NULL; }

	int const formats_request = WGL_NUMBER_PIXEL_FORMATS_ARB; int formats_capacity;
	if (!gs_gpu_library.arb.GetPixelFormatAttribiv(device, 0, 0, 1, &formats_request, &formats_capacity)) { DEBUG_BREAK(); return NULL; }
	if (formats_capacity == 0) { DEBUG_BREAK(); return NULL; }

	bool has_extension_multisample = HAS_ARB(multisample);
	bool has_extension_framebuffer_sRGB = HAS_EXT(framebuffer_sRGB);

	int const request_keys[] = {
		WGL_DRAW_TO_WINDOW_ARB,
		WGL_ACCELERATION_ARB,
		WGL_SWAP_METHOD_ARB,
		WGL_SUPPORT_GDI_ARB,
		WGL_SUPPORT_OPENGL_ARB,
		WGL_DOUBLE_BUFFER_ARB,
		WGL_PIXEL_TYPE_ARB,
		//
		WGL_RED_BITS_ARB,
		WGL_GREEN_BITS_ARB,
		WGL_BLUE_BITS_ARB,
		WGL_ALPHA_BITS_ARB,
		WGL_DEPTH_BITS_ARB,
		WGL_STENCIL_BITS_ARB,
		//
		WGL_SAMPLES_ARB,
		WGL_SAMPLE_BUFFERS_ARB,
		//
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,
	};

	int request_vals[KEYS_COUNT];

	int formats_count = 0;
	struct Pixel_Format * formats = MEMORY_ALLOCATE_ARRAY(&gs_gpu_library, struct Pixel_Format, formats_capacity + 1);
	for (int i = 0; i < formats_capacity; i++) {
		int pfd_id = i + 1;
		if (!gs_gpu_library.arb.GetPixelFormatAttribiv(device, pfd_id, 0, KEYS_COUNT, request_keys, request_vals)) { DEBUG_BREAK(); continue; }

		if (GET_VALUE(WGL_DRAW_TO_WINDOW_ARB) == false)                     { continue; }
		if (GET_VALUE(WGL_SUPPORT_GDI_ARB)    == true)                      { continue; }
		if (GET_VALUE(WGL_SUPPORT_OPENGL_ARB) == false)                     { continue; }
		if (GET_VALUE(WGL_ACCELERATION_ARB)   != WGL_FULL_ACCELERATION_ARB) { continue; }
		if (GET_VALUE(WGL_PIXEL_TYPE_ARB)     != WGL_TYPE_RGBA_ARB)         { continue; }

		if (has_extension_multisample) {
			if (GET_VALUE(WGL_SAMPLES_ARB) == 0)        { continue; }
			if (GET_VALUE(WGL_SAMPLE_BUFFERS_ARB) == 0) { continue; }
		}

		if (has_extension_framebuffer_sRGB) {
			if (!GET_VALUE(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB)) { continue; }
		}

		int swap_method = 0;
		switch (GET_VALUE(WGL_SWAP_METHOD_ARB)) {
			case WGL_SWAP_UNDEFINED_ARB: swap_method = 0; break;
			case WGL_SWAP_COPY_ARB:      swap_method = 1; break;
			case WGL_SWAP_EXCHANGE_ARB:  swap_method = 2; break;
		}

		formats[formats_count++] = (struct Pixel_Format){
			.id = pfd_id,
			.double_buffering = GET_VALUE(WGL_DOUBLE_BUFFER_ARB),
			.swap_method = swap_method,
			.r = GET_VALUE(WGL_RED_BITS_ARB),
			.g = GET_VALUE(WGL_GREEN_BITS_ARB),
			.b = GET_VALUE(WGL_BLUE_BITS_ARB),
			.a = GET_VALUE(WGL_ALPHA_BITS_ARB),
			.depth   = GET_VALUE(WGL_DEPTH_BITS_ARB),
			.stencil = GET_VALUE(WGL_STENCIL_BITS_ARB),
			.samples        = GET_VALUE(WGL_SAMPLES_ARB),
			.sample_buffers = GET_VALUE(WGL_SAMPLE_BUFFERS_ARB),
		};
	}

	formats[formats_count] = (struct Pixel_Format){0};
	return formats;
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt

#undef HAS_ARB
#undef HAS_EXT
#undef KEYS_COUNT
#undef GET_VALUE
}

static struct Pixel_Format * allocate_pixel_formats_legacy(HDC device) {
	int formats_capacity = DescribePixelFormat(device, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
	if (formats_capacity == 0) { return NULL; }

	int formats_count = 0;
	struct Pixel_Format * formats = MEMORY_ALLOCATE_ARRAY(&gs_gpu_library, struct Pixel_Format, formats_capacity + 1);
	for (int i = 0; i < formats_capacity; i++) {
		int pfd_id = i + 1;
		PIXELFORMATDESCRIPTOR pfd;
		if (!DescribePixelFormat(device, pfd_id, sizeof(pfd), &pfd)) { DEBUG_BREAK(); continue; }

		if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW)) { continue; }
		if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL)) { continue; }
		if (!(pfd.dwFlags & PFD_GENERIC_FORMAT)) { continue; }

		if ((pfd.dwFlags & PFD_SUPPORT_GDI))         { continue; }
		if ((pfd.dwFlags & PFD_GENERIC_ACCELERATED)) { continue; }

		if (pfd.iPixelType != PFD_TYPE_RGBA) { continue; }

		int swap_method = 0;
		if (false) {}
		else if ((pfd.dwFlags & PFD_SWAP_COPY))     { swap_method = 1; }
		else if ((pfd.dwFlags & PFD_SWAP_EXCHANGE)) { swap_method = 2; }

		formats[formats_count++] = (struct Pixel_Format){
			.id = pfd_id,
			.double_buffering = (pfd.dwFlags & PFD_DOUBLEBUFFER),
			.swap_method = swap_method,
			.r = pfd.cRedBits,
			.g = pfd.cGreenBits,
			.b = pfd.cBlueBits,
			.a = pfd.cAlphaBits,
			.depth   = pfd.cDepthBits,
			.stencil = pfd.cStencilBits,
		};
	}

	formats[formats_count] = (struct Pixel_Format){0};
	return formats;
}

static struct Pixel_Format choose_pixel_format(struct Pixel_Format const * formats, struct Pixel_Format const * hint) {
	for (struct Pixel_Format const * format = formats; format->id != 0; format++) {
		if (!format->double_buffering) { DEBUG_BREAK(); }
	}

	for (struct Pixel_Format const * format = formats; format->id != 0; format++) {
		if (format->r       < hint->r)       { continue; }
		if (format->g       < hint->g)       { continue; }
		if (format->b       < hint->b)       { continue; }
		if (format->a       < hint->a)       { continue; }
		if (format->depth   < hint->depth)   { continue; }
		if (format->stencil < hint->stencil) { continue; }

		if (format->double_buffering < hint->double_buffering) { continue; }
		if (hint->swap_method != 0) {
			if (format->swap_method != hint->swap_method) { continue; }
		}

		if (format->samples < hint->samples) { continue; }
		if (format->sample_buffers < hint->sample_buffers) { continue; }

		return *format;
	}
	return (struct Pixel_Format){0};
}

struct Context_Format {
	int version;
	int profile;
	int deprecate;
	int flush;
	int no_error;
	int debug;
	int robust;
};

static HGLRC create_context_arb(HDC device, HGLRC shared, struct Context_Format const * context_format) {
#define HAS_ARB(name) contains_full_word(gs_gpu_library.arb.extensions, S_("WGL_ARB_" # name))
#define ADD_ATTRIBUTE(key, value) \
	do { \
		attributes[attributes_count++] = key; \
		attributes[attributes_count++] = value; \
	} while (false) \

	if (!HAS_ARB(create_context)) { return NULL; }

	int attributes_count = 0;
	int attributes[16];

	int context_flags = 0;
	int context_profile_mask  = 0;

	// version
	if (context_format->version != 0) {
		ADD_ATTRIBUTE(WGL_CONTEXT_MAJOR_VERSION_ARB, context_format->version / 10);
		ADD_ATTRIBUTE(WGL_CONTEXT_MINOR_VERSION_ARB, context_format->version % 10);
	}

	// profile
	if (context_format->version >= 32) {
		switch (context_format->profile) {
			case 1: context_profile_mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB; break;
			case 2: context_profile_mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB; break;
		}
	}

	// deprecation
	if (context_format->deprecate != 0 && context_format->version >= 30) {
		context_flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
	}

	// flush control
	if (HAS_ARB(context_flush_control)) {
		switch (context_format->flush) {
			case 1: ADD_ATTRIBUTE(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB, WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB); break;
			case 2: ADD_ATTRIBUTE(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB, WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB); break;
		}
	}

	// error control
	if (context_format->no_error != 0 && HAS_ARB(create_context_no_error)) {
		ADD_ATTRIBUTE(WGL_CONTEXT_OPENGL_NO_ERROR_ARB, true);
	}
	else {
		if (context_format->debug != 0) {
			context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
		}

		if (context_format->robust != 0 && HAS_ARB(create_context_robustness)) {
			context_flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
			switch (context_format->robust) {
				case 1: ADD_ATTRIBUTE(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, WGL_NO_RESET_NOTIFICATION_ARB); break;
				case 2: ADD_ATTRIBUTE(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, WGL_LOSE_CONTEXT_ON_RESET_ARB); break;
			}
		}
	}

	// flags and masks
	if (context_flags != 0) {
		ADD_ATTRIBUTE(WGL_CONTEXT_FLAGS_ARB, context_flags);
	}

	if (context_profile_mask != 0 && HAS_ARB(create_context_profile)) {
		ADD_ATTRIBUTE(WGL_CONTEXT_PROFILE_MASK_ARB, context_profile_mask);
	}

	//
	ADD_ATTRIBUTE(0, 0);
	return gs_gpu_library.arb.CreateContextAttribs(device, shared, attributes);
	// https://www.khronos.org/opengl/wiki/OpenGL_Context
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
	// https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_context_flush_control.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_create_context_no_error.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context_robustness.txt

#undef HAS_ARB
#undef ADD_ATTRIBUTE
}

static HGLRC create_context_legacy(HDC device, HGLRC shared) {
	HGLRC result = gs_gpu_library.dll.CreateContext(device);
	if (shared != NULL) { gs_gpu_library.dll.ShareLists(shared, result); }
	return result;
}

static HGLRC create_context_auto(HDC device, HGLRC shared, struct Pixel_Format * selected_pixel_format) {
	struct Pixel_Format const hint = (struct Pixel_Format){
		.double_buffering = true, .swap_method = 0,
		.r = 8, .g = 8, .b = 8, .a = 8,
		.depth = 24, .stencil = 8,
	};
	struct Context_Format const settings = (struct Context_Format){
		.version = 46, .profile = 2, .deprecate = 1,
		.flush = 0, .no_error = 0, .debug = 0, .robust = 0,
	};

	struct Pixel_Format * pixel_formats = allocate_pixel_formats_arb(device);
	if (pixel_formats == NULL) { pixel_formats = allocate_pixel_formats_legacy(device); }

	struct Pixel_Format pixel_format = choose_pixel_format(pixel_formats, &hint);
	if (pixel_format.id == 0) {
		logger_to_console("failed to choose format\n"); DEBUG_BREAK();
		common_exit_failure();
	}

	MEMORY_FREE(&gs_gpu_library, pixel_formats);

	PIXELFORMATDESCRIPTOR pfd;
	int formats_count = DescribePixelFormat(device, pixel_format.id, sizeof(pfd), &pfd);
	if (formats_count == 0) {
		logger_to_console("failed to describe format\n"); DEBUG_BREAK();
		common_exit_failure();
	}

	BOOL pfd_found = SetPixelFormat(device, pixel_format.id, &pfd);
	if (!pfd_found) {
		logger_to_console("'SetPixelFormat' failed\n"); DEBUG_BREAK();
		common_exit_failure();
	}

	HGLRC result = create_context_arb(device, shared, &settings);
	if (result == NULL) { result = create_context_legacy(device, shared); }
	if (result == NULL) {
		logger_to_console("failed to create context\n"); DEBUG_BREAK();
		common_exit_failure();
	}

	*selected_pixel_format = pixel_format;
	return result;
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

static void gpu_library_do_using_temporary_context(bool (* action)(void * device)) {
	if (action == NULL) { DEBUG_BREAK(); return; }
	bool success = false;

// #define OPENGL_CLASS_NAME "gpu_library_class"
#define OPENGL_CLASS_NAME APPLICATION_CLASS_NAME

	// create a temporary window class
	bool const use_application_class = strcmp(OPENGL_CLASS_NAME, APPLICATION_CLASS_NAME) == 0;
	ATOM const class_atom = !use_application_class
		? RegisterClassEx(&(WNDCLASSEX){
			.cbSize = sizeof(WNDCLASSEX),
			.lpszClassName = TEXT(OPENGL_CLASS_NAME),
			.hInstance = (HANDLE)system_to_internal_get_module(),
			.lpfnWndProc = DefWindowProc,
		})
		: 0;
	if (!use_application_class && class_atom == 0) { goto clean_up_class; }

	// create a temporary window
	HWND const window = CreateWindowEx(
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
		TEXT(OPENGL_CLASS_NAME), TEXT(""),
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 1, 1,
		HWND_DESKTOP, NULL, (HANDLE)system_to_internal_get_module(), NULL
	);
	if (window == NULL) { goto clean_up_window; }

	HDC const device = GetDC(window);
	if (device == NULL) { goto clean_up_device; }

	// create a temporary rendering context
	PIXELFORMATDESCRIPTOR pfd_hint = {
		.nSize        = sizeof(pfd_hint),
		.nVersion     = 1,
		.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType   = PFD_TYPE_RGBA,
		.iLayerType   = PFD_MAIN_PLANE,
		.cColorBits   = 32,
		.cDepthBits   = 24,
		.cStencilBits = 8,
	};

	int pfd_id = ChoosePixelFormat(device, &pfd_hint);
	if (pfd_id == 0) { goto clean_up_device; }

	PIXELFORMATDESCRIPTOR pfd;
	int formats_count = DescribePixelFormat(device, pfd_id, sizeof(pfd), &pfd);
	if (formats_count == 0) { goto clean_up_device; }

	BOOL const pfd_found = SetPixelFormat(device, pfd_id, &pfd);
	if (!pfd_found) { goto clean_up_device; }

	HGLRC const context = gs_gpu_library.dll.CreateContext(device);
	if (context == NULL) { goto clean_up_context; }

	BOOL const rc_is_current = gs_gpu_library.dll.MakeCurrent(device, context);
	if (!rc_is_current) { goto clean_up_context; }

	//
	success = action(device);

	// clean up
	clean_up_context: if (!success) { DEBUG_BREAK(); }
	if (context != NULL) {
		gs_gpu_library.dll.MakeCurrent(NULL, NULL);
		gs_gpu_library.dll.DeleteContext(context);
	}
	else { logger_to_console("failed to create a temporary context\n"); }

	clean_up_device: if (!success) { DEBUG_BREAK(); }
	if (device != NULL) { ReleaseDC(window, device); }
	else { logger_to_console("failed to fetch a temporary device\n"); }

	clean_up_window: if (!success) { DEBUG_BREAK(); }
	if (window != NULL) { DestroyWindow(window); }
	else { logger_to_console("failed to create a temporary window\n"); }

	clean_up_class: if (!success) { DEBUG_BREAK(); }
	if (class_atom != 0) { UnregisterClass(TEXT(OPENGL_CLASS_NAME), (HANDLE)system_to_internal_get_module()); }
	else if (!use_application_class) { logger_to_console("failed to create a temporary window class\n"); }

#undef OPENGL_CLASS_NAME
}

//

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#endif

// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
// Global Variable NvOptimusEnablement (new in Driver Release 302)
// Starting with the Release 302 drivers, application developers can direct the Optimus driver at runtime to use the High Performance Graphics to render any application–even those applications for which there is no existing application profile. They can do this by exporting a global variable named NvOptimusEnablement. The Optimus driver looks for the existence and value of the export. Only the LSB of the DWORD matters at this time. Avalue of 0x00000001 indicates that rendering should be performed using High Performance Graphics. A value of 0x00000000 indicates that this method should beignored.
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001UL;

// https://community.amd.com/thread/169965
// https://community.amd.com/thread/223376
// https://gpuopen.com/amdpowerxpressrequesthighperformance/
// Summary
// Many Gaming and workstation laptops are available with both (1) integrated power saving and (2) discrete high performance graphics devices. Unfortunately, 3D intensive application performance may suffer greatly if the best graphics device is not selected. For example, a game may run at 30 Frames Per Second (FPS) on the integrated GPU rather than the 60 FPS the discrete GPU would enable. As a developer you can easily fix this problem by adding only one line to your executable’s source code:
// Yes, it’s that easy. This line will ensure that the high-performance graphics device is chosen when running your application.
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001UL;

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif
