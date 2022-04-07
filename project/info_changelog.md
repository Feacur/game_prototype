# -
[framework] make Ref_Table and Ref zero-initializable
[framework] reuse Array_Any code for Ref_Table
[framework] introduce macros for iterators
[framework] free GPU objects with a macro
[framework] rename some functions `graphics_*` into `gpu_*`
[framework] introduce Ref and Asset_Ref equality comparers
[framework] consider string_id {0} empty
[prototype] check codepoints visibility with a function
[prototype] rename `resource` -> `name`
[framework] get rid of asset system's allocations upon init
[prototype] tweak some API regarding return values
[prototype] tweak application API
[framework] pass binary buffers to the basic asset structures
[project] add missing compiler flags for the clangd LSP and ECC
[framework] introduce iterators macros
[framework] make iterators accept const containers
[application] tweak assets extensions (is a WIP)
[project] write font glyphs directly to the atlas
[project] document `any` containers data types
[framework] use arrays and tables at gpu code side
[application] expose font scale setting

# -
[project] reorganize documentation
[framework] track memory through a doubly linked list; the memory system is automatically zero-initialized
[framework] remove CS_OWNDC dependence
[framework] untangle GPU context from window
[framework] reduce exit-with-error usage
[framework] fix legacy WGL initialization
[framework] allow single-buffered WGL
[framework] allow multiline commnents in JSON files
[framework] keep error context for WFObj and JSON scanners
[refactor] add explicit prefix to global states, statics, and constants
[refactor] remove some global state
[refactor] rename GPU context enitites using better wording
[prototype] migrate all entities to JSON initialization
[prototype] operate asset refs instead of gpu refs or asset instances where applicable
[prototype] unhardcode startup settings
[prototype] correctly intertwine bactched and non-batched entities
[prototype] correctly anchor UI elements
[application] make render target assets
[application] make material assets
[application] migrate assets from hardcode to json-based files

# 2021, august 12
```
[>] asset system
    [x] text data
[>] entity system
    [x] operate [crude] entities for rendering
    [x] a separate list of camera entities for rendering
[>] text rendering
    [x] automatically build atlas based on input text
    [x] grabage collect glyphs
    [x] collect all the relevant glyphs, then update textures AND texture coordinates
    [x] glyphs GC threshold
    [x] postpone fonts atlases generation and texts quads creation
[>] clean up containers implementation
    [x] ever need a uint64_t hashes for hash tables?
    [x] `set` methods for arrays
    [x] some bound checks
    [x] refactor iterators
[>] build system
    [x] streamline build settings switching
[x] substitute `printf` with `stb_sprintf` and `fwrite`
[x] mark some pointers as weak
[x] fix leaks
[x] report a `stb_truetype.h` bug (a duplicate, actually)
```

# 2021, may 03
```
[>] entity system
    [x] create relocation table to expose handles as an alternative to raw pointers
[>] asset system
    [x] create an asset-tracking system
    [x] migrate some asset loading to the system: shaders, models, textures, fonts
    [x] make file extension to type id association
[>] graphics
    [x] expose GPU object handles instead of opaque pointers
    [x] API to correctly batch-update meshes/textures and create a render pass (or whatever)
[>] memory
    [x] track allocations/deallocations and report everying that remains when program exits
[x] support large files? more than 2gb? more than 4gb?
[x] make build system more flexible
[>] IDE
    [x] try using `Sublime Text + LSP + clangd + RemedyBG` instead of `VSCode + ms-vscode.cpptools + vadimcn.vscode-lldb`
    [x] implement a couple of `Sublime Text` plugins for better UX of `Build Output` and `Find Results` buffers
[x] report a bunch of bugs to `LSP`, `EasyClangComplete`, `RemedyBG`, `clangd`
```

# 2021, march 26
```
[>] text rendering
    [x] GPU texture data streaming
    [x] improve font glyphs packer
[>] graphics
    [x] introduce immutable buffers
[>] platform
    [x] make the application Unicode-aware: file paths, input
```

# 2021, march 24
```
[>] text rendering
    [x] UTF-8 decoding
[>] build system
	[x] make translation units list explicit
```

# 2021, march 21
```
 - framework
[>] text rendering
    [x] GPU mesh data streaming
    [x] 2d batch renderer (quite universal, actually, but needs testing)
    [x] asset loader: .ttf
    [x] simplistic font glyphs packer
    [x] hashtable for glyphs (turned out to be quite generic)
```

# 2021, march 19
```
--- a mind dump of the current state ---
 - framework
[x] centralized memory allocator
[>] abstract platform layer
    [x] graphics library initialization: WGL -> OpenGL
    [x] filesystem access
    [x] mouse/keyboard input
[>] abstract graphics layer
    [x] GPU objects: program, texture, target, mesh
    [x] a naive render pass block
[x] asset loaders: .png, .obj
[x] a set of container objects
[x] maths
 - application
[x] automatically initialize the platform layer
[x] expose callbacks API to the prototype
 - project
[x] custom MSVC/Clang build "system" (direct use of compilators and linkers)
[x] unity build option
```

[Markdown](https://www.markdownguide.org/basic-syntax/)
