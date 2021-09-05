# -
[documentation] reorganize
[system] track memory through a doubly linked list; the memory system is automatically zero-initialized

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
