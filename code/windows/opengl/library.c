#include "code/memory.h"
#include "code/opengl/functions.h"

#include "code/windows/window_to_graphics_library.h"
#include "code/opengl/graphics_to_graphics_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <GL/wgl.h>

static struct {
	HMODULE handle;

	struct {
		PFNWGLGETPROCADDRESSPROC GetProcAddress;
		PFNWGLCREATECONTEXTPROC  CreateContext;
		PFNWGLDELETECONTEXTPROC  DeleteContext;
		PFNWGLMAKECURRENTPROC    MakeCurrent;
		PFNWGLSHARELISTSPROC     ShareLists;
	} dll;

	struct {
		PFNWGLGETEXTENSIONSSTRINGARBPROC    GetExtensionsString;
		PFNWGLGETPIXELFORMATATTRIBIVARBPROC GetPixelFormatAttribiv;
		PFNWGLCREATECONTEXTATTRIBSARBPROC   CreateContextAttribs;
		char const * extensions;
	} arb;

	struct {
		PFNWGLGETEXTENSIONSSTRINGEXTPROC GetExtensionsString;
		PFNWGLSWAPINTERVALEXTPROC        SwapInterval;
		char const * extensions;
		bool has_extension_swap_control;
	} ext;
} rlib;

//
#include "code/windows/graphics_library.h"

// -- library part
static bool contains_full_word(char const * container, char const * value);
void graphics_library_init(void) {
// #define OPENGL_CLASS_NAME "opengl_class"
#define OPENGL_CLASS_NAME window_to_graphics_library_get_class()

	rlib.handle = LoadLibraryA("opengl32.dll");
	if (rlib.handle == NULL) { fprintf(stderr, "'LoadLibrary' failed\n"); DEBUG_BREAK(); exit(1); }

	HMODULE application_module = GetModuleHandle(NULL);

	// fetch basic DLL functions
	rlib.dll.GetProcAddress = (PFNWGLGETPROCADDRESSPROC)GetProcAddress(rlib.handle, "wglGetProcAddress");
	rlib.dll.CreateContext  = (PFNWGLCREATECONTEXTPROC) GetProcAddress(rlib.handle, "wglCreateContext");
	rlib.dll.DeleteContext  = (PFNWGLDELETECONTEXTPROC) GetProcAddress(rlib.handle, "wglDeleteContext");
	rlib.dll.MakeCurrent    = (PFNWGLMAKECURRENTPROC)   GetProcAddress(rlib.handle, "wglMakeCurrent");
	rlib.dll.ShareLists     = (PFNWGLSHARELISTSPROC)    GetProcAddress(rlib.handle, "wglShareLists");

	// create temporary class
	// ATOM atom = RegisterClassExA(&(WNDCLASSEXA){
	// 	.cbSize = sizeof(WNDCLASSEXA),
	// 	.lpszClassName = OPENGL_CLASS_NAME,
	// 	.hInstance = application_module,
	// 	.lpfnWndProc = DefWindowProcA,
	// });
	// if (atom == 0) { fprintf(stderr, "'RegisterClassExA' failed\n"); DEBUG_BREAK(); exit(1); }

	// create temporary window
	HWND hwnd = CreateWindowExA(
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
		OPENGL_CLASS_NAME, "",
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 1, 1,
		HWND_DESKTOP, NULL, application_module, NULL
	);
	if (hwnd == NULL) { fprintf(stderr, "'CreateWindow' failed\n"); DEBUG_BREAK(); exit(1); }

	HDC hdc = GetDC(hwnd);
	if (hdc == NULL) { fprintf(stderr, "'GetDC' failed\n"); DEBUG_BREAK(); exit(1); }

	// create temporary rendering context
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

	int pfd_id = ChoosePixelFormat(hdc, &pfd_hint);
	if (pfd_id == 0) { fprintf(stderr, "'ChoosePixelFormat' failed\n"); DEBUG_BREAK(); exit(1); }

	PIXELFORMATDESCRIPTOR pfd;
	int formats_count = DescribePixelFormat(hdc, pfd_id, sizeof(pfd), &pfd);
	if (formats_count == 0) { fprintf(stderr, "'DescribePixelFormat' failed\n"); DEBUG_BREAK(); exit(1); }

	BOOL pfd_found = SetPixelFormat(hdc, pfd_id, &pfd);
	if (!pfd_found) { fprintf(stderr, "'SetPixelFormat' failed\n"); DEBUG_BREAK(); exit(1); }

	HGLRC rc_handle = rlib.dll.CreateContext(hdc);
	if (rc_handle == NULL) { fprintf(stderr, "'CreateContext' failed\n"); DEBUG_BREAK(); exit(1); }

	BOOL rc_is_current = rlib.dll.MakeCurrent(hdc, rc_handle);
	if (!rc_is_current) { fprintf(stderr, "'MakeCurrent' failed\n"); DEBUG_BREAK(); exit(1); }

	// fetch extensions
	rlib.arb.GetExtensionsString    = (PFNWGLGETEXTENSIONSSTRINGARBPROC)   rlib.dll.GetProcAddress("wglGetExtensionsStringARB");
	rlib.arb.GetPixelFormatAttribiv = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)rlib.dll.GetProcAddress("wglGetPixelFormatAttribivARB");
	rlib.arb.CreateContextAttribs   = (PFNWGLCREATECONTEXTATTRIBSARBPROC)  rlib.dll.GetProcAddress("wglCreateContextAttribsARB");

	rlib.ext.GetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)rlib.dll.GetProcAddress("wglGetExtensionsStringEXT");
	rlib.ext.SwapInterval        = (PFNWGLSWAPINTERVALEXTPROC)       rlib.dll.GetProcAddress("wglSwapIntervalEXT");

	rlib.arb.extensions = rlib.arb.GetExtensionsString(hdc);
	rlib.ext.extensions = rlib.ext.GetExtensionsString();

#define HAS_EXT(name) contains_full_word(rlib.ext.extensions, "WGL_EXT_" # name)
	rlib.ext.has_extension_swap_control = HAS_EXT(swap_control);
#undef HAS_EXT

	//
	rlib.dll.MakeCurrent(NULL, NULL);
	rlib.dll.DeleteContext(rc_handle);

	// ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	// UnregisterClassA(OPENGL_CLASS_NAME, application_module);

	// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/
	// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)

#undef OPENGL_CLASS_NAME
}

void graphics_library_free(void) {
	FreeLibrary(rlib.handle);
	memset(&rlib, 0, sizeof(rlib));
}

// -- graphics part
struct Pixel_Format {
	int id;
	int double_buffering, swap_method;
	int r, g, b, a;
	int depth, stencil;
	int samples, sample_buffers;
};

struct Graphics {
	HGLRC handle;
	HDC private_device;
	struct Pixel_Format pixel_format;
	int32_t vsync;
};

static HGLRC create_context_auto(HDC device, HGLRC shared, struct Pixel_Format * selected_pixel_format);
static void * rlib_get_function(char const * name);
struct Graphics * graphics_init(struct Window * window) {
	struct Graphics * context = MEMORY_ALLOCATE(struct Graphics);

	context->private_device = window_to_graphics_library_get_private_device(window);
	context->handle = create_context_auto(context->private_device, NULL, &context->pixel_format);
	rlib.dll.MakeCurrent(context->private_device , context->handle);
	context->vsync = 0;

	graphics_load_functions(rlib_get_function);
	graphics_to_graphics_library_init();

	return context;
}

void graphics_free(struct Graphics * context) {
	graphics_to_graphics_library_free();

	rlib.dll.MakeCurrent(NULL, NULL);
	rlib.dll.DeleteContext(context->handle);

	memset(context, 0, sizeof(*context));
	MEMORY_FREE(context);
}

int32_t graphics_get_vsync(struct Graphics * context) {
	return context->vsync;
}

void graphics_set_vsync(struct Graphics * context, int32_t value) {
	if (!context->pixel_format.double_buffering) { return; }
	if (rlib.ext.has_extension_swap_control) {
		if (rlib.ext.SwapInterval((int)value)) {
			context->vsync = value;
			return;
		}
	}
}

void graphics_display(struct Graphics * context) {
	if (context->pixel_format.double_buffering) {
		if (SwapBuffers(context->private_device)) { return; }
	}
	glFlush();
	// glFinish();
}

// void graphics_size(struct Graphics * context, int32_t size_x, int32_t size_y) {
// 	(void)context;
// 	glViewport(0, 0, (GLsizei)size_x, (GLsizei)size_y);
// }

//

static bool contains_full_word(char const * container, char const * value) {
	if (container == NULL) { return false; }
	if (value == NULL)     { return false; }

	size_t value_size = strlen(value);

	for (char const * start = container, * end = container; *start; start = end) {
		while (*start == ' ') { start++; }

		end = strchr(start, ' ');
		if (end == NULL) { end = container + strlen(container); }

		if ((size_t)(end - start) != value_size) { continue; }
		if (strncmp(start, value, value_size) == 0) { return true; }
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
#define HAS_ARB(name) contains_full_word(rlib.arb.extensions, "WGL_ARB_" # name)
#define HAS_EXT(name) contains_full_word(rlib.ext.extensions, "WGL_EXT_" # name)
#define KEYS_COUNT (sizeof(request_keys) / sizeof(*request_keys))
#define GET_VALUE(key) dictionary_int_int_get_value(request_keys, request_vals, key)

	if (!HAS_ARB(pixel_format)) { return NULL; }

	int const formats_request = WGL_NUMBER_PIXEL_FORMATS_ARB; int formats_capacity;
	if (!rlib.arb.GetPixelFormatAttribiv(device, 0, 0, 1, &formats_request, &formats_capacity)) { DEBUG_BREAK(); return NULL; }
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
	struct Pixel_Format * formats = MEMORY_ALLOCATE_ARRAY(struct Pixel_Format, formats_capacity + 1);
	for (int i = 0; i < formats_capacity; i++) {
		int pfd_id = i + 1;
		if (!rlib.arb.GetPixelFormatAttribiv(device, pfd_id, 0, KEYS_COUNT, request_keys, request_vals)) { DEBUG_BREAK(); continue; }

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
	struct Pixel_Format * formats = MEMORY_ALLOCATE_ARRAY(struct Pixel_Format, formats_capacity + 1);
	for (int i = 0; i < formats_capacity; i++) {
		int pfd_id = i + 1;
		PIXELFORMATDESCRIPTOR pfd;
		if (!DescribePixelFormat(device, pfd_id, sizeof(pfd), &pfd)) { DEBUG_BREAK(); continue; }

		if ((pfd.dwFlags & PFD_DRAW_TO_WINDOW)      != PFD_DRAW_TO_WINDOW)      { continue; }
		if ((pfd.dwFlags & PFD_SUPPORT_GDI)         == PFD_SUPPORT_GDI)         { continue; }
		if ((pfd.dwFlags & PFD_SUPPORT_OPENGL)      != PFD_SUPPORT_OPENGL)      { continue; }
		if ((pfd.dwFlags & PFD_GENERIC_FORMAT)      != PFD_GENERIC_FORMAT)      { continue; }
		if ((pfd.dwFlags & PFD_GENERIC_ACCELERATED) == PFD_GENERIC_ACCELERATED) { continue; }

		if (pfd.iPixelType != PFD_TYPE_RGBA) { continue; }

		int swap_method = 0;
		if (false) {}
		else if ((pfd.dwFlags & PFD_SWAP_COPY)     == PFD_SWAP_COPY)     { swap_method = 1; }
		else if ((pfd.dwFlags & PFD_SWAP_EXCHANGE) == PFD_SWAP_EXCHANGE) { swap_method = 2; }

		formats[formats_count++] = (struct Pixel_Format){
			.id = pfd_id,
			.double_buffering = (pfd.dwFlags & PFD_DOUBLEBUFFER)  == PFD_DOUBLEBUFFER,
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

static struct Pixel_Format choose_pixel_format(struct Pixel_Format const * formats, struct Pixel_Format hint) {
	for (struct Pixel_Format const * format = formats; format->id != 0; format++) {
		if (format->r       < hint.r)       { continue; }
		if (format->g       < hint.g)       { continue; }
		if (format->b       < hint.b)       { continue; }
		if (format->a       < hint.a)       { continue; }
		if (format->depth   < hint.depth)   { continue; }
		if (format->stencil < hint.stencil) { continue; }

		if (format->double_buffering < hint.double_buffering) { continue; }
		if (hint.swap_method != 0) {
			if (format->swap_method != hint.swap_method) { continue; }
		}

		if (format->samples < hint.samples) { continue; }
		if (format->sample_buffers < hint.sample_buffers) { continue; }

		return *format;
	}
	return (struct Pixel_Format){0};
}

struct Context_Format {
	int version;
	int core;
	int flush;
	int debug;
};

static HGLRC create_context_arb(HDC device, HGLRC shared, struct Context_Format context_format) {
#define HAS_ARB(name) contains_full_word(rlib.arb.extensions, "WGL_ARB_" # name)
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
	if (context_format.version != 0) {
		ADD_ATTRIBUTE(WGL_CONTEXT_MAJOR_VERSION_ARB, context_format.version / 10);
		ADD_ATTRIBUTE(WGL_CONTEXT_MINOR_VERSION_ARB, context_format.version % 10);
	}

	// up to date
	if (context_format.core) {
		if (context_format.version >= 30) {
			context_flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		}
		if (context_format.version >= 32) {
			context_profile_mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
		}
	}
	else {
		context_profile_mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
	}

	// flush control
	if (HAS_ARB(context_flush_control)) {
		ADD_ATTRIBUTE(
			WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
			context_format.flush ? WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB : WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB
		);
	}

	// error control
	if (context_format.debug == 0 && HAS_ARB(create_context_no_error)) {
		ADD_ATTRIBUTE(WGL_CONTEXT_OPENGL_NO_ERROR_ARB, true);
	}
	else {
		int const context_robustness_mode = 0;
		if (context_robustness_mode != 0 && HAS_ARB(create_context_robustness)) {
			ADD_ATTRIBUTE(
				WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
				context_robustness_mode == 1 ? WGL_NO_RESET_NOTIFICATION_ARB : WGL_LOSE_CONTEXT_ON_RESET_ARB
			);
			context_flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
		}

		if (context_format.debug == 2) { context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB; }
	}

	// flags and masks
	if (context_flags != 0) {
		ADD_ATTRIBUTE(WGL_CONTEXT_FLAGS_ARB, context_flags);
	}

	if (HAS_ARB(create_context_profile) && context_profile_mask != 0) {
		ADD_ATTRIBUTE(WGL_CONTEXT_PROFILE_MASK_ARB, context_profile_mask);
	}

	//
	ADD_ATTRIBUTE(0, 0);
	return rlib.arb.CreateContextAttribs(device, shared, attributes);
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
	// https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_context_flush_control.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_create_context_no_error.txt
	// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context_robustness.txt

#undef HAS_ARB
#undef ADD_ATTRIBUTE
}

static HGLRC create_context_legacy(HDC device, HGLRC shared) {
	HGLRC result = rlib.dll.CreateContext(device);
	if (shared != NULL) { rlib.dll.ShareLists(shared, result); }
	return result;
}

static HGLRC create_context_auto(HDC device, HGLRC shared, struct Pixel_Format * selected_pixel_format) {
	struct Pixel_Format hint = (struct Pixel_Format){
		.double_buffering = true, .swap_method = 0,
		.r = 8, .g = 8, .b = 8, .a = 8,
		.depth = 24, .stencil = 8,
	};
	struct Context_Format settings = (struct Context_Format){
		.version = 46, .core = 1,
		.flush = 0, .debug = 1,
	};

	struct Pixel_Format * pixel_formats = allocate_pixel_formats_arb(device);
	if (pixel_formats == NULL) { pixel_formats = allocate_pixel_formats_legacy(device); }

	struct Pixel_Format pixel_format = choose_pixel_format(pixel_formats, hint);
	if (pixel_format.id == 0) { fprintf(stderr, "failed to choose format\n"); DEBUG_BREAK(); exit(1); }

	MEMORY_FREE(pixel_formats);

	PIXELFORMATDESCRIPTOR pfd;
	int formats_count = DescribePixelFormat(device, pixel_format.id, sizeof(pfd), &pfd);
	if (formats_count == 0) { fprintf(stderr, "failed to describe format\n"); DEBUG_BREAK(); exit(1); }

	BOOL pfd_found = SetPixelFormat(device, pixel_format.id, &pfd);
	if (!pfd_found) { fprintf(stderr, "'SetPixelFormat' failed\n"); DEBUG_BREAK(); exit(1); }

	HGLRC result = create_context_arb(device, shared, settings);
	if (result == NULL) { result = create_context_legacy(device, shared); }
	if (result == NULL) { fprintf(stderr, "failed to create context\n"); DEBUG_BREAK(); exit(1); }

	*selected_pixel_format = pixel_format;
	return result;
}

//

static void * rlib_get_function(char const * name) {
	if (name == NULL) { return NULL; }

	PROC ogl_address = rlib.dll.GetProcAddress(name);
	if (ogl_address != NULL) { return (void *)ogl_address; }

	FARPROC dll_address = GetProcAddress(rlib.handle, name);
	if (dll_address != NULL) { return (void *)dll_address; }

	return NULL;
}

#undef OPENGL_CLASS_NAME
